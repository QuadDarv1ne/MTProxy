/*
    This file is part of Mtproto-proxy Library.

    Mtproto-proxy Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Mtproto-proxy Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Mtproto-proxy Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2024-2026 MTProto Proxy Enhanced Project
              2024-2026 Dupley Maxim Igorevich (Maestro7IT)
*/

#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "crypto/aes-optimized.h"
#include "common/kprintf.h"
#include "common/common-stats.h"
#include "common/memory-limits.h"

// Глобальная статистика AES (thread-safe с atomic operations)
struct aes_optimized_stats aes_stats = {0};

// Макрос для атомарного инкремента (portable)
#ifdef __GNUC__
#define ATOMIC_INC(var) __sync_fetch_and_add(&(var), 1)
#elif defined(_WIN32)
#define ATOMIC_INC(var) InterlockedIncrement64(&(var))
#else
#define ATOMIC_INC(var) (var)++  // Fallback для не-GCC
#endif

// Кэш предвычисленных ключей AES - ОГРАНИЧЕННЫЙ РАЗМЕР
#ifndef AES_KEY_CACHE_SIZE
#define AES_KEY_CACHE_SIZE 256  // Уменьшено с 1024 для экономии памяти
#endif
#define AES_KEY_CACHE_MASK (AES_KEY_CACHE_SIZE - 1)

struct aes_key_cache_entry {
    unsigned char key[32];      // AES-256 ключ
    unsigned char iv[16];       // Вектор инициализации
    EVP_CIPHER_CTX *encrypt_ctx;
    EVP_CIPHER_CTX *decrypt_ctx;
    unsigned long long last_used;
    int valid;
};

static struct aes_key_cache_entry *aes_key_cache = NULL;
static unsigned long long aes_cache_counter = 0;

// Mutex для защиты кэша AES (thread safety)
#ifdef _WIN32
static HANDLE aes_cache_mutex = NULL;
#else
static pthread_mutex_t aes_cache_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

// Инициализация mutex для AES кэша
static void aes_cache_mutex_init(void) {
#ifdef _WIN32
    if (!aes_cache_mutex) {
        aes_cache_mutex = CreateMutex(NULL, FALSE, NULL);
    }
#else
    // PTHREAD_MUTEX_INITIALIZER уже инициализирован
#endif
}

static void aes_cache_mutex_lock(void) {
#ifdef _WIN32
    if (aes_cache_mutex) {
        WaitForSingleObject(aes_cache_mutex, INFINITE);
    }
#else
    pthread_mutex_lock(&aes_cache_mutex);
#endif
}

static void aes_cache_mutex_unlock(void) {
#ifdef _WIN32
    if (aes_cache_mutex) {
        ReleaseMutex(aes_cache_mutex);
    }
#else
    pthread_mutex_unlock(&aes_cache_mutex);
#endif
}

// Предвычисленные константы для ускорения (зарезервировано для будущего использования)
static const unsigned char precomputed_round_keys[15][16] __attribute__((aligned(16))) __attribute__((unused)) = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63},
    {0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6},
    {0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5,0xa5},
    {0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83,0x83},
    {0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f},
    {0x5e,0x5e,0x5e,0x5e,0x5e,0x5e,0x5e,0x5e,0x5e,0x5e,0x5e,0x5e,0x5e,0x5e,0x5e,0x5e},
    {0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c},
    {0x9a,0x9a,0x9a,0x9a,0x9a,0x9a,0x9a,0x9a,0x9a,0x9a,0x9a,0x9a,0x9a,0x9a,0x9a,0x9a},
    {0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09,0x09},
    {0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50},
    {0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f},
    {0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1,0xa1},
    {0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1},
    {0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74,0x74}
};

// Инициализация кэша ключей AES
int aes_optimized_init(void) {
    if (aes_key_cache) {
        return 0; // Уже инициализирован
    }

    // Инициализация mutex
    aes_cache_mutex_init();

    // Проверка лимита памяти перед выделением
    size_t cache_size = AES_KEY_CACHE_SIZE * sizeof(struct aes_key_cache_entry);
    if (cache_size > MAX_CRYPTO_CACHE_MB * 1024 * 1024 / 2) {
        vkprintf(1, "WARNING: AES cache size limited to %d MB\n", MAX_CRYPTO_CACHE_MB/2);
    }

    aes_cache_mutex_lock();
    aes_key_cache = calloc(AES_KEY_CACHE_SIZE, sizeof(struct aes_key_cache_entry));
    aes_cache_mutex_unlock();

    if (!aes_key_cache) {
        vkprintf(0, "ERROR: Failed to allocate AES key cache\n");
        return -1;
    }

    vkprintf(1, "AES optimized cache initialized with %d entries (%zu KB)\n",
             AES_KEY_CACHE_SIZE, cache_size / 1024);
    return 0;
}

