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

#ifndef __STRUCTURED_LOGGER_H__
#define __STRUCTURED_LOGGER_H__

#include <time.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Log levels
enum log_level {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL,
    LOG_LEVEL_MAX
};

// Log formats
enum log_format {
    LOG_FORMAT_STANDARD = 0,
    LOG_FORMAT_JSON,
    LOG_FORMAT_SYSLOG,
    LOG_FORMAT_MAX
};

// Forward declarations
struct log_entry;
struct logger_stats;

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

// Log entry structure
struct log_entry {
    time_t timestamp;
    struct timeval precise_time;
    enum log_level level;
    enum log_format format;
    char component[64];
    char subsystem[64];
    char message[1024];
    char context_data[512];
    int thread_id;
    unsigned int connection_id;
    unsigned int client_ip;
    int is_error;
    int is_security_event;
};

// Инициализация structured logger
int structured_logger_init(const char *log_file_path);

// Основная logging функция
int structured_log(enum log_level level, 
                  const char *component,
                  const char *subsystem,
                  const char *message,
                  const char *context_format, ...);

// Установка контекста для текущего thread
int structured_logger_set_context(const char *session_id, 
                                 const char *request_id,
                                 const char *user_id,
                                 const char *client_info);

// Сброс контекста
int structured_logger_clear_context(void);

// Установка log level
int structured_logger_set_level(enum log_level level);

// Получение статистики
void structured_logger_get_stats(struct logger_stats *stats);

// Вывод статистики
void structured_logger_print_stats(void);

// Очистка logger
void structured_logger_cleanup(void);

// Helper macros для удобного использования
#define LOG_DEBUG(component, subsystem, message, ...) \
    structured_log(LOG_LEVEL_DEBUG, component, subsystem, message, ##__VA_ARGS__)

#define LOG_INFO(component, subsystem, message, ...) \
    structured_log(LOG_LEVEL_INFO, component, subsystem, message, ##__VA_ARGS__)

#define LOG_WARNING(component, subsystem, message, ...) \
    structured_log(LOG_LEVEL_WARNING, component, subsystem, message, ##__VA_ARGS__)

#define LOG_ERROR(component, subsystem, message, ...) \
    structured_log(LOG_LEVEL_ERROR, component, subsystem, message, ##__VA_ARGS__)

#define LOG_CRITICAL(component, subsystem, message, ...) \
    structured_log(LOG_LEVEL_CRITICAL, component, subsystem, message, ##__VA_ARGS__)

// Security logging macros
#define LOG_SECURITY_INFO(message, ...) \
    structured_log(LOG_LEVEL_INFO, "security", "audit", message, ##__VA_ARGS__)

#define LOG_SECURITY_WARNING(message, ...) \
    structured_log(LOG_LEVEL_WARNING, "security", "audit", message, ##__VA_ARGS__)

#define LOG_SECURITY_ERROR(message, ...) \
    structured_log(LOG_LEVEL_ERROR, "security", "audit", message, ##__VA_ARGS__)

// Network logging macros
#define LOG_NETWORK_INFO(message, ...) \
    structured_log(LOG_LEVEL_INFO, "network", "main", message, ##__VA_ARGS__)

#define LOG_NETWORK_ERROR(message, ...) \
    structured_log(LOG_LEVEL_ERROR, "network", "main", message, ##__VA_ARGS__)

// Performance logging macros
#define LOG_PERFORMANCE_INFO(message, ...) \
    structured_log(LOG_LEVEL_INFO, "performance", "metrics", message, ##__VA_ARGS__)

#define LOG_PERFORMANCE_WARNING(message, ...) \
    structured_log(LOG_LEVEL_WARNING, "performance", "metrics", message, ##__VA_ARGS__)

// Convenience functions
int structured_log_with_connection(enum log_level level,
                                  const char *component,
                                  const char *subsystem,
                                  unsigned int connection_id,
                                  unsigned int client_ip,
                                  const char *message,
                                  const char *context_format, ...);

int structured_log_hexdump(enum log_level level,
                          const char *component,
                          const char *subsystem,
                          const void *data,
                          size_t data_len,
                          const char *description);

// Configuration functions
int structured_logger_set_config(const struct logger_config *config);
int structured_logger_get_config(struct logger_config *config);
int structured_logger_reload_config(void);

// Log file management
int structured_logger_rotate_logs(void);
int structured_logger_get_log_file_size(const char *log_file);
int structured_logger_cleanup_old_logs(int max_age_days);

// Query and analysis functions
int structured_logger_query_logs(time_t start_time,
                                time_t end_time,
                                enum log_level min_level,
                                const char *component,
                                const char *subsystem,
                                struct log_entry *results,
                                int max_results);

int structured_logger_get_error_summary(time_t start_time,
                                       time_t end_time,
                                       struct logger_stats *summary);

#ifdef __cplusplus
}
#endif

#endif // __STRUCTURED_LOGGER_H__