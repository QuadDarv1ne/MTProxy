/*
    Система управления кэшированием MTProxy
    Расширенная система кэширования с поддержкой различных алгоритмов
*/

#ifndef CACHE_MANAGER_H
#define CACHE_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Типы алгоритмов вытеснения
typedef enum {
    CACHE_LRU = 0,        // Least Recently Used
    CACHE_LFU = 1,        // Least Frequently Used
    CACHE_FIFO = 2,       // First In First Out
    CACHE_TTL = 3,        // Time To Live
    CACHE_ARC = 4         // Adaptive Replacement Cache
} cache_eviction_policy_t;

// Типы кэша
typedef enum {
    CACHE_TYPE_MEMORY = 0,
    CACHE_TYPE_DISK,
    CACHE_TYPE_HYBRID
} cache_type_t;

// Статусы операций кэша
typedef enum {
    CACHE_OK = 0,
    CACHE_MISS,
    CACHE_EXPIRED,
    CACHE_FULL,
    CACHE_ERROR,
    CACHE_INVALID_KEY,
    CACHE_INVALID_VALUE
} cache_status_t;

// Статистика кэша
typedef struct {
    long long hits;
    long long misses;
    long long evictions;
    long long inserts;
    long long updates;
    long long deletes;
    long long expirations;
    double hit_rate;
    size_t current_size_bytes;
    size_t max_size_bytes;
    size_t current_entries;
    size_t max_entries;
    double avg_access_time_ms;
    double memory_efficiency;
} cache_stats_t;

// Конфигурация кэша
typedef struct {
    cache_type_t type;
    cache_eviction_policy_t policy;
    size_t max_entries;
    size_t max_size_mb;
    time_t default_ttl_sec;
    int enable_compression;
    int enable_locking;
    int enable_statistics;
    int enable_persistence;
    char persistence_path[256];
    int persistence_interval_sec;
    
    // Расширенные настройки
    int enable_prefetch;
    double prefetch_threshold;
    int enable_warming;
    size_t warm_entries;
    int enable_partitioning;
    int partition_count;
} cache_config_t;

// Запись кэша
typedef struct cache_entry {
    char *key;
    void *data;
    size_t data_size;
    time_t creation_time;
    time_t last_access_time;
    time_t expiry_time;
    uint32_t access_count;
    uint32_t frequency;  // Для LFU
    uint64_t hash;
    struct cache_entry *next;       // Для хэш-таблицы (бакет)
    struct cache_entry *prev;       // Для хэш-таблицы (бакет)
    struct cache_entry *lru_next;   // Для LRU списка
    struct cache_entry *lru_prev;   // Для LRU списка
    struct cache_entry *older;      // Для ARC
    struct cache_entry *newer;
} cache_entry_t;

// Раздел кэша (partition)
typedef struct {
    cache_entry_t **buckets;
    size_t bucket_count;
    size_t entry_count;
    size_t size_bytes;
    void *mutex;
} cache_partition_t;

// Контекст кэша
typedef struct {
    cache_config_t config;
    cache_stats_t stats;
    
    // Partitioned cache
    cache_partition_t *partitions;
    int partition_count;
    
    // LRU list
    cache_entry_t *lru_head;
    cache_entry_t *lru_tail;
    
    // LFU heap
    cache_entry_t **lfu_heap;
    int lfu_size;
    
    // Глобальная блокировка
    void *global_mutex;
    
    // Состояние
    int is_initialized;
    time_t start_time;
    time_t last_persistence_time;
    
    // Callback функции
    void (*on_eviction)(const char *key, const void *data);
    void (*on_expire)(const char *key, const void *data);
    void *user_data;
} cache_manager_t;

// Инициализация и очистка
cache_manager_t* cache_manager_init(const cache_config_t *config);
int cache_manager_configure(cache_manager_t *cache, const cache_config_t *config);
void cache_manager_cleanup(cache_manager_t *cache);

