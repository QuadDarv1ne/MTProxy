/*
    Система обработки ошибок (Error Handler) для MTProxy
    Централизованное управление ошибками, коды ошибок, восстановление
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

#include "common/error-handler.h"
#include "common/kprintf.h"

// Глобальный контекст
static error_handler_context_t global_error_ctx = {0};

// Таблица сообщений об ошибках
static const struct {
    int code;
    const char *message;
} error_messages[] = {
    {MTERR_OK, "Success"},
    {MTERR_UNKNOWN, "Unknown error"},
    {MTERR_SYSTEM_INIT, "System initialization failed"},
    {MTERR_SYSTEM_MUTEX, "Mutex operation failed"},
    {MTERR_SYSTEM_THREAD, "Thread operation failed"},
    {MTERR_SYSTEM_PERMISSION, "Permission denied"},
    {MTERR_MEMORY_ALLOC, "Memory allocation failed"},
    {MTERR_MEMORY_FREE, "Memory free failed"},
    {MTERR_MEMORY_OVERFLOW, "Buffer overflow detected"},
    {MTERR_MEMORY_LEAK, "Memory leak detected"},
    {MTERR_NETWORK_CONNECT, "Connection failed"},
    {MTERR_NETWORK_DISCONNECT, "Connection lost"},
    {MTERR_NETWORK_TIMEOUT, "Network timeout"},
    {MTERR_NETWORK_UNREACHABLE, "Network unreachable"},
    {MTERR_NETWORK_DNS, "DNS resolution failed"},
    {MTERR_NETWORK_SSL, "SSL/TLS error"},
    {MTERR_IO_READ, "Read operation failed"},
    {MTERR_IO_WRITE, "Write operation failed"},
    {MTERR_IO_OPEN, "Failed to open file"},
    {MTERR_IO_CLOSE, "Failed to close file"},
    {MTERR_IO_PERMISSION, "File permission denied"},
    {MTERR_CONFIG_PARSE, "Configuration parse error"},
    {MTERR_CONFIG_MISSING, "Configuration file missing"},
    {MTERR_CONFIG_INVALID, "Invalid configuration value"},
    {MTERR_CONFIG_DEPRECATED, "Deprecated configuration option"},
    {MTERR_SECURITY_INVALID_CERT, "Invalid certificate"},
    {MTERR_SECURITY_AUTH_FAILED, "Authentication failed"},
    {MTERR_SECURITY_ACCESS_DENIED, "Access denied"},
    {MTERR_SECURITY_ENCRYPTION, "Encryption error"},
    {MTERR_SECURITY_SIGNATURE, "Signature verification failed"},
    {MTERR_PROTOCOL_INVALID, "Invalid protocol message"},
    {MTERR_PROTOCOL_VERSION, "Protocol version mismatch"},
    {MTERR_PROTOCOL_FORMAT, "Protocol format error"},
    {MTERR_PROTOCOL_HANDSHAKE, "Handshake failed"},
    {MTERR_PROTOCOL_DECRYPT, "Decryption failed"},
    {MTERR_RESOURCE_EXHAUSTED, "Resources exhausted"},
    {MTERR_RESOURCE_LIMIT, "Resource limit exceeded"},
    {MTERR_RESOURCE_UNAVAILABLE, "Resource unavailable"},
    {MTERR_TIMEOUT_REQUEST, "Request timeout"},
    {MTERR_TIMEOUT_CONNECTION, "Connection timeout"},
    {MTERR_TIMEOUT_OPERATION, "Operation timeout"},
    {MTERR_AUTH_INVALID_TOKEN, "Invalid authentication token"},
    {MTERR_AUTH_EXPIRED, "Authentication expired"},
    {MTERR_AUTH_INVALID_CREDENTIALS, "Invalid credentials"},
    {MTERR_RATE_LIMIT_EXCEEDED, "Rate limit exceeded"},
    {MTERR_RATE_LIMIT_QUOTA, "Quota exceeded"},
    {-1, NULL}
};

// Получение сообщения об ошибке
static const char* error_get_default_message(int error_code) {
    for (int i = 0; error_messages[i].message != NULL; i++) {
        if (error_messages[i].code == error_code) {
            return error_messages[i].message;
        }
    }
    return "Unknown error";
}

// Инициализация
int error_handler_init(error_handler_context_t *ctx, const recovery_config_t *config) {
    if (!ctx) return -1;
    
    memset(ctx, 0, sizeof(error_handler_context_t));
    
    // Копирование конфигурации восстановления
    if (config) {
        memcpy(&ctx->recovery_config, config, sizeof(recovery_config_t));
    } else {
        // Конфигурация по умолчанию
        ctx->recovery_config.strategy = RECOVERY_RETRY;
        ctx->recovery_config.max_retries = 3;
        ctx->recovery_config.retry_delay_ms = 1000;
        ctx->recovery_config.backoff_multiplier = 2.0;
        ctx->recovery_config.max_retry_delay_ms = 30000;
        ctx->recovery_config.enable_circuit_breaker = 1;
        ctx->recovery_config.circuit_breaker_threshold = 5;
        ctx->recovery_config.circuit_breaker_timeout_ms = 60000;
    }
    
    ctx->enable_logging = 1;
    ctx->enable_correlation_tracking = 1;

    // Инициализация блокировки
#ifdef _WIN32
    ctx->mutex = malloc(sizeof(CRITICAL_SECTION));
    if (ctx->mutex) {
        InitializeCriticalSection((CRITICAL_SECTION*)ctx->mutex);
    } else {
        return -1;
    }
#else
    ctx->mutex = malloc(sizeof(pthread_mutex_t));
    if (ctx->mutex) {
        pthread_mutex_init((pthread_mutex_t*)ctx->mutex, NULL);
    } else {
        return -1;
    }
#endif

    ctx->is_initialized = 1;
    
    vkprintf(1, "Error handler initialized\n");
    return 0;
}

// Очистка
void error_handler_cleanup(error_handler_context_t *ctx) {
    if (!ctx || !ctx->is_initialized) return;
    
    // Освобождение блокировки
#ifdef _WIN32
    if (ctx->mutex) {
        DeleteCriticalSection((CRITICAL_SECTION*)(ctx->mutex));
        free(ctx->mutex);
    }
#else
    if (ctx->mutex) {
        pthread_mutex_destroy((pthread_mutex_t*)(ctx->mutex));
        free(ctx->mutex);
    }
#endif
    
    ctx->is_initialized = 0;
    vkprintf(1, "Error handler cleaned up\n");
}

// Создание ошибки
error_info_t* error_create(int error_code, error_level_t level,
                          error_category_t category, const char *message) {
    error_info_t *error = calloc(1, sizeof(error_info_t));
    if (!error) return NULL;
    
    error->error_code = error_code;
    error->level = level;
    error->category = category;
    error->timestamp = time(NULL);
    error->os_error = errno;
    
    // Сообщение об ошибке
    if (message) {
        strncpy(error->message, message, sizeof(error->message) - 1);
    } else {
        strncpy(error->message, error_get_default_message(error_code),
                sizeof(error->message) - 1);
    }
    
    return error;
}

// Создание ошибки с деталями
error_info_t* error_create_with_details(int error_code, error_level_t level,
                                       error_category_t category,
                                       const char *message, const char *details,
                                       const char *file, int line, const char *function) {
    error_info_t *error = error_create(error_code, level, category, message);
    if (!error) return NULL;
    
    // Информация о источнике
    if (file) {
        // Копируем только имя файла без пути
        const char *filename = strrchr(file, '/');
        if (!filename) filename = strrchr(file, '\\');
        filename = filename ? filename + 1 : file;
        strncpy(error->source_file, filename, sizeof(error->source_file) - 1);
    }
    error->source_line = line;
    
    if (function) {
        strncpy(error->function, function, sizeof(error->function) - 1);
    }
    
    // Детали
    if (details) {
        strncpy(error->details, details, sizeof(error->details) - 1);
    }
    
    return error;
}

// Освобождение ошибки
void error_free(error_info_t *error) {
    if (error) {
        if (error->context) free(error->context);
        free(error);
    }
}

// Установка последней ошибки
int error_set_last(error_handler_context_t *ctx, const error_info_t *error) {
    if (!ctx || !error) return -1;
    
#ifdef _WIN32
    if (ctx->mutex) EnterCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_lock((pthread_mutex_t*)ctx->mutex);
#endif
    
    memcpy(&ctx->last_error, error, sizeof(error_info_t));
    ctx->has_last_error = 1;
    
    // Обновление статистики
    ctx->stats.total_errors++;
    if (error->level < 5) {
        ctx->stats.errors_by_level[error->level]++;
    }
    if (error->category < 12) {
        ctx->stats.errors_by_category[error->category]++;
    }
    ctx->stats.last_error_time = error->timestamp;
    
    // Подсчет ошибок за последнюю минуту
    time_t now = time(NULL);
    if (now - ctx->stats.last_error_time < 60) {
        ctx->stats.errors_last_minute++;
    } else {
        ctx->stats.errors_last_minute = 1;
    }
    
#ifdef _WIN32
    if (ctx->mutex) LeaveCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_unlock((pthread_mutex_t*)ctx->mutex);
#endif
    
    return 0;
}

// Получение последней ошибки
int error_get_last(error_handler_context_t *ctx, error_info_t *error) {
    if (!ctx || !error) return -1;
    
#ifdef _WIN32
    if (ctx->mutex) EnterCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_lock((pthread_mutex_t*)ctx->mutex);
#endif
    
    if (ctx->has_last_error) {
        memcpy(error, &ctx->last_error, sizeof(error_info_t));
    } else {
        memset(error, 0, sizeof(error_info_t));
    }
    
#ifdef _WIN32
    if (ctx->mutex) LeaveCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_unlock((pthread_mutex_t*)ctx->mutex);
#endif
    
    return ctx->has_last_error ? 0 : -1;
}

// Очистка последней ошибки
void error_clear_last(error_handler_context_t *ctx) {
    if (!ctx) return;
    
#ifdef _WIN32
    if (ctx->mutex) EnterCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_lock((pthread_mutex_t*)ctx->mutex);
#endif
    
    memset(&ctx->last_error, 0, sizeof(error_info_t));
    ctx->has_last_error = 0;
    
#ifdef _WIN32
    if (ctx->mutex) LeaveCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_unlock((pthread_mutex_t*)ctx->mutex);
#endif
}

// Обработка ошибки
int error_handle(error_handler_context_t *ctx, const error_info_t *error) {
    if (!ctx || !error) return -1;
    
    // Логирование
    if (ctx->enable_logging) {
        vkprintf(1, "[%s] Error %d: %s (category: %s, source: %s:%d)\n",
                error_level_to_string(error->level),
                error->error_code,
                error->message,
                error_category_to_string(error->category),
                error->source_file[0] ? error->source_file : "unknown",
                error->source_line);
        
        if (error->details[0]) {
            vkprintf(1, "  Details: %s\n", error->details);
        }
    }
    
    // Установка последней ошибки
    error_set_last(ctx, error);
    
    // Вызов глобального обработчика
    if (ctx->global_handler) {
        ctx->global_handler(error, ctx->user_data);
    }
    
    // Вызов обработчика категории
    if (error->category < 12 && ctx->category_handlers[error->category]) {
        ctx->category_handlers[error->category](error, ctx->user_data);
    }
    
    // Circuit breaker для критических ошибок
    if (error->level >= ERROR_LEVEL_CRITICAL &&
        ctx->recovery_config.enable_circuit_breaker) {
        error_circuit_breaker_record_failure(ctx);
    }
    
    return 0;
}

// Проверка восстанавливаемости
int error_is_recoverable(const error_info_t *error) {
    if (!error) return 0;
    
    // Невосстанавливаемые ошибки
    if (error->level == ERROR_LEVEL_FATAL) return 0;
    
    switch (error->category) {
        case ERROR_CATEGORY_MEMORY:
        case ERROR_CATEGORY_SECURITY:
            return 0;  // Обычно не восстанавливаются
        
        case ERROR_CATEGORY_NETWORK:
        case ERROR_CATEGORY_TIMEOUT:
        case ERROR_CATEGORY_RESOURCE:
            return 1;  // Обычно восстанавливаются
        
        default:
            return error->level < ERROR_LEVEL_CRITICAL;
    }
}

// Попытка восстановления
int error_attempt_recovery(error_handler_context_t *ctx, error_info_t *error) {
    if (!ctx || !error) return -1;
    
    if (!error_is_recoverable(error)) {
        ctx->stats.unrecovered_errors++;
        return -1;
    }
    
    recovery_strategy_t strategy = ctx->recovery_config.strategy;
    
    switch (strategy) {
        case RECOVERY_RETRY:
            // Логика retry реализуется в error_execute_with_retry
            vkprintf(2, "Recovery: retry strategy selected\n");
            return 0;
        
        case RECOVERY_FALLBACK:
            vkprintf(2, "Recovery: fallback strategy selected\n");
            return 0;
        
        case RECOVERY_RESTART:
            vkprintf(2, "Recovery: restart strategy selected\n");
            return 0;
        
        case RECOVERY_SHUTDOWN:
            vkprintf(1, "Recovery: shutdown strategy selected\n");
            return -1;
        
        default:
            return -1;
    }
}

// Выполнение с retry
int error_execute_with_retry(error_handler_context_t *ctx,
                            int (*operation)(void*),
                            void *user_data,
                            int max_retries) {
    if (!ctx || !operation) return -1;
    
    int retries = 0;
    time_t delay = ctx->recovery_config.retry_delay_ms;
    
    while (retries < max_retries) {
        int result = operation(user_data);
        
        if (result == 0) {
            // Успех
            error_circuit_breaker_record_success(ctx);
            return 0;
        }
        
        retries++;
        
        if (retries < max_retries) {
            vkprintf(2, "Retry %d/%d after %ld ms\n", retries, max_retries, (long)delay);
            
            // Exponential backoff
            #ifdef _WIN32
            Sleep((DWORD)delay);
            #else
            usleep(delay * 1000);
            #endif
            
            delay = (time_t)(delay * ctx->recovery_config.backoff_multiplier);
            if (delay > ctx->recovery_config.max_retry_delay_ms) {
                delay = ctx->recovery_config.max_retry_delay_ms;
            }
        }
    }
    
    // Все попытки исчерпаны
    ctx->stats.unrecovered_errors++;
    return -1;
}

// Circuit breaker: запись неудачи
int error_circuit_breaker_record_failure(error_handler_context_t *ctx) {
    if (!ctx || !ctx->recovery_config.enable_circuit_breaker) return -1;
    
#ifdef _WIN32
    if (ctx->mutex) EnterCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_lock((pthread_mutex_t*)ctx->mutex);
#endif
    
    time_t now = time(NULL);
    
    // Сброс окна если истекло
    if (now - ctx->recent_failure_window_start > 60) {
        ctx->recent_failure_count = 0;
        ctx->recent_failure_window_start = now;
    }
    
    ctx->recent_failure_count++;
    
    // Проверка порога
    if (ctx->recent_failure_count >= (uint32_t)ctx->recovery_config.circuit_breaker_threshold) {
        ctx->circuit_breaker_open = 1;
        ctx->circuit_breaker_opened_at = now;
        ctx->stats.circuit_breaker_trips++;
        
        vkprintf(1, "Circuit breaker OPEN after %d failures\n", ctx->recent_failure_count);
    }
    
#ifdef _WIN32
    if (ctx->mutex) LeaveCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_unlock((pthread_mutex_t*)ctx->mutex);
#endif
    
    return 0;
}

// Circuit breaker: запись успеха
int error_circuit_breaker_record_success(error_handler_context_t *ctx) {
    if (!ctx) return -1;
    
#ifdef _WIN32
    if (ctx->mutex) EnterCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_lock((pthread_mutex_t*)ctx->mutex);
#endif
    
    // Сброс при успехе
    ctx->recent_failure_count = 0;
    
    // Закрытие circuit breaker если открыт
    if (ctx->circuit_breaker_open) {
        ctx->circuit_breaker_open = 0;
        vkprintf(2, "Circuit breaker CLOSED after successful operation\n");
    }
    
#ifdef _WIN32
    if (ctx->mutex) LeaveCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_unlock((pthread_mutex_t*)ctx->mutex);
#endif
    
    return 0;
}

// Circuit breaker: проверка возможности выполнения
int error_circuit_breaker_can_execute(error_handler_context_t *ctx) {
    if (!ctx || !ctx->recovery_config.enable_circuit_breaker) return 1;
    
    if (!ctx->circuit_breaker_open) {
        return 1;
    }
    
    // Проверка timeout
    time_t now = time(NULL);
    if (now - ctx->circuit_breaker_opened_at >= 
        ctx->recovery_config.circuit_breaker_timeout_ms / 1000) {
        
        // Полуоткрытое состояние - разрешить одну попытку
        vkprintf(2, "Circuit breaker HALF-OPEN, allowing test request\n");
        return 1;
    }
    
    return 0;  // Circuit breaker открыт
}

// Получение статистики
void error_get_stats(error_handler_context_t *ctx, error_stats_t *stats) {
    if (!ctx || !stats) return;
    
#ifdef _WIN32
    if (ctx->mutex) EnterCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_lock((pthread_mutex_t*)ctx->mutex);
#endif
    
    memcpy(stats, &ctx->stats, sizeof(error_stats_t));
    
#ifdef _WIN32
    if (ctx->mutex) LeaveCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_unlock((pthread_mutex_t*)ctx->mutex);
#endif
}

// Сброс статистики
void error_reset_stats(error_handler_context_t *ctx) {
    if (!ctx) return;
    
#ifdef _WIN32
    if (ctx->mutex) EnterCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_lock((pthread_mutex_t*)ctx->mutex);
#endif
    
    memset(&ctx->stats, 0, sizeof(error_stats_t));
    
#ifdef _WIN32
    if (ctx->mutex) LeaveCriticalSection((CRITICAL_SECTION*)ctx->mutex);
#else
    if (ctx->mutex) pthread_mutex_unlock((pthread_mutex_t*)ctx->mutex);
#endif
}

// Вывод статистики
void error_print_stats(error_handler_context_t *ctx) {
    if (!ctx) return;
    
    error_stats_t stats;
    error_get_stats(ctx, &stats);
    
    vkprintf(1, "Error Handler Statistics:\n");
    vkprintf(1, "  Total Errors: %llu\n", (unsigned long long)stats.total_errors);
    vkprintf(1, "  Recovered: %llu\n", 
             (unsigned long long)(stats.total_errors - stats.unrecovered_errors));
    vkprintf(1, "  Unrecovered: %llu\n", (unsigned long long)stats.unrecovered_errors);
    vkprintf(1, "  Circuit Breaker Trips: %llu\n", 
             (unsigned long long)stats.circuit_breaker_trips);
    vkprintf(1, "  Errors (last minute): %u\n", stats.errors_last_minute);
    
    // По уровням
    vkprintf(1, "  By Level:\n");
    for (int i = 0; i < 5; i++) {
        if (stats.errors_by_level[i] > 0) {
            vkprintf(1, "    %s: %llu\n", 
                    error_level_to_string((error_level_t)i),
                    (unsigned long long)stats.errors_by_level[i]);
        }
    }
}

// Утилиты
const char* error_code_to_string(int error_code) {
    return error_get_default_message(error_code);
}

const char* error_level_to_string(error_level_t level) {
    switch (level) {
        case ERROR_LEVEL_INFO: return "INFO";
        case ERROR_LEVEL_WARNING: return "WARNING";
        case ERROR_LEVEL_ERROR: return "ERROR";
        case ERROR_LEVEL_CRITICAL: return "CRITICAL";
        case ERROR_LEVEL_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

const char* error_category_to_string(error_category_t category) {
    switch (category) {
        case ERROR_CATEGORY_SYSTEM: return "System";
        case ERROR_CATEGORY_NETWORK: return "Network";
        case ERROR_CATEGORY_MEMORY: return "Memory";
        case ERROR_CATEGORY_IO: return "I/O";
        case ERROR_CATEGORY_CONFIG: return "Configuration";
        case ERROR_CATEGORY_SECURITY: return "Security";
        case ERROR_CATEGORY_PROTOCOL: return "Protocol";
        case ERROR_CATEGORY_RESOURCE: return "Resource";
        case ERROR_CATEGORY_TIMEOUT: return "Timeout";
        case ERROR_CATEGORY_AUTH: return "Authentication";
        case ERROR_CATEGORY_RATE_LIMIT: return "Rate Limit";
        default: return "Unknown";
    }
}

int error_code_from_string(const char *str) {
    if (!str) return MTERR_UNKNOWN;
    
    for (int i = 0; error_messages[i].message != NULL; i++) {
        if (strcmp(error_messages[i].message, str) == 0) {
            return error_messages[i].code;
        }
    }
    
    return MTERR_UNKNOWN;
}

// Установка глобального обработчика
void error_handler_set_global_context(error_handler_context_t *ctx) {
    if (ctx) {
        memcpy(&global_error_ctx, ctx, sizeof(error_handler_context_t));
    }
}

error_handler_context_t* error_handler_get_global_context(void) {
    return &global_error_ctx;
}
