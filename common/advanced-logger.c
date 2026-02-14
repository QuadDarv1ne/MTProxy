#include "advanced-logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
#else
    #include <unistd.h>
    #include <pthread.h>
#endif

// Глобальный логгер
static advanced_logger_t *global_logger = NULL;

// Платформенно-зависимые функции
#ifdef _WIN32
    static void* platform_create_mutex(void) {
        CRITICAL_SECTION *cs = malloc(sizeof(CRITICAL_SECTION));
        if (cs) {
            InitializeCriticalSection(cs);
        }
        return cs;
    }
    
    static void platform_destroy_mutex(void *mutex) {
        if (mutex) {
            DeleteCriticalSection((CRITICAL_SECTION*)mutex);
            free(mutex);
        }
    }
    
    static void platform_lock_mutex(void *mutex) {
        if (mutex) {
            EnterCriticalSection((CRITICAL_SECTION*)mutex);
        }
    }
    
    static void platform_unlock_mutex(void *mutex) {
        if (mutex) {
            LeaveCriticalSection((CRITICAL_SECTION*)mutex);
        }
    }
#else
    static void* platform_create_mutex(void) {
        pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
        if (mutex) {
            pthread_mutex_init(mutex, NULL);
        }
        return mutex;
    }
    
    static void platform_destroy_mutex(void *mutex) {
        if (mutex) {
            pthread_mutex_destroy((pthread_mutex_t*)mutex);
            free(mutex);
        }
    }
    
    static void platform_lock_mutex(void *mutex) {
        if (mutex) {
            pthread_mutex_lock((pthread_mutex_t*)mutex);
        }
    }
    
    static void platform_unlock_mutex(void *mutex) {
        if (mutex) {
            pthread_mutex_unlock((pthread_mutex_t*)mutex);
        }
    }
#endif

// Получение текущего времени в миллисекундах
static long long get_current_time_ms(void) {
    return time(NULL) * 1000LL;
}

// Инициализация логгера
advanced_logger_t* logger_init(const logger_config_t *config) {
    advanced_logger_t *logger = calloc(1, sizeof(advanced_logger_t));
    if (!logger) {
        return NULL;
    }
    
    // Конфигурация по умолчанию
    logger_config_t default_config = {
        .min_level = LOG_LEVEL_INFO,
        .format = LOG_FORMAT_DETAILED,
        .output_type = LOG_OUTPUT_CONSOLE,
        .max_file_size_bytes = 10 * 1024 * 1024, // 10MB
        .max_backup_files = 5,
        .enable_rotation = 1,
        .buffer_size = 8192,
        .enable_buffering = 1,
        .flush_interval_seconds = 5,
        .enable_async_logging = 0,
        .queue_size = 1000,
        .enable_colors = 1,
        .enable_timestamps = 1,
        .enable_thread_ids = 0,
        .enable_caller_info = 1
    };
    
    // Применение пользовательской конфигурации
    if (config) {
        logger->config = *config;
    } else {
        logger->config = default_config;
    }
    
    // Инициализация мьютекса
    logger->mutex = platform_create_mutex();
    if (!logger->mutex) {
        free(logger);
        return NULL;
    }
    
    // Инициализация буфера
    if (logger->config.enable_buffering) {
        logger->buffer = malloc(logger->config.buffer_size);
        if (!logger->buffer) {
            platform_destroy_mutex(logger->mutex);
            free(logger);
            return NULL;
        }
        logger->buffer_capacity = logger->config.buffer_size;
    }
    
    // Инициализация асинхронной очереди
    if (logger->config.enable_async_logging) {
        logger->async_queue.entries = calloc(logger->config.queue_size, sizeof(char*));
        logger->async_queue.levels = calloc(logger->config.queue_size, sizeof(int));
        logger->async_queue.timestamps = calloc(logger->config.queue_size, sizeof(time_t));
        logger->async_queue.capacity = logger->config.queue_size;
        if (!logger->async_queue.entries || !logger->async_queue.levels || 
            !logger->async_queue.timestamps) {
            logger_cleanup(logger);
            return NULL;
        }
    }
    
    // Открытие файлов если нужно
    if (logger->config.output_type == LOG_OUTPUT_FILE && 
        strlen(logger->config.log_file_path) > 0) {
        logger->log_file = fopen(logger->config.log_file_path, "a");
        if (!logger->log_file) {
            logger_cleanup(logger);
            return NULL;
        }
    }
    
    logger->is_initialized = 1;
    logger->start_time = get_current_time_ms();
    logger->last_flush_time = time(NULL);
    
    return logger;
}

