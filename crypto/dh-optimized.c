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
#include <openssl/bn.h>
#include <openssl/rand.h>

#include "crypto/dh-optimized.h"
#include "common/kprintf.h"
#include "common/common-stats.h"

// Статистика для DH оптимизации
struct dh_optimized_stats {
    long long precomputed_values_used;
    long long fast_path_operations;
    long long fallback_operations;
    long long total_dh_generations;
    long long cached_results_used;
    long long montgomery_reductions;
};

static struct dh_optimized_stats dh_stats = {0};

// Кэш предвычисленных DH значений
#define DH_CACHE_SIZE 512
#define DH_CACHE_MASK (DH_CACHE_SIZE - 1)

struct dh_cache_entry {
    unsigned char base[256];    // Базовое значение g^x
    unsigned char result[256];  // Результат g^(xy)
    unsigned long long last_used;
    int valid;
    unsigned int hash_key;
};

static struct dh_cache_entry *dh_cache = NULL;
static unsigned long long dh_cache_counter = 0;

// Предвычисленные константы для ускорения DH
static const unsigned char dh_prime_bin[256] = {
    0x89, 0x52, 0x13, 0x1b, 0x1e, 0x3a, 0x69, 0xba, 0x5f, 0x85, 0xcf, 0x8b, 0xd2, 0x66, 0xc1, 0x2b,
    0x13, 0x83, 0x16, 0x13, 0xbd, 0x2a, 0x4e, 0xf8, 0x35, 0xa4, 0xd5, 0x3f, 0x9d, 0xbb, 0x42, 0x48,
    0x2d, 0xbd, 0x46, 0x2b, 0x31, 0xd8, 0x6c, 0x81, 0x6c, 0x59, 0x77, 0x52, 0x0f, 0x11, 0x70, 0x73,
    0x9e, 0xd2, 0xdd, 0xd6, 0xd8, 0x1b, 0x9e, 0xb6, 0x5f, 0xaa, 0xac, 0x14, 0x87, 0x53, 0xc9, 0xe4,
    0xf0, 0x72, 0xdc, 0x11, 0xa4, 0x92, 0x73, 0x06, 0x83, 0xfa, 0x00, 0x67, 0x82, 0x6b, 0x18, 0xc5,
    0x1d, 0x7e, 0xcb, 0xa5, 0x2b, 0x82, 0x60, 0x75, 0xc0, 0xb9, 0x55, 0xe5, 0xac, 0xaf, 0xdd, 0x74,
    0xc3, 0x79, 0x5f, 0xd9, 0x52, 0x0b, 0x48, 0x0f, 0x3b, 0xe3, 0xba, 0x06, 0x65, 0x33, 0x8a, 0x49,
    0x8c, 0xa5, 0xda, 0xf1, 0x01, 0x76, 0x05, 0x09, 0xa3, 0x8c, 0x49, 0xe3, 0x00, 0x74, 0x64, 0x08,
    0x77, 0x4b, 0xb3, 0xed, 0x26, 0x18, 0x1a, 0x64, 0x55, 0x76, 0x6a, 0xe9, 0x49, 0x7b, 0xb9, 0xc3,
    0xa3, 0xad, 0x5c, 0xba, 0xf7, 0x6b, 0x73, 0x84, 0x5f, 0xbb, 0x96, 0xbb, 0x6d, 0x0f, 0x68, 0x4f,
    0x95, 0xd2, 0xd3, 0x9c, 0xcb, 0xb4, 0xa9, 0x04, 0xfa, 0xb1, 0xde, 0x43, 0x49, 0xce, 0x1c, 0x20,
    0x87, 0xb6, 0xc9, 0x51, 0xed, 0x99, 0xf9, 0x52, 0xe3, 0x4f, 0xd1, 0xa3, 0xfd, 0x14, 0x83, 0x35,
    0x75, 0x41, 0x47, 0x29, 0xa3, 0x8b, 0xe8, 0x68, 0xa4, 0xf9, 0xec, 0x62, 0x3a, 0x5d, 0x24, 0x62,
    0x1a, 0xba, 0x01, 0xb2, 0x55, 0xc7, 0xe8, 0x38, 0x5d, 0x16, 0xac, 0x93, 0xb0, 0x2d, 0x2a, 0x54,
    0x0a, 0x76, 0x42, 0x98, 0x2d, 0x22, 0xad, 0xa3, 0xcc, 0xde, 0x5c, 0x8d, 0x26, 0x6f, 0xaa, 0x25,
    0xdd, 0x2d, 0xe9, 0xf6, 0xd4, 0x91, 0x04, 0x16, 0x2f, 0x68, 0x5c, 0x45, 0xfe, 0x34, 0xdd, 0xab
};

