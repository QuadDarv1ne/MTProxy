/*
    Система ограничения скорости (Rate Limiter) для MTProxy
    Поддержка различных алгоритмов: Token Bucket, Sliding Window, Fixed Window, Leaky Bucket
*/

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "common/rate-limiter.h"
#include "common/kprintf.h"
#include "common/crc32.h"

// Хэш-функция для ключа
static uint64_t rate_limit_hash(const char *key) {
    if (!key) return 0;
    return crc32_partial(key, strlen(key), -1) ^ -1;
}

// Создание новой записи
static rate_limit_entry_t* rate_limit_create_entry(const char *key) {
    rate_limit_entry_t *entry = calloc(1, sizeof(rate_limit_entry_t));
    if (!entry) return NULL;
    
    entry->key = strdup(key);
    if (!entry->key) {
        free(entry);
        return NULL;
    }
    
    entry->tokens = 0;
    entry->last_refill = time(NULL);
    entry->current_count = 0;
    entry->window_start = time(NULL);
    entry->queue_size = 0;
    entry->last_leak = time(NULL);
    entry->current_rate = 1.0;
    entry->recent_requests = 0;
    entry->recent_rejections = 0;
    entry->last_request_time = 0;
    entry->last_rejection_time = 0;
    
    return entry;
}

// Освобождение записи
static void rate_limit_free_entry(rate_limit_entry_t *entry) {
    if (!entry) return;
    if (entry->key) free(entry->key);
    free(entry);
}

