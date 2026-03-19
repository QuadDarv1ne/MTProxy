#ifndef ADVANCED_LOGGER_H
#define ADVANCED_LOGGER_H

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>

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
    LOG_FORMAT_SYSLOG,
    LOG_FORMAT_CSV,
    LOG_FORMAT_GELF
} log_format_t;

// Типы вывода
typedef enum {
    LOG_OUTPUT_CONSOLE = 0,
    LOG_OUTPUT_FILE,
    LOG_OUTPUT_SYSLOG,
    LOG_OUTPUT_REMOTE,
    LOG_OUTPUT_MULTICAST  // Несколько выводов одновременно
} log_output_t;

// Расширенные флаги логирования
typedef enum {
    LOG_FLAG_NONE = 0,
    LOG_FLAG_NO_COLOR = 1,
    LOG_FLAG_FORCE_FLUSH = 2,
    LOG_FLAG_SKIP_BUFFER = 4,
    LOG_FLAG_INCLUDE_STACK = 8,
    LOG_FLAG_CORRELATION_ID = 16
} log_flags_t;

// Контекст для распределенной трассировки
typedef struct {
    char trace_id[64];      // Идентификатор трассировки
    char span_id[32];       // Идентификатор текущего спана
    char parent_span_id[32]; // Идентификатор родительского спана
    uint64_t correlation_id; // ID корреляции для связанных событий
} log_context_t;

// Структура для хранения информации о стеке вызовов
typedef struct {
    void **frames;
    int frame_count;
    int max_frames;
    char **symbolicated_frames;
} stack_trace_t;

// Статистика логирования
typedef struct {
    long long total_entries;
    long long entries_by_level[7];  // Для каждого уровня
    long long dropped_entries;
    long long filtered_entries;
    double avg_entry_size_bytes;
    double total_logging_time_ms;
    
    // Расширенная статистика
    long long total_bytes_written;
    long long rotation_count;
    long long flush_count;
    double avg_write_time_ms;
    double max_write_time_ms;
    long long async_queue_full_count;
    long long context_switch_count;
    long long correlation_events;
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
    
    // Расширенная ротация
    int enable_time_rotation;
    int rotation_hour;
    int rotation_minute;
    char archive_dir[256];
    int enable_compression;

    // Сетевой вывод
    char remote_host[64];
    int remote_port;
    int enable_remote_logging;
    char remote_protocol[16]; // "tcp", "udp", "tls"

    // Фильтрация
    char component_filter[64];
    int enable_component_filtering;
    char *keyword_filters[16];
    int keyword_filter_count;
    
    // Расширенная фильтрация
    int (*custom_filter)(log_level_t level, const char *component, const char *message);

    // Буферизация
    size_t buffer_size;
    int enable_buffering;
    int flush_interval_seconds;
    
    // Асинхронность
    int enable_async_logging;
    int queue_size;
    
    // Sampling (для high-load систем)
    int enable_sampling;
    double sample_rate; // 0.0 - 1.0

    // Форматирование
    int enable_colors;
    int enable_timestamps;
    int enable_thread_ids;
    int enable_caller_info;
    int enable_pid;
    int enable_hostname;
    
    // Расширенные возможности
    int enable_stack_trace_on_error;
    int enable_correlation_tracking;
    int max_stack_depth;
    
    // Кастомное форматирование
    char timestamp_format[32];
    char custom_prefix[64];
} logger_config_t;