// Глобальные переменные для DH
static BIGNUM *dh_prime = NULL;
static BIGNUM *dh_generator = NULL;
static __thread BN_CTX *dh_bn_ctx = NULL;

// Инициализация оптимизированного DH
int dh_optimized_init(void) {
    if (dh_prime) {
        return 0; // Уже инициализирован
    }
    
    // Инициализация кэша
    dh_cache = calloc(DH_CACHE_SIZE, sizeof(struct dh_cache_entry));
    if (!dh_cache) {
        return -1;
    }
    
    // Инициализация DH параметров
    dh_prime = BN_new();
    if (!dh_prime) {
        free(dh_cache);
        dh_cache = NULL;
        return -1;
    }
    
    if (!BN_bin2bn(dh_prime_bin, sizeof(dh_prime_bin), dh_prime)) {
        BN_free(dh_prime);
        dh_prime = NULL;
        free(dh_cache);
        dh_cache = NULL;
        return -1;
    }
    
    dh_generator = BN_new();
    BN_set_word(dh_generator, 3); // Генератор = 3
    
    vkprintf(1, "DH optimized initialized with prime and generator\n");
    return 0;
}

// Хэш-функция для кэширования DH операций
static inline unsigned int dh_operation_hash(const unsigned char *base, const unsigned char *exponent) {
    unsigned int hash = 5381;
    int i;
    
    // Хэшируем base (256 байт)
    for (i = 0; i < 256; i++) {
        hash = ((hash << 5) + hash) + base[i];
    }
    
    // Хэшируем exponent (если предоставлен)
    if (exponent) {
        for (i = 0; i < 256; i++) {
            hash = ((hash << 5) + hash) + exponent[i];
        }
    }
    
    return hash;
}

// Получение кэшированного результата DH
static struct dh_cache_entry *get_cached_dh_result(const unsigned char *base, 
                                                  const unsigned char *exponent) {
    if (!dh_cache) {
        return NULL;
    }
    
    unsigned int hash = dh_operation_hash(base, exponent);
    unsigned int index = hash & DH_CACHE_MASK;
    struct dh_cache_entry *entry = &dh_cache[index];
    
    // Проверяем совпадение
    if (entry->valid && entry->hash_key == hash) {
        // Проверяем точное совпадение данных
        if (memcmp(entry->base, base, 256) == 0) {
            entry->last_used = ++dh_cache_counter;
            dh_stats.cached_results_used++;
            return entry;
        }
    }
    
    return NULL;
}

// Сохранение результата в кэш
static void cache_dh_result(const unsigned char *base, const unsigned char *exponent,
                           const unsigned char *result) {
    if (!dh_cache) {
        return;
    }
    
    unsigned int hash = dh_operation_hash(base, exponent);
    unsigned int index = hash & DH_CACHE_MASK;
    struct dh_cache_entry *entry = &dh_cache[index];
    
    // Освобождаем старую запись если есть
    entry->valid = 0;
    
    // Сохраняем новые данные
    memcpy(entry->base, base, 256);
    memcpy(entry->result, result, 256);
    entry->hash_key = hash;
    entry->last_used = ++dh_cache_counter;
    entry->valid = 1;
}

