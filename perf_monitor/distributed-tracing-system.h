/*
 * Distributed Tracing and Observability System for MTProxy
 * Comprehensive monitoring, tracing, and observability for distributed systems
 */

#ifndef _DISTRIBUTED_TRACING_SYSTEM_H_
#define _DISTRIBUTED_TRACING_SYSTEM_H_

#include <stdint.h>
#include <stddef.h>

// Trace types
typedef enum {
    TRACE_TYPE_REQUEST = 0,        // Client request processing
    TRACE_TYPE_CONNECTION = 1,     // Network connection handling
    TRACE_TYPE_CRYPTO = 2,         // Cryptographic operations
    TRACE_TYPE_MEMORY = 3,         // Memory allocation/deallocation
    TRACE_TYPE_DATABASE = 4,       // Database operations
    TRACE_TYPE_CACHE = 5,          // Cache operations
    TRACE_TYPE_NETWORK = 6,        // Network I/O operations
    TRACE_TYPE_AUTHENTICATION = 7, // Authentication operations
    TRACE_TYPE_ERROR = 8,          // Error handling
    TRACE_TYPE_BACKGROUND = 9      // Background processing
} trace_type_t;

// Span status
typedef enum {
    SPAN_STATUS_UNKNOWN = 0,
    SPAN_STATUS_OK = 1,
    SPAN_STATUS_ERROR = 2,
    SPAN_STATUS_CANCELED = 3
} span_status_t;

// Trace context
typedef struct {
    char trace_id[32];             // Unique trace identifier
    char span_id[16];              // Current span identifier
    char parent_span_id[16];       // Parent span identifier (if any)
    int sampled;                   // Whether this trace is sampled
    long long trace_flags;         // Additional trace flags
} trace_context_t;

// Span attributes
typedef struct {
    char key[64];
    char value[256];
    int value_type;                // 0=string, 1=integer, 2=double, 3=boolean
} span_attribute_t;

// Span event
typedef struct {
    long long timestamp;
    char name[64];
    char description[256];
    int attribute_count;
    span_attribute_t attributes[10];
} span_event_t;

// Span structure
typedef struct {
    char span_id[16];              // Unique span identifier
    char parent_span_id[16];       // Parent span identifier
    char trace_id[32];             // Associated trace identifier
    char name[128];                // Span name
    trace_type_t type;             // Type of operation
    span_status_t status;          // Span status
    long long start_time;          // Start timestamp (microseconds)
    long long end_time;            // End timestamp (microseconds)
    long long duration_micros;     // Duration in microseconds
    int attribute_count;           // Number of attributes
    span_attribute_t attributes[20]; // Span attributes
    int event_count;               // Number of events
    span_event_t events[10];       // Span events
    long long error_count;         // Number of errors in this span
    char error_message[256];       // Error message if status is ERROR
    int is_remote;                 // Whether this span is from remote service
    char service_name[64];         // Service name
    char component_name[64];       // Component name
} trace_span_t;

// Trace structure
typedef struct {
    char trace_id[32];             // Unique trace identifier
    trace_span_t *spans;           // Array of spans in this trace
    int span_count;                // Number of spans
    int max_spans;                 // Maximum spans capacity
    long long start_time;          // Trace start time
    long long end_time;            // Trace end time
    long long duration_micros;     // Total duration
    span_status_t overall_status;  // Overall trace status
    long long error_count;         // Total errors in trace
    int sampled;                   // Whether trace was sampled
    char root_service[64];         // Root service name
} trace_t;

// Metrics collection
typedef struct {
    // Timing metrics
    double min_duration_ms;
    double max_duration_ms;
    double avg_duration_ms;
    double p50_duration_ms;        // 50th percentile
    double p90_duration_ms;        // 90th percentile
    double p95_duration_ms;        // 95th percentile
    double p99_duration_ms;        // 99th percentile
    
    // Count metrics
    long long total_spans;
    long long error_spans;
    long long success_spans;
    long long total_traces;
    long long error_traces;
    
    // Rate metrics
    double spans_per_second;
    double traces_per_second;
    double error_rate_percent;
    
    // Resource metrics
    double avg_cpu_usage;
    double avg_memory_usage;
    double avg_network_io;
    
    // Time window
    long long time_window_start;
    long long time_window_end;
} trace_metrics_t;

// Tracing configuration
typedef struct {
    int enable_tracing;
    int enable_sampling;
    int sampling_rate;             // 1 in N sampling rate
    int max_trace_history;
    int max_span_attributes;
    int max_span_events;
    int enable_metrics_collection;
    int metrics_collection_interval_seconds;
    int enable_export;
    char export_endpoint[256];     // Export endpoint for traces
    int export_format;             // 0=JSON, 1=protobuf, 2=custom
    int enable_correlation;
    int correlation_window_seconds;
    int enable_anomaly_detection;
    double anomaly_threshold;
    int enable_alerts;
    int alert_threshold_error_rate;
} tracing_config_t;

