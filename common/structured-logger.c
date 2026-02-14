/*
    This file is part of Mtproto-proxy Library.

    Mtproto-proxy Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Mtproto-proxy Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Mtproto-proxy Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2024-2026 MTProto Proxy Enhanced Project
              2024-2026 Dupley Maxim Igorevich (Maestro7IT)
*/

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "common/structured-logger.h"
#include "common/kprintf.h"
#include "common/common-stats.h"

// Logging statistics
struct logger_stats {
    long long total_log_entries;
    long long log_level_distribution[LOG_LEVEL_MAX];
    long long log_format_distribution[LOG_FORMAT_MAX];
    long long buffer_overflows;
    long long failed_writes;
    long long async_log_operations;
    long long sync_log_operations;
};

static struct logger_stats logger_stats = {0};

// Log entry structure
struct log_entry {
    time_t timestamp;
    struct timeval precise_time;
    enum log_level level;
    enum log_format format;
    char component[64];
    char subsystem[64];
    char message[1024];
    char context_data[512]; // JSON-like context
    int thread_id;
    unsigned int connection_id;
    unsigned int client_ip;
    int is_error;
    int is_security_event;
};

// Logger configuration
struct logger_config {
    enum log_level min_level;
    enum log_format output_format;
    int enable_async_logging;
    int enable_context_logging;
    int max_message_size;
    int buffer_size;
    int flush_interval_seconds;
    char log_file_path[512];
    char error_log_file_path[512];
    int enable_file_logging;
    int enable_stdout_logging;
    int enable_stderr_logging;
    int enable_json_format;
    int enable_log_rotation;
    long long max_log_file_size;
    int max_log_files;
};

static struct logger_config global_logger_config = {
    .min_level = LOG_LEVEL_INFO,
    .output_format = LOG_FORMAT_STANDARD,
    .enable_async_logging = 1,
    .enable_context_logging = 1,
    .max_message_size = 1024,
    .buffer_size = 10000,
    .flush_interval_seconds = 5,
    .enable_file_logging = 1,
    .enable_stdout_logging = 1,
    .enable_stderr_logging = 1,
    .enable_json_format = 1,
    .enable_log_rotation = 1,
    .max_log_file_size = 100 * 1024 * 1024, // 100MB
    .max_log_files = 10
};

// Async logging buffer
struct log_buffer {
    struct log_entry *entries;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t buffer_mutex;
    pthread_cond_t buffer_not_full;
    pthread_cond_t buffer_not_empty;
};

static struct log_buffer async_log_buffer = {0};
static pthread_t async_logger_thread = 0;
static int async_logger_running = 0;
static pthread_mutex_t logger_mutex = PTHREAD_MUTEX_INITIALIZER;

// Log file handles
static FILE *log_file_handle = NULL;
static FILE *error_log_file_handle = NULL;
static FILE *json_log_file_handle = NULL;

// Context information
struct log_context {
    char session_id[64];
    char request_id[64];
    char user_id[64];
    char client_info[128];
    int trace_level;
};

static __thread struct log_context current_context = {0};

// Инициализация structured logger
int structured_logger_init(const char *log_file_path) {
    pthread_mutex_lock(&logger_mutex);
    
    if (async_logger_running) {
        pthread_mutex_unlock(&logger_mutex);
        return 0; // Уже инициализирован
    }
    
    // Инициализация конфигурации
    if (log_file_path) {
        strncpy(global_logger_config.log_file_path, log_file_path, 
                sizeof(global_logger_config.log_file_path) - 1);
    } else {
        strcpy(global_logger_config.log_file_path, "/var/log/mtproxy.log");
    }
    
    // Создание async buffer
    async_log_buffer.capacity = global_logger_config.buffer_size;
    async_log_buffer.entries = calloc(async_log_buffer.capacity, sizeof(struct log_entry));
    if (!async_log_buffer.entries) {
        pthread_mutex_unlock(&logger_mutex);
        return -1;
    }
    
    async_log_buffer.head = 0;
    async_log_buffer.tail = 0;
    async_log_buffer.count = 0;
    
    pthread_mutex_init(&async_log_buffer.buffer_mutex, NULL);
    pthread_cond_init(&async_log_buffer.buffer_not_full, NULL);
    pthread_cond_init(&async_log_buffer.buffer_not_empty, NULL);
    
    // Открытие log файлов
    if (global_logger_config.enable_file_logging) {
        log_file_handle = fopen(global_logger_config.log_file_path, "a");
        if (!log_file_handle) {
            pthread_mutex_unlock(&logger_mutex);
            return -1;
        }
        
        // Error log file
        snprintf(global_logger_config.error_log_file_path, 
                 sizeof(global_logger_config.error_log_file_path),
                 "%s.error", global_logger_config.log_file_path);
        error_log_file_handle = fopen(global_logger_config.error_log_file_path, "a");
    }
    
    // Запуск async logger thread
    if (global_logger_config.enable_async_logging) {
        async_logger_running = 1;
        if (pthread_create(&async_logger_thread, NULL, async_logger_worker, NULL) != 0) {
            async_logger_running = 0;
            pthread_mutex_unlock(&logger_mutex);
            return -1;
        }
    }
    
    pthread_mutex_unlock(&logger_mutex);
    
    // Запись startup message
    structured_log(LOG_LEVEL_INFO, "system", "startup", 
                   "Structured logger initialized", 
                   "version=1.0;config=%s", global_logger_config.log_file_path);
    
    vkprintf(1, "Structured logger initialized with async=%s, format=%s\n",
             global_logger_config.enable_async_logging ? "enabled" : "disabled",
             global_logger_config.enable_json_format ? "JSON" : "standard");
    
    return 0;
}