// Контекст логгера
typedef struct {
    logger_config_t config;
    logger_stats_t stats;

    // Файловые дескрипторы
    FILE *log_file;
    FILE *error_file;
    char current_log_path[512];
    time_t current_rotation_time;

    // Буферы
    char *buffer;
    size_t buffer_pos;
    size_t buffer_capacity;
    
    // Буфер для стека вызовов
    char *stack_buffer;
    size_t stack_buffer_capacity;

    // Синхронизация
    void *mutex;  // platform-specific mutex
    
    // Condition variable для async flushing
    void *flush_cond;
    void *flush_mutex;

    // Асинхронная очередь
    struct {
        char **entries;
        int *levels;
        time_t *timestamps;
        log_context_t *contexts;  // Контексты для распределенной трассировки
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
    
    // Расширенное состояние
    uint64_t current_correlation_id;
    log_context_t current_context;
    pthread_t async_thread;
    int async_thread_running;
    
    // Thread-local storage для контекста
    void *tls_key;
    int tls_initialized;
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
void logger_log_with_flags(advanced_logger_t *logger, log_level_t level,
                          int flags, const char *component,
                          const char *format, ...);
void logger_log_with_context(advanced_logger_t *logger, log_level_t level,
                            const log_context_t *context,
                            const char *component, const char *format, ...);

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

// Логирование с флагами
#define logger_error_with_stack(logger, component, ...) \
    logger_log_with_flags(logger, LOG_LEVEL_ERROR, LOG_FLAG_INCLUDE_STACK, component, __VA_ARGS__)

#define logger_info_force_flush(logger, component, ...) \
    logger_log_with_flags(logger, LOG_LEVEL_INFO, LOG_FLAG_FORCE_FLUSH, component, __VA_ARGS__)

// Управление контекстом трассировки
log_context_t logger_create_context(void);
void logger_set_context(advanced_logger_t *logger, const log_context_t *context);
void logger_get_current_context(advanced_logger_t *logger, log_context_t *context);
uint64_t logger_generate_correlation_id(advanced_logger_t *logger);
void logger_push_context(advanced_logger_t *logger, const log_context_t *context);
void logger_pop_context(advanced_logger_t *logger);

// Управление логгером
int logger_flush(advanced_logger_t *logger);
int logger_rotate(advanced_logger_t *logger);
int logger_rotate_by_time(advanced_logger_t *logger);
void logger_reset_stats(advanced_logger_t *logger);
int logger_set_level(advanced_logger_t *logger, log_level_t level);
int logger_set_output(advanced_logger_t *logger, log_output_t output, const char *path);

// Получение информации
const logger_stats_t* logger_get_stats(advanced_logger_t *logger);
const logger_config_t* logger_get_config(advanced_logger_t *logger);
int logger_is_level_enabled(advanced_logger_t *logger, log_level_t level);
char* logger_get_log_file_path(advanced_logger_t *logger, char *buffer, size_t size);

// Специализированные логгеры
void logger_log_hex_dump(advanced_logger_t *logger, log_level_t level,
                        const char *component, const void *data,
                        size_t length, const char *description);
void logger_log_performance(advanced_logger_t *logger, const char *component,
                           const char *operation, double duration_ms,
                           size_t data_size_bytes);
void logger_log_security_event(advanced_logger_t *logger, const char *event_type,
                              const char *source_ip, const char *details);
void logger_log_stack_trace(advanced_logger_t *logger, log_level_t level,
                           const char *component, const char *message);

// Расширенные функции
int logger_capture_stack_trace(stack_trace_t *trace, int max_depth);
void logger_free_stack_trace(stack_trace_t *trace);
char* logger_symbolicate_stack_trace(const stack_trace_t *trace);

// Асинхронное логирование
int logger_start_async_thread(advanced_logger_t *logger);
void logger_stop_async_thread(advanced_logger_t *logger);
int logger_is_async_running(advanced_logger_t *logger);

// Интеграция с существующими системами
int logger_integrate_with_kprintf(advanced_logger_t *logger);
int logger_integrate_with_structured_logger(advanced_logger_t *logger);

// Утилиты
const char* logger_level_to_string(log_level_t level);
log_level_t logger_string_to_level(const char *level_str);
void logger_set_global_logger(advanced_logger_t *logger);
advanced_logger_t* logger_get_global_logger(void);

// Макросы для удобного логирования с контекстом
#define LOG_WITH_CTX(logger, level, ctx, comp, ...) \
    logger_log_with_context(logger, level, ctx, comp, __VA_ARGS__)

#define LOG_CTX_PUSH(logger, ctx) logger_push_context(logger, ctx)
#define LOG_CTX_POP(logger) logger_pop_context(logger)

#endif // ADVANCED_LOGGER_H