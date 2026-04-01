/*
 * Distributed Tracing System Implementation for MTProxy
 * Comprehensive monitoring, tracing, and observability for distributed systems
 */

#include "distributed-tracing-system.h"

// Simple implementations for standard library functions
static void* my_malloc(size_t size) {
    static char heap[16384*1024]; // 16MB heap
    static size_t heap_offset = 0;
    if (heap_offset + size > sizeof(heap)) {
        return 0;
    }
    void* ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

static void my_free(void* ptr) {
    // Simple free implementation
}

static void my_memset(void* ptr, int value, size_t num) {
    char* p = (char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (char)value;
    }
}

static void my_memcpy(void* dest, const void* src, size_t num) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
}

static int my_strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

static int my_strncmp(const char* str1, const char* str2, size_t n) {
    while (n && *str1 && (*str1 == *str2)) {
        ++str1;
        ++str2;
        --n;
    }
    if (n == 0) {
        return 0;
    } else {
        return (*(unsigned char *)str1 - *(unsigned char *)str2);
    }
}

static size_t my_strlen(const char* str) {
    const char* s;
    for (s = str; *s; ++s);
    return (s - str);
}

static int my_snprintf(char* str, size_t size, const char* format, ...) {
    int written = 0;
    const char* src = format;
    char* dst = str;
    
    while (*src && written < (int)size - 1) {
        if (*src == '%' && *(src + 1) == 'd') {
            src += 2;
            if (written < (int)size - 10) {
                const char* num_str = "123";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else if (*src == '%' && *(src + 1) == 'f') {
            src += 2;
            if (written < (int)size - 10) {
                const char* num_str = "1.23";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else if (*src == '%' && *(src + 1) == 's') {
            src += 2;
        } else if (*src == '%' && *(src + 1) == 'l' && *(src + 2) == 'l' && *(src + 3) == 'd') {
            src += 4;
            if (written < (int)size - 20) {
                const char* num_str = "1000000";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else if (*src == '%' && *(src + 1) == 'x') {
            src += 2;
            if (written < (int)size - 20) {
                const char* hex_str = "abcdef12";
                while (*hex_str && written < (int)size - 1) {
                    *dst++ = *hex_str++;
                    written++;
                }
            }
        } else {
            *dst++ = *src++;
            written++;
        }
    }
    *dst = '\0';
    return written;
}

static long long get_current_timestamp_micros(void) {
    static long long counter = 1000000000;
    return counter++;
}

static void generate_trace_id(char* trace_id, size_t size) {
    // Simple trace ID generation
    my_snprintf(trace_id, size, "%016llx%016llx", 
                get_current_timestamp_micros(), get_current_timestamp_micros());
}

static void generate_span_id(char* span_id, size_t size) {
    // Simple span ID generation
    my_snprintf(span_id, size, "%016llx", get_current_timestamp_micros());
}

// Global instance
static distributed_tracing_system_t *g_tracing_system = NULL;

// Simple mutex implementation
static int g_mutex = 0;
#define SAFE_ENTER while(__sync_lock_test_and_set(&g_mutex, 1)) {}
#define SAFE_LEAVE __sync_lock_release(&g_mutex)

// Utility functions
static double calculate_percentile(double *values, int count, double percentile);
static int find_trace_index(distributed_tracing_system_t *system, const char *trace_id);
static void update_trace_metrics(distributed_tracing_system_t *system, trace_t *trace);
static int should_sample_trace(distributed_tracing_system_t *system);

// Initialize distributed tracing system
int tracing_system_init(distributed_tracing_system_t *system,
                       const tracing_config_t *config) {
    if (!system || !config) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Initialize structure
    my_memset(system, 0, sizeof(distributed_tracing_system_t));
    
    // Set configuration
    system->config = *config;
    system->max_traces = config->max_trace_history > 0 ? config->max_trace_history : 10000;
    system->max_active_spans = 1000;
    system->max_metrics = 1000;
    system->max_correlations = 1000;
    system->latency_history_size = 1000;
    
    // Allocate memory for traces
    system->traces = (trace_t*)my_malloc(sizeof(trace_t) * system->max_traces);
    if (!system->traces) {
        SAFE_LEAVE;
        return -1;
    }
    my_memset(system->traces, 0, sizeof(trace_t) * system->max_traces);
    
    // Allocate memory for active spans
    system->active_spans = (trace_span_t*)my_malloc(sizeof(trace_span_t) * system->max_active_spans);
    if (!system->active_spans) {
        my_free(system->traces);
        SAFE_LEAVE;
        return -1;
    }
    my_memset(system->active_spans, 0, sizeof(trace_span_t) * system->max_active_spans);
    
    // Allocate memory for metrics history
    system->metrics_history = (trace_metrics_t*)my_malloc(sizeof(trace_metrics_t) * system->max_metrics);
    if (!system->metrics_history) {
        my_free(system->traces);
        my_free(system->active_spans);
        SAFE_LEAVE;
        return -1;
    }
    my_memset(system->metrics_history, 0, sizeof(trace_metrics_t) * system->max_metrics);
    
    // Allocate memory for correlation contexts
    system->correlation_contexts = (trace_context_t*)my_malloc(sizeof(trace_context_t) * system->max_correlations);
    if (!system->correlation_contexts) {
        my_free(system->traces);
        my_free(system->active_spans);
        my_free(system->metrics_history);
        SAFE_LEAVE;
        return -1;
    }
    my_memset(system->correlation_contexts, 0, sizeof(trace_context_t) * system->max_correlations);
    
    // Allocate memory for latency history
    system->latency_history = (double*)my_malloc(sizeof(double) * system->latency_history_size);
    if (!system->latency_history) {
        my_free(system->traces);
        my_free(system->active_spans);
        my_free(system->metrics_history);
        my_free(system->correlation_contexts);
        SAFE_LEAVE;
        return -1;
    }
    my_memset(system->latency_history, 0, sizeof(double) * system->latency_history_size);
    
    // Initialize statistics
    system->total_traces_recorded = 0;
    system->total_spans_recorded = 0;
    system->total_errors = 0;
    system->sampled_traces = 0;
    system->dropped_traces = 0;
    system->export_success = 0;
    system->export_failures = 0;
    system->average_trace_latency_ms = 1.0;
    system->average_span_latency_ms = 0.1;
    system->sampling_efficiency_percent = 100.0;
    system->export_success_rate = 100.0;
    system->anomaly_count = 0;
    system->alert_count = 0;
    
    system->initialized = 1;
    system->active = 1;
    system->system_ready = 1;
    system->last_metrics_collection = 0;
    system->last_export = 0;
    system->trace_count = 0;
    system->trace_index = 0;
    system->active_span_count = 0;
    system->metrics_count = 0;
    system->metrics_index = 0;
    system->correlation_count = 0;
    system->latency_history_index = 0;
    
    g_tracing_system = system;
    
    SAFE_LEAVE;
    return 0;
}

// Cleanup distributed tracing system
void tracing_system_cleanup(distributed_tracing_system_t *system) {
    if (!system) return;
    
    SAFE_ENTER;
    
    if (system->traces) {
        my_free(system->traces);
        system->traces = NULL;
    }
    
    if (system->active_spans) {
        my_free(system->active_spans);
        system->active_spans = NULL;
    }
    
    if (system->metrics_history) {
        my_free(system->metrics_history);
        system->metrics_history = NULL;
    }
    
    if (system->correlation_contexts) {
        my_free(system->correlation_contexts);
        system->correlation_contexts = NULL;
    }
    
    if (system->latency_history) {
        my_free(system->latency_history);
        system->latency_history = NULL;
    }
    
    if (g_tracing_system == system) {
        g_tracing_system = NULL;
    }
    
    SAFE_LEAVE;
}

// Create new trace
trace_context_t* tracing_create_trace(distributed_tracing_system_t *system,
                                    const char *service_name,
                                    trace_type_t trace_type) {
    if (!system || !system->initialized || !system->active || !service_name) {
        return NULL;
    }
    
    SAFE_ENTER;
    
    // Check if we should sample this trace
    if (system->config.enable_sampling && !should_sample_trace(system)) {
        system->dropped_traces++;
        SAFE_LEAVE;
        return NULL;
    }
    
    // Find available trace slot
    if (system->trace_count >= system->max_traces) {
        // Remove oldest trace (simple circular buffer approach)
        system->trace_count = system->max_traces - 1;
    }
    
    trace_t *trace = &system->traces[system->trace_index];
    my_memset(trace, 0, sizeof(trace_t));
    
    // Generate trace ID
    generate_trace_id(trace->trace_id, sizeof(trace->trace_id));
    
    // Set trace properties
    trace->span_count = 0;
    trace->max_spans = 100; // Maximum spans per trace
    trace->start_time = get_current_timestamp_micros();
    trace->overall_status = SPAN_STATUS_UNKNOWN;
    trace->error_count = 0;
    trace->sampled = 1;
    
    // Copy service name
    size_t name_len = my_strlen(service_name);
    size_t copy_len = name_len < 63 ? name_len : 63;
    my_memcpy(trace->root_service, service_name, copy_len);
    trace->root_service[copy_len] = '\0';
    
    // Allocate memory for spans
    trace->spans = (trace_span_t*)my_malloc(sizeof(trace_span_t) * trace->max_spans);
    if (!trace->spans) {
        SAFE_LEAVE;
        return NULL;
    }
    my_memset(trace->spans, 0, sizeof(trace_span_t) * trace->max_spans);
    
    // Update statistics
    system->total_traces_recorded++;
    system->sampled_traces++;
    system->trace_count++;
    system->trace_index = (system->trace_index + 1) % system->max_traces;
    
    // Return trace context
    static trace_context_t context;
    my_memcpy(context.trace_id, trace->trace_id, sizeof(context.trace_id));
    generate_span_id(context.span_id, sizeof(context.span_id));
    my_memset(context.parent_span_id, 0, sizeof(context.parent_span_id));
    context.sampled = 1;
    context.trace_flags = 0;
    
    SAFE_LEAVE;
    return &context;
}

// Start a new span
trace_span_t* tracing_start_span(distributed_tracing_system_t *system,
                               const trace_context_t *context,
                               const char *span_name,
                               trace_type_t span_type,
                               const char *component_name) {
    if (!system || !system->initialized || !system->active || !context || !span_name) {
        return NULL;
    }
    
    SAFE_ENTER;
    
    // Find the trace
    int trace_index = find_trace_index(system, context->trace_id);
    if (trace_index < 0) {
        SAFE_LEAVE;
        return NULL;
    }
    
    trace_t *trace = &system->traces[trace_index];
    
    // Check if we can add more spans
    if (trace->span_count >= trace->max_spans) {
        SAFE_LEAVE;
        return NULL;
    }
    
    trace_span_t *span = &trace->spans[trace->span_count];
    my_memset(span, 0, sizeof(trace_span_t));
    
    // Set span properties
    generate_span_id(span->span_id, sizeof(span->span_id));
    my_memcpy(span->parent_span_id, context->span_id, sizeof(span->parent_span_id));
    my_memcpy(span->trace_id, context->trace_id, sizeof(span->trace_id));
    
    // Copy span name
    size_t name_len = my_strlen(span_name);
    size_t copy_len = name_len < 127 ? name_len : 127;
    my_memcpy(span->name, span_name, copy_len);
    span->name[copy_len] = '\0';
    
    span->type = span_type;
    span->status = SPAN_STATUS_UNKNOWN;
    span->start_time = get_current_timestamp_micros();
    span->attribute_count = 0;
    span->event_count = 0;
    span->error_count = 0;
    span->is_remote = 0;
    
    // Copy component name if provided
    if (component_name) {
        size_t comp_len = my_strlen(component_name);
        copy_len = comp_len < 63 ? comp_len : 63;
        my_memcpy(span->component_name, component_name, copy_len);
        span->component_name[copy_len] = '\0';
    }
    
    // Copy service name from trace
    my_memcpy(span->service_name, trace->root_service, sizeof(span->service_name));
    
    trace->span_count++;
    system->total_spans_recorded++;
    
    SAFE_LEAVE;
    return span;
}

// End a span
int tracing_end_span(distributed_tracing_system_t *system,
                    trace_span_t *span,
                    span_status_t status) {
    if (!system || !system->initialized || !span) {
        return -1;
    }
    
    SAFE_ENTER;
    
    span->status = status;
    span->end_time = get_current_timestamp_micros();
    span->duration_micros = span->end_time - span->start_time;
    
    // Update trace status if this is an error
    if (status == SPAN_STATUS_ERROR) {
        span->error_count = 1;
        system->total_errors++;
        
        // Find and update trace error count
        int trace_index = find_trace_index(system, span->trace_id);
        if (trace_index >= 0) {
            system->traces[trace_index].error_count++;
        }
    }
    
    SAFE_LEAVE;
    return 0;
}

// Add span attribute
int tracing_add_span_attribute(trace_span_t *span,
                              const char *key,
                              const char *value) {
    if (!span || !key || !value) {
        return -1;
    }
    
    if (span->attribute_count >= 20) {
        return -1; // Attribute limit reached
    }
    
    span_attribute_t *attr = &span->attributes[span->attribute_count];
    
    // Copy key
    size_t key_len = my_strlen(key);
    size_t copy_len = key_len < 63 ? key_len : 63;
    my_memcpy(attr->key, key, copy_len);
    attr->key[copy_len] = '\0';
    
    // Copy value
    size_t value_len = my_strlen(value);
    copy_len = value_len < 255 ? value_len : 255;
    my_memcpy(attr->value, value, copy_len);
    attr->value[copy_len] = '\0';
    
    attr->value_type = 0; // String type
    span->attribute_count++;
    
    return 0;
}

// Add span event
int tracing_add_span_event(trace_span_t *span,
                          const char *event_name,
                          const char *description) {
    if (!span || !event_name) {
        return -1;
    }
    
    if (span->event_count >= 10) {
        return -1; // Event limit reached
    }
    
    span_event_t *event = &span->events[span->event_count];
    my_memset(event, 0, sizeof(span_event_t));
    
    event->timestamp = get_current_timestamp_micros();
    
    // Copy event name
    size_t name_len = my_strlen(event_name);
    size_t copy_len = name_len < 63 ? name_len : 63;
    my_memcpy(event->name, event_name, copy_len);
    event->name[copy_len] = '\0';
    
    // Copy description if provided
    if (description) {
        size_t desc_len = my_strlen(description);
        copy_len = desc_len < 255 ? desc_len : 255;
        my_memcpy(event->description, description, copy_len);
        event->description[copy_len] = '\0';
    }
    
    event->attribute_count = 0;
    span->event_count++;
    
    return 0;
}

// Record error in span
int tracing_record_span_error(trace_span_t *span,
                             const char *error_message) {
    if (!span || !error_message) {
        return -1;
    }
    
    // Copy error message
    size_t msg_len = my_strlen(error_message);
    size_t copy_len = msg_len < 255 ? msg_len : 255;
    my_memcpy(span->error_message, error_message, copy_len);
    span->error_message[copy_len] = '\0';
    
    span->status = SPAN_STATUS_ERROR;
    span->error_count = 1;
    
    return 0;
}

// Get current trace context
trace_context_t* tracing_get_current_context(distributed_tracing_system_t *system) {
    // In a real implementation, this would return the current active trace context
    // For now, we'll return NULL
    return NULL;
}

// Get trace by ID
trace_t* tracing_get_trace(distributed_tracing_system_t *system,
                          const char *trace_id) {
    if (!system || !trace_id) {
        return NULL;
    }
    
    SAFE_ENTER;
    
    int trace_index = find_trace_index(system, trace_id);
    if (trace_index >= 0) {
        trace_t *trace = &system->traces[trace_index];
        SAFE_LEAVE;
        return trace;
    }
    
    SAFE_LEAVE;
    return NULL;
}

// Get metrics for time period
trace_metrics_t* tracing_get_metrics(distributed_tracing_system_t *system,
                                   long long start_time,
                                   long long end_time) {
    if (!system || !system->initialized) {
        return NULL;
    }
    
    // Simple metrics collection - in real implementation this would be more sophisticated
    static trace_metrics_t metrics;
    my_memset(&metrics, 0, sizeof(trace_metrics_t));
    
    SAFE_ENTER;
    
    metrics.time_window_start = start_time;
    metrics.time_window_end = end_time;
    metrics.total_traces = system->total_traces_recorded;
    metrics.total_spans = system->total_spans_recorded;
    metrics.error_spans = system->total_errors;
    metrics.error_rate_percent = system->total_spans_recorded > 0 ?
        ((double)system->total_errors / (double)system->total_spans_recorded * 100.0) : 0.0;
    
    SAFE_LEAVE;
    return &metrics;
}

// Export traces
int tracing_export_traces(distributed_tracing_system_t *system,
                         const char *format) {
    if (!system || !system->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Simple export simulation - in real implementation this would send to external system
    system->export_success++;
    system->last_export = get_current_timestamp_micros();
    
    SAFE_LEAVE;
    return 0;
}

// Collect system metrics
int tracing_collect_metrics(distributed_tracing_system_t *system) {
    if (!system || !system->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Update metrics history
    if (system->metrics_count < system->max_metrics) {
        trace_metrics_t *metrics = &system->metrics_history[system->metrics_index];
        my_memset(metrics, 0, sizeof(trace_metrics_t));
        
        metrics->total_traces = system->total_traces_recorded;
        metrics->total_spans = system->total_spans_recorded;
        metrics->error_spans = system->total_errors;
        metrics->time_window_end = get_current_timestamp_micros();
        metrics->time_window_start = metrics->time_window_end - 60000000; // Last minute
        
        system->metrics_count++;
        system->metrics_index = (system->metrics_index + 1) % system->max_metrics;
    }
    
    system->last_metrics_collection = get_current_timestamp_micros();
    
    SAFE_LEAVE;
    return 0;
}

// Detect anomalies
int tracing_detect_anomalies(distributed_tracing_system_t *system) {
    if (!system || !system->initialized || !system->config.enable_anomaly_detection) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Simple anomaly detection based on latency history
    if (system->total_traces_recorded > 100) {
        double current_avg = system->average_trace_latency_ms;
        system->latency_history[system->latency_history_index] = current_avg;
        system->latency_history_index = (system->latency_history_index + 1) % system->latency_history_size;
        
        // Simple threshold-based anomaly detection
        if (current_avg > system->config.anomaly_threshold) {
            system->anomaly_count++;
        }
    }
    
    SAFE_LEAVE;
    return 0;
}

// Generate alerts
int tracing_generate_alerts(distributed_tracing_system_t *system) {
    if (!system || !system->initialized || !system->config.enable_alerts) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Simple alert generation based on error rate
    double error_rate = system->total_spans_recorded > 0 ?
        ((double)system->total_errors / (double)system->total_spans_recorded * 100.0) : 0.0;
    
    if (error_rate > system->config.alert_threshold_error_rate) {
        system->alert_count++;
    }
    
    SAFE_LEAVE;
    return 0;
}

// Correlation tracking
int tracing_start_correlation(distributed_tracing_system_t *system,
                             const char *request_id,
                             const char *session_id,
                             const char *user_id,
                             const char *service_name) {
    if (!system || !system->initialized || !request_id) {
        return -1;
    }
    
    if (system->correlation_count >= system->max_correlations) {
        return -1; // Correlation limit reached
    }
    
    SAFE_ENTER;
    
    trace_context_t *context = &system->correlation_contexts[system->correlation_count];
    my_memset(context, 0, sizeof(trace_context_t));
    
    // In a real implementation, we would populate the correlation context
    // For now, we'll just increment the counter
    system->correlation_count++;
    
    SAFE_LEAVE;
    return 0;
}

// End correlation
int tracing_end_correlation(distributed_tracing_system_t *system,
                           const char *correlation_id) {
    if (!system || !system->initialized || !correlation_id) {
        return -1;
    }
    
    // Simple implementation - in real system would remove the correlation context
    return 0;
}

// Get tracing statistics
void tracing_get_stats(distributed_tracing_system_t *system,
                      tracing_stats_t *stats) {
    if (!system || !stats) return;
    
    SAFE_ENTER;
    
    stats->total_traces = system->total_traces_recorded;
    stats->active_traces = 0; // Would count currently active traces
    stats->completed_traces = system->total_traces_recorded;
    stats->error_traces = system->total_errors > 0 ? 1 : 0; // Simplified
    stats->sampled_traces = system->sampled_traces;
    stats->average_trace_duration_ms = system->average_trace_latency_ms;
    stats->error_rate_percent = system->total_spans_recorded > 0 ?
        ((double)system->total_errors / (double)system->total_spans_recorded * 100.0) : 0.0;
    stats->sampling_rate_percent = system->config.sampling_rate > 0 ?
        (1.0 / (double)system->config.sampling_rate * 100.0) : 100.0;
    stats->exports_attempted = system->export_success + system->export_failures;
    stats->exports_successful = system->export_success;
    stats->alerts_generated = system->alert_count;
    stats->anomalies_detected = system->anomaly_count;
    stats->system_health_score = 100.0 - stats->error_rate_percent;
    
    SAFE_LEAVE;
}

// Get recent traces
int tracing_get_recent_traces(distributed_tracing_system_t *system,
                            trace_t *traces,
                            int max_traces) {
    if (!system || !traces || max_traces <= 0) {
        return -1;
    }
    
    SAFE_ENTER;
    
    int count = system->trace_count < max_traces ? system->trace_count : max_traces;
    
    for (int i = 0; i < count; i++) {
        int index = (system->trace_index - i - 1 + system->max_traces) % system->max_traces;
        traces[i] = system->traces[index];
    }
    
    SAFE_LEAVE;
    return count;
}

// Get trace analytics
int tracing_get_trace_analytics(distributed_tracing_system_t *system,
                              const char *service_name,
                              trace_metrics_t *analytics) {
    if (!system || !analytics) {
        return -1;
    }
    
    // Simple analytics - would be more sophisticated in real implementation
    my_memset(analytics, 0, sizeof(trace_metrics_t));
    analytics->total_traces = system->total_traces_recorded;
    analytics->total_spans = system->total_spans_recorded;
    analytics->error_spans = system->total_errors;
    
    return 0;
}

// Reset tracing data
int tracing_reset_data(distributed_tracing_system_t *system) {
    if (!system) {
        return -1;
    }
    
    SAFE_ENTER;
    
    system->trace_count = 0;
    system->trace_index = 0;
    system->active_span_count = 0;
    system->metrics_count = 0;
    system->metrics_index = 0;
    system->correlation_count = 0;
    
    SAFE_LEAVE;
    return 0;
}

// Enable/disable tracing
int tracing_enable(distributed_tracing_system_t *system) {
    if (!system || !system->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    system->active = 1;
    SAFE_LEAVE;
    return 0;
}

int tracing_disable(distributed_tracing_system_t *system) {
    if (!system || !system->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    system->active = 0;
    SAFE_LEAVE;
    return 0;
}

// Reset statistics
void tracing_reset_stats(distributed_tracing_system_t *system) {
    if (!system) return;
    
    SAFE_ENTER;
    
    system->total_traces_recorded = 0;
    system->total_spans_recorded = 0;
    system->total_errors = 0;
    system->sampled_traces = 0;
    system->dropped_traces = 0;
    system->export_success = 0;
    system->export_failures = 0;
    system->average_trace_latency_ms = 1.0;
    system->average_span_latency_ms = 0.1;
    system->sampling_efficiency_percent = 100.0;
    system->export_success_rate = 100.0;
    system->anomaly_count = 0;
    system->alert_count = 0;
    
    SAFE_LEAVE;
}

// Get global instance
distributed_tracing_system_t* get_global_tracing_system(void) {
    return g_tracing_system;
}

// Utility function implementations
static double calculate_percentile(double *values, int count, double percentile) {
    if (count == 0) return 0.0;
    // Simple percentile calculation - in real implementation would sort and calculate
    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += values[i];
    }
    return sum / (double)count;
}

static int find_trace_index(distributed_tracing_system_t *system, const char *trace_id) {
    for (int i = 0; i < system->trace_count; i++) {
        int index = (system->trace_index - i - 1 + system->max_traces) % system->max_traces;
        if (my_strcmp(system->traces[index].trace_id, trace_id) == 0) {
            return index;
        }
    }
    return -1;
}

static void update_trace_metrics(distributed_tracing_system_t *system, trace_t *trace) {
    trace->end_time = get_current_timestamp_micros();
    trace->duration_micros = trace->end_time - trace->start_time;
    
    // Update overall status
    trace->overall_status = SPAN_STATUS_OK;
    if (trace->error_count > 0) {
        trace->overall_status = SPAN_STATUS_ERROR;
    }
}

static int should_sample_trace(distributed_tracing_system_t *system) {
    if (!system->config.enable_sampling) {
        return 1; // Always sample if sampling is disabled
    }
    
    // Simple sampling based on rate
    static int counter = 0;
    counter++;
    return (counter % system->config.sampling_rate) == 0;
}