// Async logger worker thread
static void *async_logger_worker(void *arg) {
    struct log_entry entry;
    
    while (async_logger_running) {
        // Получение entry из buffer
        pthread_mutex_lock(&async_log_buffer.buffer_mutex);
        
        while (async_log_buffer.count == 0 && async_logger_running) {
            struct timespec timeout;
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += global_logger_config.flush_interval_seconds;
            pthread_cond_timedwait(&async_log_buffer.buffer_not_empty, 
                                 &async_log_buffer.buffer_mutex, &timeout);
        }
        
        if (!async_logger_running) {
            pthread_mutex_unlock(&async_log_buffer.buffer_mutex);
            break;
        }
        
        if (async_log_buffer.count > 0) {
            entry = async_log_buffer.entries[async_log_buffer.tail];
            async_log_buffer.tail = (async_log_buffer.tail + 1) % async_log_buffer.capacity;
            async_log_buffer.count--;
            pthread_cond_signal(&async_log_buffer.buffer_not_full);
        }
        
        pthread_mutex_unlock(&async_log_buffer.buffer_mutex);
        
        // Запись log entry
        if (async_log_buffer.count >= 0) {
            write_log_entry(&entry);
            logger_stats.async_log_operations++;
        }
    }
    
    return NULL;
}

// Запись log entry
static int write_log_entry(const struct log_entry *entry) {
    char formatted_message[2048];
    int result = 0;
    
    // Форматирование сообщения
    if (global_logger_config.enable_json_format) {
        format_json_log(entry, formatted_message, sizeof(formatted_message));
    } else {
        format_standard_log(entry, formatted_message, sizeof(formatted_message));
    }
    
    // Запись в файлы
    if (global_logger_config.enable_file_logging && log_file_handle) {
        fprintf(log_file_handle, "%s\n", formatted_message);
        if (entry->is_error && error_log_file_handle) {
            fprintf(error_log_file_handle, "%s\n", formatted_message);
        }
        fflush(log_file_handle);
        if (entry->is_error && error_log_file_handle) {
            fflush(error_log_file_handle);
        }
    }
    
    // Запись в stdout/stderr
    if (global_logger_config.enable_stdout_logging && entry->level <= LOG_LEVEL_INFO) {
        printf("%s\n", formatted_message);
        fflush(stdout);
    }
    
    if (global_logger_config.enable_stderr_logging && entry->level >= LOG_LEVEL_WARNING) {
        fprintf(stderr, "%s\n", formatted_message);
        fflush(stderr);
    }
    
    return result;
}

// Форматирование JSON log
static int format_json_log(const struct log_entry *entry, char *buffer, size_t buffer_size) {
    struct tm tm_time;
    char time_str[32];
    
    localtime_r(&entry->timestamp, &tm_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_time);
    
    return snprintf(buffer, buffer_size,
        "{\"timestamp\":\"%s.%06ld\","
        "\"level\":\"%s\","
        "\"component\":\"%s\","
        "\"subsystem\":\"%s\","
        "\"message\":\"%s\","
        "\"context\":%s,"
        "\"thread_id\":%d,"
        "\"connection_id\":%u,"
        "\"client_ip\":\"%u.%u.%u.%u\","
        "\"is_error\":%s,"
        "\"is_security\":%s"
        "}",
        time_str, entry->precise_time.tv_usec,
        log_level_to_string(entry->level),
        entry->component,
        entry->subsystem,
        entry->message,
        entry->context_data[0] ? entry->context_data : "\"{}\"",
        entry->thread_id,
        entry->connection_id,
        (entry->client_ip >> 24) & 0xFF,
        (entry->client_ip >> 16) & 0xFF,
        (entry->client_ip >> 8) & 0xFF,
        entry->client_ip & 0xFF,
        entry->is_error ? "true" : "false",
        entry->is_security_event ? "true" : "false"
    );
}