// Основные операции
cache_status_t cache_put(cache_manager_t *cache, const char *key, 
                         const void *data, size_t data_size);
cache_status_t cache_get(cache_manager_t *cache, const char *key,
                         void **data, size_t *data_size);
cache_status_t cache_delete(cache_manager_t *cache, const char *key);
int cache_exists(cache_manager_t *cache, const char *key);

// Операции с TTL
cache_status_t cache_put_with_ttl(cache_manager_t *cache, const char *key,
                                  const void *data, size_t data_size,
                                  time_t ttl_seconds);
time_t cache_get_ttl(cache_manager_t *cache, const char *key);
int cache_refresh_ttl(cache_manager_t *cache, const char *key);

// Массовые операции
int cache_put_batch(cache_manager_t *cache, const char **keys,
                    const void **values, const size_t *sizes, int count);
int cache_get_batch(cache_manager_t *cache, const char **keys,
                    void ***values, size_t **sizes, int count);
int cache_delete_batch(cache_manager_t *cache, const char **keys, int count);

// Управление памятью
int cache_evict(cache_manager_t *cache, int count);
int cache_evict_expired(cache_manager_t *cache);
int cache_evict_lru(cache_manager_t *cache, int count);
int cache_evict_lfu(cache_manager_t *cache, int count);
void cache_clear(cache_manager_t *cache);
int cache_resize(cache_manager_t *cache, size_t new_max_entries, size_t new_max_size_mb);

// Статистика
void cache_get_stats(cache_manager_t *cache, cache_stats_t *stats);
void cache_reset_stats(cache_manager_t *cache);
void cache_print_stats(cache_manager_t *cache);
double cache_get_hit_rate(cache_manager_t *cache);
size_t cache_get_memory_usage(cache_manager_t *cache);

// Утилиты
uint64_t cache_hash_key(const char *key);
char* cache_generate_key(const char *prefix, const void *data, size_t size);
int cache_validate_key(const char *key);
int cache_compress_data(const void *in, size_t in_size, void **out, size_t *out_size);
int cache_decompress_data(const void *in, size_t in_size, void **out, size_t *out_size);

// Персистентность
int cache_save_to_disk(cache_manager_t *cache, const char *filename);
int cache_load_from_disk(cache_manager_t *cache, const char *filename);
int cache_start_persistence_thread(cache_manager_t *cache);
void cache_stop_persistence_thread(cache_manager_t *cache);

// Предвыборка и прогрев
int cache_prefetch(cache_manager_t *cache, const char **keys, int count);
int cache_warm_up(cache_manager_t *cache, const char **keys, 
                  const void **values, const size_t *sizes, int count);

// Callback функции
void cache_set_eviction_callback(cache_manager_t *cache,
                                 void (*callback)(const char*, const void*));
void cache_set_expiration_callback(cache_manager_t *cache,
                                   void (*callback)(const char*, const void*));

// Мониторинг
int cache_get_health(cache_manager_t *cache);
int cache_needs_eviction(cache_manager_t *cache);
time_t cache_get_oldest_entry_age(cache_manager_t *cache);
time_t cache_get_newest_entry_age(cache_manager_t *cache);

// Расширенные функции
int cache_set_priority(cache_manager_t *cache, const char *key, int priority);
int cache_get_priority(cache_manager_t *cache, const char *key);
int cache_touch(cache_manager_t *cache, const char *key);  // Обновить время доступа без получения данных
int cache_replace(cache_manager_t *cache, const char *key, const void *new_data, size_t new_size);

// Атомарные операции
long long cache_increment(cache_manager_t *cache, const char *key, long long delta);
int cache_decrement(cache_manager_t *cache, const char *key, long long delta);

// Проверка целостности
int cache_verify_integrity(cache_manager_t *cache);
int cache_defragment(cache_manager_t *cache);

#ifdef __cplusplus
}
#endif

#endif // CACHE_MANAGER_H
