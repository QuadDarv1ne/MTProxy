#include "crypto-optimizer.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

// Вспомогательные функции для измерения времени
static double get_current_time_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / frequency.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
#endif
}

// Обнаружение поддерживаемых оптимизаций
int crypto_optimizer_detect_capabilities(void) {
    int capabilities = CRYPTO_OPT_NONE;
    
    // Проверка AES-NI поддержки через CPUID
#ifdef __x86_64__
    unsigned int eax, ebx, ecx, edx;
    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1), "c"(0)
    );
    
    if (ecx & (1 << 25)) {  // AES-NI бит
        capabilities |= CRYPTO_OPT_AES_NI;
    }
    
    // Проверка SSE/AVX
    if (ecx & (1 << 28)) {  // AVX
        capabilities |= CRYPTO_OPT_VECTORIZED;
    } else if (edx & (1 << 25)) {  // SSE
        capabilities |= CRYPTO_OPT_VECTORIZED;
    }
#endif
    
    // Пакетная обработка и предвычисления всегда доступны
    capabilities |= CRYPTO_OPT_BATCH;
    capabilities |= CRYPTO_OPT_PRECOMPUTED;
    
    return capabilities;
}

// Получение лучшей доступной оптимизации
crypto_optimization_t crypto_optimizer_get_best_optimization(void) {
    int capabilities = crypto_optimizer_detect_capabilities();
    
    if (capabilities & CRYPTO_OPT_AES_NI) {
        return CRYPTO_OPT_AES_NI;
    } else if (capabilities & CRYPTO_OPT_VECTORIZED) {
        return CRYPTO_OPT_VECTORIZED;
    } else if (capabilities & CRYPTO_OPT_BATCH) {
        return CRYPTO_OPT_BATCH;
    }
    
    return CRYPTO_OPT_NONE;
}

// Инициализация оптимизатора
crypto_optimizer_t* crypto_optimizer_init(void) {
    crypto_optimizer_t *optimizer = calloc(1, sizeof(crypto_optimizer_t));
    if (!optimizer) {
        return NULL;
    }
    
    // Обнаружение возможностей
    optimizer->supported_optimizations = crypto_optimizer_detect_capabilities();
    optimizer->active_optimization = crypto_optimizer_get_best_optimization();
    
    // Конфигурация по умолчанию
    optimizer->config.enable_aes_ni = 1;
    optimizer->config.enable_vectorization = 1;
    optimizer->config.enable_batching = 1;
    optimizer->config.enable_precomputation = 1;
    optimizer->config.cache_size = 1024;
    optimizer->config.batch_size = 32;
    optimizer->config.optimization_threshold_ms = 0.1; // 100 мкс
    
    // Инициализация кэша ключей
    optimizer->cache_size = optimizer->config.cache_size;
    memset(optimizer->key_cache, 0, sizeof(optimizer->key_cache));
    
    // Инициализация пакетного процессора
    optimizer->batch_processor.max_batch_size = optimizer->config.batch_size;
    optimizer->batch_processor.input_buffers = calloc(optimizer->config.batch_size, sizeof(unsigned char*));
    optimizer->batch_processor.output_buffers = calloc(optimizer->config.batch_size, sizeof(unsigned char*));
    optimizer->batch_processor.buffer_sizes = calloc(optimizer->config.batch_size, sizeof(size_t));
    
    // Инициализация векторизованных буферов
    optimizer->vector_buffers.alignment = 32; // 32-byte alignment for AVX
    optimizer->vector_buffers.buffer_size = 4096;
    optimizer->vector_buffers.aligned_input = aligned_alloc(32, 4096);
    optimizer->vector_buffers.aligned_output = aligned_alloc(32, 4096);
    
    optimizer->is_initialized = 1;
    return optimizer;
}

// Конфигурация оптимизатора
int crypto_optimizer_configure(crypto_optimizer_t *optimizer, 
                              crypto_optimization_t optimization_type) {
    if (!optimizer || !optimizer->is_initialized) {
        return -1;
    }
    
    // Проверка поддержки
    if (!(optimizer->supported_optimizations & optimization_type)) {
        return -1;
    }
    
    optimizer->active_optimization = optimization_type;
    return 0;
}

// Поиск ключа в кэше
static int find_key_in_cache(crypto_optimizer_t *optimizer,
                           const unsigned char *key,
                           const unsigned char *iv) {
    for (int i = 0; i < optimizer->cache_size; i++) {
        if (optimizer->key_cache[i].valid &&
            memcmp(optimizer->key_cache[i].key, key, 32) == 0 &&
            memcmp(optimizer->key_cache[i].iv, iv, 16) == 0) {
            optimizer->key_cache[i].last_used = (unsigned long long)time(NULL);
            return i;
        }
    }
    return -1;
}

