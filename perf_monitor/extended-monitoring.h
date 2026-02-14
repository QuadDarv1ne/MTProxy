#ifndef EXTENDED_MONITORING_H
#define EXTENDED_MONITORING_H

#include <stdint.h>
#include <time.h>

// Типы метрик
typedef enum {
    METRIC_TYPE_COUNTER = 0,
    METRIC_TYPE_GAUGE,
    METRIC_TYPE_HISTOGRAM,
    METRIC_TYPE_SUMMARY,
    METRIC_TYPE_TIMER
} metric_type_t;

// Уровни важности
typedef enum {
    ALERT_LEVEL_INFO = 0,
    ALERT_LEVEL_WARNING,
    ALERT_LEVEL_CRITICAL,
    ALERT_LEVEL_EMERGENCY
} alert_level_t;

// Типы алертов
typedef enum {
    ALERT_TYPE_THRESHOLD = 0,
    ALERT_TYPE_ANOMALY,
    ALERT_TYPE_TREND,
    ALERT_TYPE_CORRELATION
} alert_type_t;

// Структура метрики
typedef struct {
    char name[128];
    char description[256];
    char labels[256];  // key=value,key2=value2 формат
    metric_type_t type;
    
    // Значения
    double value;
    double min_value;
    double max_value;
    double sum;
    uint64_t count;
    
    // Гистограмма
    double *buckets;
    uint64_t *bucket_counts;
    int bucket_count;
    
    // Статистика
    time_t last_update;
    uint64_t update_count;
    double rate_per_second;
} metric_t;

// Правило алерта
typedef struct {
    char name[64];
    char metric_name[128];
    alert_type_t type;
    alert_level_t level;
    
    // Пороговые значения
    double threshold_value;
    double warning_threshold;
    double critical_threshold;
    
    // Временные параметры
    int evaluation_period_seconds;
    int cooldown_period_seconds;
    time_t last_triggered;
    
    // Статистика
    uint64_t trigger_count;
    int is_active;
} alert_rule_t;

// Событие мониторинга
typedef struct {
    time_t timestamp;
    char component[64];
    char message[256];
    alert_level_t level;
    char details[512];
} monitoring_event_t;

// Статистика компонента
typedef struct {
    char name[64];
    uint64_t total_requests;
    uint64_t successful_requests;
    uint64_t failed_requests;
    double avg_response_time_ms;
    double error_rate_percent;
    double throughput_rps;
    time_t last_activity;
} component_stats_t;

// Расширенный мониторинг
typedef struct {
    // Метрики
    metric_t *metrics;
    int metric_count;
    int max_metrics;
    
    // Алерты
    alert_rule_t *alert_rules;
    int alert_count;
    int max_alerts;
    
    // События
    monitoring_event_t *event_buffer;
    int event_count;
    int event_buffer_size;
    int event_head;
    int event_tail;
    
    // Статистика компонентов
    component_stats_t *component_stats;
    int component_count;
    int max_components;
    
    // Экспорт данных
    struct {
        int enable_prometheus_export;
        int prometheus_port;
        int enable_json_export;
        char json_file_path[256];
        int enable_influxdb_export;
        char influxdb_url[256];
        char influxdb_database[64];
    } exporters;
    
    // Конфигурация
    struct {
        int enable_auto_alerting;
        int enable_performance_monitoring;
        int enable_security_monitoring;
        int enable_resource_monitoring;
        double sampling_rate;
        int retention_days;
    } config;
    
    // Статистика
    struct {
        uint64_t total_metrics_collected;
        uint64_t total_alerts_triggered;
        uint64_t total_events_logged;
        uint64_t dropped_events;
        double avg_collection_interval_ms;
    } stats;
    
    // Состояние
    int is_initialized;
    int is_running;
    time_t start_time;
    time_t last_collection_time;
} extended_monitoring_t;

// Инициализация и конфигурация
extended_monitoring_t* monitoring_init(int max_metrics, int max_alerts, int event_buffer_size);
int monitoring_configure(extended_monitoring_t *monitoring, 
                        const char *config_file);
void monitoring_cleanup(extended_monitoring_t *monitoring);

