/*
    Cache Memory Pool - Пул памяти для кэша
    Оптимизация выделения памяти для cache_entry_t
*/

#include "cache-memory-pool.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
    #include <windows.h>
    #define POLOCK_INIT() InitializeCriticalSection(pool->mutex)
    #define POLOCK_DESTROY() DeleteCriticalSection(pool->mutex)
    #define POLOCK_ENTER() EnterCriticalSection(pool->mutex)
    #define POLOCK_LEAVE() LeaveCriticalSection(pool->mutex)
#else
    #include <pthread.h>
    #define POLOCK_INIT() pthread_mutex_init(pool->mutex, NULL)
    #define POLOCK_DESTROY() pthread_mutex_destroy(pool->mutex)
    #define POLOCK_ENTER() pthread_mutex_lock(pool->mutex)
    #define POLOCK_LEAVE() pthread_mutex_unlock(pool->mutex)
#endif

// Инициализация пула
int cache_pool_init(cache_entry_pool_t *pool, size_t initial_size) {
    if (!pool) return -1;
    
    if (initial_size > CACHE_POOL_MAX_SIZE) {
        initial_size = CACHE_POOL_MAX_SIZE;
    }
    if (initial_size < 1) {
        initial_size = CACHE_POOL_INITIAL_SIZE;
    }
    
    memset(pool, 0, sizeof(cache_entry_pool_t));
    
    // Выделение статического пула
    pool->preallocated_entries = (void**)calloc(initial_size, sizeof(void*));
    if (!pool->preallocated_entries) {
        return -1;
    }
    
    // Предварительное выделение записей
    for (size_t i = 0; i < initial_size; i++) {
        pool->preallocated_entries[i] = calloc(1, 512);  // 512 байт на запись
        if (!pool->preallocated_entries[i]) {
            // Ошибка - освобождаем всё
            for (size_t j = 0; j < i; j++) {
                free(pool->preallocated_entries[j]);
            }
            free(pool->preallocated_entries);
            return -1;
        }
    }
    
    pool->preallocated_count = initial_size;
    pool->next_free_index = 0;
    pool->dynamic_capacity = 1024;
    pool->dynamic_entries = (void**)calloc(pool->dynamic_capacity, sizeof(void*));
    if (!pool->dynamic_entries) {
        for (size_t i = 0; i < initial_size; i++) {
            free(pool->preallocated_entries[i]);
        }
        free(pool->preallocated_entries);
        return -1;
    }
    
    // Инициализация мьютекса
    #ifdef _WIN32
    pool->mutex = malloc(sizeof(CRITICAL_SECTION));
    if (!pool->mutex) {
        cache_pool_cleanup(pool);
        return -1;
    }
    InitializeCriticalSection((CRITICAL_SECTION*)pool->mutex);
    #else
    pool->mutex = malloc(sizeof(pthread_mutex_t));
    if (!pool->mutex) {
        cache_pool_cleanup(pool);
        return -1;
    }
    pthread_mutex_init((pthread_mutex_t*)pool->mutex, NULL);
    #endif
    
    pool->initialized = 1;
    return 0;
}

// Освобождение пула
void cache_pool_cleanup(cache_entry_pool_t *pool) {
    if (!pool) return;
    
    // Освобождение статического пула
    if (pool->preallocated_entries) {
        for (size_t i = 0; i < pool->preallocated_count; i++) {
            if (pool->preallocated_entries[i]) {
                free(pool->preallocated_entries[i]);
            }
        }
        free(pool->preallocated_entries);
        pool->preallocated_entries = NULL;
    }
    
    // Освобождение динамического пула
    if (pool->dynamic_entries) {
        for (size_t i = 0; i < pool->dynamic_count; i++) {
            if (pool->dynamic_entries[i]) {
                free(pool->dynamic_entries[i]);
            }
        }
        free(pool->dynamic_entries);
        pool->dynamic_entries = NULL;
    }
    
    // Уничтожение мьютекса
    if (pool->mutex) {
        #ifdef _WIN32
        DeleteCriticalSection((CRITICAL_SECTION*)pool->mutex);
        #else
        pthread_mutex_destroy((pthread_mutex_t*)pool->mutex);
        #endif
        free(pool->mutex);
        pool->mutex = NULL;
    }
    
    pool->initialized = 0;
}

