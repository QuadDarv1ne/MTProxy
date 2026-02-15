/*
 * enhanced-observability.h
 * Advanced Observability and Metrics System
 *
 * This system provides comprehensive monitoring, metrics collection,
 * and observability features for the MTProxy application.
 */

#ifndef ENHANCED_OBSERVABILITY_H
#define ENHANCED_OBSERVABILITY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Metric types
typedef enum {
    METRIC_TYPE_COUNTER = 0,
    METRIC_TYPE_GAUGE,
    METRIC_TYPE_HISTOGRAM,
    METRIC_TYPE_SUMMARY,
    METRIC_TYPE_TIMER
} metric_type_t;

// Log levels
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL
} log_level_t;

// Export formats
typedef enum {
    EXPORT_FORMAT_JSON = 0,
    EXPORT_FORMAT_PROMETHEUS,
    EXPORT_FORMAT_INFLUXDB,
    EXPORT_FORMAT_OPENTELEMETRY,
    EXPORT_FORMAT_CUSTOM
} export_format_t;

// Metric structure
typedef struct {
    char name[128];
    char description[256];
    metric_type_t type;
    double value;
    uint64_t timestamp;
    char labels[256];
    char unit[32];
    int sample_count;
    double sum;
    double min_value;
    double max_value;
    double* histogram_buckets;
    int bucket_count;
} metric_t;

// Log entry structure
typedef struct {
    uint64_t timestamp;
    log_level_t level;
    char component[64];
    char message[512];
    char context[256];
    uint32_t thread_id;
    char trace_id[32];
    char span_id[16];
} log_entry_t;

// Tracing span structure
typedef struct {
    char trace_id[32];
    char span_id[16];
    char parent_span_id[16];
    char operation_name[128];
    uint64_t start_time;
    uint64_t end_time;
    char tags[512];
    bool is_error;
    char error_message[256];
    struct span_struct* next;
} span_t;

// Forward declaration for the struct
typedef struct span_struct span_struct_t;

// Alert structure
typedef struct {
    uint64_t alert_id;
    char name[128];
    char description[256];
    log_level_t severity;
    double current_value;
    double threshold;
    uint64_t trigger_time;
    bool is_active;
    bool is_acknowledged;
    char notification_targets[256];
} alert_t;

// Observability configuration
typedef struct {
    int enable_metrics_collection;
    int enable_logging;
    int enable_tracing;
    int enable_alerting;
    int metrics_collection_interval_ms;
    int log_rotation_size_mb;
    int log_retention_days;
    int max_log_files;
    export_format_t export_format;
    char export_endpoint[256];
    int export_interval_seconds;
    bool enable_remote_write;
    int remote_write_timeout_seconds;
    bool enable_local_storage;
    char local_storage_path[256];
    int max_metrics_stored;
    int max_logs_stored;
    bool enable_compression;
    int compression_level;
} observability_config_t;

// Performance metrics
typedef struct {
    uint64_t total_metrics_collected;
    uint64_t total_logs_written;
    uint64_t total_spans_traced;
    uint64_t total_alerts_generated;
    uint64_t metrics_export_success;
    uint64_t metrics_export_failures;
    uint64_t log_write_success;
    uint64_t log_write_failures;
    double average_export_latency_ms;
    double average_log_write_latency_ms;
    uint64_t disk_usage_bytes;
    uint64_t memory_usage_bytes;
    uint64_t last_export_time;
    uint64_t last_log_rotation_time;
} observability_stats_t;

// Observability context
typedef struct {
    observability_config_t config;
    observability_stats_t stats;
    metric_t* metrics;
    int metric_count;
    log_entry_t* log_buffer;
    int log_count;
    span_t* trace_spans;
    int span_count;
    alert_t* active_alerts;
    int alert_count;
    uint64_t start_time;
    bool is_initialized;
    bool is_exporting;
    void* exporter_context;
    char* metric_storage_buffer;
    size_t storage_buffer_size;
} observability_ctx_t;

// Histogram bucket definition
typedef struct {
    double upper_bound;
    uint64_t count;
} histogram_bucket_t;

// Summary quantile definition
typedef struct {
    double quantile;
    double value;
} quantile_t;

// Callback function types
typedef void (*metric_callback_t)(const metric_t* metric);
typedef void (*log_callback_t)(const log_entry_t* log_entry);
typedef void (*alert_callback_t)(const alert_t* alert);
typedef void (*export_callback_t)(const char* exported_data, size_t data_size);

// Function prototypes

// Initialization and cleanup
int init_observability(observability_ctx_t* ctx);
int init_observability_with_config(observability_ctx_t* ctx, const observability_config_t* config);
void cleanup_observability(observability_ctx_t* ctx);

// Configuration management
void get_observability_config(observability_ctx_t* ctx, observability_config_t* config);
int set_observability_config(observability_ctx_t* ctx, const observability_config_t* config);

// Metrics management
int register_metric(observability_ctx_t* ctx, const char* name, const char* description, 
                   metric_type_t type, const char* unit, const char* labels);