// Добавление ключа в кэш
static int add_key_to_cache(crypto_optimizer_t *optimizer,
                          const unsigned char *key,
                          const unsigned char *iv) {
    // Найти наименее используемую запись
    int oldest_index = 0;
    unsigned long long oldest_time = (unsigned long long)time(NULL);
    
    for (int i = 0; i < optimizer->cache_size; i++) {
        if (!optimizer->key_cache[i].valid) {
            oldest_index = i;
            break;
        }
        if (optimizer->key_cache[i].last_used < oldest_time) {
            oldest_time = optimizer->key_cache[i].last_used;
            oldest_index = i;
        }
    }
    
    // Очистить старый контекст
    if (optimizer->key_cache[oldest_index].encrypt_ctx) {
        EVP_CIPHER_CTX_free(optimizer->key_cache[oldest_index].encrypt_ctx);
    }
    if (optimizer->key_cache[oldest_index].decrypt_ctx) {
        EVP_CIPHER_CTX_free(optimizer->key_cache[oldest_index].decrypt_ctx);
    }
    
    // Создать новые контексты
    optimizer->key_cache[oldest_index].encrypt_ctx = EVP_CIPHER_CTX_new();
    optimizer->key_cache[oldest_index].decrypt_ctx = EVP_CIPHER_CTX_new();
    
    if (!optimizer->key_cache[oldest_index].encrypt_ctx || 
        !optimizer->key_cache[oldest_index].decrypt_ctx) {
        return -1;
    }
    
    // Инициализация контекстов
    EVP_EncryptInit_ex(optimizer->key_cache[oldest_index].encrypt_ctx, 
                       EVP_aes_256_cbc(), NULL, key, iv);
    EVP_DecryptInit_ex(optimizer->key_cache[oldest_index].decrypt_ctx, 
                       EVP_aes_256_cbc(), NULL, key, iv);
    
    memcpy(optimizer->key_cache[oldest_index].key, key, 32);
    memcpy(optimizer->key_cache[oldest_index].iv, iv, 16);
    optimizer->key_cache[oldest_index].last_used = (unsigned long long)time(NULL);
    optimizer->key_cache[oldest_index].valid = 1;
    
    return oldest_index;
}

// Оптимизированное шифрование
int crypto_optimized_encrypt(crypto_optimizer_t *optimizer,
                           const unsigned char *key,
                           const unsigned char *iv,
                           const unsigned char *plaintext,
                           size_t plaintext_len,
                           unsigned char *ciphertext,
                           size_t *ciphertext_len) {
    if (!optimizer || !optimizer->is_initialized) {
        return -1;
    }
    
    double start_time = get_current_time_ms();
    optimizer->stats.total_operations++;
    
    // Попытка использовать кэш
    int cache_index = find_key_in_cache(optimizer, key, iv);
    if (cache_index >= 0) {
        optimizer->stats.cache_hits++;
        optimizer->stats.optimized_operations++;
        
        int len;
        EVP_EncryptUpdate(optimizer->key_cache[cache_index].encrypt_ctx,
                         ciphertext, &len, plaintext, plaintext_len);
        *ciphertext_len = len;
        
        EVP_EncryptFinal_ex(optimizer->key_cache[cache_index].encrypt_ctx,
                           ciphertext + len, &len);
        *ciphertext_len += len;
        
    } else {
        optimizer->stats.cache_misses++;
        optimizer->stats.fallback_operations++;
        
        // Fallback на стандартное шифрование
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return -1;
        
        EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
        int len;
        EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);
        *ciphertext_len = len;
        
        EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);
        *ciphertext_len += len;
        
        EVP_CIPHER_CTX_free(ctx);
        
        // Добавить в кэш для будущих операций
        add_key_to_cache(optimizer, key, iv);
    }
    
    double end_time = get_current_time_ms();
    double operation_time = end_time - start_time;
    optimizer->stats.total_processing_time_ms += operation_time;
    
    if (cache_index >= 0) {
        optimizer->stats.optimized_processing_time_ms += operation_time;
    }
    
    return 0;
}

// Оптимизированное дешифрование
int crypto_optimized_decrypt(crypto_optimizer_t *optimizer,
                           const unsigned char *key,
                           const unsigned char *iv,
                           const unsigned char *ciphertext,
                           size_t ciphertext_len,
                           unsigned char *plaintext,
                           size_t *plaintext_len) {
    if (!optimizer || !optimizer->is_initialized) {
        return -1;
    }
    
    optimizer->stats.total_operations++;
    
    // Попытка использовать кэш
    int cache_index = find_key_in_cache(optimizer, key, iv);
    if (cache_index >= 0) {
        optimizer->stats.cache_hits++;
        optimizer->stats.optimized_operations++;
        
        int len;
        EVP_DecryptUpdate(optimizer->key_cache[cache_index].decrypt_ctx,
                         plaintext, &len, ciphertext, ciphertext_len);
        *plaintext_len = len;
        
        EVP_DecryptFinal_ex(optimizer->key_cache[cache_index].decrypt_ctx,
                           plaintext + len, &len);
        *plaintext_len += len;
        
    } else {
        optimizer->stats.cache_misses++;
        optimizer->stats.fallback_operations++;
        
        // Fallback на стандартное дешифрование
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return -1;
        
        EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
        int len;
        EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
        *plaintext_len = len;
        
        EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
        *plaintext_len += len;
        
        EVP_CIPHER_CTX_free(ctx);
        
        // Добавить в кэш
        add_key_to_cache(optimizer, key, iv);
    }
    
    return 0;
}