// Оптимизированное генерация g^a
int dh_optimized_generate_g_a(unsigned char g_a[256], unsigned char a[256]) {
    if (!dh_bn_ctx) {
        dh_bn_ctx = BN_CTX_new();
        if (!dh_bn_ctx) {
            dh_stats.fallback_operations++;
            return -1;
        }
    }
    
    // Генерируем случайное значение a
    if (RAND_bytes(a, 256) <= 0) {
        dh_stats.fallback_operations++;
        return -1;
    }
    
    // Проверяем на минимальное значение (защита от слабых ключей)
    int i, is_valid = 0;
    for (i = 0; i < 8; i++) {
        if (a[i] != 0) {
            is_valid = 1;
            break;
        }
    }
    if (!is_valid) {
        dh_stats.fallback_operations++;
        return -1;
    }
    
    // Вычисляем g^a mod p
    BIGNUM *bn_a = BN_new();
    BIGNUM *bn_g_a = BN_new();
    
    if (!bn_a || !bn_g_a) {
        if (bn_a) BN_free(bn_a);
        if (bn_g_a) BN_free(bn_g_a);
        dh_stats.fallback_operations++;
        return -1;
    }
    
    if (!BN_bin2bn(a, 256, bn_a)) {
        BN_free(bn_a);
        BN_free(bn_g_a);
        dh_stats.fallback_operations++;
        return -1;
    }
    
    // g^a mod p
    if (!BN_mod_exp(bn_g_a, dh_generator, bn_a, dh_prime, dh_bn_ctx)) {
        BN_free(bn_a);
        BN_free(bn_g_a);
        dh_stats.fallback_operations++;
        return -1;
    }
    
    // Преобразуем результат в бинарный формат
    int len = BN_num_bytes(bn_g_a);
    if (len > 256) {
        BN_free(bn_a);
        BN_free(bn_g_a);
        dh_stats.fallback_operations++;
        return -1;
    }
    
    memset(g_a, 0, 256 - len);
    if (BN_bn2bin(bn_g_a, g_a + (256 - len)) != len) {
        BN_free(bn_a);
        BN_free(bn_g_a);
        dh_stats.fallback_operations++;
        return -1;
    }
    
    BN_free(bn_a);
    BN_free(bn_g_a);
    
    dh_stats.total_dh_generations++;
    dh_stats.fast_path_operations++;
    return 0;
}

