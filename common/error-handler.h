/*
    Система обработки ошибок (Error Handler) для MTProxy
    Централизованное управление ошибками, коды ошибок, восстановление
*/

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

// Уровни серьезности ошибок
typedef enum {
    ERROR_LEVEL_INFO = 0,
    ERROR_LEVEL_WARNING = 1,
    ERROR_LEVEL_ERROR = 2,
    ERROR_LEVEL_CRITICAL = 3,
    ERROR_LEVEL_FATAL = 4
} error_level_t;

// Категории ошибок
typedef enum {
    ERROR_CATEGORY_NONE = 0,
    ERROR_CATEGORY_SYSTEM = 1,      // Системные ошибки
    ERROR_CATEGORY_NETWORK = 2,     // Сетевые ошибки
    ERROR_CATEGORY_MEMORY = 3,      // Ошибки памяти
    ERROR_CATEGORY_IO = 4,          // Ошибки ввода-вывода
    ERROR_CATEGORY_CONFIG = 5,      // Ошибки конфигурации
    ERROR_CATEGORY_SECURITY = 6,    // Ошибки безопасности
    ERROR_CATEGORY_PROTOCOL = 7,    // Ошибки протокола
    ERROR_CATEGORY_RESOURCE = 8,    // Нехватка ресурсов
    ERROR_CATEGORY_TIMEOUT = 9,     // Таймауты
    ERROR_CATEGORY_AUTH = 10,       // Ошибки аутентификации
    ERROR_CATEGORY_RATE_LIMIT = 11  // Превышение лимитов
} error_category_t;

// Коды ошибок MTProxy
typedef enum {
    MTERR_OK = 0,
    MTERR_UNKNOWN = 1,
    
    // Системные ошибки (1-99)
    MTERR_SYSTEM_INIT = 10,
    MTERR_SYSTEM_MUTEX = 11,
    MTERR_SYSTEM_THREAD = 12,
    MTERR_SYSTEM_PERMISSION = 13,
    
    // Ошибки памяти (100-199)
    MTERR_MEMORY_ALLOC = 100,
    MTERR_MEMORY_FREE = 101,
    MTERR_MEMORY_OVERFLOW = 102,
    MTERR_MEMORY_LEAK = 103,
    
    // Сетевые ошибки (200-299)
    MTERR_NETWORK_CONNECT = 200,
    MTERR_NETWORK_DISCONNECT = 201,
    MTERR_NETWORK_TIMEOUT = 202,
    MTERR_NETWORK_UNREACHABLE = 203,
    MTERR_NETWORK_DNS = 204,
    MTERR_NETWORK_SSL = 205,
    
    // Ошибки ввода-вывода (300-399)
    MTERR_IO_READ = 300,
    MTERR_IO_WRITE = 301,
    MTERR_IO_OPEN = 302,
    MTERR_IO_CLOSE = 303,
    MTERR_IO_PERMISSION = 304,
    
    // Ошибки конфигурации (400-499)
    MTERR_CONFIG_PARSE = 400,
    MTERR_CONFIG_MISSING = 401,
    MTERR_CONFIG_INVALID = 402,
    MTERR_CONFIG_DEPRECATED = 403,
    
    // Ошибки безопасности (500-599)
    MTERR_SECURITY_INVALID_CERT = 500,
    MTERR_SECURITY_AUTH_FAILED = 501,
    MTERR_SECURITY_ACCESS_DENIED = 502,
    MTERR_SECURITY_ENCRYPTION = 503,
    MTERR_SECURITY_SIGNATURE = 504,
    
    // Ошибки протокола (600-699)
    MTERR_PROTOCOL_INVALID = 600,
    MTERR_PROTOCOL_VERSION = 601,
    MTERR_PROTOCOL_FORMAT = 602,
    MTERR_PROTOCOL_HANDSHAKE = 603,
    MTERR_PROTOCOL_DECRYPT = 604,
    
    // Ошибки ресурсов (700-799)
    MTERR_RESOURCE_EXHAUSTED = 700,
    MTERR_RESOURCE_LIMIT = 701,
    MTERR_RESOURCE_UNAVAILABLE = 702,
    
    // Таймауты (800-899)
    MTERR_TIMEOUT_REQUEST = 800,
    MTERR_TIMEOUT_CONNECTION = 801,
    MTERR_TIMEOUT_OPERATION = 802,
    
    // Ошибки аутентификации (900-999)
    MTERR_AUTH_INVALID_TOKEN = 900,
    MTERR_AUTH_EXPIRED = 901,
    MTERR_AUTH_INVALID_CREDENTIALS = 902,
    
    // Rate limiting (1000-1099)
    MTERR_RATE_LIMIT_EXCEEDED = 1000,
    MTERR_RATE_LIMIT_QUOTA = 1001
} mtproxy_error_code_t;

// Структура ошибки
typedef struct {
    int error_code;
    error_level_t level;
    error_category_t category;
    char message[512];
    char details[256];
    char source_file[128];
    int source_line;
    char function[64];
    time_t timestamp;
    int os_error;  // errno или GetLastError()
    void *context; // Дополнительный контекст
    uint64_t correlation_id; // ID для трассировки
} error_info_t;

// Обработчик ошибок
typedef void (*error_handler_callback_t)(const error_info_t *error, void *user_data);

// Стратегии восстановления
typedef enum {
    RECOVERY_NONE = 0,
    RECOVERY_RETRY = 1,
    RECOVERY_FALLBACK = 2,
    RECOVERY_RESTART = 3,
    RECOVERY_SHUTDOWN = 4
} recovery_strategy_t;

