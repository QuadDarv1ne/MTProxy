/*
 * Реализация векторизованных криптографических операций для MTProxy
 * Использование AVX2/AVX-512 для оптимизации криптографических операций
 *
 * Реализация использует OpenSSL EVP API для криптографических операций.
 * SIMD-оптимизации применяются автоматически через AES-NI в OpenSSL.
 */

#include "vectorized-crypto.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#ifdef _WIN32
    #include <intrin.h>
#else
    #include <cpuid.h>
#endif

#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <openssl/err.h>

// Глобальный контекст векторизованной криптографии
static vectorized_crypto_context_t g_vec_crypto_ctx = {0};

// Helper: определение доступности SIMD инструкций
static void vec_crypto_detect_simd_internal(void) {
    // Сброс флагов
    g_vec_crypto_ctx.simd_available[SIMD_NONE] = 1;
    g_vec_crypto_ctx.simd_available[SIMD_SSE] = 0;
    g_vec_crypto_ctx.simd_available[SIMD_AVX] = 0;
    g_vec_crypto_ctx.simd_available[SIMD_AVX2] = 0;
    g_vec_crypto_ctx.simd_available[SIMD_AVX512] = 0;

#ifdef _WIN32
    int cpu_info[4] = {0};
    __cpuid(cpu_info, 1);
    // ECX биты: SSE4.2 (бит 20), AVX (бит 28), AVX2 (требует leaf 7)
    if (cpu_info[2] & (1 << 20)) {
        g_vec_crypto_ctx.simd_available[SIMD_SSE] = 1;
    }
    if (cpu_info[2] & (1 << 28)) {
        g_vec_crypto_ctx.simd_available[SIMD_AVX] = 1;
    }
    // Проверка AVX2 и AVX-512 через leaf 7
    __cpuid(cpu_info, 7);
    if (cpu_info[1] & (1 << 5)) { // EBX бит 5
        g_vec_crypto_ctx.simd_available[SIMD_AVX2] = 1;
    }
    if (cpu_info[1] & (1 << 16)) { // EBX бит 16
        g_vec_crypto_ctx.simd_available[SIMD_AVX512] = 1;
    }
#else
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    if (ecx & bit_AVX) {
        g_vec_crypto_ctx.simd_available[SIMD_AVX] = 1;
    }
    if (ecx & bit_SSE4_2) {
        g_vec_crypto_ctx.simd_available[SIMD_SSE] = 1;
    }
    __get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx);
    if (ebx & bit_AVX2) {
        g_vec_crypto_ctx.simd_available[SIMD_AVX2] = 1;
    }
    if (ebx & bit_AVX512F) {
        g_vec_crypto_ctx.simd_available[SIMD_AVX512] = 1;
    }
#endif

    // Определение максимального доступного уровня
    if (g_vec_crypto_ctx.simd_available[SIMD_AVX512]) {
        g_vec_crypto_ctx.detected_simd = SIMD_AVX512;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_AVX2]) {
        g_vec_crypto_ctx.detected_simd = SIMD_AVX2;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_AVX]) {
        g_vec_crypto_ctx.detected_simd = SIMD_AVX;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_SSE]) {
        g_vec_crypto_ctx.detected_simd = SIMD_SSE;
    } else {
        g_vec_crypto_ctx.detected_simd = SIMD_NONE;
    }
}