// Метрики
metric_t* monitoring_register_metric(extended_monitoring_t *monitoring,
                                   const char *name, 
                                   const char *description,
                                   const char *labels,
                                   metric_type_t type);
int monitoring_update_metric(extended_monitoring_t *monitoring, 
                           const char *name,
                           double value);
int monitoring_increment_metric(extended_monitoring_t *monitoring,
                              const char *name,
                              double increment);
double monitoring_get_metric_value(extended_monitoring_t *monitoring,
                                 const char *name);

// Гистограммы
int monitoring_histogram_observe(extended_monitoring_t *monitoring,
                               const char *name,
                               double value);
int monitoring_histogram_init_buckets(extended_monitoring_t *monitoring,
                                    const char *name,
                                    double *buckets,
                                    int bucket_count);

// Алерты
alert_rule_t* monitoring_create_alert_rule(extended_monitoring_t *monitoring,
                                         const char *name,
                                         const char *metric_name,
                                         alert_type_t type,
                                         alert_level_t level);
int monitoring_update_alert_threshold(extended_monitoring_t *monitoring,
                                    const char *alert_name,
                                    double threshold);
int monitoring_check_alerts(extended_monitoring_t *monitoring);

// События
int monitoring_log_event(extended_monitoring_t *monitoring,
                        const char *component,
                        alert_level_t level,
                        const char *message,
                        const char *details);
int monitoring_log_security_event(extended_monitoring_t *monitoring,
                                 const char *event_type,
                                 const char *source,
                                 const char *details);
int monitoring_log_performance_event(extended_monitoring_t *monitoring,
                                   const char *operation,
                                   double duration_ms,
                                   size_t data_size);

// Статистика компонентов
component_stats_t* monitoring_get_component_stats(extended_monitoring_t *monitoring,
                                                 const char *component_name);
int monitoring_update_component_stats(extended_monitoring_t *monitoring,
                                     const char *component_name,
                                     int success,
                                     double response_time_ms);

// Сбор системных метрик
int monitoring_collect_system_metrics(extended_monitoring_t *monitoring);
int monitoring_collect_process_metrics(extended_monitoring_t *monitoring);
int monitoring_collect_network_metrics(extended_monitoring_t *monitoring);
int monitoring_collect_crypto_metrics(extended_monitoring_t *monitoring);

// Экспорт данных
int monitoring_export_to_prometheus(extended_monitoring_t *monitoring,
                                  const char *host,
                                  int port);
int monitoring_export_to_json(extended_monitoring_t *monitoring,
                             const char *file_path);
int monitoring_export_to_influxdb(extended_monitoring_t *monitoring,
                                 const char *url,
                                 const char *database);

// Управление
int monitoring_start(extended_monitoring_t *monitoring);
int monitoring_stop(extended_monitoring_t *monitoring);
int monitoring_reset_stats(extended_monitoring_t *monitoring);

// Получение информации
const metric_t* monitoring_get_metrics(extended_monitoring_t *monitoring, 
                                     int *count);
const alert_rule_t* monitoring_get_alerts(extended_monitoring_t *monitoring,
                                        int *count);
const monitoring_event_t* monitoring_get_events(extended_monitoring_t *monitoring,
                                              int *count);
const component_stats_t* monitoring_get_component_stats_all(extended_monitoring_t *monitoring,
                                                          int *count);

// Утилиты
void monitoring_print_stats(extended_monitoring_t *monitoring);
time_t monitoring_get_current_time(void);
double monitoring_get_uptime_seconds(extended_monitoring_t *monitoring);

// Расширенные функции мониторинга
int monitoring_add_custom_exporter(extended_monitoring_t *monitoring,
                                 const char *name,
                                 int (*export_func)(extended_monitoring_t*, const char*));
int monitoring_set_dynamic_threshold(extended_monitoring_t *monitoring,
                                   const char *metric_name,
                                   double (*threshold_func)(void));
int monitoring_create_dashboard_endpoint(extended_monitoring_t *monitoring,
                                      int port);
int monitoring_export_for_grafana(extended_monitoring_t *monitoring,
                                 const char *output_file);
void monitoring_set_retention_policy(extended_monitoring_t *monitoring,
                                   int days,
                                   int samples_per_day);

#endif // EXTENDED_MONITORING_H