// Форматирование стандартного log
static int format_standard_log(const struct log_entry *entry, char *buffer, size_t buffer_size) {
    struct tm tm_time;
    char time_str[32];
    
    localtime_r(&entry->timestamp, &tm_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_time);
    
    return snprintf(buffer, buffer_size,
        "[%s.%06ld] [%s] [%s:%s] %s %s",
        time_str, entry->precise_time.tv_usec,
        log_level_to_string(entry->level),
        entry->component,
        entry->subsystem,
        entry->message,
        entry->context_data
    );
}

// Основная logging функция
int structured_log(enum log_level level, 
                  const char *component,
                  const char *subsystem,
                  const char *message,
                  const char *context_format, ...) {
    
    if (level < global_logger_config.min_level) {
        return 0;
    }
    
    struct log_entry entry;
    struct timeval tv;
    
    // Получение времени
    gettimeofday(&tv, NULL);
    entry.timestamp = tv.tv_sec;
    entry.precise_time = tv;
    entry.level = level;
    entry.format = global_logger_config.output_format;
    entry.thread_id = (int)pthread_self();
    entry.connection_id = 0; // Будет установлен вызывающим
    entry.client_ip = 0;     // Будет установлен вызывающим
    entry.is_error = (level >= LOG_LEVEL_ERROR);
    entry.is_security_event = (strcmp(component, "security") == 0);
    
    // Копирование базовых полей
    strncpy(entry.component, component ? component : "unknown", sizeof(entry.component) - 1);
    strncpy(entry.subsystem, subsystem ? subsystem : "main", sizeof(entry.subsystem) - 1);
    strncpy(entry.message, message ? message : "No message", sizeof(entry.message) - 1);
    
    // Обработка контекста
    if (context_format && global_logger_config.enable_context_logging) {
        va_list args;
        va_start(args, context_format);
        vsnprintf(entry.context_data, sizeof(entry.context_data), context_format, args);
        va_end(args);
    } else {
        entry.context_data[0] = '\0';
    }
    
    // Добавление контекстной информации
    if (current_context.session_id[0] || current_context.request_id[0]) {
        char context_append[256];
        snprintf(context_append, sizeof(context_append), 
                "%s%s%s%s", 
                current_context.session_id[0] ? "session_id=" : "",
                current_context.session_id[0] ? current_context.session_id : "",
                current_context.request_id[0] ? " request_id=" : "",
                current_context.request_id[0] ? current_context.request_id : "");
        
        if (entry.context_data[0]) {
            strncat(entry.context_data, ";", sizeof(entry.context_data) - strlen(entry.context_data) - 1);
            strncat(entry.context_data, context_append, sizeof(entry.context_data) - strlen(entry.context_data) - 1);
        } else {
            strncpy(entry.context_data, context_append, sizeof(entry.context_data) - 1);
        }
    }
    
    // Обновление статистики
    logger_stats.total_log_entries++;
    logger_stats.log_level_distribution[level]++;
    logger_stats.log_format_distribution[entry.format]++;
    
    // Запись через async или sync
    if (global_logger_config.enable_async_logging) {
        return async_log_enqueue(&entry);
    } else {
        logger_stats.sync_log_operations++;
        return write_log_entry(&entry);
    }
}

// Async log enqueue
static int async_log_enqueue(const struct log_entry *entry) {
    pthread_mutex_lock(&async_log_buffer.buffer_mutex);
    
    // Ожидание места в buffer
    while (async_log_buffer.count >= async_log_buffer.capacity) {
        logger_stats.buffer_overflows++;
        pthread_cond_wait(&async_log_buffer.buffer_not_full, &async_log_buffer.buffer_mutex);
    }
    
    // Добавление в buffer
    async_log_buffer.entries[async_log_buffer.head] = *entry;
    async_log_buffer.head = (async_log_buffer.head + 1) % async_log_buffer.capacity;
    async_log_buffer.count++;
    
    pthread_cond_signal(&async_log_buffer.buffer_not_empty);
    pthread_mutex_unlock(&async_log_buffer.buffer_mutex);
    
    return 0;
}