int update_metric(observability_ctx_t* ctx, const char* name, double value);
int increment_counter(observability_ctx_t* ctx, const char* name, double increment);
int set_gauge(observability_ctx_t* ctx, const char* name, double value);
int observe_histogram(observability_ctx_t* ctx, const char* name, double value);
int observe_timer(observability_ctx_t* ctx, const char* name, uint64_t duration_ms);
metric_t* get_metric(observability_ctx_t* ctx, const char* name);
int get_all_metrics(observability_ctx_t* ctx, metric_t* metrics, int max_metrics);

// Logging functions
int log_message(observability_ctx_t* ctx, log_level_t level, const char* component,
               const char* message, const char* context);
int log_debug(observability_ctx_t* ctx, const char* component, const char* message, const char* context);
int log_info(observability_ctx_t* ctx, const char* component, const char* message, const char* context);
int log_warn(observability_ctx_t* ctx, const char* component, const char* message, const char* context);
int log_error(observability_ctx_t* ctx, const char* component, const char* message, const char* context);
int log_critical(observability_ctx_t* ctx, const char* component, const char* message, const char* context);
int get_log_entries(observability_ctx_t* ctx, log_entry_t* entries, int max_entries, log_level_t min_level);

// Tracing functions
int start_trace_span(observability_ctx_t* ctx, const char* operation_name, 
                    const char* parent_span_id, span_t* span);
int end_trace_span(observability_ctx_t* ctx, span_t* span);
int add_span_tag(span_t* span, const char* key, const char* value);
int set_span_error(span_t* span, const char* error_message);
int get_trace_spans(observability_ctx_t* ctx, span_t* spans, int max_spans);

// Alerting functions
int define_alert(observability_ctx_t* ctx, const char* name, const char* description,
                log_level_t severity, double threshold, const char* metric_name);
int evaluate_alerts(observability_ctx_t* ctx);
int acknowledge_alert(observability_ctx_t* ctx, uint64_t alert_id);
int get_active_alerts(observability_ctx_t* ctx, alert_t* alerts, int max_alerts);
bool is_alert_active(observability_ctx_t* ctx, const char* alert_name);

// Export functions
int start_metrics_export(observability_ctx_t* ctx);
int stop_metrics_export(observability_ctx_t* ctx);
int export_metrics_now(observability_ctx_t* ctx);
int export_logs_now(observability_ctx_t* ctx);
int export_traces_now(observability_ctx_t* ctx);
char* format_metrics_export(observability_ctx_t* ctx, export_format_t format, size_t* output_size);
int write_metrics_to_file(observability_ctx_t* ctx, const char* filename);
int send_metrics_to_endpoint(observability_ctx_t* ctx, const char* endpoint);

// Storage management
int rotate_log_files(observability_ctx_t* ctx);
int cleanup_old_metrics(observability_ctx_t* ctx, uint64_t older_than_timestamp);
int cleanup_old_logs(observability_ctx_t* ctx, uint64_t older_than_timestamp);
int cleanup_old_traces(observability_ctx_t* ctx, uint64_t older_than_timestamp);
uint64_t get_disk_usage(observability_ctx_t* ctx);
uint64_t get_memory_usage(observability_ctx_t* ctx);

// Performance monitoring
observability_stats_t get_observability_statistics(observability_ctx_t* ctx);
void reset_observability_statistics(observability_ctx_t* ctx);
double get_system_metrics_collection_rate(observability_ctx_t* ctx);
double get_logging_rate(observability_ctx_t* ctx);
double get_tracing_rate(observability_ctx_t* ctx);

// Utility functions
const char* metric_type_to_string(metric_type_t type);
const char* log_level_to_string(log_level_t level);
const char* export_format_to_string(export_format_t format);
uint64_t get_current_timestamp_ms(void);
char* generate_trace_id(void);
char* generate_span_id(void);
int parse_labels(const char* labels_string, char* parsed_labels, size_t max_size);
double calculate_percentile(double* values, int count, double percentile);

// Histogram functions
int create_histogram_buckets(observability_ctx_t* ctx, const char* metric_name, 
                           double* boundaries, int boundary_count);
int get_histogram_data(observability_ctx_t* ctx, const char* metric_name, 
                      histogram_bucket_t* buckets, int max_buckets);
double get_histogram_quantile(observability_ctx_t* ctx, const char* metric_name, double quantile);

// Summary functions
int create_summary_quantiles(observability_ctx_t* ctx, const char* metric_name,
                           double* quantiles, int quantile_count);
int get_summary_data(observability_ctx_t* ctx, const char* metric_name,
                    quantile_t* quantiles, int max_quantiles);

// Callback registration
void register_metric_callback(metric_callback_t callback);
void register_log_callback(log_callback_t callback);
void register_alert_callback(alert_callback_t callback);
void register_export_callback(export_callback_t callback);

// Integration functions
int integrate_with_system_monitor(observability_ctx_t* ctx);
int integrate_with_performance_analyzer(observability_ctx_t* ctx);
int apply_observability_enhancements(observability_ctx_t* ctx);
int verify_observability_integrity(observability_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // ENHANCED_OBSERVABILITY_H