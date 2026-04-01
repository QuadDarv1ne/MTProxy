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

#ifndef __DISTRIBUTED_TRACING_H__
#define __DISTRIBUTED_TRACING_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
#define MAX_TAGS_PER_SPAN 32
#define MAX_LOGS_PER_SPAN 16
#define MAX_OPERATION_NAME_LEN 128
#define MAX_COMPONENT_NAME_LEN 64
#define MAX_TAG_KEY_LEN 64
#define MAX_TAG_VALUE_LEN 256
#define MAX_LOG_MESSAGE_LEN 512

/* Tracing backend types */
typedef enum {
    TRACING_BACKEND_NONE = 0,
    TRACING_BACKEND_JAEGER,
    TRACING_BACKEND_ZIPKIN,
    TRACING_BACKEND_OTLP  // OpenTelemetry Protocol
} tracing_backend_type_t;

/* Trace ID structure */
typedef struct {
    uint64_t high;
    uint64_t low;
} trace_id_t;

/* Span tag structure */
typedef struct {
    char key[MAX_TAG_KEY_LEN];
    char value[MAX_TAG_VALUE_LEN];
} span_tag_t;

/* Span log structure */
typedef struct {
    char message[MAX_LOG_MESSAGE_LEN];
    uint64_t timestamp_us;
} span_log_t;

/* Forward declarations */
struct trace_span;
struct trace_context;

/* Trace span structure */
typedef struct trace_span {
    trace_id_t trace_id;
    uint64_t span_id;
    uint64_t parent_span_id;
    
    char operation_name[MAX_OPERATION_NAME_LEN];
    char component[MAX_COMPONENT_NAME_LEN];
    
    uint64_t start_time_us;
    uint64_t duration_us;
    
    span_tag_t tags[MAX_TAGS_PER_SPAN];
    int tag_count;
    
    span_log_t logs[MAX_LOGS_PER_SPAN];
    int log_count;
    
    struct trace_span *parent;
    struct trace_span *next;
} trace_span_t;

/* Trace context structure */
typedef struct trace_context {
    trace_id_t trace_id;
    uint64_t span_id;
    uint64_t parent_span_id;
    
    char operation_name[MAX_OPERATION_NAME_LEN];
    char component[MAX_COMPONENT_NAME_LEN];
    
    uint64_t start_time_us;
    uint64_t duration_us;
    
    trace_span_t *spans;
    int span_count;
    
    struct trace_context *next;
} trace_context_t;

/* Tracing configuration */
typedef struct {
    int enabled;
    double sampling_rate;          /* 0.0 - 1.0 */
    int max_spans_per_trace;
    int trace_timeout_ms;
    tracing_backend_type_t backend_type;
    char backend_endpoint[256];    /* Backend URL/endpoint */
} tracing_config_t;

/* Trace registry */
typedef struct {
    trace_context_t *active_traces;
    trace_context_t *completed_traces;
    int trace_count;
    int dropped_traces;
    long start_time;
} trace_registry_t;

/* Tracing statistics */
typedef struct {
    int total_traces;
    int active_traces;
    int completed_traces;
    int dropped_traces;
    double sampling_rate;
} tracing_stats_t;

/* Function prototypes */

/**
 * Initialize distributed tracing system
 * @param config: Tracing configuration (optional, uses defaults if NULL)
 * @return: 0 on success, -1 on error
 */
int init_distributed_tracing(tracing_config_t *config);

/**
 * Generate new trace ID
 * @param trace_id: Pointer to trace ID structure to fill
 */
void generate_trace_id(trace_id_t *trace_id);

/**
 * Generate new span ID
 * @return: New span ID
 */
uint64_t generate_span_id();

/**
 * Start new trace
 * @param operation_name: Name of the operation being traced
 * @param component: Component name (optional)
 * @return: Pointer to trace context or NULL if not sampled
 */
trace_context_t* start_trace(const char *operation_name, const char *component);

/**
 * Create child span
 * @param parent_context: Parent trace context
 * @param operation_name: Name of the operation being traced
 * @return: Pointer to new span or NULL on error
 */
trace_span_t* create_span(trace_context_t *parent_context, const char *operation_name);

/**
 * Add tag to span
 * @param span: Span to add tag to
 * @param key: Tag key
 * @param value: Tag value
 * @return: 0 on success, -1 on error
 */
int add_span_tag(trace_span_t *span, const char *key, const char *value);

/**
 * Add log to span
 * @param span: Span to add log to
 * @param message: Log message
 * @return: 0 on success, -1 on error
 */
int add_span_log(trace_span_t *span, const char *message);

/**
 * Finish span
 * @param span: Span to finish
 * @return: 0 on success, -1 on error
 */
int finish_span(trace_span_t *span);

/**
 * Finish trace
 * @param context: Trace context to finish
 * @return: 0 on success, -1 on error
 */
int finish_trace(trace_context_t *context);

/**
 * Get current active span
 * @return: Pointer to current span or NULL
 */
trace_span_t* get_current_span();

/**
 * Inject trace context into headers
 * @param context: Trace context to inject
 * @param headers: Buffer to write headers to
 * @param headers_size: Size of headers buffer
 * @return: 0 on success, -1 on error
 */
int inject_trace_context(trace_context_t *context, char *headers, size_t headers_size);

/**
 * Extract trace context from headers
 * @param headers: Headers containing trace context
 * @return: Pointer to extracted context or NULL on error
 */
trace_context_t* extract_trace_context(const char *headers);

/**
 * Export traces to configured backend
 * @return: 0 on success, -1 on error
 */
int export_traces_to_backend();

/**
 * Get tracing statistics
 * @return: Tracing statistics structure
 */
tracing_stats_t get_tracing_stats();

/**
 * Cleanup tracing system
 */
void cleanup_distributed_tracing();

/* Internal functions (not part of public API) */
void add_trace_to_registry(trace_context_t *context);
void move_trace_to_completed(trace_context_t *context);
void export_single_trace(trace_context_t *trace);
void clear_completed_traces();
int count_active_traces();
int count_completed_traces();
void drop_oldest_traces();
void free_trace_context(trace_context_t *context);

#ifdef __cplusplus
}
#endif

#endif /* __DISTRIBUTED_TRACING_H__ */