// Выделение записи из пула
void* cache_pool_alloc(cache_entry_pool_t *pool, size_t size) {
    if (!pool || !pool->initialized) {
        return calloc(1, size);  // Fallback
    }
    
    void *ptr = NULL;
    
    POLOCK_ENTER();
    
    pool->total_allocations++;
    
    // Попытка выделить из статического пула
    if (pool->next_free_index < pool->preallocated_count) {
        ptr = pool->preallocated_entries[pool->next_free_index];
        pool->preallocated_entries[pool->next_free_index] = NULL;
        pool->next_free_index++;
        pool->pool_hits++;
        pool->current_usage++;
    } else {
        // Fallback: динамическое выделение
        pool->pool_misses++;
        
        if (pool->dynamic_count >= pool->dynamic_capacity) {
            // Расширение динамического пула
            size_t new_capacity = pool->dynamic_capacity * 2;
            void **new_entries = (void**)realloc(pool->dynamic_entries, 
                                                  new_capacity * sizeof(void*));
            if (!new_entries) {
                POLOCK_LEAVE();
                return calloc(1, size);  // Fallback
            }
            pool->dynamic_entries = new_entries;
            pool->dynamic_capacity = new_capacity;
        }
        
        ptr = calloc(1, size);
        if (ptr && pool->dynamic_count < pool->dynamic_capacity) {
            pool->dynamic_entries[pool->dynamic_count] = ptr;
            pool->dynamic_count++;
        }
        pool->current_usage++;
    }
    
    POLOCK_LEAVE();
    
    return ptr;
}

// Освобождение записи в пул
void cache_pool_free(cache_entry_pool_t *pool, void *ptr) {
    if (!pool || !ptr) {
        free(ptr);
        return;
    }
    
    if (!pool->initialized) {
        free(ptr);
        return;
    }
    
    POLOCK_ENTER();
    
    // Проверка: принадлежит ли ptr статическому пулу
    int found = 0;
    for (size_t i = 0; i < pool->preallocated_count; i++) {
        if (pool->preallocated_entries[i] == ptr) {
            found = 1;
            break;
        }
    }
    
    if (!found) {
        // Проверка динамического пула
        for (size_t i = 0; i < pool->dynamic_count; i++) {
            if (pool->dynamic_entries[i] == ptr) {
                pool->dynamic_entries[i] = NULL;
                found = 1;
                break;
            }
        }
    }
    
    // Очистка памяти (но не освобождение для статического пула)
    if (found && ptr) {
        memset(ptr, 0, 512);  // Очистка для безопасности
    }
    
    if (pool->current_usage > 0) {
        pool->current_usage--;
    }
    
    POLOCK_LEAVE();
}

// Статистика пула
void cache_pool_get_stats(cache_entry_pool_t *pool, cache_pool_stats_t *stats) {
    if (!pool || !stats) return;
    
    POLOCK_ENTER();
    
    stats->total_allocations = pool->total_allocations;
    stats->pool_hits = pool->pool_hits;
    stats->pool_misses = pool->pool_misses;
    stats->current_usage = pool->current_usage;
    stats->preallocated_count = pool->preallocated_count;
    stats->dynamic_count = pool->dynamic_count;
    
    if (pool->total_allocations > 0) {
        stats->hit_rate = (pool->pool_hits * 100.0) / pool->total_allocations;
    } else {
        stats->hit_rate = 0.0;
    }
    
    POLOCK_LEAVE();
}
