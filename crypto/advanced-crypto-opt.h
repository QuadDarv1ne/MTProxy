/*
    Расширенная система криптографической оптимизации MTProxy
    Поддержка ARM64, AVX-512, улучшенные алгоритмы балансировки
*/

#ifndef ADVANCED_CRYPTO_OPT_H
#define ADVANCED_CRYPTO_OPT_H

#include <stdint.h>
#include <stddef.h>

// Конфигурация оптимизации
#define MAX_CRYPTO_CONTEXTS 2048
#define CRYPTO_CACHE_SIZE 4096
#define MAX_KEY_SIZES 16
#define ARM64_CRYPTO_EXTENSIONS 1

// Архитектурные флаги
#define ARCH_X86_64     0x01
#define ARCH_ARM64      0x02
#define ARCH_AVX2       0x04
#define ARCH_AVX512     0x08
#define ARCH_NEON       0x10
#define ARCH_AESNI      0x20

// Типы оптимизаций
typedef enum {
    CRYPTO_OPT_NONE = 0,
    CRYPTO_OPT_AESNI = 1,
    CRYPTO_OPT_AVX2 = 2,
    CRYPTO_OPT_AVX512 = 3,
    CRYPTO_OPT_NEON = 4,
    CRYPTO_OPT_AUTO = 5
} crypto_optimization_t;

// Типы балансировки
typedef enum {
    BALANCE_ROUND_ROBIN = 0,
    BALANCE_LEAST_CONNECTIONS = 1,
    BALANCE_WEIGHTED = 2,
    BALANCE_ADAPTIVE = 3,
    BALANCE_LEAST_LATENCY = 4
} balance_strategy_t;

// Статус криптографического контекста
typedef enum {
    CRYPTO_STATUS_IDLE = 0,
    CRYPTO_STATUS_ACTIVE = 1,
    CRYPTO_STATUS_ERROR = 2,
    CRYPTO_STATUS_RETIRED = 3
} crypto_status_t;

// Криптографический контекст
typedef struct crypto_context {
    uint64_t context_id;
    crypto_status_t status;
    int key_size;
    unsigned char key[64];
    unsigned char iv[16];
    void *hardware_context;  // Указатель на аппаратный контекст
    long long last_used;
    long long use_count;
    double avg_processing_time_us;
    int cpu_core_affinity;
    int is_precomputed;
} crypto_context_t;

// Кэш криптографических операций
typedef struct crypto_cache_entry {
    uint64_t key_hash;
    unsigned char ciphertext[4096];
    size_t data_length;
    long long timestamp;
    int hit_count;
} crypto_cache_entry_t;

// Информация об архитектуре
typedef struct architecture_info {
    int supported_extensions;
    int cpu_cores;
    int cache_line_size;
    int has_crypto_extensions;
    char cpu_vendor[32];
    char cpu_model[64];
} architecture_info_t;

// Статистика балансировки
typedef struct balance_stats {
    long long total_operations;
    long long balanced_operations;
    long long failed_balances;
    double avg_latency_us;
    int current_load_distribution[64]; // Для до 64 ядер
    long long rebalance_count;
} balance_stats_t;

// Расширенная криптографическая оптимизация
typedef struct advanced_crypto_opt {
    // Контексты
    crypto_context_t *contexts;
    int context_count;
    int max_contexts;
    
    // Кэш
    crypto_cache_entry_t *cache;
    int cache_size;
    int cache_entries;
    
    // Архитектура
    architecture_info_t arch_info;
    crypto_optimization_t optimization_level;
    
    // Балансировка
    balance_strategy_t balance_strategy;
    balance_stats_t balance_stats;
    double *load_weights;
    int weight_count;
    
    // Память
    void *memory_pool;
    size_t pool_size;
    size_t used_memory;
    int enable_memory_pooling;
    
    // Статистика
    long long total_crypto_operations;
    long long cache_hits;
    long long cache_misses;
    double avg_encryption_time_us;
    double avg_decryption_time_us;
    
    // Состояние
    int is_initialized;
    int is_optimized;
    long long init_time;
} advanced_crypto_opt_t;

// Инициализация
advanced_crypto_opt_t* crypto_opt_init(int max_contexts, size_t memory_pool_size);
int crypto_opt_configure(advanced_crypto_opt_t *opt, crypto_optimization_t level, 
                        balance_strategy_t strategy);
void crypto_opt_cleanup(advanced_crypto_opt_t *opt);

// Архитектурные функции
int crypto_opt_detect_architecture(architecture_info_t *info);
int crypto_opt_get_supported_extensions(void);
const char* crypto_opt_get_architecture_name(int extensions);

// Криптографические операции
int crypto_opt_encrypt(advanced_crypto_opt_t *opt, const unsigned char *key, 
                      const unsigned char *iv, const void *plaintext, 
                      void *ciphertext, size_t length);
int crypto_opt_decrypt(advanced_crypto_opt_t *opt, const unsigned char *key, 
                      const unsigned char *iv, const void *ciphertext, 
                      void *plaintext, size_t length);

// Кэширование
int crypto_opt_cache_encrypt(advanced_crypto_opt_t *opt, const unsigned char *key,
                           const void *plaintext, void *ciphertext, size_t length);
int crypto_opt_cache_decrypt(advanced_crypto_opt_t *opt, const unsigned char *key,
                           const void *ciphertext, void *plaintext, size_t length);
void crypto_opt_flush_cache(advanced_crypto_opt_t *opt);

// Балансировка нагрузки
int crypto_opt_balance_operation(advanced_crypto_opt_t *opt, int operation_type);
int crypto_opt_update_load_weights(advanced_crypto_opt_t *opt, double *weights, int count);
void crypto_opt_rebalance_contexts(advanced_crypto_opt_t *opt);

// Оптимизация памяти
int crypto_opt_preallocate_contexts(advanced_crypto_opt_t *opt, int count);
int crypto_opt_release_unused_contexts(advanced_crypto_opt_t *opt);
size_t crypto_opt_get_memory_usage(advanced_crypto_opt_t *opt);

// Управление контекстами
crypto_context_t* crypto_opt_acquire_context(advanced_crypto_opt_t *opt, 
                                           const unsigned char *key, int key_size);
int crypto_opt_release_context(advanced_crypto_opt_t *opt, crypto_context_t *context);
int crypto_opt_precompute_keys(advanced_crypto_opt_t *opt, const unsigned char *keys[], 
                              int key_count, int key_size);

// Мониторинг и статистика
void crypto_opt_get_stats(advanced_crypto_opt_t *opt, char *buffer, size_t buffer_size);
void crypto_opt_get_performance_report(advanced_crypto_opt_t *opt, char *buffer, size_t buffer_size);
void crypto_opt_reset_stats(advanced_crypto_opt_t *opt);

// Утилиты
uint64_t crypto_opt_hash_key(const unsigned char *key, int key_size);
int crypto_opt_is_extension_supported(int extension);
double crypto_opt_get_cache_hit_rate(advanced_crypto_opt_t *opt);
int crypto_opt_get_optimal_thread_count(advanced_crypto_opt_t *opt);

#endif // ADVANCED_CRYPTO_OPT_H