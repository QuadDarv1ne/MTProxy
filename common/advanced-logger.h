#ifndef ADVANCED_LOGGER_H
#define ADVANCED_LOGGER_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>

// Уровни логирования
typedef enum {
    LOG_LEVEL_OFF = 0,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE
} log_level_t;

// Форматы вывода
typedef enum {
    LOG_FORMAT_SIMPLE = 0,
    LOG_FORMAT_DETAILED,
    LOG_FORMAT_JSON,
    LOG_FORMAT_SYSLOG
} log_format_t;

// Типы вывода
typedef enum {
    LOG_OUTPUT_CONSOLE = 0,
    LOG_OUTPUT_FILE,
    LOG_OUTPUT_SYSLOG,
    LOG_OUTPUT_REMOTE
} log_output_t;

// Статистика логирования
typedef struct {
    long long total_entries;
    long long entries_by_level[7];  // Для каждого уровня
    long long dropped_entries;
    long long filtered_entries;
    double avg_entry_size_bytes;
    double total_logging_time_ms;
} logger_stats_t;

// Конфигурация логгера
typedef struct {
    log_level_t min_level;
    log_format_t format;
    log_output_t output_type;
    
    // Файл вывода
    char log_file_path[256];
    size_t max_file_size_bytes;
    int max_backup_files;
    int enable_rotation;
    
    // Сетевой вывод
    char remote_host[64];
    int remote_port;
    int enable_remote_logging;
    
    // Фильтрация
    char component_filter[64];
    int enable_component_filtering;
    char *keyword_filters[16];
    int keyword_filter_count;
    
    // Буферизация
    size_t buffer_size;
    int enable_buffering;
    int flush_interval_seconds;
    
    // Асинхронность
    int enable_async_logging;
    int queue_size;
    
    // Форматирование
    int enable_colors;
    int enable_timestamps;
    int enable_thread_ids;
    int enable_caller_info;
} logger_config_t;

// Контекст логгера
typedef struct {
    logger_config_t config;
    logger_stats_t stats;
    
    // Файловые дескрипторы
    FILE *log_file;
    FILE *error_file;
    
    // Буферы
    char *buffer;
    size_t buffer_pos;
    size_t buffer_capacity;
    
    // Синхронизация
    void *mutex;  // platform-specific mutex
    
    // Асинхронная очередь
    struct {
        char **entries;
        int *levels;
        time_t *timestamps;
        int head;
        int tail;
        int count;
        int capacity;
    } async_queue;
    
    // Состояние
    int is_initialized;
    int is_running;
    time_t last_flush_time;
    long long start_time;
} advanced_logger_t;

// Инициализация и конфигурация
advanced_logger_t* logger_init(const logger_config_t *config);
int logger_configure(advanced_logger_t *logger, const logger_config_t *config);
void logger_cleanup(advanced_logger_t *logger);

// Основные функции логирования
void logger_log(advanced_logger_t *logger, log_level_t level, 
                const char *component, const char *format, ...);
void logger_log_with_caller(advanced_logger_t *logger, log_level_t level,
                           const char *component, const char *file, 
                           int line, const char *function, 
                           const char *format, ...);

// Уровневое логирование
#define logger_fatal(logger, component, ...) \
    logger_log(logger, LOG_LEVEL_FATAL, component, __VA_ARGS__)
    
#define logger_error(logger, component, ...) \
    logger_log(logger, LOG_LEVEL_ERROR, component, __VA_ARGS__)
    
#define logger_warn(logger, component, ...) \
    logger_log(logger, LOG_LEVEL_WARN, component, __VA_ARGS__)
    
#define logger_info(logger, component, ...) \
    logger_log(logger, LOG_LEVEL_INFO, component, __VA_ARGS__)
    
#define logger_debug(logger, component, ...) \
    logger_log(logger, LOG_LEVEL_DEBUG, component, __VA_ARGS__)
    
#define logger_trace(logger, component, ...) \
    logger_log(logger, LOG_LEVEL_TRACE, component, __VA_ARGS__)

// Логирование с информацией о вызове
#define logger_log_caller(logger, level, component, ...) \
    logger_log_with_caller(logger, level, component, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

// Управление логгером
int logger_flush(advanced_logger_t *logger);
int logger_rotate(advanced_logger_t *logger);
void logger_reset_stats(advanced_logger_t *logger);

// Получение информации
const logger_stats_t* logger_get_stats(advanced_logger_t *logger);
const logger_config_t* logger_get_config(advanced_logger_t *logger);
int logger_is_level_enabled(advanced_logger_t *logger, log_level_t level);

// Специализированные логгеры
void logger_log_hex_dump(advanced_logger_t *logger, log_level_t level,
                        const char *component, const void *data, 
                        size_t length, const char *description);
void logger_log_performance(advanced_logger_t *logger, const char *component,
                           const char *operation, double duration_ms,
                           size_t data_size_bytes);
void logger_log_security_event(advanced_logger_t *logger, const char *event_type,
                              const char *source_ip, const char *details);

// Интеграция с существующими системами
int logger_integrate_with_kprintf(advanced_logger_t *logger);
int logger_integrate_with_structured_logger(advanced_logger_t *logger);

// Утилиты
const char* logger_level_to_string(log_level_t level);
log_level_t logger_string_to_level(const char *level_str);
void logger_set_global_logger(advanced_logger_t *logger);
advanced_logger_t* logger_get_global_logger(void);

#endif // ADVANCED_LOGGER_H