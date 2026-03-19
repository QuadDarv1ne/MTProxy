/*
    Система управления кэшированием MTProxy
    Расширенная система кэширования с поддержкой различных алгоритмов
*/

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "common/cache-manager.h"
#include "common/kprintf.h"
#include "common/crc32.h"

// Простая реализация хэш-функции Jenkins
uint64_t cache_hash_key(const char *key) {
    if (!key) return 0;
    
    uint64_t hash = 0;
    while (*key) {
        hash = hash * 31 + (unsigned char)(*key);
        key++;
    }
    return hash;
}

// Создание новой записи кэша
static cache_entry_t* cache_create_entry(const char *key, const void *data, 
                                         size_t data_size, time_t ttl) {
    cache_entry_t *entry = calloc(1, sizeof(cache_entry_t));
    if (!entry) return NULL;
    
    entry->key = strdup(key);
    if (!entry->key) {
        free(entry);
        return NULL;
    }
    
    entry->data = malloc(data_size);
    if (!entry->data) {
        free(entry->key);
        free(entry);
        return NULL;
    }
    
    memcpy(entry->data, data, data_size);
    entry->data_size = data_size;
    entry->creation_time = time(NULL);
    entry->last_access_time = entry->creation_time;
    entry->expiry_time = (ttl > 0) ? entry->creation_time + ttl : 0;
    entry->access_count = 0;
    entry->frequency = 0;
    entry->hash = cache_hash_key(key);
    
    return entry;
}

// Освобождение записи кэша
static void cache_free_entry(cache_entry_t *entry) {
    if (!entry) return;
    
    if (entry->key) free(entry->key);
    if (entry->data) free(entry->data);
    free(entry);
}

// Проверка истечения TTL
static int cache_is_expired(cache_entry_t *entry) {
    if (!entry) return 1;
    if (entry->expiry_time == 0) return 0;
    return time(NULL) > entry->expiry_time;
}