// Конфигурация логгера
int logger_configure(advanced_logger_t *logger, const logger_config_t *config) {
    if (!logger || !config) {
        return -1;
    }
    
    platform_lock_mutex(logger->mutex);
    
    logger->config = *config;
    
    // Переоткрытие файлов при необходимости
    if (logger->config.output_type == LOG_OUTPUT_FILE && 
        strlen(logger->config.log_file_path) > 0) {
        if (logger->log_file) {
            fclose(logger->log_file);
        }
        logger->log_file = fopen(logger->config.log_file_path, "a");
    }
    
    platform_unlock_mutex(logger->mutex);
    return 0;
}

// Основная функция логирования
void logger_log(advanced_logger_t *logger, log_level_t level, 
                const char *component, const char *format, ...) {
    if (!logger || !logger->is_initialized || level > logger->config.min_level) {
        return;
    }
    
    platform_lock_mutex(logger->mutex);
    
    // Статистика
    logger->stats.total_entries++;
    logger->stats.entries_by_level[level]++;
    
    // Форматирование сообщения
    char message[4096];
    va_list args;
    va_start(args, format);
    int message_len = vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    if (message_len < 0) {
        platform_unlock_mutex(logger->mutex);
        return;
    }
    
    // Добавление временной метки
    char timestamp[64] = "";
    if (logger->config.enable_timestamps) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    }
    
    // Форматирование в зависимости от типа вывода
    char output_buffer[8192];
    int output_len = 0;
    
    switch (logger->config.format) {
        case LOG_FORMAT_SIMPLE:
            output_len = snprintf(output_buffer, sizeof(output_buffer),
                                "[%s] %s: %s\n", 
                                logger_level_to_string(level), component, message);
            break;
            
        case LOG_FORMAT_DETAILED:
            output_len = snprintf(output_buffer, sizeof(output_buffer),
                                "[%s] [%s] [%s] %s\n",
                                timestamp, logger_level_to_string(level), 
                                component, message);
            break;
            
        case LOG_FORMAT_JSON:
            output_len = snprintf(output_buffer, sizeof(output_buffer),
                                "{\"timestamp\":\"%s\",\"level\":\"%s\",\"component\":\"%s\",\"message\":\"%s\"}\n",
                                timestamp, logger_level_to_string(level), component, message);
            break;
            
        case LOG_FORMAT_SYSLOG:
            output_len = snprintf(output_buffer, sizeof(output_buffer),
                                "<%d>%s %s: %s\n",
                                level + 8, timestamp, component, message);
            break;
    }
    
    // Вывод в зависимости от типа
    FILE *output_stream = stdout;
    if (level <= LOG_LEVEL_ERROR && logger->error_file) {
        output_stream = logger->error_file;
    } else if (logger->log_file) {
        output_stream = logger->log_file;
    }
    
    if (logger->config.enable_buffering && logger->buffer) {
        // Буферизованный вывод
        if (logger->buffer_pos + output_len < logger->buffer_capacity) {
            memcpy(logger->buffer + logger->buffer_pos, output_buffer, output_len);
            logger->buffer_pos += output_len;
        } else {
            // Буфер полон - сброс
            logger_flush(logger);
            if (output_len < logger->buffer_capacity) {
                memcpy(logger->buffer, output_buffer, output_len);
                logger->buffer_pos = output_len;
            } else {
                // Сообщение слишком большое для буфера
                fwrite(output_buffer, 1, output_len, output_stream);
            }
        }
    } else {
        // Прямой вывод
        fwrite(output_buffer, 1, output_len, output_stream);
        fflush(output_stream);
    }
    
    // Проверка необходимости ротации
    if (logger->config.enable_rotation && logger->log_file) {
        long current_pos = ftell(logger->log_file);
        if (current_pos > (long)logger->config.max_file_size_bytes) {
            logger_rotate(logger);
        }
    }
    
    platform_unlock_mutex(logger->mutex);
}

// Логирование с информацией о вызове
void logger_log_with_caller(advanced_logger_t *logger, log_level_t level,
                           const char *component, const char *file, 
                           int line, const char *function, 
                           const char *format, ...) {
    if (!logger || !logger->is_initialized || level > logger->config.min_level) {
        return;
    }
    
    char full_message[4096];
    va_list args;
    va_start(args, format);
    int base_len = vsnprintf(full_message, sizeof(full_message), format, args);
    va_end(args);
    
    if (logger->config.enable_caller_info) {
        char caller_info[512];
        snprintf(caller_info, sizeof(caller_info), " [%s:%d in %s]", 
                file, line, function);
        strncat(full_message, caller_info, sizeof(full_message) - base_len - 1);
    }
    
    logger_log(logger, level, component, "%s", full_message);
}