// Конфигурация восстановления
typedef struct {
    recovery_strategy_t strategy;
    int max_retries;
    time_t retry_delay_ms;
    double backoff_multiplier;  // Для exponential backoff
    time_t max_retry_delay_ms;
    int enable_circuit_breaker;
    int circuit_breaker_threshold;
    time_t circuit_breaker_timeout_ms;
} recovery_config_t;

// Статистика ошибок
typedef struct {
    uint64_t total_errors;
    uint64_t errors_by_level[5];
    uint64_t errors_by_category[12];
    uint64_t recovered_errors;
    uint64_t unrecovered_errors;
    uint64_t circuit_breaker_trips;
    time_t last_error_time;
    uint32_t errors_last_minute;
    uint32_t errors_last_hour;
} error_stats_t;

// Контекст системы обработки ошибок
typedef struct {
    // Конфигурация
    recovery_config_t recovery_config;
    int enable_logging;
    int enable_stack_trace;
    int enable_correlation_tracking;
    
    // Обработчики
    error_handler_callback_t global_handler;
    error_handler_callback_t category_handlers[12];
    void *user_data;
    
    // Статистика
    error_stats_t stats;
    
    // Circuit breaker
    int circuit_breaker_open;
    time_t circuit_breaker_opened_at;
    uint32_t recent_failure_count;
    time_t recent_failure_window_start;
    
    // Хранение последней ошибки
    error_info_t last_error;
    int has_last_error;
    
    // Синхронизация
    void *mutex;
    
    // Состояние
    int is_initialized;
} error_handler_context_t;

// Инициализация и очистка
int error_handler_init(error_handler_context_t *ctx, const recovery_config_t *config);
void error_handler_cleanup(error_handler_context_t *ctx);

// Создание ошибок
error_info_t* error_create(int error_code, error_level_t level, 
                          error_category_t category, const char *message);
error_info_t* error_create_with_details(int error_code, error_level_t level,
                                       error_category_t category,
                                       const char *message, const char *details,
                                       const char *file, int line, const char *function);
void error_free(error_info_t *error);

// Установка последней ошибки
int error_set_last(error_handler_context_t *ctx, const error_info_t *error);
int error_get_last(error_handler_context_t *ctx, error_info_t *error);
void error_clear_last(error_handler_context_t *ctx);

// Обработка ошибок
int error_handle(error_handler_context_t *ctx, const error_info_t *error);
int error_handle_and_recover(error_handler_context_t *ctx, error_info_t *error);

// Проверки
int error_is_recoverable(const error_info_t *error);
int error_should_retry(const error_info_t *error);
int error_is_circuit_breaker_open(error_handler_context_t *ctx);

// Восстановление
int error_attempt_recovery(error_handler_context_t *ctx, error_info_t *error);
int error_execute_with_retry(error_handler_context_t *ctx,
                            int (*operation)(void*),
                            void *user_data,
                            int max_retries);

// Circuit breaker
int error_circuit_breaker_record_failure(error_handler_context_t *ctx);
int error_circuit_breaker_record_success(error_handler_context_t *ctx);
int error_circuit_breaker_can_execute(error_handler_context_t *ctx);

// Статистика
void error_get_stats(error_handler_context_t *ctx, error_stats_t *stats);
void error_reset_stats(error_handler_context_t *ctx);
void error_print_stats(error_handler_context_t *ctx);

// Утилиты
const char* error_code_to_string(int error_code);
const char* error_level_to_string(error_level_t level);
const char* error_category_to_string(error_category_t category);
int error_code_from_string(const char *str);

// Вспомогательные макросы
#define ERROR_CREATE(code, level, category, msg) \
    error_create((code), (level), (category), (msg))

#define ERROR_CREATE_DETAIL(code, level, category, msg, details) \
    error_create_with_details((code), (level), (category), (msg), (details), \
                             __FILE__, __LINE__, __FUNCTION__)

#define ERROR_HANDLE(ctx, error) \
    error_handle((ctx), (error))

#define ERROR_CHECK(ctx, operation, error_code, level, category, msg) \
    do { \
        if ((operation) != 0) { \
            error_info_t *_err = ERROR_CREATE_DETAIL(error_code, level, category, msg, NULL); \
            error_handle((ctx), _err); \
            error_free(_err); \
        } \
    } while(0)

// Макросы для конкретных категорий ошибок
#define SYSTEM_ERROR(ctx, code, msg) \
    ERROR_CREATE_DETAIL(code, ERROR_LEVEL_ERROR, ERROR_CATEGORY_SYSTEM, msg, NULL)

#define NETWORK_ERROR(ctx, code, msg) \
    ERROR_CREATE_DETAIL(code, ERROR_LEVEL_ERROR, ERROR_CATEGORY_NETWORK, msg, NULL)

#define MEMORY_ERROR(ctx, code, msg) \
    ERROR_CREATE_DETAIL(code, ERROR_LEVEL_CRITICAL, ERROR_CATEGORY_MEMORY, msg, NULL)

#define SECURITY_ERROR(ctx, code, msg) \
    ERROR_CREATE_DETAIL(code, ERROR_LEVEL_CRITICAL, ERROR_CATEGORY_SECURITY, msg, NULL)

#define CONFIG_ERROR(ctx, code, msg) \
    ERROR_CREATE_DETAIL(code, ERROR_LEVEL_ERROR, ERROR_CATEGORY_CONFIG, msg, NULL)

#ifdef __cplusplus
}
#endif

#endif // ERROR_HANDLER_H