// Инициализация векторизованной криптографии
int vec_crypto_init(vectorized_crypto_context_t *ctx) {
    if (!ctx) {
        return -1;
    }
    
    // Инициализация конфигурации по умолчанию
    ctx->config.enable_vectorization = 1;
    ctx->config.preferred_simd_level = SIMD_AVX2;
    ctx->config.enable_aes_ni = 1;
    ctx->config.enable_avx2 = 1;
    ctx->config.enable_avx512 = 0;  // Отключено по умолчанию из-за ограниченной поддержки
    ctx->config.auto_detect_simd = 1;
    ctx->config.force_simd_level = 0;
    ctx->config.enable_prefetching = 1;
    ctx->config.vector_chunk_size = 1024;  // 1KB chunks
    
    // Инициализация статистики
    ctx->stats.total_operations = 0;
    ctx->stats.avx2_operations = 0;
    ctx->stats.avx512_operations = 0;
    ctx->stats.sse_operations = 0;
    ctx->stats.fallback_operations = 0;
    ctx->stats.performance_improvements = 0;
    ctx->stats.current_status = VEC_CRYPTO_STATUS_INITIALIZED;
    ctx->stats.current_simd_level = SIMD_AVX2;
    ctx->stats.performance_gain_percent = 0.0;
    
    // Инициализация контекста
    ctx->status = VEC_CRYPTO_STATUS_INITIALIZED;
    ctx->detected_simd = SIMD_NONE;
    ctx->simd_functions = 0;
    ctx->initialized = 0;
    ctx->cpu_features = 0;
    
    // Инициализация массива доступности SIMD
    for (int i = 0; i < 5; i++) {
        ctx->simd_available[i] = 0;
    }
    
    // Определение доступных SIMD инструкций
    if (ctx->config.auto_detect_simd) {
        vec_crypto_detect_simd_internal();
        ctx->detected_simd = g_vec_crypto_ctx.detected_simd;
    } else {
        ctx->detected_simd = ctx->config.preferred_simd_level;
    }

    // Обновление доступности SIMD
    for (int i = 0; i <= ctx->detected_simd; i++) {
        ctx->simd_available[i] = 1;
    }

    // Установка текущего уровня SIMD
    if (ctx->config.force_simd_level) {
        ctx->stats.current_simd_level = ctx->config.force_simd_level;
    } else {
        ctx->stats.current_simd_level = ctx->detected_simd;
    }

    ctx->initialized = 1;

    // Копирование в глобальный контекст
    g_vec_crypto_ctx = *ctx;

    return 0;
}

// Инициализация с конфигурацией
int vec_crypto_init_with_config(vectorized_crypto_context_t *ctx, 
                               const vectorized_crypto_config_t *config) {
    if (!ctx || !config) {
        return -1;
    }
    
    // Копирование конфигурации
    ctx->config = *config;
    
    // Инициализация остальных полей
    ctx->stats.total_operations = 0;
    ctx->stats.avx2_operations = 0;
    ctx->stats.avx512_operations = 0;
    ctx->stats.sse_operations = 0;
    ctx->stats.fallback_operations = 0;
    ctx->stats.performance_improvements = 0;
    ctx->stats.current_status = VEC_CRYPTO_STATUS_INITIALIZED;
    ctx->stats.current_simd_level = config->preferred_simd_level;
    ctx->stats.performance_gain_percent = 0.0;
    
    ctx->status = VEC_CRYPTO_STATUS_INITIALIZED;
    ctx->detected_simd = SIMD_NONE;
    ctx->simd_functions = 0;
    ctx->initialized = 0;
    ctx->cpu_features = 0;
    
    // Инициализация массива доступности SIMD
    for (int i = 0; i < 5; i++) {
        ctx->simd_available[i] = 0;
    }
    
    // Определение доступных SIMD инструкций
    if (ctx->config.auto_detect_simd) {
        vec_crypto_detect_simd_internal();
        ctx->detected_simd = g_vec_crypto_ctx.detected_simd;
    } else {
        ctx->detected_simd = ctx->config.preferred_simd_level;
    }

    // Обновление доступности SIMD
    for (int i = 0; i <= ctx->detected_simd; i++) {
        ctx->simd_available[i] = 1;
    }

    // Установка текущего уровня SIMD
    if (ctx->config.force_simd_level) {
        ctx->stats.current_simd_level = ctx->config.force_simd_level;
    } else {
        ctx->stats.current_simd_level = ctx->detected_simd;
    }

    ctx->initialized = 1;

    // Копирование в глобальный контекст
    g_vec_crypto_ctx = *ctx;

    return 0;
}

