/*
    Упрощенная система мониторинга и логирования
    Без зависимостей от стандартной библиотеки
*/

#ifndef SIMPLE_MONITORING_H
#define SIMPLE_MONITORING_H

// Базовые типы
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef long long int64_t;
typedef unsigned long long size_t;

// Конфигурация
#define MAX_SIMPLE_METRICS 256
#define MAX_SIMPLE_LOGS 1024
#define MAX_COMPONENTS 64

// Уровни логирования
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4
} log_level_t;

// Статус алерта
typedef enum {
    ALERT_STATUS_OK = 0,
    ALERT_STATUS_WARNING = 1,
    ALERT_STATUS_CRITICAL = 2
} alert_status_t;

// Метрика
typedef struct simple_metric {
    char name[32];
    double value;
    double min_value;
    double max_value;
    int64_t sample_count;
    int64_t last_update;
} simple_metric_t;

// Лог запись
typedef struct simple_log_entry {
    int64_t timestamp;
    log_level_t level;
    char component[16];
    char message[128];
} simple_log_entry_t;

// Статистика компонента
typedef struct simple_component_stats {
    char name[16];
    int64_t total_requests;
    int64_t successful_requests;
    int64_t failed_requests;
    double avg_response_time_ms;
} simple_component_stats_t;

// Основная структура
typedef struct simple_monitoring {
    // Метрики
    simple_metric_t metrics[MAX_SIMPLE_METRICS];
    int metric_count;
    
    // Логи
    simple_log_entry_t log_buffer[MAX_SIMPLE_LOGS];
    int log_head;
    int log_count;
    log_level_t log_level;
    
    // Статистика компонентов
    simple_component_stats_t components[MAX_COMPONENTS];
    int component_count;
    
    // Алерты
    double cpu_threshold;
    double memory_threshold;
    alert_status_t cpu_alert;
    alert_status_t memory_alert;
    
    // Статус
    int is_initialized;
    int64_t start_time;
    
    // Счетчики
    int64_t total_logs;
    int64_t total_alerts;
    int64_t metrics_samples;
} simple_monitoring_t;

// API функции

// Инициализация
simple_monitoring_t* simple_monitoring_init(void);
int simple_monitoring_configure(simple_monitoring_t *mon, log_level_t level);
void simple_monitoring_cleanup(simple_monitoring_t *mon);

// Метрики
int simple_monitoring_add_metric(simple_monitoring_t *mon, const char *name);
int simple_monitoring_update_metric(simple_monitoring_t *mon, const char *name, double value);
int simple_monitoring_increment_metric(simple_monitoring_t *mon, const char *name, double increment);
simple_metric_t* simple_monitoring_get_metric(simple_monitoring_t *mon, const char *name);
void simple_monitoring_collect_system_metrics(simple_monitoring_t *mon);

// Логирование
int simple_monitoring_log(simple_monitoring_t *mon, log_level_t level, const char *component, const char *message);
int simple_monitoring_log_error(simple_monitoring_t *mon, const char *component, const char *message);
int simple_monitoring_log_warning(simple_monitoring_t *mon, const char *component, const char *message);
int simple_monitoring_log_info(simple_monitoring_t *mon, const char *component, const char *message);

// Статистика компонентов
int simple_monitoring_register_component(simple_monitoring_t *mon, const char *name);
int simple_monitoring_update_component_stats(simple_monitoring_t *mon, const char *name, 
                                           int success, double response_time);
simple_component_stats_t* simple_monitoring_get_component_stats(simple_monitoring_t *mon, const char *name);

// Алерты
void simple_monitoring_check_alerts(simple_monitoring_t *mon);
alert_status_t simple_monitoring_get_cpu_alert(simple_monitoring_t *mon);
alert_status_t simple_monitoring_get_memory_alert(simple_monitoring_t *mon);
void simple_monitoring_set_thresholds(simple_monitoring_t *mon, double cpu, double memory);

// Отчеты
void simple_monitoring_get_report(simple_monitoring_t *mon, char *buffer, size_t buffer_size);
void simple_monitoring_get_metrics_report(simple_monitoring_t *mon, char *buffer, size_t buffer_size);
void simple_monitoring_get_alerts_report(simple_monitoring_t *mon, char *buffer, size_t buffer_size);

// Управление
void simple_monitoring_reset_stats(simple_monitoring_t *mon);
int64_t simple_monitoring_get_time_ms(void);

// Утилиты
const char* simple_monitoring_level_to_string(log_level_t level);
const char* simple_monitoring_alert_to_string(alert_status_t status);

#endif // SIMPLE_MONITORING_H