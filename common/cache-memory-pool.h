/*
    Cache Memory Pool - Пул памяти для кэша
    Оптимизация выделения памяти для cache_entry_t
*/

#ifndef CACHE_MEMORY_POOL_H
#define CACHE_MEMORY_POOL_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Конфигурация пула
#define CACHE_POOL_INITIAL_SIZE 1024
#define CACHE_POOL_MAX_SIZE 100000
#define CACHE_ENTRY_DATA_SIZE 256  // Средний размер данных

// Пул записей кэша
typedef struct cache_entry_pool {
    // Статический пул (быстрые аллокации)
    void **preallocated_entries;
    size_t preallocated_count;
    size_t next_free_index;
    
    // Динамический пул (fallback)
    void **dynamic_entries;
    size_t dynamic_count;
    size_t dynamic_capacity;
    
    // Статистика
    size_t total_allocations;
    size_t pool_hits;
    size_t pool_misses;
    size_t current_usage;
    
    // Блокировка для многопоточности
    void *mutex;
    
    // Флаги
    int initialized;
} cache_entry_pool_t;

// Инициализация пула
int cache_pool_init(cache_entry_pool_t *pool, size_t initial_size);

// Освобождение пула
void cache_pool_cleanup(cache_entry_pool_t *pool);

// Выделение записи из пула
void* cache_pool_alloc(cache_entry_pool_t *pool, size_t size);

// Освобождение записи в пул
void cache_pool_free(cache_entry_pool_t *pool, void *ptr);

// Статистика пула
typedef struct {
    size_t total_allocations;
    size_t pool_hits;
    size_t pool_misses;
    size_t current_usage;
    double hit_rate;
    size_t preallocated_count;
    size_t dynamic_count;
} cache_pool_stats_t;

void cache_pool_get_stats(cache_entry_pool_t *pool, cache_pool_stats_t *stats);

// Макросы для удобного использования
#define CACHE_POOL_ALLOC(pool, type) \
    ((type*)cache_pool_alloc(pool, sizeof(type)))

#define CACHE_POOL_FREE(pool, ptr) \
    cache_pool_free(pool, ptr)

#ifdef __cplusplus
}
#endif

#endif /* CACHE_MEMORY_POOL_H */
