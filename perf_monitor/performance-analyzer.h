#ifndef PERFORMANCE_ANALYZER_H
#define PERFORMANCE_ANALYZER_H

#include <stdint.h>
#include <time.h>

// Типы анализа производительности
typedef enum {
    PERF_ANALYSIS_LATENCY = 0,
    PERF_ANALYSIS_THROUGHPUT,
    PERF_ANALYSIS_RESOURCE_USAGE,
    PERF_ANALYSIS_BOTTLENECKS,
    PERF_ANALYSIS_SCALABILITY
} perf_analysis_type_t;

// Уровни детализации анализа
typedef enum {
    PERF_DETAIL_LOW = 0,
    PERF_DETAIL_MEDIUM,
    PERF_DETAIL_HIGH,
    PERF_DETAIL_MAXIMUM
} perf_detail_level_t;

// Статистика производительности
typedef struct {
    // Временные метрики
    double avg_response_time_ms;
    double min_response_time_ms;
    double max_response_time_ms;
    double p95_response_time_ms;
    double p99_response_time_ms;
    
    // Пропускная способность
    double requests_per_second;
    double max_throughput_rps;
    uint64_t total_requests;
    uint64_t successful_requests;
    uint64_t failed_requests;
    
    // Ресурсы
    double cpu_usage_percent;
    uint64_t memory_usage_bytes;
    uint64_t peak_memory_usage_bytes;
    double disk_io_mb_per_sec;
    double network_io_mb_per_sec;
    
    // Ошибки
    double error_rate_percent;
    uint64_t total_errors;
    uint64_t timeouts_count;
    
    // Временные метки
    time_t analysis_start_time;
    time_t analysis_end_time;
    double analysis_duration_seconds;
} performance_stats_t;

// Конфигурация анализатора
typedef struct {
    perf_analysis_type_t analysis_type;
    perf_detail_level_t detail_level;
    
    // Временные параметры
    int analysis_duration_seconds;
    int sampling_interval_ms;
    int warmup_period_seconds;
    
    // Пороговые значения
    double latency_warning_threshold_ms;
    double latency_critical_threshold_ms;
    double error_rate_warning_percent;
    double error_rate_critical_percent;
    double cpu_usage_warning_percent;
    double cpu_usage_critical_percent;
    
    // Параметры масштабируемости
    int min_concurrent_connections;
    int max_concurrent_connections;
    int connection_step;
    
    // Фильтрация
    char component_filter[64];
    int enable_component_filtering;
} perf_analyzer_config_t;

// Результаты анализа
typedef struct {
    performance_stats_t baseline_stats;
    performance_stats_t current_stats;
    
    // Аномалии
    struct {
        int has_latency_spikes;
        int has_throughput_degradation;
        int has_resource_bottlenecks;
        int has_scalability_issues;
    } anomalies;
    
    // Рекомендации
    char recommendations[10][256];
    int recommendation_count;
    
    // Метрики сравнения
    double performance_improvement_percent;
    double regression_detected_percent;
    
    // Статус
    int analysis_complete;
    int has_issues;
    int severity_level; // 0-10 scale
} perf_analysis_results_t;

// Контекст анализатора
typedef struct {
    perf_analyzer_config_t config;
    perf_analysis_results_t results;
    
    // История метрик
    struct {
        double *response_times;
        uint64_t *timestamps;
        size_t *buffer_sizes;
        int sample_count;
        int max_samples;
        int current_index;
    } metrics_history;
    
    // Статистика по компонентам
    struct {
        char component_name[64];
        performance_stats_t stats;
        int active;
    } component_stats[32];
    int component_count;
    
    // Состояние
    int is_running;
    int is_analyzing;
    time_t start_time;
    time_t last_update_time;
    
    // Счетчики
    uint64_t total_samples_collected;
    uint64_t anomaly_count;
} perf_analyzer_t;

// Инициализация анализатора
perf_analyzer_t* perf_analyzer_init(const perf_analyzer_config_t *config);
int perf_analyzer_configure(perf_analyzer_t *analyzer, 
                           const perf_analyzer_config_t *config);
void perf_analyzer_cleanup(perf_analyzer_t *analyzer);

// Запуск и остановка анализа
int perf_analyzer_start(perf_analyzer_t *analyzer);
int perf_analyzer_stop(perf_analyzer_t *analyzer);
int perf_analyzer_pause(perf_analyzer_t *analyzer);
int perf_analyzer_resume(perf_analyzer_t *analyzer);

// Сбор метрик
int perf_analyzer_record_request(perf_analyzer_t *analyzer,
                               const char *component,
                               double response_time_ms,
                               int success,
                               size_t request_size,
                               size_t response_size);
int perf_analyzer_record_resource_usage(perf_analyzer_t *analyzer,
                                      double cpu_percent,
                                      uint64_t memory_bytes,
                                      double disk_io,
                                      double network_io);
int perf_analyzer_record_error(perf_analyzer_t *analyzer,
                             const char *component,
                             const char *error_type,
                             const char *details);

// Анализ
int perf_analyzer_run_analysis(perf_analyzer_t *analyzer);
int perf_analyzer_compare_with_baseline(perf_analyzer_t *analyzer);
int perf_analyzer_detect_anomalies(perf_analyzer_t *analyzer);
int perf_analyzer_generate_recommendations(perf_analyzer_t *analyzer);

// Получение результатов
const perf_analysis_results_t* perf_analyzer_get_results(perf_analyzer_t *analyzer);
const performance_stats_t* perf_analyzer_get_stats(perf_analyzer_t *analyzer,
                                                 const char *component);
void perf_analyzer_print_report(perf_analyzer_t *analyzer);
int perf_analyzer_export_results(perf_analyzer_t *analyzer,
                               const char *filename,
                               const char *format); // "json", "csv", "html"

// Утилиты
double perf_analyzer_get_current_cpu_usage(void);
uint64_t perf_analyzer_get_current_memory_usage(void);
double perf_analyzer_calculate_percentile(double *values, int count, double percentile);
int perf_analyzer_wait_for_steady_state(perf_analyzer_t *analyzer);

// Интеграция
int perf_analyzer_integrate_with_monitoring(perf_analyzer_t *analyzer,
                                          void *monitoring_instance);
int perf_analyzer_set_callback(perf_analyzer_t *analyzer,
                             void (*callback)(const perf_analysis_results_t *results));

#endif // PERFORMANCE_ANALYZER_H