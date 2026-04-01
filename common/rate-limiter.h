/*
    Система ограничения скорости (Rate Limiter) для MTProxy
    Поддержка различных алгоритмов: Token Bucket, Sliding Window, Fixed Window, Leaky Bucket
*/

#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Алгоритмы rate limiting
typedef enum {
    RATE_LIMIT_TOKEN_BUCKET = 0,    // Бакет с токенами
    RATE_LIMIT_SLIDING_WINDOW,       // Скользящее окно
    RATE_LIMIT_FIXED_WINDOW,         // Фиксированное окно
    RATE_LIMIT_LEAKY_BUCKET,         // Протекающий бакет
    RATE_LIMIT_ADAPTIVE              // Адаптивный (динамический)
} rate_limit_algorithm_t;

// Статусы операций
typedef enum {
    RATE_LIMIT_OK = 0,
    RATE_LIMIT_EXCEEDED,
    RATE_LIMIT_ERROR,
    RATE_LIMIT_NOT_FOUND,
    RATE_LIMIT_INVALID_CONFIG
} rate_limit_status_t;

// Конфигурация rate limiter
typedef struct {
    rate_limit_algorithm_t algorithm;
    
    // Основные параметры
    uint64_t max_requests;      // Максимум запросов
    time_t window_seconds;      // Окно времени в секундах
    
    // Для token bucket
    uint64_t bucket_capacity;   // Вместимость бакета
    uint64_t refill_rate;       // Скорость пополнения (токенов/сек)
    
    // Для leaky bucket
    uint64_t leak_rate;         // Скорость утечки (запросов/сек)
    
    // Для adaptive
    double min_rate;            // Минимальный rate (0.0-1.0)
    double max_rate;            // Максимальный rate (0.0-1.0)
    double adaptation_factor;   // Фактор адаптации
    
    // Общие настройки
    int enable_burst;           // Разрешить burst
    uint64_t burst_capacity;    // Вместимость burst
    int enable_whitelist;       // Белый список
    int enable_blacklist;       // Черный список
    
    // Хранение
    int enable_persistence;
    char persistence_path[256];
    time_t persistence_interval;
} rate_limit_config_t;

// Запись rate limiter (для одного клиента/ключа)
typedef struct rate_limit_entry {
    char *key;                      // Идентификатор (IP, user_id, etc)
    
    // Token bucket state
    uint64_t tokens;
    time_t last_refill;
    
    // Window state
    uint64_t current_count;
    time_t window_start;
    
    // Leaky bucket state
    uint64_t queue_size;
    time_t last_leak;
    
    // Adaptive state
    double current_rate;
    uint64_t recent_requests;
    uint64_t recent_rejections;
    
    // Statistics
    uint64_t total_requests;
    uint64_t total_rejections;
    time_t last_request_time;
    time_t last_rejection_time;
    
    // Linked list for cleanup
    struct rate_limit_entry *next;
    struct rate_limit_entry *prev;
} rate_limit_entry_t;

// Контекст rate limiter
typedef struct {
    rate_limit_config_t config;
    
    // Хэш-таблица записей
    rate_limit_entry_t **buckets;
    size_t bucket_count;
    size_t entry_count;
    
    // LRU список для eviction
    rate_limit_entry_t *lru_head;
    rate_limit_entry_t *lru_tail;
    
    // Whitelist/Blacklist
    char **whitelist;
    int whitelist_count;
    char **blacklist;
    int blacklist_count;
    
    // Глобальная статистика
    uint64_t total_requests;
    uint64_t total_rejections;
    uint64_t total_whitelisted;
    uint64_t total_blacklisted;
    time_t start_time;
    
    // Синхронизация
    void *mutex;
    
    // Состояние
    int is_initialized;
    int is_enabled;
    
    // Callback функции
    void (*on_limit_exceeded)(const char *key, uint64_t current, uint64_t limit);
    void (*on_request_allowed)(const char *key);
    void *user_data;
} rate_limiter_t;