// Поиск записи
static rate_limit_entry_t* rate_limit_find_entry(rate_limiter_t *limiter, const char *key) {
    if (!limiter || !key || !limiter->buckets) return NULL;
    
    uint64_t hash = rate_limit_hash(key);
    size_t bucket_idx = hash % limiter->bucket_count;
    
    rate_limit_entry_t *entry = limiter->buckets[bucket_idx];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

// Добавление записи в хэш-таблицу
static int rate_limit_add_entry(rate_limiter_t *limiter, rate_limit_entry_t *entry) {
    if (!limiter || !entry || !limiter->buckets) return -1;
    
    uint64_t hash = rate_limit_hash(entry->key);
    size_t bucket_idx = hash % limiter->bucket_count;
    
    entry->next = limiter->buckets[bucket_idx];
    if (limiter->buckets[bucket_idx]) {
        limiter->buckets[bucket_idx]->prev = entry;
    }
    limiter->buckets[bucket_idx] = entry;
    
    limiter->entry_count++;
    
    return 0;
}

// Удаление записи из хэш-таблицы
static int rate_limit_remove_entry(rate_limiter_t *limiter, rate_limit_entry_t *entry) {
    if (!limiter || !entry || !limiter->buckets) return -1;
    
    uint64_t hash = rate_limit_hash(entry->key);
    size_t bucket_idx = hash % limiter->bucket_count;
    
    if (entry->prev) {
        entry->prev->next = entry->next;
    } else {
        limiter->buckets[bucket_idx] = entry->next;
    }
    
    if (entry->next) {
        entry->next->prev = entry->prev;
    }
    
    limiter->entry_count--;
    
    return 0;
}

// Пополнение токенов (Token Bucket)
static void rate_limit_refill_tokens(rate_limit_entry_t *entry, 
                                     const rate_limit_config_t *config) {
    time_t now = time(NULL);
    time_t elapsed = now - entry->last_refill;
    
    if (elapsed > 0) {
        uint64_t new_tokens = elapsed * config->refill_rate;
        entry->tokens = entry->tokens + new_tokens;
        
        if (entry->tokens > config->bucket_capacity) {
            entry->tokens = config->bucket_capacity;
        }
        
        entry->last_refill = now;
    }
}

// Проверка Token Bucket
static rate_limit_status_t rate_limit_token_bucket(rate_limiter_t *limiter,
                                                   rate_limit_entry_t *entry) {
    const rate_limit_config_t *config = &limiter->config;
    
    // Пополнение токенов
    rate_limit_refill_tokens(entry, config);
    
    // Проверка доступности токена
    if (entry->tokens > 0) {
        entry->tokens--;
        return RATE_LIMIT_OK;
    }
    
    return RATE_LIMIT_EXCEEDED;
}

// Проверка Fixed Window
static rate_limit_status_t rate_limit_fixed_window(rate_limiter_t *limiter,
                                                   rate_limit_entry_t *entry) {
    time_t now = time(NULL);
    time_t window = limiter->config.window_seconds;
    
    // Проверка истечения окна
    if (now - entry->window_start >= window) {
        entry->current_count = 0;
        entry->window_start = now;
    }
    
    // Проверка лимита
    if (entry->current_count < limiter->config.max_requests) {
        entry->current_count++;
        return RATE_LIMIT_OK;
    }
    
    return RATE_LIMIT_EXCEEDED;
}

// Проверка Sliding Window
static rate_limit_status_t rate_limit_sliding_window(rate_limiter_t *limiter,
                                                     rate_limit_entry_t *entry) {
    time_t now = time(NULL);
    time_t window = limiter->config.window_seconds;
    
    // Сброс если окно истекло
    if (now - entry->window_start >= window) {
        entry->current_count = 0;
        entry->window_start = now;
    }
    
    // Проверка лимита
    if (entry->current_count < limiter->config.max_requests) {
        entry->current_count++;
        entry->last_request_time = now;
        return RATE_LIMIT_OK;
    }
    
    return RATE_LIMIT_EXCEEDED;
}

// Проверка Leaky Bucket
static rate_limit_status_t rate_limit_leaky_bucket(rate_limiter_t *limiter,
                                                   rate_limit_entry_t *entry) {
    time_t now = time(NULL);
    
    // "Утечка" запросов
    time_t elapsed = now - entry->last_leak;
    if (elapsed > 0) {
        uint64_t leaked = elapsed * limiter->config.leak_rate;
        if (entry->queue_size >= leaked) {
            entry->queue_size -= leaked;
        } else {
            entry->queue_size = 0;
        }
        entry->last_leak = now;
    }
    
    // Проверка вместимости
    if (entry->queue_size < limiter->config.max_requests) {
        entry->queue_size++;
        return RATE_LIMIT_OK;
    }
    
    return RATE_LIMIT_EXCEEDED;
}

// Проверка Adaptive Rate Limiting
static rate_limit_status_t rate_limit_adaptive(rate_limiter_t *limiter,
                                               rate_limit_entry_t *entry) {
    time_t now = time(NULL);
    
    // Адаптация rate на основе истории
    if (entry->recent_requests > 0) {
        double rejection_ratio = (double)entry->recent_rejections / 
                                (double)entry->recent_requests;
        
        // Уменьшение rate при высокой rejection ratio
        if (rejection_ratio > 0.5) {
            entry->current_rate *= 0.9;
            if (entry->current_rate < limiter->config.min_rate) {
                entry->current_rate = limiter->config.min_rate;
            }
        } else {
            // Увеличение rate при низкой rejection ratio
            entry->current_rate *= 1.1;
            if (entry->current_rate > limiter->config.max_rate) {
                entry->current_rate = limiter->config.max_rate;
            }
        }
        
        // Сброс счетчиков периодически
        if (entry->recent_requests > 1000) {
            entry->recent_requests = entry->recent_requests / 2;
            entry->recent_rejections = entry->recent_rejections / 2;
        }
    }
    
    // Проверка с адаптивным rate
    uint64_t adjusted_limit = (uint64_t)(limiter->config.max_requests * entry->current_rate);
    
    if (entry->current_count < adjusted_limit) {
        entry->current_count++;
        entry->recent_requests++;
        return RATE_LIMIT_OK;
    }
    
    entry->recent_requests++;
    entry->recent_rejections++;
    return RATE_LIMIT_EXCEEDED;
}

// Основная проверка rate limit
static rate_limit_status_t rate_limit_check_internal(rate_limiter_t *limiter,
                                                     rate_limit_entry_t *entry) {
    switch (limiter->config.algorithm) {
        case RATE_LIMIT_TOKEN_BUCKET:
            return rate_limit_token_bucket(limiter, entry);
        
        case RATE_LIMIT_FIXED_WINDOW:
            return rate_limit_fixed_window(limiter, entry);
        
        case RATE_LIMIT_SLIDING_WINDOW:
            return rate_limit_sliding_window(limiter, entry);
        
        case RATE_LIMIT_LEAKY_BUCKET:
            return rate_limit_leaky_bucket(limiter, entry);
        
        case RATE_LIMIT_ADAPTIVE:
            return rate_limit_adaptive(limiter, entry);
        
        default:
            return rate_limit_fixed_window(limiter, entry);
    }
}

// Инициализация rate limiter
rate_limiter_t* rate_limiter_init(const rate_limit_config_t *config) {
    if (!config) return NULL;
    
    rate_limiter_t *limiter = calloc(1, sizeof(rate_limiter_t));
    if (!limiter) return NULL;
    
    // Копирование конфигурации
    memcpy(&limiter->config, config, sizeof(rate_limit_config_t));
    
    // Инициализация хэш-таблицы
    limiter->bucket_count = config->max_requests * 10;
    if (limiter->bucket_count < 256) limiter->bucket_count = 256;
    
    limiter->buckets = calloc(limiter->bucket_count, sizeof(rate_limit_entry_t*));
    if (!limiter->buckets) {
        free(limiter);
        return NULL;
    }
    
    // Инициализация whitelist/blacklist
    limiter->whitelist = NULL;
    limiter->whitelist_count = 0;
    limiter->blacklist = NULL;
    limiter->blacklist_count = 0;
    
    // Глобальная блокировка
#ifdef _WIN32
    limiter->mutex = malloc(sizeof(CRITICAL_SECTION));
    if (limiter->mutex) {
        InitializeCriticalSection((CRITICAL_SECTION*)limiter->mutex);
    }
#else
    limiter->mutex = malloc(sizeof(pthread_mutex_t));
    if (limiter->mutex) {
        pthread_mutex_init((pthread_mutex_t*)limiter->mutex, NULL);
    }
#endif
    
    limiter->start_time = time(NULL);
    limiter->is_enabled = 1;
    limiter->is_initialized = 1;
    
    vkprintf(1, "Rate limiter initialized: algorithm=%s, max_requests=%llu, window=%lds\n",
             rate_limit_algorithm_to_string(config->algorithm),
             (unsigned long long)config->max_requests,
             (long)config->window_seconds);
    
    return limiter;
}

// Проверка rate limit (публичная функция)
rate_limit_status_t rate_limit_check(rate_limiter_t *limiter, const char *key) {
    if (!limiter || !key) return RATE_LIMIT_ERROR;
    
    // Проверка включения
    if (!limiter->is_enabled) {
        return RATE_LIMIT_OK;
    }
    
#ifdef _WIN32
    if (limiter->mutex) EnterCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_lock((pthread_mutex_t*)limiter->mutex);
#endif
    
    // Проверка blacklist
    if (limiter->config.enable_blacklist && 
        rate_limit_is_blacklisted(limiter, key)) {
        limiter->total_blacklisted++;
#ifdef _WIN32
        if (limiter->mutex) LeaveCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
        if (limiter->mutex) pthread_mutex_unlock((pthread_mutex_t*)limiter->mutex);
#endif
        return RATE_LIMIT_EXCEEDED;
    }
    
    // Проверка whitelist
    if (limiter->config.enable_whitelist && 
        rate_limit_is_whitelisted(limiter, key)) {
        limiter->total_whitelisted++;
#ifdef _WIN32
        if (limiter->mutex) LeaveCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
        if (limiter->mutex) pthread_mutex_unlock((pthread_mutex_t*)limiter->mutex);
#endif
        return RATE_LIMIT_OK;
    }
    
    // Поиск или создание записи
    rate_limit_entry_t *entry = rate_limit_find_entry(limiter, key);
    
    if (!entry) {
        // Создание новой записи
        entry = rate_limit_create_entry(key);
        if (!entry) {
#ifdef _WIN32
            if (limiter->mutex) LeaveCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
            if (limiter->mutex) pthread_mutex_unlock((pthread_mutex_t*)limiter->mutex);
#endif
            return RATE_LIMIT_ERROR;
        }
        
        rate_limit_add_entry(limiter, entry);
    }
    
    // Обновление статистики
    limiter->total_requests++;
    entry->total_requests++;
    
    // Проверка rate limit
    rate_limit_status_t status = rate_limit_check_internal(limiter, entry);
    
    if (status == RATE_LIMIT_OK) {
        // Успех
        entry->last_request_time = time(NULL);
        limiter->total_rejections += 0;
        
        // Callback
        if (limiter->on_request_allowed) {
            limiter->on_request_allowed(key);
        }
    } else {
        // Превышение лимита
        entry->total_rejections++;
        entry->last_rejection_time = time(NULL);
        limiter->total_rejections++;
        
        // Callback
        if (limiter->on_limit_exceeded) {
            limiter->on_limit_exceeded(key, entry->total_requests, 
                                      limiter->config.max_requests);
        }
    }
    
#ifdef _WIN32
    if (limiter->mutex) LeaveCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_unlock((pthread_mutex_t*)limiter->mutex);
#endif
    
    return status;
}

// Попытка acquire с количеством токенов
rate_limit_status_t rate_limit_try_acquire(rate_limiter_t *limiter, const char *key,
                                          uint64_t tokens) {
    if (!limiter || !key || tokens == 0) return RATE_LIMIT_ERROR;
    
    // Для token bucket
    if (limiter->config.algorithm == RATE_LIMIT_TOKEN_BUCKET) {
        rate_limit_entry_t *entry = rate_limit_find_entry(limiter, key);
        
        if (!entry) {
            entry = rate_limit_create_entry(key);
            if (!entry) return RATE_LIMIT_ERROR;
            rate_limit_add_entry(limiter, entry);
        }
        
        rate_limit_refill_tokens(entry, &limiter->config);
        
        if (entry->tokens >= tokens) {
            entry->tokens -= tokens;
            return RATE_LIMIT_OK;
        }
        
        return RATE_LIMIT_EXCEEDED;
    }
    
    // Для других алгоритмов - обычная проверка
    return rate_limit_check(limiter, key);
}

// Добавление в whitelist
int rate_limit_add_to_whitelist(rate_limiter_t *limiter, const char *key) {
    if (!limiter || !key) return -1;
    
    limiter->whitelist_count++;
    limiter->whitelist = realloc(limiter->whitelist, 
                                limiter->whitelist_count * sizeof(char*));
    if (!limiter->whitelist) return -1;
    
    limiter->whitelist[limiter->whitelist_count - 1] = strdup(key);
    return 0;
}

// Удаление из whitelist
int rate_limit_remove_from_whitelist(rate_limiter_t *limiter, const char *key) {
    if (!limiter || !key || !limiter->whitelist) return -1;
    
    for (int i = 0; i < limiter->whitelist_count; i++) {
        if (strcmp(limiter->whitelist[i], key) == 0) {
            free(limiter->whitelist[i]);
            
            // Сдвиг массива
            for (int j = i; j < limiter->whitelist_count - 1; j++) {
                limiter->whitelist[j] = limiter->whitelist[j + 1];
            }
            
            limiter->whitelist_count--;
            return 0;
        }
    }
    
    return -1;
}

// Добавление в blacklist
int rate_limit_add_to_blacklist(rate_limiter_t *limiter, const char *key) {
    if (!limiter || !key) return -1;
    
    limiter->blacklist_count++;
    limiter->blacklist = realloc(limiter->blacklist,
                                limiter->blacklist_count * sizeof(char*));
    if (!limiter->blacklist) return -1;
    
    limiter->blacklist[limiter->blacklist_count - 1] = strdup(key);
    return 0;
}

// Удаление из blacklist
int rate_limit_remove_from_blacklist(rate_limiter_t *limiter, const char *key) {
    if (!limiter || !key || !limiter->blacklist) return -1;
    
    for (int i = 0; i < limiter->blacklist_count; i++) {
        if (strcmp(limiter->blacklist[i], key) == 0) {
            free(limiter->blacklist[i]);
            
            for (int j = i; j < limiter->blacklist_count - 1; j++) {
                limiter->blacklist[j] = limiter->blacklist[j + 1];
            }
            
            limiter->blacklist_count--;
            return 0;
        }
    }
    
    return -1;
}

// Проверка whitelist
int rate_limit_is_whitelisted(rate_limiter_t *limiter, const char *key) {
    if (!limiter || !key || !limiter->whitelist) return 0;
    
    for (int i = 0; i < limiter->whitelist_count; i++) {
        if (strcmp(limiter->whitelist[i], key) == 0) {
            return 1;
        }
    }
    
    return 0;
}

// Проверка blacklist
int rate_limit_is_blacklisted(rate_limiter_t *limiter, const char *key) {
    if (!limiter || !key || !limiter->blacklist) return 0;
    
    for (int i = 0; i < limiter->blacklist_count; i++) {
        if (strcmp(limiter->blacklist[i], key) == 0) {
            return 1;
        }
    }
    
    return 0;
}

// Получение статистики
void rate_limiter_get_stats(rate_limiter_t *limiter, rate_limiter_stats_t *stats) {
    if (!limiter || !stats) return;
    
#ifdef _WIN32
    if (limiter->mutex) EnterCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_lock((pthread_mutex_t*)limiter->mutex);
#endif
    
    stats->total_requests = limiter->total_requests;
    stats->total_rejections = limiter->total_rejections;
    stats->total_whitelisted = limiter->total_whitelisted;
    stats->total_blacklisted = limiter->total_blacklisted;
    stats->active_entries = limiter->entry_count;
    stats->max_entries = limiter->bucket_count;
    stats->uptime_seconds = time(NULL) - limiter->start_time;
    
    if (stats->total_requests > 0) {
        stats->rejection_rate = (double)stats->total_rejections / 
                               (double)stats->total_requests * 100.0;
    } else {
        stats->rejection_rate = 0.0;
    }
    
    if (stats->uptime_seconds > 0) {
        stats->requests_per_second = stats->total_requests / stats->uptime_seconds;
    } else {
        stats->requests_per_second = 0;
    }
    
#ifdef _WIN32
    if (limiter->mutex) LeaveCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_unlock((pthread_mutex_t*)limiter->mutex);
#endif
}

// Вывод статистики
void rate_limiter_print_stats(rate_limiter_t *limiter) {
    if (!limiter) return;
    
    rate_limiter_stats_t stats;
    rate_limiter_get_stats(limiter, &stats);
    
    vkprintf(1, "Rate Limiter Statistics:\n");
    vkprintf(1, "  Total Requests: %llu\n", (unsigned long long)stats.total_requests);
    vkprintf(1, "  Total Rejections: %llu\n", (unsigned long long)stats.total_rejections);
    vkprintf(1, "  Rejection Rate: %.2f%%\n", stats.rejection_rate);
    vkprintf(1, "  Whitelisted: %llu\n", (unsigned long long)stats.total_whitelisted);
    vkprintf(1, "  Blacklisted: %llu\n", (unsigned long long)stats.total_blacklisted);
    vkprintf(1, "  Active Entries: %zu\n", stats.active_entries);
    vkprintf(1, "  Uptime: %lld seconds\n", (long long)stats.uptime_seconds);
    vkprintf(1, "  Requests/sec: %llu\n", (unsigned long long)stats.requests_per_second);
}

// Сброс статистики
void rate_limiter_reset_stats(rate_limiter_t *limiter) {
    if (!limiter) return;
    
#ifdef _WIN32
    if (limiter->mutex) EnterCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_lock((pthread_mutex_t*)limiter->mutex);
#endif
    
    limiter->total_requests = 0;
    limiter->total_rejections = 0;
    limiter->total_whitelisted = 0;
    limiter->total_blacklisted = 0;
    
#ifdef _WIN32
    if (limiter->mutex) LeaveCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_unlock((pthread_mutex_t*)limiter->mutex);
#endif
}

// Получение информации о клиенте
int rate_limiter_get_info(rate_limiter_t *limiter, const char *key, 
                         rate_limit_info_t *info) {
    if (!limiter || !key || !info) return -1;
    
#ifdef _WIN32
    if (limiter->mutex) EnterCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_lock((pthread_mutex_t*)limiter->mutex);
#endif
    
    rate_limit_entry_t *entry = rate_limit_find_entry(limiter, key);
    
    if (!entry) {
#ifdef _WIN32
        if (limiter->mutex) LeaveCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
        if (limiter->mutex) pthread_mutex_unlock((pthread_mutex_t*)limiter->mutex);
#endif
        return -1;
    }
    
    strncpy(info->key, key, sizeof(info->key) - 1);
    info->limit = limiter->config.max_requests;
    info->total_requests = entry->total_requests;
    info->total_rejections = entry->total_rejections;
    
    if (entry->total_requests > 0) {
        info->rejection_rate = (double)entry->total_rejections / 
                              (double)entry->total_requests * 100.0;
    } else {
        info->rejection_rate = 0.0;
    }
    
    // Расчет оставшихся запросов и времени сброса
    time_t now = time(NULL);
    time_t window = limiter->config.window_seconds;
    
    info->reset_time = entry->window_start + window;
    
    if (entry->current_count >= limiter->config.max_requests) {
        info->remaining_requests = 0;
        info->retry_after = info->reset_time - now;
    } else {
        info->remaining_requests = limiter->config.max_requests - entry->current_count;
        info->retry_after = 0;
    }
    
#ifdef _WIN32
    if (limiter->mutex) LeaveCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_unlock((pthread_mutex_t*)limiter->mutex);
#endif
    
    return 0;
}

// Сброс клиента
int rate_limiter_reset_client(rate_limiter_t *limiter, const char *key) {
    if (!limiter || !key) return -1;
    
#ifdef _WIN32
    if (limiter->mutex) EnterCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_lock((pthread_mutex_t*)limiter->mutex);
#endif
    
    rate_limit_entry_t *entry = rate_limit_find_entry(limiter, key);
    
    if (entry) {
        entry->current_count = 0;
        entry->tokens = limiter->config.bucket_capacity;
        entry->window_start = time(NULL);
        entry->queue_size = 0;
    }
    
#ifdef _WIN32
    if (limiter->mutex) LeaveCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_unlock((pthread_mutex_t*)limiter->mutex);
#endif
    
    return entry ? 0 : -1;
}

// Удаление клиента
int rate_limiter_remove_client(rate_limiter_t *limiter, const char *key) {
    if (!limiter || !key) return -1;
    
#ifdef _WIN32
    if (limiter->mutex) EnterCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_lock((pthread_mutex_t*)limiter->mutex);
#endif
    
    rate_limit_entry_t *entry = rate_limit_find_entry(limiter, key);
    
    if (entry) {
        rate_limit_remove_entry(limiter, entry);
        rate_limit_free_entry(entry);
    }
    
#ifdef _WIN32
    if (limiter->mutex) LeaveCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_unlock((pthread_mutex_t*)limiter->mutex);
#endif
    
    return entry ? 0 : -1;
}

// Очистка expired записей
void rate_limiter_cleanup_expired(rate_limiter_t *limiter) {
    if (!limiter) return;
    
#ifdef _WIN32
    if (limiter->mutex) EnterCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_lock((pthread_mutex_t*)limiter->mutex);
#endif
    
    time_t now = time(NULL);
    time_t expiry_threshold = 3600; // 1 час без активности
    
    for (size_t i = 0; i < limiter->bucket_count; i++) {
        rate_limit_entry_t *entry = limiter->buckets[i];
        while (entry) {
            rate_limit_entry_t *next = entry->next;
            
            if (now - entry->last_request_time > expiry_threshold) {
                rate_limit_remove_entry(limiter, entry);
                rate_limit_free_entry(entry);
            }
            
            entry = next;
        }
    }
    
#ifdef _WIN32
    if (limiter->mutex) LeaveCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_unlock((pthread_mutex_t*)limiter->mutex);
#endif
}

// Включение/выключение
int rate_limiter_enable(rate_limiter_t *limiter) {
    if (!limiter) return -1;
    limiter->is_enabled = 1;
    return 0;
}

int rate_limiter_disable(rate_limiter_t *limiter) {
    if (!limiter) return -1;
    limiter->is_enabled = 0;
    return 0;
}

int rate_limiter_is_enabled(rate_limiter_t *limiter) {
    return limiter ? limiter->is_enabled : 0;
}

// Установка callback'ов
void rate_limiter_set_exceeded_callback(rate_limiter_t *limiter,
                                       void (*callback)(const char*, uint64_t, uint64_t)) {
    if (limiter) {
        limiter->on_limit_exceeded = callback;
    }
}

void rate_limiter_set_allowed_callback(rate_limiter_t *limiter,
                                      void (*callback)(const char*)) {
    if (limiter) {
        limiter->on_request_allowed = callback;
    }
}

// Утилиты
char* rate_limit_status_to_string(rate_limit_status_t status) {
    switch (status) {
        case RATE_LIMIT_OK: return "OK";
        case RATE_LIMIT_EXCEEDED: return "EXCEEDED";
        case RATE_LIMIT_ERROR: return "ERROR";
        case RATE_LIMIT_NOT_FOUND: return "NOT_FOUND";
        case RATE_LIMIT_INVALID_CONFIG: return "INVALID_CONFIG";
        default: return "UNKNOWN";
    }
}

const char* rate_limit_algorithm_to_string(rate_limit_algorithm_t algo) {
    switch (algo) {
        case RATE_LIMIT_TOKEN_BUCKET: return "Token Bucket";
        case RATE_LIMIT_SLIDING_WINDOW: return "Sliding Window";
        case RATE_LIMIT_FIXED_WINDOW: return "Fixed Window";
        case RATE_LIMIT_LEAKY_BUCKET: return "Leaky Bucket";
        case RATE_LIMIT_ADAPTIVE: return "Adaptive";
        default: return "Unknown";
    }
}

// Очистка ресурсов
void rate_limiter_cleanup(rate_limiter_t *limiter) {
    if (!limiter) return;
    
#ifdef _WIN32
    if (limiter->mutex) EnterCriticalSection((CRITICAL_SECTION*)limiter->mutex);
#else
    if (limiter->mutex) pthread_mutex_lock((pthread_mutex_t*)limiter->mutex);
#endif
    
    // Очистка всех записей
    for (size_t i = 0; i < limiter->bucket_count; i++) {
        rate_limit_entry_t *entry = limiter->buckets[i];
        while (entry) {
            rate_limit_entry_t *next = entry->next;
            rate_limit_free_entry(entry);
            entry = next;
        }
        limiter->buckets[i] = NULL;
    }
    
    // Очистка whitelist
    for (int i = 0; i < limiter->whitelist_count; i++) {
        free(limiter->whitelist[i]);
    }
    if (limiter->whitelist) free(limiter->whitelist);
    
    // Очистка blacklist
    for (int i = 0; i < limiter->blacklist_count; i++) {
        free(limiter->blacklist[i]);
    }
    if (limiter->blacklist) free(limiter->blacklist);
    
    // Освобождение бакетов
    if (limiter->buckets) free(limiter->buckets);
    
    // Освобождение блокировки
#ifdef _WIN32
    if (limiter->mutex) {
        DeleteCriticalSection((CRITICAL_SECTION*)(limiter->mutex));
        free(limiter->mutex);
    }
#else
    if (limiter->mutex) {
        pthread_mutex_destroy((pthread_mutex_t*)(limiter->mutex));
        free(limiter->mutex);
    }
#endif
    
    vkprintf(1, "Rate limiter cleaned up\n");
    free(limiter);
}