// Пакетное шифрование
int crypto_batch_encrypt(crypto_optimizer_t *optimizer,
                        const unsigned char *key,
                        const unsigned char *iv,
                        unsigned char **plaintext_array,
                        size_t *plaintext_lengths,
                        int array_size,
                        unsigned char **ciphertext_array,
                        size_t *ciphertext_lengths) {
    if (!optimizer || array_size > optimizer->config.batch_size) {
        return -1;
    }
    
    // Используем кэшированный контекст если доступен
    int cache_index = find_key_in_cache(optimizer, key, iv);
    EVP_CIPHER_CTX *ctx;
    
    if (cache_index >= 0) {
        ctx = optimizer->key_cache[cache_index].encrypt_ctx;
    } else {
        ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return -1;
        EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    }
    
    // Обработка пакета
    for (int i = 0; i < array_size; i++) {
        int len;
        EVP_EncryptUpdate(ctx, ciphertext_array[i], &len, 
                         plaintext_array[i], plaintext_lengths[i]);
        ciphertext_lengths[i] = len;
        
        EVP_EncryptFinal_ex(ctx, ciphertext_array[i] + len, &len);
        ciphertext_lengths[i] += len;
    }
    
    if (cache_index < 0) {
        EVP_CIPHER_CTX_free(ctx);
    } else {
        // Сброс контекста для следующего использования
        EVP_CIPHER_CTX_reset(optimizer->key_cache[cache_index].encrypt_ctx);
        EVP_EncryptInit_ex(optimizer->key_cache[cache_index].encrypt_ctx, 
                          EVP_aes_256_cbc(), NULL, key, iv);
    }
    
    return 0;
}

// Очистка оптимизатора
void crypto_optimizer_cleanup(crypto_optimizer_t *optimizer) {
    if (!optimizer) return;
    
    // Очистка кэша ключей
    for (int i = 0; i < optimizer->cache_size; i++) {
        if (optimizer->key_cache[i].encrypt_ctx) {
            EVP_CIPHER_CTX_free(optimizer->key_cache[i].encrypt_ctx);
        }
        if (optimizer->key_cache[i].decrypt_ctx) {
            EVP_CIPHER_CTX_free(optimizer->key_cache[i].decrypt_ctx);
        }
    }
    
    // Очистка пакетного процессора
    free(optimizer->batch_processor.input_buffers);
    free(optimizer->batch_processor.output_buffers);
    free(optimizer->batch_processor.buffer_sizes);
    
    // Очистка векторизованных буферов
    free(optimizer->vector_buffers.aligned_input);
    free(optimizer->vector_buffers.aligned_output);
    
    free(optimizer);
}

// Печать статистики
void crypto_optimizer_print_stats(crypto_optimizer_t *optimizer) {
    if (!optimizer) return;
    
    printf("=== Crypto Optimizer Statistics ===\n");
    printf("Total operations: %lld\n", optimizer->stats.total_operations);
    printf("Optimized operations: %lld\n", optimizer->stats.optimized_operations);
    printf("Fallback operations: %lld\n", optimizer->stats.fallback_operations);
    printf("Cache hits: %lld\n", optimizer->stats.cache_hits);
    printf("Cache misses: %lld\n", optimizer->stats.cache_misses);
    
    if (optimizer->stats.total_operations > 0) {
        double cache_hit_rate = (double)optimizer->stats.cache_hits * 100.0 / 
                               optimizer->stats.total_operations;
        printf("Cache hit rate: %.2f%%\n", cache_hit_rate);
    }
    
    if (optimizer->stats.optimized_operations > 0) {
        double avg_optimized_time = optimizer->stats.optimized_processing_time_ms / 
                                   optimizer->stats.optimized_operations;
        printf("Average optimized time: %.3f ms\n", avg_optimized_time);
    }
    
    printf("Total processing time: %.3f ms\n", optimizer->stats.total_processing_time_ms);
    printf("==================================\n");
}

// Сброс статистики
void crypto_optimizer_reset_stats(crypto_optimizer_t *optimizer) {
    if (!optimizer) return;
    memset(&optimizer->stats, 0, sizeof(optimizer->stats));
}