// Хэш-функция для кэширования ключей
static inline unsigned int aes_key_hash(const unsigned char *key, const unsigned char *iv) {
    unsigned int hash = 5381;
    int i;
    
    // Хэшируем ключ (32 байта)
    for (i = 0; i < 32; i++) {
        hash = ((hash << 5) + hash) + key[i];
    }
    
    // Хэшируем IV (16 байт)
    for (i = 0; i < 16; i++) {
        hash = ((hash << 5) + hash) + iv[i];
    }
    
    return hash;
}

// Создание контекста AES с предвычисленными ключами
static EVP_CIPHER_CTX *create_aes_context(const unsigned char *key, const unsigned char *iv, int is_encrypt) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return NULL;
    }
    
    const EVP_CIPHER *cipher = EVP_aes_256_cbc();
    if (EVP_CipherInit_ex(ctx, cipher, NULL, key, iv, is_encrypt) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    
    // Отключаем padding для MTProto
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    
    return ctx;
}

// Получение кэшированного контекста AES
static struct aes_key_cache_entry *get_cached_aes_context(const unsigned char *key, const unsigned char *iv) {
    aes_cache_mutex_lock();

    if (!aes_key_cache) {
        aes_cache_mutex_unlock();
        return NULL;
    }

    unsigned int hash = aes_key_hash(key, iv);
    unsigned int index = hash & AES_KEY_CACHE_MASK;
    struct aes_key_cache_entry *entry = &aes_key_cache[index];

    // Проверяем совпадение ключа и IV
    if (entry->valid &&
        memcmp(entry->key, key, 32) == 0 &&
        memcmp(entry->iv, iv, 16) == 0) {
        entry->last_used = ++aes_cache_counter;
        ATOMIC_INC(aes_stats.key_cache_hits);
        aes_cache_mutex_unlock();
        return entry;
    }

    ATOMIC_INC(aes_stats.key_cache_misses);

    // Освобождаем старый контекст если есть
    if (entry->valid) {
        if (entry->encrypt_ctx) {
            EVP_CIPHER_CTX_free(entry->encrypt_ctx);
            entry->encrypt_ctx = NULL;
        }
        if (entry->decrypt_ctx) {
            EVP_CIPHER_CTX_free(entry->decrypt_ctx);
            entry->decrypt_ctx = NULL;
        }
        entry->valid = 0;
    }

    // Создаем новые контексты
    entry->encrypt_ctx = create_aes_context(key, iv, 1);
    entry->decrypt_ctx = create_aes_context(key, iv, 0);

    if (!entry->encrypt_ctx || !entry->decrypt_ctx) {
        if (entry->encrypt_ctx) {
            EVP_CIPHER_CTX_free(entry->encrypt_ctx);
            entry->encrypt_ctx = NULL;
        }
        if (entry->decrypt_ctx) {
            EVP_CIPHER_CTX_free(entry->decrypt_ctx);
            entry->decrypt_ctx = NULL;
        }
        entry->valid = 0;
        aes_cache_mutex_unlock();
        return NULL;
    }

    // Сохраняем ключ и IV
    memcpy(entry->key, key, 32);
    memcpy(entry->iv, iv, 16);
    entry->last_used = ++aes_cache_counter;
    entry->valid = 1;

    aes_cache_mutex_unlock();
    return entry;
}

// Оптимизированное шифрование AES
int aes_optimized_encrypt(const unsigned char *key, const unsigned char *iv,
                         const void *plaintext, void *ciphertext, int length) {
    if (length <= 0 || length % 16 != 0) {
        return -1; // Длина должна быть кратна 16 байтам
    }

    ATOMIC_INC(aes_stats.total_encryptions);

    struct aes_key_cache_entry *entry = get_cached_aes_context(key, iv);
    if (!entry || !entry->encrypt_ctx) {
        ATOMIC_INC(aes_stats.fallback_operations);
        // Fallback на стандартную реализацию
        EVP_CIPHER_CTX *ctx = create_aes_context(key, iv, 1);
        if (!ctx) return -1;
        
        int out_len;
        int result = EVP_CipherUpdate(ctx, ciphertext, &out_len, plaintext, length);
        EVP_CIPHER_CTX_free(ctx);
        
        if (result == 1 && out_len == length) {
            ATOMIC_INC(aes_stats.total_encryptions);
            return length;
        }
        return -1;
    }

    int out_len;
    if (EVP_CipherUpdate(entry->encrypt_ctx, ciphertext, &out_len, plaintext, length) != 1) {
        return -1;
    }

    ATOMIC_INC(aes_stats.total_encryptions);
    return out_len;
}