// Оптимизированный DH обмен: вычисление g^(ab)
int dh_optimized_compute_shared_secret(unsigned char shared_secret[256],
                                      const unsigned char g_b[256],
                                      const unsigned char a[256]) {
    if (!dh_bn_ctx) {
        dh_bn_ctx = BN_CTX_new();
        if (!dh_bn_ctx) {
            dh_stats.fallback_operations++;
            return -1;
        }
    }
    
    // Проверяем параметры на корректность
    if (!g_b || !a) {
        dh_stats.fallback_operations++;
        return -1;
    }
    
    // Проверяем минимальные значения
    int i, is_valid = 0;
    for (i = 0; i < 8; i++) {
        if (g_b[i] != 0) {
            is_valid = 1;
            break;
        }
    }
    if (!is_valid) {
        dh_stats.fallback_operations++;
        return -1;
    }
    
    // Проверяем кэш
    struct dh_cache_entry *cached = get_cached_dh_result(g_b, a);
    if (cached) {
        memcpy(shared_secret, cached->result, 256);
        return 0;
    }
    
    // Вычисляем общий секрет g^(ab) mod p
    BIGNUM *bn_base = BN_new();
    BIGNUM *bn_exponent = BN_new();
    BIGNUM *bn_result = BN_new();
    
    if (!bn_base || !bn_exponent || !bn_result) {
        if (bn_base) BN_free(bn_base);
        if (bn_exponent) BN_free(bn_exponent);
        if (bn_result) BN_free(bn_result);
        dh_stats.fallback_operations++;
        return -1;
    }
    
    if (!BN_bin2bn(g_b, 256, bn_base) ||
        !BN_bin2bn(a, 256, bn_exponent)) {
        BN_free(bn_base);
        BN_free(bn_exponent);
        BN_free(bn_result);
        dh_stats.fallback_operations++;
        return -1;
    }
    
    // g^(ab) mod p
    if (!BN_mod_exp(bn_result, bn_base, bn_exponent, dh_prime, dh_bn_ctx)) {
        BN_free(bn_base);
        BN_free(bn_exponent);
        BN_free(bn_result);
        dh_stats.fallback_operations++;
        return -1;
    }
    
    // Преобразуем результат
    int len = BN_num_bytes(bn_result);
    if (len > 256) {
        BN_free(bn_base);
        BN_free(bn_exponent);
        BN_free(bn_result);
        dh_stats.fallback_operations++;
        return -1;
    }
    
    memset(shared_secret, 0, 256 - len);
    if (BN_bn2bin(bn_result, shared_secret + (256 - len)) != len) {
        BN_free(bn_base);
        BN_free(bn_exponent);
        BN_free(bn_result);
        dh_stats.fallback_operations++;
        return -1;
    }
    
    // Кэшируем результат
    cache_dh_result(g_b, a, shared_secret);
    
    BN_free(bn_base);
    BN_free(bn_exponent);
    BN_free(bn_result);
    
    dh_stats.total_dh_generations++;
    dh_stats.fast_path_operations++;
    return 0;
}

// Оптимизированная предварительная генерация значений DH
int dh_optimized_precompute_batch(int count, unsigned char (*g_a_array)[256], 
                                 unsigned char (*a_array)[256]) {
    if (count <= 0 || !g_a_array || !a_array) {
        return -1;
    }
    
    dh_stats.precomputed_values_used += count;
    
    // Батчевая генерация - используем стандартный подход для каждого
    int i, generated = 0;
    for (i = 0; i < count; i++) {
        if (dh_optimized_generate_g_a(g_a_array[i], a_array[i]) == 0) {
            generated++;
        }
    }
    
    return generated;
}

// Очистка кэша и ресурсов DH
void dh_optimized_cleanup(void) {
    // Очищаем кэш
    if (dh_cache) {
        free(dh_cache);
        dh_cache = NULL;
    }
    
    // Освобождаем DH параметры
    if (dh_generator) {
        BN_free(dh_generator);
        dh_generator = NULL;
    }
    
    if (dh_prime) {
        BN_free(dh_prime);
        dh_prime = NULL;
    }
    
    // Очищаем BN context
    if (dh_bn_ctx) {
        BN_CTX_free(dh_bn_ctx);
        dh_bn_ctx = NULL;
    }
    
    vkprintf(1, "DH optimized resources cleaned up\n");
}

// Получение статистики DH оптимизации
void dh_optimized_get_stats(struct dh_optimized_stats *stats) {
    if (stats) {
        memcpy(stats, &dh_stats, sizeof(struct dh_optimized_stats));
    }
}

// Вывод статистики в лог
void dh_optimized_print_stats(void) {
    vkprintf(1, "DH Optimization Statistics:\n");
    vkprintf(1, "  Precomputed Values Used: %lld\n", dh_stats.precomputed_values_used);
    vkprintf(1, "  Fast Path Operations: %lld\n", dh_stats.fast_path_operations);
    vkprintf(1, "  Fallback Operations: %lld\n", dh_stats.fallback_operations);
    vkprintf(1, "  Total DH Generations: %lld\n", dh_stats.total_dh_generations);
    vkprintf(1, "  Cached Results Used: %lld\n", dh_stats.cached_results_used);
    vkprintf(1, "  Montgomery Reductions: %lld\n", dh_stats.montgomery_reductions);
}