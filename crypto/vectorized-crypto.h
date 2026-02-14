/*
 * Векторизованные криптографические операции для MTProxy
 * Использование AVX2/AVX-512 для оптимизации криптографических операций
 */

#ifndef _VECTORIZED_CRYPTO_H_
#define _VECTORIZED_CRYPTO_H_

#include <stdint.h>

// Поддерживаемые SIMD инструкции
typedef enum {
    SIMD_NONE = 0,
    SIMD_SSE = 1,
    SIMD_AVX = 2,
    SIMD_AVX2 = 3,
    SIMD_AVX512 = 4
} simd_instruction_set_t;

// Статус векторизованной криптографии
typedef enum {
    VEC_CRYPTO_STATUS_UNINITIALIZED = 0,
    VEC_CRYPTO_STATUS_INITIALIZED = 1,
    VEC_CRYPTO_STATUS_AVAILABLE = 2,
    VEC_CRYPTO_STATUS_ERROR = 3
} vec_crypto_status_t;

// Статистика векторизованных операций
typedef struct {
    long long total_operations;
    long long avx2_operations;
    long long avx512_operations;
    long long sse_operations;
    long long fallback_operations;
    long long performance_improvements;
    vec_crypto_status_t current_status;
    int current_simd_level;
    double performance_gain_percent;
} vectorized_crypto_stats_t;

// Конфигурация векторизованной криптографии
typedef struct {
    int enable_vectorization;
    simd_instruction_set_t preferred_simd_level;
    int enable_aes_ni;
    int enable_avx2;
    int enable_avx512;
    int auto_detect_simd;
    int force_simd_level;
    int enable_prefetching;
    int vector_chunk_size;
} vectorized_crypto_config_t;

// Контекст векторизованной криптографии
typedef struct {
    vectorized_crypto_config_t config;
    vectorized_crypto_stats_t stats;
    vec_crypto_status_t status;
    simd_instruction_set_t detected_simd;
    void *simd_functions;
    int simd_available[5]; // NONE, SSE, AVX, AVX2, AVX512
    int initialized;
    int cpu_features;
} vectorized_crypto_context_t;

// Функции инициализации
int vec_crypto_init(vectorized_crypto_context_t *ctx);
int vec_crypto_init_with_config(vectorized_crypto_context_t *ctx, 
                               const vectorized_crypto_config_t *config);
void vec_crypto_cleanup(vectorized_crypto_context_t *ctx);

// Функции AES векторизации
int vec_crypto_aes_ecb_encrypt(const unsigned char *in, unsigned char *out, 
                              const unsigned char *key, int key_bits, 
                              size_t data_len);
int vec_crypto_aes_ecb_decrypt(const unsigned char *in, unsigned char *out, 
                              const unsigned char *key, int key_bits, 
                              size_t data_len);
int vec_crypto_aes_ctr_encrypt(const unsigned char *in, unsigned char *out, 
                              const unsigned char *key, int key_bits, 
                              unsigned char *counter, size_t data_len);
int vec_crypto_aes_gcm_encrypt(const unsigned char *in, unsigned char *out, 
                              const unsigned char *key, int key_bits,
                              const unsigned char *iv, size_t iv_len,
                              const unsigned char *aad, size_t aad_len,
                              unsigned char *tag, size_t tag_len,
                              size_t data_len);

// Функции SHA векторизации
int vec_crypto_sha256_process(const unsigned char *data, size_t data_len, 
                             unsigned char *hash);
int vec_crypto_sha512_process(const unsigned char *data, size_t data_len, 
                             unsigned char *hash);

// Функции определения SIMD
simd_instruction_set_t vec_crypto_detect_simd(void);
int vec_crypto_is_simd_available(simd_instruction_set_t simd_level);

// Функции векторизованной обработки данных
int vec_crypto_process_blocks(const unsigned char *in, unsigned char *out, 
                            size_t data_len, 
                            int (*process_func)(const unsigned char*, unsigned char*, size_t));

// Функции статистики
vectorized_crypto_stats_t vec_crypto_get_stats(vectorized_crypto_context_t *ctx);
void vec_crypto_reset_stats(vectorized_crypto_context_t *ctx);

// Функции конфигурации
void vec_crypto_get_config(vectorized_crypto_context_t *ctx, 
                          vectorized_crypto_config_t *config);
int vec_crypto_update_config(vectorized_crypto_context_t *ctx, 
                            const vectorized_crypto_config_t *new_config);

// Вспомогательные функции
int vec_crypto_is_available(void);
int vec_crypto_get_simd_level(vectorized_crypto_context_t *ctx);
const char* vec_crypto_get_simd_string(simd_instruction_set_t simd_level);
int vec_crypto_get_optimal_chunk_size(vectorized_crypto_context_t *ctx);

#endif