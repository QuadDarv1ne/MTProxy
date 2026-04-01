#ifndef CRYPTO_OPTIMIZER_H
#define CRYPTO_OPTIMIZER_H

#include <stdint.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

// Типы оптимизации
typedef enum {
    CRYPTO_OPT_NONE = 0,
    CRYPTO_OPT_AES_NI,          // AES-NI инструкции
    CRYPTO_OPT_VECTORIZED,      // Векторизация (SSE/AVX)
    CRYPTO_OPT_PARALLEL,        // Параллельная обработка
    CRYPTO_OPT_BATCH,           // Пакетная обработка
    CRYPTO_OPT_PRECOMPUTED      // Предвычисленные ключи
} crypto_optimization_t;

// Статистика оптимизации
typedef struct {
    long long total_operations;
    long long optimized_operations;
    long long fallback_operations;
    long long cache_hits;
    long long cache_misses;
    double avg_optimization_ratio;
    double total_processing_time_ms;
    double optimized_processing_time_ms;
} crypto_optimization_stats_t;

// Контекст оптимизированной криптографии
typedef struct {
    // Поддерживаемые оптимизации
    int supported_optimizations;
    crypto_optimization_t active_optimization;
    
    // Кэш ключей
    struct {
        unsigned char key[32];
        unsigned char iv[16];
        EVP_CIPHER_CTX *encrypt_ctx;
        EVP_CIPHER_CTX *decrypt_ctx;
        unsigned long long last_used;
        int valid;
    } key_cache[1024];
    int cache_size;
    
    // Пакетная обработка
    struct {
        unsigned char **input_buffers;
        unsigned char **output_buffers;
        size_t *buffer_sizes;
        int batch_size;
        int max_batch_size;
    } batch_processor;
    
    // Векторизованные буферы
    struct {
        unsigned char *aligned_input;
        unsigned char *aligned_output;
        size_t buffer_size;
        int alignment;
    } vector_buffers;
    
    // Статистика
    crypto_optimization_stats_t stats;
    
    // Конфигурация
    struct {
        int enable_aes_ni;
        int enable_vectorization;
        int enable_batching;
        int enable_precomputation;
        int cache_size;
        int batch_size;
        double optimization_threshold_ms;
    } config;
    
    int is_initialized;
} crypto_optimizer_t;

// Инициализация оптимизатора
crypto_optimizer_t* crypto_optimizer_init(void);
int crypto_optimizer_configure(crypto_optimizer_t *optimizer, 
                              crypto_optimization_t optimization_type);
void crypto_optimizer_cleanup(crypto_optimizer_t *optimizer);

// Оптимизированные криптографические операции
int crypto_optimized_encrypt(crypto_optimizer_t *optimizer,
                           const unsigned char *key,
                           const unsigned char *iv,
                           const unsigned char *plaintext,
                           size_t plaintext_len,
                           unsigned char *ciphertext,
                           size_t *ciphertext_len);

int crypto_optimized_decrypt(crypto_optimizer_t *optimizer,
                           const unsigned char *key,
                           const unsigned char *iv,
                           const unsigned char *ciphertext,
                           size_t ciphertext_len,
                           unsigned char *plaintext,
                           size_t *plaintext_len);

// Пакетная обработка
int crypto_batch_encrypt(crypto_optimizer_t *optimizer,
                        const unsigned char *key,
                        const unsigned char *iv,
                        unsigned char **plaintext_array,
                        size_t *plaintext_lengths,
                        int array_size,
                        unsigned char **ciphertext_array,
                        size_t *ciphertext_lengths);

int crypto_batch_decrypt(crypto_optimizer_t *optimizer,
                        const unsigned char *key,
                        const unsigned char *iv,
                        unsigned char **ciphertext_array,
                        size_t *ciphertext_lengths,
                        int array_size,
                        unsigned char **plaintext_array,
                        size_t *plaintext_lengths);

// Утилиты
int crypto_optimizer_detect_capabilities(void);
crypto_optimization_t crypto_optimizer_get_best_optimization(void);
void crypto_optimizer_print_stats(crypto_optimizer_t *optimizer);
void crypto_optimizer_reset_stats(crypto_optimizer_t *optimizer);

// Интеграция с существующими системами
int crypto_optimizer_integrate_with_aes(crypto_optimizer_t *optimizer);
int crypto_optimizer_integrate_with_dh(crypto_optimizer_t *optimizer);

#endif // CRYPTO_OPTIMIZER_H