// Distributed tracing system
typedef struct {
    // Configuration
    tracing_config_t config;
    
    // Traces storage
    trace_t *traces;
    int trace_count;
    int max_traces;
    int trace_index;
    
    // Current active spans
    trace_span_t *active_spans;
    int active_span_count;
    int max_active_spans;
    
    // Metrics storage
    trace_metrics_t *metrics_history;
    int metrics_count;
    int max_metrics;
    int metrics_index;
    
    // Statistics
    long long total_traces_recorded;
    long long total_spans_recorded;
    long long total_errors;
    long long sampled_traces;
    long long dropped_traces;
    long long export_success;
    long long export_failures;
    
    // Performance metrics
    double average_trace_latency_ms;
    double average_span_latency_ms;
    double sampling_efficiency_percent;
    double export_success_rate;
    
    // Correlation tracking
    trace_context_t *correlation_contexts;
    int correlation_count;
    int max_correlations;
    
    // Anomaly detection
    double *latency_history;
    int latency_history_size;
    int latency_history_index;
    long long anomaly_count;
    long long alert_count;
    
    // State
    int initialized;
    int active;
    long long last_metrics_collection;
    long long last_export;
    int system_ready;
} distributed_tracing_system_t;

// Correlation context
typedef struct {
    char correlation_id[32];       // Correlation identifier
    char request_id[32];           // Request identifier
    char session_id[32];           // Session identifier
    char user_id[32];              // User identifier
    long long timestamp;           // Creation timestamp
    char service_name[64];         // Originating service
    int hop_count;                 // Number of service hops
    trace_context_t trace_context; // Associated trace context
} correlation_context_t;

// Tracing statistics
typedef struct {
    long long total_traces;
    long long active_traces;
    long long completed_traces;
    long long error_traces;
    long long sampled_traces;
    double average_trace_duration_ms;
    double error_rate_percent;
    double sampling_rate_percent;
    long long exports_attempted;
    long long exports_successful;
    long long alerts_generated;
    long long anomalies_detected;
    double system_health_score;
} tracing_stats_t;

// Initialize distributed tracing system
int tracing_system_init(distributed_tracing_system_t *system,
                       const tracing_config_t *config);

// Cleanup distributed tracing system
void tracing_system_cleanup(distributed_tracing_system_t *system);

// Create new trace
trace_context_t* tracing_create_trace(distributed_tracing_system_t *system,
                                    const char *service_name,
                                    trace_type_t trace_type);

// Start a new span
trace_span_t* tracing_start_span(distributed_tracing_system_t *system,
                               const trace_context_t *context,
                               const char *span_name,
                               trace_type_t span_type,
                               const char *component_name);

// End a span
int tracing_end_span(distributed_tracing_system_t *system,
                    trace_span_t *span,
                    span_status_t status);

// Add span attribute
int tracing_add_span_attribute(trace_span_t *span,
                              const char *key,
                              const char *value);

// Add span event
int tracing_add_span_event(trace_span_t *span,
                          const char *event_name,
                          const char *description);

// Record error in span
int tracing_record_span_error(trace_span_t *span,
                             const char *error_message);

// Get current trace context
trace_context_t* tracing_get_current_context(distributed_tracing_system_t *system);

// Get trace by ID
trace_t* tracing_get_trace(distributed_tracing_system_t *system,
                          const char *trace_id);

// Get metrics for time period
trace_metrics_t* tracing_get_metrics(distributed_tracing_system_t *system,
                                   long long start_time,
                                   long long end_time);

// Export traces
int tracing_export_traces(distributed_tracing_system_t *system,
                         const char *format);

// Collect system metrics
int tracing_collect_metrics(distributed_tracing_system_t *system);

// Detect anomalies
int tracing_detect_anomalies(distributed_tracing_system_t *system);

// Generate alerts
int tracing_generate_alerts(distributed_tracing_system_t *system);

// Correlation tracking
int tracing_start_correlation(distributed_tracing_system_t *system,
                             const char *request_id,
                             const char *session_id,
                             const char *user_id,
                             const char *service_name);

// End correlation
int tracing_end_correlation(distributed_tracing_system_t *system,
                           const char *correlation_id);

// Get tracing statistics
void tracing_get_stats(distributed_tracing_system_t *system,
                      tracing_stats_t *stats);

// Get recent traces
int tracing_get_recent_traces(distributed_tracing_system_t *system,
                            trace_t *traces,
                            int max_traces);

// Get trace analytics
int tracing_get_trace_analytics(distributed_tracing_system_t *system,
                              const char *service_name,
                              trace_metrics_t *analytics);

// Reset tracing data
int tracing_reset_data(distributed_tracing_system_t *system);

// Enable/disable tracing
int tracing_enable(distributed_tracing_system_t *system);
int tracing_disable(distributed_tracing_system_t *system);

// Reset statistics
void tracing_reset_stats(distributed_tracing_system_t *system);

// Get global instance
distributed_tracing_system_t* get_global_tracing_system(void);

#endif // _DISTRIBUTED_TRACING_SYSTEM_H_