// Сброс буфера
int logger_flush(advanced_logger_t *logger) {
    if (!logger || !logger->is_initialized) {
        return -1;
    }
    
    platform_lock_mutex(logger->mutex);
    
    if (logger->buffer && logger->buffer_pos > 0) {
        FILE *output_stream = logger->log_file ? logger->log_file : stdout;
        fwrite(logger->buffer, 1, logger->buffer_pos, output_stream);
        fflush(output_stream);
        logger->buffer_pos = 0;
    }
    
    logger->last_flush_time = time(NULL);
    platform_unlock_mutex(logger->mutex);
    return 0;
}

// Ротация логов
int logger_rotate(advanced_logger_t *logger) {
    if (!logger || !logger->is_initialized || !logger->config.enable_rotation) {
        return -1;
    }
    
    platform_lock_mutex(logger->mutex);
    
    if (logger->log_file) {
        fclose(logger->log_file);
        
        // Переименование текущего файла
        char backup_name[320];
        for (int i = logger->config.max_backup_files - 1; i > 0; i--) {
            snprintf(backup_name, sizeof(backup_name), "%s.%d", 
                    logger->config.log_file_path, i);
            char prev_name[320];
            snprintf(prev_name, sizeof(prev_name), "%s.%d", 
                    logger->config.log_file_path, i - 1);
            rename(prev_name, backup_name);
        }
        
        // Переименование основного файла
        char first_backup[320];
        snprintf(first_backup, sizeof(first_backup), "%s.1", 
                logger->config.log_file_path);
        rename(logger->config.log_file_path, first_backup);
        
        // Открытие нового файла
        logger->log_file = fopen(logger->config.log_file_path, "w");
    }
    
    platform_unlock_mutex(logger->mutex);
    return 0;
}

// Очистка логгера
void logger_cleanup(advanced_logger_t *logger) {
    if (!logger) return;
    
    // Сброс буфера
    logger_flush(logger);
    
    // Закрытие файлов
    if (logger->log_file) {
        fclose(logger->log_file);
    }
    if (logger->error_file) {
        fclose(logger->error_file);
    }
    
    // Очистка буфера
    if (logger->buffer) {
        free(logger->buffer);
    }
    
    // Очистка асинхронной очереди
    if (logger->async_queue.entries) {
        for (int i = 0; i < logger->async_queue.capacity; i++) {
            if (logger->async_queue.entries[i]) {
                free(logger->async_queue.entries[i]);
            }
        }
        free(logger->async_queue.entries);
        free(logger->async_queue.levels);
        free(logger->async_queue.timestamps);
    }
    
    // Очистка мьютекса
    platform_destroy_mutex(logger->mutex);
    
    free(logger);
}

// Утилиты
const char* logger_level_to_string(log_level_t level) {
    static const char* level_strings[] = {
        "OFF", "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"
    };
    if (level >= 0 && level < 7) {
        return level_strings[level];
    }
    return "UNKNOWN";
}

log_level_t logger_string_to_level(const char *level_str) {
    if (!level_str) return LOG_LEVEL_INFO;
    
    if (strcasecmp(level_str, "FATAL") == 0) return LOG_LEVEL_FATAL;
    if (strcasecmp(level_str, "ERROR") == 0) return LOG_LEVEL_ERROR;
    if (strcasecmp(level_str, "WARN") == 0) return LOG_LEVEL_WARN;
    if (strcasecmp(level_str, "INFO") == 0) return LOG_LEVEL_INFO;
    if (strcasecmp(level_str, "DEBUG") == 0) return LOG_LEVEL_DEBUG;
    if (strcasecmp(level_str, "TRACE") == 0) return LOG_LEVEL_TRACE;
    if (strcasecmp(level_str, "OFF") == 0) return LOG_LEVEL_OFF;
    
    return LOG_LEVEL_INFO;
}

void logger_set_global_logger(advanced_logger_t *logger) {
    global_logger = logger;
}

advanced_logger_t* logger_get_global_logger(void) {
    return global_logger;
}

// Получение статистики
const logger_stats_t* logger_get_stats(advanced_logger_t *logger) {
    if (!logger) return NULL;
    return &logger->stats;
}

// Проверка уровня
int logger_is_level_enabled(advanced_logger_t *logger, log_level_t level) {
    if (!logger) return 0;
    return level <= logger->config.min_level;
}

// Сброс статистики
void logger_reset_stats(advanced_logger_t *logger) {
    if (!logger) return;
    memset(&logger->stats, 0, sizeof(logger->stats));
}