// Установка контекста для текущего thread
int structured_logger_set_context(const char *session_id, 
                                 const char *request_id,
                                 const char *user_id,
                                 const char *client_info) {
    if (session_id) {
        strncpy(current_context.session_id, session_id, sizeof(current_context.session_id) - 1);
    }
    if (request_id) {
        strncpy(current_context.request_id, request_id, sizeof(current_context.request_id) - 1);
    }
    if (user_id) {
        strncpy(current_context.user_id, user_id, sizeof(current_context.user_id) - 1);
    }
    if (client_info) {
        strncpy(current_context.client_info, client_info, sizeof(current_context.client_info) - 1);
    }
    return 0;
}

// Сброс контекста
int structured_logger_clear_context(void) {
    memset(&current_context, 0, sizeof(current_context));
    return 0;
}

// Helper функция для конвертации уровня в строку
static const char *log_level_to_string(enum log_level level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_WARNING: return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

// Установка log level
int structured_logger_set_level(enum log_level level) {
    pthread_mutex_lock(&logger_mutex);
    global_logger_config.min_level = level;
    pthread_mutex_unlock(&logger_mutex);
    return 0;
}

// Получение статистики
void structured_logger_get_stats(struct logger_stats *stats) {
    if (stats) {
        memcpy(stats, &logger_stats, sizeof(struct logger_stats));
    }
}

// Вывод статистики
void structured_logger_print_stats(void) {
    vkprintf(1, "Structured Logger Statistics:\n");
    vkprintf(1, "  Total Log Entries: %lld\n", logger_stats.total_log_entries);
    vkprintf(1, "  Log Level Distribution:\n");
    vkprintf(1, "    DEBUG: %lld\n", logger_stats.log_level_distribution[LOG_LEVEL_DEBUG]);
    vkprintf(1, "    INFO: %lld\n", logger_stats.log_level_distribution[LOG_LEVEL_INFO]);
    vkprintf(1, "    WARNING: %lld\n", logger_stats.log_level_distribution[LOG_LEVEL_WARNING]);
    vkprintf(1, "    ERROR: %lld\n", logger_stats.log_level_distribution[LOG_LEVEL_ERROR]);
    vkprintf(1, "    CRITICAL: %lld\n", logger_stats.log_level_distribution[LOG_LEVEL_CRITICAL]);
    vkprintf(1, "  Buffer Overflows: %lld\n", logger_stats.buffer_overflows);
    vkprintf(1, "  Failed Writes: %lld\n", logger_stats.failed_writes);
    vkprintf(1, "  Async Operations: %lld\n", logger_stats.async_log_operations);
    vkprintf(1, "  Sync Operations: %lld\n", logger_stats.sync_log_operations);
    
    pthread_mutex_lock(&logger_mutex);
    vkprintf(1, "  Async Logger: %s\n", async_logger_running ? "Running" : "Stopped");
    vkprintf(1, "  File Logging: %s\n", global_logger_config.enable_file_logging ? "Enabled" : "Disabled");
    vkprintf(1, "  JSON Format: %s\n", global_logger_config.enable_json_format ? "Enabled" : "Disabled");
    pthread_mutex_unlock(&logger_mutex);
}

// Очистка logger
void structured_logger_cleanup(void) {
    pthread_mutex_lock(&logger_mutex);
    
    // Остановка async logger
    if (async_logger_running) {
        async_logger_running = 0;
        pthread_cond_signal(&async_log_buffer.buffer_not_empty);
        pthread_join(async_logger_thread, NULL);
    }
    
    // Закрытие файлов
    if (log_file_handle) {
        fclose(log_file_handle);
        log_file_handle = NULL;
    }
    if (error_log_file_handle) {
        fclose(error_log_file_handle);
        error_log_file_handle = NULL;
    }
    
    // Очистка buffer
    if (async_log_buffer.entries) {
        free(async_log_buffer.entries);
        async_log_buffer.entries = NULL;
    }
    
    pthread_mutex_unlock(&logger_mutex);
    
    pthread_mutex_destroy(&async_log_buffer.buffer_mutex);
    pthread_cond_destroy(&async_log_buffer.buffer_not_full);
    pthread_cond_destroy(&async_log_buffer.buffer_not_empty);
    pthread_mutex_destroy(&logger_mutex);
    
    memset(&logger_stats, 0, sizeof(logger_stats));
    memset(&global_logger_config, 0, sizeof(global_logger_config));
    
    vkprintf(1, "Structured logger cleaned up\n");
}