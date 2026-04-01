/*
 * Crypto Performance Optimizer Header for MTProxy
 * Provides optimized cryptographic operations with caching and batching
 */

#ifndef CRYPTO_PERFORMANCE_OPTIMIZER_H
#define CRYPTO_PERFORMANCE_OPTIMIZER_H

#include <stddef.h>

// Crypto optimization levels
typedef enum {
    CRYPTO_OPT_NONE = 0,
    CRYPTO_OPT_BASIC,
    CRYPTO_OPT_ADVANCED,
    CRYPTO_OPT_MAXIMUM
} crypto_opt_level_t;

// Key cache entry
typedef struct {
    unsigned char key[32];
    unsigned char iv[16];
    void *crypto_context;
    unsigned long long last_used;
    int use_count;
    int valid;
} key_cache_entry_t;

// Crypto batch processor
typedef struct {
    unsigned char **input_buffers;
    unsigned char **output_buffers;
    size_t *buffer_sizes;
    int batch_size;
    int max_batch_size;
    int current_count;
} crypto_batch_processor_t;

// Performance statistics
typedef struct {
    long long total_encryptions;
    long long total_decryptions;
    long long key_cache_hits;
    long long key_cache_misses;
    long long batch_operations;
    long long fallback_operations;
    double avg_encryption_time;
    double avg_decryption_time;
} crypto_perf_stats_t;

// Crypto optimizer configuration
typedef struct {
    crypto_opt_level_t optimization_level;
    int enable_key_caching;
    int enable_batching;
    int cache_size;
    int batch_size;
    int enable_precomputation;
    double performance_threshold;
} crypto_opt_config_t;

// Main crypto optimizer structure
typedef struct {
    crypto_opt_config_t config;
    key_cache_entry_t *key_cache;
    int cache_entries;
    crypto_batch_processor_t batch_processor;
    crypto_perf_stats_t stats;
    int is_initialized;
    unsigned long long operation_count;
} crypto_optimizer_t;

// Function declarations
crypto_optimizer_t* crypto_optimizer_init(crypto_opt_config_t *config);
int crypto_optimized_encrypt(const unsigned char *key,
                           const unsigned char *iv,
                           const unsigned char *plaintext,
                           size_t plaintext_len,
                           unsigned char *ciphertext,
                           size_t *ciphertext_len);
int crypto_optimized_decrypt(const unsigned char *key,
                           const unsigned char *iv,
                           const unsigned char *ciphertext,
                           size_t ciphertext_len,
                           unsigned char *plaintext,
                           size_t *plaintext_len);
int crypto_batch_encrypt(crypto_batch_processor_t *processor,
                        const unsigned char *key,
                        const unsigned char *iv);
int crypto_add_to_batch(crypto_batch_processor_t *processor,
                       unsigned char *input,
                       unsigned char *output,
                       size_t buffer_size);
void get_crypto_performance_stats(crypto_perf_stats_t *stats);
void cleanup_crypto_optimizer();
int init_global_crypto_optimizer();

// Convenience macros
#define CRYPTO_ENCRYPT(key, iv, plain, plain_len, cipher, cipher_len) \
    crypto_optimized_encrypt(key, iv, plain, plain_len, cipher, cipher_len)
    
#define CRYPTO_DECRYPT(key, iv, cipher, cipher_len, plain, plain_len) \
    crypto_optimized_decrypt(key, iv, cipher, cipher_len, plain, plain_len)

#endif // CRYPTO_PERFORMANCE_OPTIMIZER_H