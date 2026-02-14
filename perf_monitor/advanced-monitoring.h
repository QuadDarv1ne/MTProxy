/*
    Расширенная система мониторинга и логирования MTProxy
    Содержит метрики, трассировку, алертинг и экспорт данных
*/

#ifndef ADVANCED_MONITORING_H
#define ADVANCED_MONITORING_H

#include <stdint.h>
#include <stddef.h>

// Конфигурация мониторинга
#define MAX_METRICS 1024
#define MAX_LOG_BUFFER 65536
#define DEFAULT_LOG_LEVEL 2
#define MAX_ALERTS 256
#define METRICS_COLLECTION_INTERVAL 1000 // мс

// Уровни логирования
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5
} log_level_t;

// Типы метрик
typedef enum {
    METRIC_TYPE_COUNTER = 0,
    METRIC_TYPE_GAUGE = 1,
    METRIC_TYPE_HISTOGRAM = 2,
    METRIC_TYPE_SUMMARY = 3
} metric_type_t;

// Типы алертов
typedef enum {
    ALERT_TYPE_NONE = 0,
    ALERT_TYPE_CPU_USAGE = 1,
    ALERT_TYPE_MEMORY_USAGE = 2,
    ALERT_TYPE_NETWORK_LATENCY = 3,
    ALERT_TYPE_CONNECTION_COUNT = 4,
    ALERT_TYPE_ERROR_RATE = 5,
    ALERT_TYPE_THROUGHPUT = 6
} alert_type_t;

// Статус алерта
typedef enum {
    ALERT_STATUS_OK = 0,
    ALERT_STATUS_WARNING = 1,
    ALERT_STATUS_CRITICAL = 2,
    ALERT_STATUS_RESOLVED = 3
} alert_status_t;

// Метрика
typedef struct metric {
    char name[64];
    char description[128];
    metric_type_t type;
    double value;
    double min_value;
    double max_value;
    long long sample_count;
    double sum;
    double *histogram_buckets;
    int bucket_count;
    long long last_update;
} metric_t;

// Алерт
typedef struct alert_rule {
    char name[64];
    alert_type_t type;
    double threshold;
    double duration; // секунды
    alert_status_t status;
    long long triggered_time;
    int notification_sent;
    void (*callback)(struct alert_rule *rule);
} alert_rule_t;

// Лог запись
typedef struct log_entry {
    long long timestamp;
    log_level_t level;
    char component[32];
    char message[512];
    int thread_id;
    uint32_t connection_id;
} log_entry_t;

// Лог буфер
typedef struct log_buffer {
    log_entry_t *entries;
    int capacity;
    int head;
    int tail;
    int count;
    int dropped_entries;
} log_buffer_t;

// Статистика компонентов
typedef struct component_stats {
    char name[32];
    long long total_requests;
    long long successful_requests;
    long long failed_requests;
    double avg_response_time_ms;
    double error_rate_percent;
    long long last_request_time;
} component_stats_t;

// Расширенный мониторинг
typedef struct advanced_monitoring {
    // Метрики
    metric_t *metrics;
    int metric_count;
    int max_metrics;
    
    // Алерты
    alert_rule_t *alert_rules;
    int alert_count;
    int max_alerts;
    
    // Логирование
    log_buffer_t *log_buffer;
    log_level_t current_log_level;
    int enable_file_logging;
    char log_file_path[256];
    
    // Статистика компонентов
    component_stats_t *component_stats;
    int component_count;
    int max_components;
    
    // Экспорт данных
    int enable_prometheus_export;
    int prometheus_port;
    int enable_json_export;
    char export_file_path[256];
    
    // Статус
    int is_initialized;
    int is_running;
    long long start_time;
    
    // Счетчики
    long long total_log_entries;
    long long total_alerts_triggered;
    long long metrics_samples_collected;
} advanced_monitoring_t;

// Инициализация
advanced_monitoring_t* monitoring_init(int max_metrics, int max_alerts, int log_buffer_size);
int monitoring_configure(advanced_monitoring_t *mon, log_level_t log_level, const char *log_file);
void monitoring_cleanup(advanced_monitoring_t *mon);

// Метрики
int monitoring_register_metric(advanced_monitoring_t *mon, const char *name, const char *desc, metric_type_t type);
int monitoring_update_metric(advanced_monitoring_t *mon, const char *name, double value);
int monitoring_increment_metric(advanced_monitoring_t *mon, const char *name, double increment);
metric_t* monitoring_get_metric(advanced_monitoring_t *mon, const char *name);
void monitoring_collect_system_metrics(advanced_monitoring_t *mon);

// Алерты
int monitoring_add_alert_rule(advanced_monitoring_t *mon, const char *name, alert_type_t type, 
                           double threshold, double duration, void (*callback)(alert_rule_t*));
int monitoring_check_alerts(advanced_monitoring_t *mon);
alert_rule_t* monitoring_get_alert(advanced_monitoring_t *mon, const char *name);
void monitoring_reset_alert(alert_rule_t *alert);

// Логирование
int monitoring_log(advanced_monitoring_t *mon, log_level_t level, const char *component, 
                  const char *format, ...);
int monitoring_log_error(advanced_monitoring_t *mon, const char *component, const char *message);
int monitoring_log_warning(advanced_monitoring_t *mon, const char *component, const char *message);
int monitoring_log_info(advanced_monitoring_t *mon, const char *component, const char *message);
int monitoring_log_debug(advanced_monitoring_t *mon, const char *component, const char *message);

// Статистика компонентов
int monitoring_register_component(advanced_monitoring_t *mon, const char *name);
int monitoring_update_component_stats(advanced_monitoring_t *mon, const char *name, 
                                    int success, double response_time_ms);
component_stats_t* monitoring_get_component_stats(advanced_monitoring_t *mon, const char *name);

// Экспорт данных
int monitoring_export_to_prometheus(advanced_monitoring_t *mon, int port);
int monitoring_export_to_json(advanced_monitoring_t *mon, const char *file_path);
int monitoring_export_to_file(advanced_monitoring_t *mon);

// Получение отчетов
void monitoring_get_system_report(advanced_monitoring_t *mon, char *buffer, size_t buffer_size);
void monitoring_get_alerts_report(advanced_monitoring_t *mon, char *buffer, size_t buffer_size);
void monitoring_get_components_report(advanced_monitoring_t *mon, char *buffer, size_t buffer_size);

// Управление
int monitoring_start_collection(advanced_monitoring_t *mon);
int monitoring_stop_collection(advanced_monitoring_t *mon);
void monitoring_reset_stats(advanced_monitoring_t *mon);

// Утилиты
const char* monitoring_level_to_string(log_level_t level);
const char* monitoring_alert_status_to_string(alert_status_t status);
const char* monitoring_metric_type_to_string(metric_type_t type);
long long monitoring_get_current_time_ms(void);

#endif // ADVANCED_MONITORING_H