// Статистика rate limiter
typedef struct {
    uint64_t total_requests;
    uint64_t total_rejections;
    uint64_t total_whitelisted;
    uint64_t total_blacklisted;
    double rejection_rate;
    size_t active_entries;
    size_t max_entries;
    time_t uptime_seconds;
    uint64_t requests_per_second;
} rate_limiter_stats_t;

// Инициализация и очистка
rate_limiter_t* rate_limiter_init(const rate_limit_config_t *config);
int rate_limiter_configure(rate_limiter_t *limiter, const rate_limit_config_t *config);
void rate_limiter_cleanup(rate_limiter_t *limiter);

// Основные операции
rate_limit_status_t rate_limit_check(rate_limiter_t *limiter, const char *key);
rate_limit_status_t rate_limit_acquire(rate_limiter_t *limiter, const char *key);
rate_limit_status_t rate_limit_try_acquire(rate_limiter_t *limiter, const char *key, 
                                          uint64_t tokens);
int rate_limit_is_allowed(rate_limiter_t *limiter, const char *key);

// Управление списками
int rate_limit_add_to_whitelist(rate_limiter_t *limiter, const char *key);
int rate_limit_remove_from_whitelist(rate_limiter_t *limiter, const char *key);
int rate_limit_add_to_blacklist(rate_limiter_t *limiter, const char *key);
int rate_limit_remove_from_blacklist(rate_limiter_t *limiter, const char *key);
int rate_limit_is_whitelisted(rate_limiter_t *limiter, const char *key);
int rate_limit_is_blacklisted(rate_limiter_t *limiter, const char *key);

// Статистика
void rate_limiter_get_stats(rate_limiter_t *limiter, rate_limiter_stats_t *stats);
void rate_limiter_print_stats(rate_limiter_t *limiter);
void rate_limiter_reset_stats(rate_limiter_t *limiter);

// Информация о клиенте
typedef struct {
    char key[128];
    uint64_t remaining_requests;
    uint64_t limit;
    time_t reset_time;
    time_t retry_after;
    uint64_t total_requests;
    uint64_t total_rejections;
    double rejection_rate;
} rate_limit_info_t;

int rate_limiter_get_info(rate_limiter_t *limiter, const char *key, rate_limit_info_t *info);

// Управление записями
int rate_limiter_reset_client(rate_limiter_t *limiter, const char *key);
int rate_limiter_remove_client(rate_limiter_t *limiter, const char *key);
void rate_limiter_cleanup_expired(rate_limiter_t *limiter);
void rate_limiter_cleanup_lru(rate_limiter_t *limiter, size_t count);

// Динамическая конфигурация
int rate_limiter_set_limit(rate_limiter_t *limiter, const char *key,
                          uint64_t max_requests, time_t window_seconds);
int rate_limiter_get_limit(rate_limiter_t *limiter, const char *key,
                          uint64_t *max_requests, time_t *window_seconds);

// Включение/выключение
int rate_limiter_enable(rate_limiter_t *limiter);
int rate_limiter_disable(rate_limiter_t *limiter);
int rate_limiter_is_enabled(rate_limiter_t *limiter);

// Callback функции
void rate_limiter_set_exceeded_callback(rate_limiter_t *limiter,
                                       void (*callback)(const char*, uint64_t, uint64_t));
void rate_limiter_set_allowed_callback(rate_limiter_t *limiter,
                                      void (*callback)(const char*));

// Персистентность
int rate_limiter_save_state(rate_limiter_t *limiter, const char *filename);
int rate_limiter_load_state(rate_limiter_t *limiter, const char *filename);

// Утилиты
uint64_t rate_limit_calculate_retry_after(rate_limiter_t *limiter, const char *key);
time_t rate_limit_calculate_reset_time(rate_limiter_t *limiter, const char *key);
char* rate_limit_status_to_string(rate_limit_status_t status);
const char* rate_limit_algorithm_to_string(rate_limit_algorithm_t algo);

#ifdef __cplusplus
}
#endif

#endif // RATE_LIMITER_H