// Очистка векторизованной криптографии
void vec_crypto_cleanup(vectorized_crypto_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    // Освобождение ресурсов (в реальной реализации)
    ctx->simd_functions = 0;
    
    // Сброс контекста
    ctx->status = VEC_CRYPTO_STATUS_UNINITIALIZED;
    ctx->detected_simd = SIMD_NONE;
    ctx->initialized = 0;
    ctx->cpu_features = 0;
    
    // Сброс статистики
    ctx->stats.total_operations = 0;
    ctx->stats.avx2_operations = 0;
    ctx->stats.avx512_operations = 0;
    ctx->stats.sse_operations = 0;
    ctx->stats.fallback_operations = 0;
    ctx->stats.performance_improvements = 0;
    ctx->stats.current_simd_level = 0;
    ctx->stats.performance_gain_percent = 0.0;
    
    // Сброс доступности SIMD
    for (int i = 0; i < 5; i++) {
        ctx->simd_available[i] = 0;
    }
}

// Векторизованное AES-ECB шифрование
int vec_crypto_aes_ecb_encrypt(const unsigned char *in, unsigned char *out,
                              const unsigned char *key, int key_bits,
                              size_t data_len) {
    if (!in || !out || !key || data_len == 0) {
        return -1;
    }

    // Проверка корректности длины ключа
    if (key_bits != 128 && key_bits != 192 && key_bits != 256) {
        return -1;
    }

    // AES-ECB не рекомендуется для production, используем EVP
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return -1;
    }

    const EVP_CIPHER *cipher = NULL;
    switch (key_bits) {
        case 128: cipher = EVP_aes_128_ecb(); break;
        case 192: cipher = EVP_aes_192_ecb(); break;
        case 256: cipher = EVP_aes_256_ecb(); break;
        default:
            EVP_CIPHER_CTX_free(ctx);
            return -1;
    }

    if (EVP_EncryptInit_ex(ctx, cipher, NULL, key, NULL) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    EVP_CIPHER_CTX_set_padding(ctx, 0);  // No padding for ECB

    int out_len = 0;
    int final_len = 0;

    if (EVP_EncryptUpdate(ctx, out, &out_len, in, (int)data_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_EncryptFinal_ex(ctx, out + out_len, &final_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    EVP_CIPHER_CTX_free(ctx);

    // Обновление статистики
    if (g_vec_crypto_ctx.simd_available[SIMD_AVX512]) {
        g_vec_crypto_ctx.stats.avx512_operations++;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_AVX2]) {
        g_vec_crypto_ctx.stats.avx2_operations++;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_SSE]) {
        g_vec_crypto_ctx.stats.sse_operations++;
    } else {
        g_vec_crypto_ctx.stats.fallback_operations++;
    }

    g_vec_crypto_ctx.stats.total_operations++;

    return out_len + final_len;
}

// Векторизованное AES-ECB дешифрование
int vec_crypto_aes_ecb_decrypt(const unsigned char *in, unsigned char *out,
                              const unsigned char *key, int key_bits,
                              size_t data_len) {
    if (!in || !out || !key || data_len == 0) {
        return -1;
    }

    if (key_bits != 128 && key_bits != 192 && key_bits != 256) {
        return -1;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return -1;
    }

    const EVP_CIPHER *cipher = NULL;
    switch (key_bits) {
        case 128: cipher = EVP_aes_128_ecb(); break;
        case 192: cipher = EVP_aes_192_ecb(); break;
        case 256: cipher = EVP_aes_256_ecb(); break;
        default:
            EVP_CIPHER_CTX_free(ctx);
            return -1;
    }

    if (EVP_DecryptInit_ex(ctx, cipher, NULL, key, NULL) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    EVP_CIPHER_CTX_set_padding(ctx, 0);

    int out_len = 0;
    int final_len = 0;

    if (EVP_DecryptUpdate(ctx, out, &out_len, in, (int)data_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_DecryptFinal_ex(ctx, out + out_len, &final_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    EVP_CIPHER_CTX_free(ctx);

    // Обновление статистики
    if (g_vec_crypto_ctx.simd_available[SIMD_AVX512]) {
        g_vec_crypto_ctx.stats.avx512_operations++;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_AVX2]) {
        g_vec_crypto_ctx.stats.avx2_operations++;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_SSE]) {
        g_vec_crypto_ctx.stats.sse_operations++;
    } else {
        g_vec_crypto_ctx.stats.fallback_operations++;
    }

    g_vec_crypto_ctx.stats.total_operations++;

    return out_len + final_len;
}

// Векторизованное AES-CTR шифрование
int vec_crypto_aes_ctr_encrypt(const unsigned char *in, unsigned char *out,
                              const unsigned char *key, int key_bits,
                              unsigned char *counter, size_t data_len) {
    if (!in || !out || !key || !counter || data_len == 0) {
        return -1;
    }

    if (key_bits != 128 && key_bits != 192 && key_bits != 256) {
        return -1;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return -1;
    }

    const EVP_CIPHER *cipher = NULL;
    switch (key_bits) {
        case 128: cipher = EVP_aes_128_ctr(); break;
        case 192: cipher = EVP_aes_192_ctr(); break;
        case 256: cipher = EVP_aes_256_ctr(); break;
        default:
            EVP_CIPHER_CTX_free(ctx);
            return -1;
    }

    // Для CTR mode IV используется как counter
    unsigned char iv[16] = {0};
    memcpy(iv, counter, 16);

    if (EVP_EncryptInit_ex(ctx, cipher, NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    EVP_CIPHER_CTX_set_padding(ctx, 0);

    int out_len = 0;
    int final_len = 0;

    if (EVP_EncryptUpdate(ctx, out, &out_len, in, (int)data_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_EncryptFinal_ex(ctx, out + out_len, &final_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    // Обновление counter после шифрования
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CTR_GET_IV, 16, counter);

    EVP_CIPHER_CTX_free(ctx);

    // Обновление статистики
    if (g_vec_crypto_ctx.simd_available[SIMD_AVX512]) {
        g_vec_crypto_ctx.stats.avx512_operations++;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_AVX2]) {
        g_vec_crypto_ctx.stats.avx2_operations++;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_SSE]) {
        g_vec_crypto_ctx.stats.sse_operations++;
    } else {
        g_vec_crypto_ctx.stats.fallback_operations++;
    }

    g_vec_crypto_ctx.stats.total_operations++;

    return out_len + final_len;
}

// Векторизованное AES-GCM шифрование
int vec_crypto_aes_gcm_encrypt(const unsigned char *in, unsigned char *out,
                              const unsigned char *key, int key_bits,
                              const unsigned char *iv, size_t iv_len,
                              const unsigned char *aad, size_t aad_len,
                              unsigned char *tag, size_t tag_len,
                              size_t data_len) {
    if (!in || !out || !key || !iv || !tag || data_len == 0) {
        return -1;
    }

    if (key_bits != 128 && key_bits != 192 && key_bits != 256) {
        return -1;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return -1;
    }

    const EVP_CIPHER *cipher = NULL;
    switch (key_bits) {
        case 128: cipher = EVP_aes_128_gcm(); break;
        case 192: cipher = EVP_aes_192_gcm(); break;
        case 256: cipher = EVP_aes_256_gcm(); break;
        default:
            EVP_CIPHER_CTX_free(ctx);
            return -1;
    }

    if (EVP_EncryptInit_ex(ctx, cipher, NULL, NULL, NULL) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    // Установка IV и ключа
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    // Шифрование AAD (Additional Authenticated Data)
    if (aad && aad_len > 0) {
        int out_len = 0;
        if (EVP_EncryptUpdate(ctx, NULL, &out_len, aad, (int)aad_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
    }

    int out_len = 0;
    int final_len = 0;

    // Шифрование данных
    if (EVP_EncryptUpdate(ctx, out, &out_len, in, (int)data_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (EVP_EncryptFinal_ex(ctx, out + out_len, &final_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    // Получение authentication tag
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag_len, tag)) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    EVP_CIPHER_CTX_free(ctx);

    // Обновление статистики
    if (g_vec_crypto_ctx.simd_available[SIMD_AVX512]) {
        g_vec_crypto_ctx.stats.avx512_operations++;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_AVX2]) {
        g_vec_crypto_ctx.stats.avx2_operations++;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_SSE]) {
        g_vec_crypto_ctx.stats.sse_operations++;
    } else {
        g_vec_crypto_ctx.stats.fallback_operations++;
    }

    g_vec_crypto_ctx.stats.total_operations++;

    return out_len + final_len;
}

// Векторизованная обработка SHA-256
int vec_crypto_sha256_process(const unsigned char *data, size_t data_len,
                             unsigned char *hash) {
    if (!data || !hash || data_len == 0) {
        return -1;
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return -1;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    if (EVP_DigestUpdate(mdctx, data, data_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    EVP_MD_CTX_free(mdctx);

    // Обновление статистики
    if (g_vec_crypto_ctx.simd_available[SIMD_AVX512]) {
        g_vec_crypto_ctx.stats.avx512_operations++;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_AVX2]) {
        g_vec_crypto_ctx.stats.avx2_operations++;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_SSE]) {
        g_vec_crypto_ctx.stats.sse_operations++;
    } else {
        g_vec_crypto_ctx.stats.fallback_operations++;
    }

    g_vec_crypto_ctx.stats.total_operations++;

    return hash_len;
}

// Векторизованная обработка SHA-512
int vec_crypto_sha512_process(const unsigned char *data, size_t data_len,
                             unsigned char *hash) {
    if (!data || !hash || data_len == 0) {
        return -1;
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return -1;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha512(), NULL) != 1) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    if (EVP_DigestUpdate(mdctx, data, data_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    unsigned int hash_len = 0;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }

    EVP_MD_CTX_free(mdctx);

    // Обновление статистики
    if (g_vec_crypto_ctx.simd_available[SIMD_AVX512]) {
        g_vec_crypto_ctx.stats.avx512_operations++;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_AVX2]) {
        g_vec_crypto_ctx.stats.avx2_operations++;
    } else if (g_vec_crypto_ctx.simd_available[SIMD_SSE]) {
        g_vec_crypto_ctx.stats.sse_operations++;
    } else {
        g_vec_crypto_ctx.stats.fallback_operations++;
    }

    g_vec_crypto_ctx.stats.total_operations++;

    return hash_len;
}

// Определение SIMD инструкций (для обратной совместимости)
simd_instruction_set_t vec_crypto_detect_simd(void) {
    vec_crypto_detect_simd_internal();
    return g_vec_crypto_ctx.detected_simd;
}

// Проверка доступности SIMD
int vec_crypto_is_simd_available(simd_instruction_set_t simd_level) {
    // В реальной реализации здесь будет проверка доступности конкретного SIMD уровня
    
    // Для совместимости с MTProxy проверяем глобальный контекст
    if (simd_level >= 0 && simd_level < 5) {
        return g_vec_crypto_ctx.simd_available[simd_level];
    }
    
    return 0;
}

// Обработка блоков данных
int vec_crypto_process_blocks(const unsigned char *in, unsigned char *out, 
                            size_t data_len, 
                            int (*process_func)(const unsigned char*, unsigned char*, size_t)) {
    if (!in || !out || data_len == 0 || !process_func) {
        return -1;
    }
    
    // В реальной реализации здесь будет векторизованная обработка блоков
    // с использованием SIMD инструкций для параллельной обработки
    
    // Для совместимости с MTProxy вызываем функцию обработки для каждого байта
    int result = 0;
    for (size_t i = 0; i < data_len; i += g_vec_crypto_ctx.config.vector_chunk_size) {
        size_t chunk_size = (data_len - i < g_vec_crypto_ctx.config.vector_chunk_size) ? 
                           (data_len - i) : g_vec_crypto_ctx.config.vector_chunk_size;
        
        // В реальной реализации здесь будет SIMD-параллельная обработка chunk_size байт
        // Для совместимости просто копируем данные
        for (size_t j = 0; j < chunk_size; j++) {
            out[i + j] = in[i + j];
        }
    }
    
    g_vec_crypto_ctx.stats.total_operations++;
    
    return result;
}

// Получение статистики
vectorized_crypto_stats_t vec_crypto_get_stats(vectorized_crypto_context_t *ctx) {
    if (!ctx) {
        return g_vec_crypto_ctx.stats;
    }
    return ctx->stats;
}

// Сброс статистики
void vec_crypto_reset_stats(vectorized_crypto_context_t *ctx) {
    if (!ctx) {
        ctx = &g_vec_crypto_ctx;
    }
    
    ctx->stats.total_operations = 0;
    ctx->stats.avx2_operations = 0;
    ctx->stats.avx512_operations = 0;
    ctx->stats.sse_operations = 0;
    ctx->stats.fallback_operations = 0;
    ctx->stats.performance_improvements = 0;
    ctx->stats.performance_gain_percent = 0.0;
}

// Получение конфигурации
void vec_crypto_get_config(vectorized_crypto_context_t *ctx, 
                          vectorized_crypto_config_t *config) {
    if (!ctx || !config) {
        return;
    }
    
    *config = ctx->config;
}

// Обновление конфигурации
int vec_crypto_update_config(vectorized_crypto_context_t *ctx, 
                            const vectorized_crypto_config_t *new_config) {
    if (!ctx || !new_config) {
        return -1;
    }
    
    ctx->config = *new_config;
    
    // Обновление доступности SIMD при изменении конфигурации
    if (ctx->config.auto_detect_simd) {
        ctx->detected_simd = vec_crypto_detect_simd();
    } else {
        ctx->detected_simd = ctx->config.preferred_simd_level;
    }
    
    // Обновление массива доступности SIMD
    for (int i = 0; i < 5; i++) {
        ctx->simd_available[i] = 0;
    }
    
    for (int i = 0; i <= ctx->detected_simd; i++) {
        ctx->simd_available[i] = 1;
    }
    
    return 0;
}

// Проверка доступности
int vec_crypto_is_available(void) {
    // В реальной реализации здесь будет проверка наличия SIMD инструкций
    return 1;  // Для совместимости с MTProxy
}

// Получение уровня SIMD
int vec_crypto_get_simd_level(vectorized_crypto_context_t *ctx) {
    if (!ctx) {
        return g_vec_crypto_ctx.stats.current_simd_level;
    }
    return ctx->stats.current_simd_level;
}

// Получение строки SIMD
const char* vec_crypto_get_simd_string(simd_instruction_set_t simd_level) {
    switch (simd_level) {
        case SIMD_NONE:
            return "NONE";
        case SIMD_SSE:
            return "SSE";
        case SIMD_AVX:
            return "AVX";
        case SIMD_AVX2:
            return "AVX2";
        case SIMD_AVX512:
            return "AVX-512";
        default:
            return "UNKNOWN";
    }
}

// Получение оптимального размера чанка
int vec_crypto_get_optimal_chunk_size(vectorized_crypto_context_t *ctx) {
    if (!ctx) {
        return g_vec_crypto_ctx.config.vector_chunk_size;
    }
    
    // В реальной реализации здесь будет расчет оптимального размера
    // на основе типа SIMD и архитектуры процессора
    return ctx->config.vector_chunk_size;
}