// Поиск записи в хэш-таблице
static cache_entry_t* cache_find_entry(cache_manager_t *cache, const char *key) {
    if (!cache || !key || !cache->partitions) return NULL;
    
    // Выбор раздела по хэшу
    uint64_t hash = cache_hash_key(key);
    int partition_idx = hash % cache->partition_count;
    cache_partition_t *partition = &cache->partitions[partition_idx];
    
    // Поиск в бакете
    size_t bucket_idx = hash % partition->bucket_count;
    cache_entry_t *entry = partition->buckets[bucket_idx];
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

// Добавление записи в хэш-таблицу
static int cache_add_entry_to_bucket(cache_manager_t *cache, cache_entry_t *entry) {
    if (!cache || !entry || !cache->partitions) return -1;
    
    int partition_idx = entry->hash % cache->partition_count;
    cache_partition_t *partition = &cache->partitions[partition_idx];
    size_t bucket_idx = entry->hash % partition->bucket_count;
    
    // Добавление в начало списка бакета
    entry->next = partition->buckets[bucket_idx];
    if (partition->buckets[bucket_idx]) {
        partition->buckets[bucket_idx]->prev = entry;
    }
    partition->buckets[bucket_idx] = entry;
    
    partition->entry_count++;
    partition->size_bytes += entry->data_size;
    
    return 0;
}

// Удаление записи из хэш-таблицы
static int cache_remove_entry_from_bucket(cache_manager_t *cache, cache_entry_t *entry) {
    if (!cache || !entry || !cache->partitions) return -1;
    
    int partition_idx = entry->hash % cache->partition_count;
    cache_partition_t *partition = &cache->partitions[partition_idx];
    size_t bucket_idx = entry->hash % partition->bucket_count;
    
    // Удаление из списка
    if (entry->prev) {
        entry->prev->next = entry->next;
    } else {
        partition->buckets[bucket_idx] = entry->next;
    }
    
    if (entry->next) {
        entry->next->prev = entry->prev;
    }
    
    partition->entry_count--;
    partition->size_bytes -= entry->data_size;
    
    return 0;
}

// LRU eviction
static int cache_evict_lru_internal(cache_manager_t *cache) {
    if (!cache || !cache->lru_tail) return -1;

    cache_entry_t *entry = cache->lru_tail;

    // Удаление из хэш-таблицы
    cache_remove_entry_from_bucket(cache, entry);

    // Удаление из LRU списка
    if (entry->lru_prev) {
        entry->lru_prev->lru_next = NULL;
    }
    cache->lru_tail = entry->lru_prev;

    if (!cache->lru_tail) {
        cache->lru_head = NULL;
    }

    // Callback
    if (cache->on_eviction) {
        cache->on_eviction(entry->key, entry->data);
    }

    cache->stats.evictions++;
    cache_free_entry(entry);

    return 0;
}

// Перемещение записи в начало LRU списка
static void cache_move_to_lru_head(cache_manager_t *cache, cache_entry_t *entry) {
    if (!cache || !entry) return;

    // Удаление из текущей позиции в LRU списке
    if (entry->lru_prev) entry->lru_prev->lru_next = entry->lru_next;
    if (entry->lru_next) entry->lru_next->lru_prev = entry->lru_prev;

    if (cache->lru_tail == entry) {
        cache->lru_tail = entry->lru_prev;
    }

    // Добавление в начало LRU списка
    entry->lru_prev = NULL;
    entry->lru_next = cache->lru_head;
    if (cache->lru_head) {
        cache->lru_head->lru_prev = entry;
    }
    cache->lru_head = entry;
}

// Инициализация кэша
cache_manager_t* cache_manager_init(const cache_config_t *config) {
    if (!config) return NULL;
    
    cache_manager_t *cache = calloc(1, sizeof(cache_manager_t));
    if (!cache) return NULL;
    
    // Копирование конфигурации
    memcpy(&cache->config, config, sizeof(cache_config_t));
    
    // Инициализация partitioning
    cache->partition_count = config->enable_partitioning ? 
                            (config->partition_count > 0 ? config->partition_count : 16) : 1;
    cache->partitions = calloc(cache->partition_count, sizeof(cache_partition_t));
    if (!cache->partitions) {
        free(cache);
        return NULL;
    }
    
    // Инициализация разделов
    size_t buckets_per_partition = config->max_entries / cache->partition_count;
    if (buckets_per_partition < 16) buckets_per_partition = 16;
    
    for (int i = 0; i < cache->partition_count; i++) {
        cache->partitions[i].bucket_count = buckets_per_partition;
        cache->partitions[i].buckets = calloc(buckets_per_partition, sizeof(cache_entry_t*));
        if (!cache->partitions[i].buckets) {
            // Очистка при ошибке
            for (int j = 0; j < i; j++) {
                free(cache->partitions[j].buckets);
            }
            free(cache->partitions);
            free(cache);
            return NULL;
        }
        
        // Инициализация мьютекса раздела
#ifdef _WIN32
        cache->partitions[i].mutex = malloc(sizeof(CRITICAL_SECTION));
        if (cache->partitions[i].mutex) {
            InitializeCriticalSection((CRITICAL_SECTION*)cache->partitions[i].mutex);
        }
#else
        cache->partitions[i].mutex = malloc(sizeof(pthread_mutex_t));
        if (cache->partitions[i].mutex) {
            pthread_mutex_init((pthread_mutex_t*)cache->partitions[i].mutex, NULL);
        }
#endif
    }
    
    // Глобальная блокировка
#ifdef _WIN32
    cache->global_mutex = malloc(sizeof(CRITICAL_SECTION));
    if (cache->global_mutex) {
        InitializeCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
    }
#else
    cache->global_mutex = malloc(sizeof(pthread_mutex_t));
    if (cache->global_mutex) {
        pthread_mutex_init((pthread_mutex_t*)cache->global_mutex, NULL);
    }
#endif
    
    cache->stats.max_entries = config->max_entries;
    cache->stats.max_size_bytes = config->max_size_mb * 1024 * 1024;
    cache->start_time = time(NULL);
    cache->is_initialized = 1;
    
    vkprintf(1, "Cache manager initialized: %d partitions, %zu max entries, %zu MB\n",
             cache->partition_count, config->max_entries, config->max_size_mb);
    
    return cache;
}

// Получение из кэша
cache_status_t cache_get(cache_manager_t *cache, const char *key,
                         void **data, size_t *data_size) {
    if (!cache || !key || !data || !data_size) return CACHE_ERROR;
    
#ifdef _WIN32
    if (cache->config.enable_locking && cache->global_mutex) {
        EnterCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
    }
#else
    if (cache->config.enable_locking && cache->global_mutex) {
        pthread_mutex_lock((pthread_mutex_t*)cache->global_mutex);
    }
#endif
    
    cache_entry_t *entry = cache_find_entry(cache, key);
    
    if (!entry) {
        cache->stats.misses++;
#ifdef _WIN32
        if (cache->config.enable_locking) LeaveCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
#else
        if (cache->config.enable_locking) pthread_mutex_unlock((pthread_mutex_t*)cache->global_mutex);
#endif
        return CACHE_MISS;
    }
    
    // Проверка TTL
    if (cache_is_expired(entry)) {
        cache->stats.expirations++;
        cache->stats.misses++;
        
        // Удаление истекшей записи
        cache_remove_entry_from_bucket(cache, entry);
        cache_free_entry(entry);
        
#ifdef _WIN32
        if (cache->config.enable_locking) LeaveCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
#else
        if (cache->config.enable_locking) pthread_mutex_unlock((pthread_mutex_t*)cache->global_mutex);
#endif
        return CACHE_EXPIRED;
    }
    
    // Обновление статистики доступа
    entry->access_count++;
    entry->last_access_time = time(NULL);
    entry->frequency++;
    
    // Перемещение в начало LRU списка
    if (cache->config.policy == CACHE_LRU) {
        cache_move_to_lru_head(cache, entry);
    }
    
    // Копирование данных
    *data_size = entry->data_size;
    *data = malloc(entry->data_size);
    if (*data) {
        memcpy(*data, entry->data, entry->data_size);
    }
    
    cache->stats.hits++;
    
#ifdef _WIN32
    if (cache->config.enable_locking) LeaveCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
#else
    if (cache->config.enable_locking) pthread_mutex_unlock((pthread_mutex_t*)cache->global_mutex);
#endif
    
    return CACHE_OK;
}

// Запись в кэш
cache_status_t cache_put(cache_manager_t *cache, const char *key,
                         const void *data, size_t data_size) {
    return cache_put_with_ttl(cache, key, data, data_size, cache->config.default_ttl_sec);
}

// Запись в кэш с TTL
cache_status_t cache_put_with_ttl(cache_manager_t *cache, const char *key,
                                  const void *data, size_t data_size,
                                  time_t ttl_seconds) {
    if (!cache || !key || !data) return CACHE_ERROR;
    
#ifdef _WIN32
    if (cache->config.enable_locking && cache->global_mutex) {
        EnterCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
    }
#else
    if (cache->config.enable_locking && cache->global_mutex) {
        pthread_mutex_lock((pthread_mutex_t*)cache->global_mutex);
    }
#endif
    
    // Проверка существующей записи
    cache_entry_t *existing = cache_find_entry(cache, key);
    if (existing) {
        // Обновление существующей записи
        void *new_data = realloc(existing->data, data_size);
        if (!new_data) {
#ifdef _WIN32
            if (cache->config.enable_locking) LeaveCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
#else
            if (cache->config.enable_locking) pthread_mutex_unlock((pthread_mutex_t*)cache->global_mutex);
#endif
            return CACHE_ERROR;
        }
        
        existing->data = new_data;
        memcpy(existing->data, data, data_size);
        existing->data_size = data_size;
        existing->expiry_time = (ttl_seconds > 0) ? time(NULL) + ttl_seconds : 0;
        existing->access_count = 0;
        existing->frequency = 0;
        
        cache->stats.updates++;
        
#ifdef _WIN32
        if (cache->config.enable_locking) LeaveCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
#else
        if (cache->config.enable_locking) pthread_mutex_unlock((pthread_mutex_t*)cache->global_mutex);
#endif
        return CACHE_OK;
    }
    
    // Проверка необходимости eviction
    while (cache->stats.current_entries >= cache->config.max_entries ||
           cache->stats.current_size_bytes >= cache->stats.max_size_bytes) {
        if (cache_evict_lru_internal(cache) != 0) {
            break;
        }
    }
    
    // Создание новой записи
    cache_entry_t *entry = cache_create_entry(key, data, data_size, ttl_seconds);
    if (!entry) {
#ifdef _WIN32
        if (cache->config.enable_locking) LeaveCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
#else
        if (cache->config.enable_locking) pthread_mutex_unlock((pthread_mutex_t*)cache->global_mutex);
#endif
        return CACHE_ERROR;
    }
    
    // Добавление в хэш-таблицу
    cache_add_entry_to_bucket(cache, entry);

    // Добавление в LRU список
    if (cache->config.policy == CACHE_LRU) {
        entry->lru_next = cache->lru_head;
        entry->lru_prev = NULL;
        if (cache->lru_head) {
            cache->lru_head->lru_prev = entry;
        }
        cache->lru_head = entry;
        if (!cache->lru_tail) {
            cache->lru_tail = entry;
        }
    }
    
    // Обновление статистики
    cache->stats.current_entries++;
    cache->stats.current_size_bytes += data_size;
    cache->stats.inserts++;
    
#ifdef _WIN32
    if (cache->config.enable_locking) LeaveCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
#else
    if (cache->config.enable_locking) pthread_mutex_unlock((pthread_mutex_t*)cache->global_mutex);
#endif
    
    return CACHE_OK;
}

// Удаление из кэша
cache_status_t cache_delete(cache_manager_t *cache, const char *key) {
    if (!cache || !key) return CACHE_ERROR;
    
#ifdef _WIN32
    if (cache->config.enable_locking && cache->global_mutex) {
        EnterCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
    }
#else
    if (cache->config.enable_locking && cache->global_mutex) {
        pthread_mutex_lock((pthread_mutex_t*)cache->global_mutex);
    }
#endif
    
    cache_entry_t *entry = cache_find_entry(cache, key);
    
    if (!entry) {
#ifdef _WIN32
        if (cache->config.enable_locking) LeaveCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
#else
        if (cache->config.enable_locking) pthread_mutex_unlock((pthread_mutex_t*)cache->global_mutex);
#endif
        return CACHE_MISS;
    }

    // Удаление из хэш-таблицы (включая связанный список бакета)
    cache_remove_entry_from_bucket(cache, entry);

    // Обновление LRU списка (если запись была в нем)
    if (cache->config.policy == CACHE_LRU) {
        if (entry->lru_prev) entry->lru_prev->lru_next = entry->lru_next;
        if (entry->lru_next) entry->lru_next->lru_prev = entry->lru_prev;

        if (cache->lru_head == entry) cache->lru_head = entry->lru_next;
        if (cache->lru_tail == entry) cache->lru_tail = entry->lru_prev;
    }

    // Обновление статистики
    cache->stats.current_entries--;
    cache->stats.current_size_bytes -= entry->data_size;
    cache->stats.deletes++;

    cache_free_entry(entry);
    
#ifdef _WIN32
    if (cache->config.enable_locking) LeaveCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
#else
    if (cache->config.enable_locking) pthread_mutex_unlock((pthread_mutex_t*)cache->global_mutex);
#endif
    
    return CACHE_OK;
}

// Проверка существования
int cache_exists(cache_manager_t *cache, const char *key) {
    if (!cache || !key) return 0;
    
#ifdef _WIN32
    if (cache->config.enable_locking && cache->global_mutex) {
        EnterCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
    }
#else
    if (cache->config.enable_locking && cache->global_mutex) {
        pthread_mutex_lock((pthread_mutex_t*)cache->global_mutex);
    }
#endif
    
    cache_entry_t *entry = cache_find_entry(cache, key);
    int exists = (entry != NULL && !cache_is_expired(entry));
    
#ifdef _WIN32
    if (cache->config.enable_locking) LeaveCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
#else
    if (cache->config.enable_locking) pthread_mutex_unlock((pthread_mutex_t*)cache->global_mutex);
#endif
    
    return exists;
}

// Получение статистики
void cache_get_stats(cache_manager_t *cache, cache_stats_t *stats) {
    if (!cache || !stats) return;
    
#ifdef _WIN32
    if (cache->config.enable_locking && cache->global_mutex) {
        EnterCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
    }
#else
    if (cache->config.enable_locking && cache->global_mutex) {
        pthread_mutex_lock((pthread_mutex_t*)cache->global_mutex);
    }
#endif
    
    memcpy(stats, &cache->stats, sizeof(cache_stats_t));
    
    // Вычисление hit rate
    long long total = cache->stats.hits + cache->stats.misses;
    if (total > 0) {
        stats->hit_rate = (double)cache->stats.hits / total * 100.0;
    } else {
        stats->hit_rate = 0.0;
    }
    
#ifdef _WIN32
    if (cache->config.enable_locking) LeaveCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
#else
    if (cache->config.enable_locking) pthread_mutex_unlock((pthread_mutex_t*)cache->global_mutex);
#endif
}

// Очистка кэша
void cache_clear(cache_manager_t *cache) {
    if (!cache || !cache->partitions) return;
    
#ifdef _WIN32
    if (cache->config.enable_locking && cache->global_mutex) {
        EnterCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
    }
#else
    if (cache->config.enable_locking && cache->global_mutex) {
        pthread_mutex_lock((pthread_mutex_t*)cache->global_mutex);
    }
#endif
    
    // Очистка всех разделов
    for (int i = 0; i < cache->partition_count; i++) {
        cache_partition_t *partition = &cache->partitions[i];
        
        for (size_t j = 0; j < partition->bucket_count; j++) {
            cache_entry_t *entry = partition->buckets[j];
            while (entry) {
                cache_entry_t *next = entry->next;
                cache_free_entry(entry);
                entry = next;
            }
            partition->buckets[j] = NULL;
        }
        
        partition->entry_count = 0;
        partition->size_bytes = 0;
    }
    
    // Сброс LRU списка
    cache->lru_head = NULL;
    cache->lru_tail = NULL;
    
    // Сброс статистики
    cache->stats.current_entries = 0;
    cache->stats.current_size_bytes = 0;
    
#ifdef _WIN32
    if (cache->config.enable_locking) LeaveCriticalSection((CRITICAL_SECTION*)cache->global_mutex);
#else
    if (cache->config.enable_locking) pthread_mutex_unlock((pthread_mutex_t*)cache->global_mutex);
#endif
    
    vkprintf(2, "Cache cleared\n");
}

// Очистка ресурсов
void cache_manager_cleanup(cache_manager_t *cache) {
    if (!cache) return;
    
    // Очистка всех записей
    cache_clear(cache);
    
    // Освобождение разделов
    if (cache->partitions) {
        for (int i = 0; i < cache->partition_count; i++) {
            if (cache->partitions[i].buckets) {
                free(cache->partitions[i].buckets);
            }
#ifdef _WIN32
            if (cache->partitions[i].mutex) {
                DeleteCriticalSection((CRITICAL_SECTION*)(cache->partitions[i].mutex));
                free(cache->partitions[i].mutex);
            }
#else
            if (cache->partitions[i].mutex) {
                pthread_mutex_destroy((pthread_mutex_t*)(cache->partitions[i].mutex));
                free(cache->partitions[i].mutex);
            }
#endif
        }
        free(cache->partitions);
    }

    // Освобождение глобальной блокировки
#ifdef _WIN32
    if (cache->global_mutex) {
        DeleteCriticalSection((CRITICAL_SECTION*)(cache->global_mutex));
        free(cache->global_mutex);
    }
#else
    if (cache->global_mutex) {
        pthread_mutex_destroy((pthread_mutex_t*)(cache->global_mutex));
        free(cache->global_mutex);
    }
#endif
    
    vkprintf(1, "Cache manager cleaned up\n");
    free(cache);
}

// Вывод статистики
void cache_print_stats(cache_manager_t *cache) {
    if (!cache) return;
    
    cache_stats_t stats;
    cache_get_stats(cache, &stats);
    
    vkprintf(1, "Cache Statistics:\n");
    vkprintf(1, "  Entries: %zu / %zu\n", stats.current_entries, stats.max_entries);
    vkprintf(1, "  Size: %zu / %zu bytes\n", stats.current_size_bytes, stats.max_size_bytes);
    vkprintf(1, "  Hits: %lld\n", stats.hits);
    vkprintf(1, "  Misses: %lld\n", stats.misses);
    vkprintf(1, "  Hit Rate: %.2f%%\n", stats.hit_rate);
    vkprintf(1, "  Evictions: %lld\n", stats.evictions);
    vkprintf(1, "  Expirations: %lld\n", stats.expirations);
}

// Получение hit rate
double cache_get_hit_rate(cache_manager_t *cache) {
    if (!cache) return 0.0;
    
    long long total = cache->stats.hits + cache->stats.misses;
    if (total > 0) {
        return (double)cache->stats.hits / total * 100.0;
    }
    return 0.0;
}

// Использование памяти
size_t cache_get_memory_usage(cache_manager_t *cache) {
    if (!cache) return 0;
    return cache->stats.current_size_bytes;
}

// Валидация ключа
int cache_validate_key(const char *key) {
    if (!key || strlen(key) == 0) return 0;
    if (strlen(key) > 256) return 0;
    return 1;
}

// Генерация ключа
char* cache_generate_key(const char *prefix, const void *data, size_t size) {
    if (!prefix || !data) return NULL;
    
    char *key = malloc(256);
    if (!key) return NULL;

    uint32_t crc = crc32_partial(data, size, -1) ^ -1;
    snprintf(key, 256, "%s_%08X", prefix, crc);

    return key;
}

// Установка callback eviction
void cache_set_eviction_callback(cache_manager_t *cache,
                                 void (*callback)(const char*, const void*)) {
    if (cache) {
        cache->on_eviction = callback;
    }
}

// Установка callback expiration
void cache_set_expiration_callback(cache_manager_t *cache,
                                   void (*callback)(const char*, const void*)) {
    if (cache) {
        cache->on_expire = callback;
    }
}

// Атомарное увеличение
long long cache_increment(cache_manager_t *cache, const char *key, long long delta) {
    if (!cache || !key) return 0;
    
    long long current_value = 0;
    size_t data_size = 0;
    void *data = NULL;
    
    cache_status_t status = cache_get(cache, key, &data, &data_size);
    
    if (status == CACHE_OK && data && data_size == sizeof(long long)) {
        current_value = *(long long*)data;
        free(data);
    }
    
    current_value += delta;
    
    cache_put(cache, key, &current_value, sizeof(long long));
    
    return current_value;
}

// Проверка здоровья
int cache_get_health(cache_manager_t *cache) {
    if (!cache) return 0;
    if (!cache->is_initialized) return 0;
    if (!cache->partitions) return 0;
    return 1;
}

// Нужен ли eviction
int cache_needs_eviction(cache_manager_t *cache) {
    if (!cache) return 0;
    return (cache->stats.current_entries >= cache->config.max_entries ||
            cache->stats.current_size_bytes >= cache->stats.max_size_bytes);
}