// Оптимизированное дешифрование AES
int aes_optimized_decrypt(const unsigned char *key, const unsigned char *iv,
                         const void *ciphertext, void *plaintext, int length) {
    if (length <= 0 || length % 16 != 0) {
        return -1; // Длина должна быть кратна 16 байтам
    }

    struct aes_key_cache_entry *entry = get_cached_aes_context(key, iv);
    if (!entry || !entry->decrypt_ctx) {
        ATOMIC_INC(aes_stats.fallback_operations);
        // Fallback на стандартную реализацию
        EVP_CIPHER_CTX *ctx = create_aes_context(key, iv, 0);
        if (!ctx) return -1;
        
        int out_len;
        int result = EVP_CipherUpdate(ctx, plaintext, &out_len, ciphertext, length);
        EVP_CIPHER_CTX_free(ctx);
        
        if (result == 1 && out_len == length) {
            aes_stats.total_decryptions++;
            return length;
        }
        return -1;
    }
    
    int out_len;
    if (EVP_CipherUpdate(entry->decrypt_ctx, plaintext, &out_len, ciphertext, length) != 1) {
        return -1;
    }
    
    aes_stats.total_decryptions++;
    return out_len;
}

// Предвычисленное шифрование для часто используемых ключей
int aes_optimized_encrypt_precomputed(const unsigned char *key, const unsigned char *iv,
                                    const void *plaintext, void *ciphertext, int length) {
    // Проверяем, является ли ключ одним из часто используемых
    static const unsigned char common_keys[][32] = {
        // Здесь можно добавить часто используемые ключи
        {0}
    };

    // Для демонстрации - используем первый общий ключ
    if (memcmp(key, common_keys[0], 32) == 0) {
        ATOMIC_INC(aes_stats.precomputed_keys_used);
        // Здесь можно реализовать специализированную оптимизацию
        // для конкретных ключей
    }

    return aes_optimized_encrypt(key, iv, plaintext, ciphertext, length);
}

// Очистка кэша AES
void aes_optimized_cleanup(void) {
    if (!aes_key_cache) {
        return;
    }

    int i;
    for (i = 0; i < AES_KEY_CACHE_SIZE; i++) {
        struct aes_key_cache_entry *entry = &aes_key_cache[i];
        if (entry->valid) {
            if (entry->encrypt_ctx) {
                EVP_CIPHER_CTX_free(entry->encrypt_ctx);
            }
            if (entry->decrypt_ctx) {
                EVP_CIPHER_CTX_free(entry->decrypt_ctx);
            }
            entry->valid = 0;
        }
    }
    
    free(aes_key_cache);
    aes_key_cache = NULL;

    // Очистка mutex (fix H2: handle leak на Windows)
#ifdef _WIN32
    if (aes_cache_mutex) {
        CloseHandle(aes_cache_mutex);
        aes_cache_mutex = NULL;
    }
#endif

    vkprintf(1, "AES optimized cache cleaned up\n");
}

// Получение статистики AES оптимизации
void aes_optimized_get_stats(struct aes_optimized_stats *stats) {
    if (stats) {
        memcpy(stats, &aes_stats, sizeof(struct aes_optimized_stats));
    }
}

// Вывод статистики в лог
void aes_optimized_print_stats(void) {
    vkprintf(1, "AES Optimization Statistics:\n");
    vkprintf(1, "  Cache Hits: %lld\n", aes_stats.key_cache_hits);
    vkprintf(1, "  Cache Misses: %lld\n", aes_stats.key_cache_misses);
    vkprintf(1, "  Hit Rate: %.2f%%\n", 
             aes_stats.key_cache_hits + aes_stats.key_cache_misses > 0 ? 
             (double)aes_stats.key_cache_hits / (aes_stats.key_cache_hits + aes_stats.key_cache_misses) * 100 : 0.0);
    vkprintf(1, "  Precomputed Keys Used: %lld\n", aes_stats.precomputed_keys_used);
    vkprintf(1, "  Fallback Operations: %lld\n", aes_stats.fallback_operations);
    vkprintf(1, "  Total Encryptions: %lld\n", aes_stats.total_encryptions);
    vkprintf(1, "  Total Decryptions: %lld\n", aes_stats.total_decryptions);
}

// Батчевая обработка AES операций
int aes_optimized_batch_encrypt(const unsigned char *key, const unsigned char *iv,
                               const void **plaintext_array, void **ciphertext_array, 
                               int *length_array, int count) {
    if (count <= 0) {
        return 0;
    }
    
    struct aes_key_cache_entry *entry = get_cached_aes_context(key, iv);
    if (!entry || !entry->encrypt_ctx) {
        // Fallback: обрабатываем по одному
        int i, total = 0;
        for (i = 0; i < count; i++) {
            if (aes_optimized_encrypt(key, iv, plaintext_array[i], ciphertext_array[i], length_array[i]) > 0) {
                total++;
            }
        }
        return total;
    }
    
    // Оптимизированная батчевая обработка
    int i, processed = 0;
    for (i = 0; i < count; i++) {
        int out_len;
        if (EVP_CipherUpdate(entry->encrypt_ctx, ciphertext_array[i], &out_len, 
                           plaintext_array[i], length_array[i]) == 1) {
            processed++;
        }
    }
    
    return processed;
}