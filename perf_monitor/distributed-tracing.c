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

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "distributed-tracing.h"

// Global tracing configuration
static tracing_config_t global_tracing_config = {
    .enabled = 0,
    .sampling_rate = 1.0,  // 100% sampling by default
    .max_spans_per_trace = 1000,
    .trace_timeout_ms = 30000,  // 30 seconds
    .backend_type = TRACING_BACKEND_NONE
};

// Thread-local storage for active span
static __thread trace_span_t *current_span = NULL;

// Global trace registry
static trace_registry_t *global_trace_registry = NULL;
static pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;
static int tracing_initialized = 0;

// Initialize distributed tracing system
int init_distributed_tracing(tracing_config_t *config) {
    if (tracing_initialized) {
        return 0; // Already initialized
    }

    pthread_mutex_lock(&registry_mutex);
    
    // Apply configuration
    if (config) {
        global_tracing_config = *config;
    }
    
    // Initialize registry
    global_trace_registry = calloc(1, sizeof(trace_registry_t));
    if (!global_trace_registry) {
        pthread_mutex_unlock(&registry_mutex);
        return -1;
    }
    
    global_trace_registry->trace_count = 0;
    global_trace_registry->active_traces = NULL;
    global_trace_registry->completed_traces = NULL;
    global_trace_registry->start_time = (long)time(NULL);
    
    tracing_initialized = 1;
    pthread_mutex_unlock(&registry_mutex);
    
    return 0;
}

// Generate new trace ID
void generate_trace_id(trace_id_t *trace_id) {
    if (!trace_id) return;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    // Create trace ID from timestamp and random components
    trace_id->high = ((uint64_t)tv.tv_sec << 32) | (uint64_t)tv.tv_usec;
    trace_id->low = (uint64_t)rand() << 32 | (uint64_t)rand();
}

// Generate new span ID
uint64_t generate_span_id() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec << 32) | (uint64_t)tv.tv_usec ^ (uint64_t)rand();
}

// Start new trace
trace_context_t* start_trace(const char *operation_name, const char *component) {
    if (!tracing_initialized || !global_tracing_config.enabled) {
        return NULL;
    }
    
    // Apply sampling
    if (global_tracing_config.sampling_rate < 1.0) {
        double random_value = (double)rand() / RAND_MAX;
        if (random_value > global_tracing_config.sampling_rate) {
            return NULL; // Not sampled
        }
    }
    
    trace_context_t *context = calloc(1, sizeof(trace_context_t));
    if (!context) {
        return NULL;
    }
    
    // Generate trace and span IDs
    generate_trace_id(&context->trace_id);
    context->span_id = generate_span_id();
    context->parent_span_id = 0;  // Root span
    
    // Set metadata
    strncpy(context->operation_name, operation_name, sizeof(context->operation_name) - 1);
    if (component) {
        strncpy(context->component, component, sizeof(context->component) - 1);
    }
    
    // Set timestamps
    struct timeval tv;
    gettimeofday(&tv, NULL);
    context->start_time_us = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    context->duration_us = 0;
    
    // Add to registry
    pthread_mutex_lock(&registry_mutex);
    add_trace_to_registry(context);
    pthread_mutex_unlock(&registry_mutex);
    
    return context;
}

// Create child span
trace_span_t* create_span(trace_context_t *parent_context, const char *operation_name) {
    if (!tracing_initialized || !global_tracing_config.enabled || !parent_context) {
        return NULL;
    }
    
    trace_span_t *span = calloc(1, sizeof(trace_span_t));
    if (!span) {
        return NULL;
    }
    
    // Copy trace context
    span->trace_id = parent_context->trace_id;
    span->span_id = generate_span_id();
    span->parent_span_id = parent_context->span_id;
    
    // Set operation name
    strncpy(span->operation_name, operation_name, sizeof(span->operation_name) - 1);
    
    // Set timestamps
    struct timeval tv;
    gettimeofday(&tv, NULL);
    span->start_time_us = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    span->duration_us = 0;
    
    // Link to parent
    span->parent = current_span;
    current_span = span;
    
    return span;
}

// Add tag to span
int add_span_tag(trace_span_t *span, const char *key, const char *value) {
    if (!span || !key || !value) {
        return -1;
    }
    
    if (span->tag_count >= MAX_TAGS_PER_SPAN) {
        return -1; // Tag limit reached
    }
    
    span_tag_t *tag = &span->tags[span->tag_count];
    strncpy(tag->key, key, sizeof(tag->key) - 1);
    strncpy(tag->value, value, sizeof(tag->value) - 1);
    span->tag_count++;
    
    return 0;
}

// Add log to span
int add_span_log(trace_span_t *span, const char *message) {
    if (!span || !message) {
        return -1;
    }
    
    if (span->log_count >= MAX_LOGS_PER_SPAN) {
        return -1; // Log limit reached
    }
    
    span_log_t *log = &span->logs[span->log_count];
    strncpy(log->message, message, sizeof(log->message) - 1);
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    log->timestamp_us = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    
    span->log_count++;
    
    return 0;
}

// Finish span
int finish_span(trace_span_t *span) {
    if (!span) {
        return -1;
    }
    
    // Set end time and duration
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t end_time_us = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    span->duration_us = end_time_us - span->start_time_us;
    
    // Remove from thread-local storage
    if (current_span == span) {
        current_span = span->parent;
    }
    
    return 0;
}

// Finish trace
int finish_trace(trace_context_t *context) {
    if (!context) {
        return -1;
    }
    
    // Set end time and duration
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t end_time_us = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    context->duration_us = end_time_us - context->start_time_us;
    
    // Move to completed traces
    pthread_mutex_lock(&registry_mutex);
    move_trace_to_completed(context);
    pthread_mutex_unlock(&registry_mutex);
    
    return 0;
}

// Get current active span
trace_span_t* get_current_span() {
    return current_span;
}

// Inject trace context into headers
int inject_trace_context(trace_context_t *context, char *headers, size_t headers_size) {
    if (!context || !headers || headers_size == 0) {
        return -1;
    }
    
    // Format: trace-id:span-id:parent-span-id
    snprintf(headers, headers_size, 
             "trace-id: %016llx%016llx\r\n"
             "span-id: %016llx\r\n"
             "parent-span-id: %016llx\r\n",
             (unsigned long long)context->trace_id.high,
             (unsigned long long)context->trace_id.low,
             (unsigned long long)context->span_id,
             (unsigned long long)context->parent_span_id);
    
    return 0;
}

// Extract trace context from headers
trace_context_t* extract_trace_context(const char *headers) {
    if (!headers) {
        return NULL;
    }
    
    trace_context_t *context = calloc(1, sizeof(trace_context_t));
    if (!context) {
        return NULL;
    }
    
    // Parse trace context from headers
    // This is a simplified implementation - in production, use proper HTTP header parsing
    unsigned long long high, low, span_id, parent_span_id;
    
    int parsed = sscanf(headers, 
                       "trace-id: %016llx%016llx\r\n"
                       "span-id: %016llx\r\n"
                       "parent-span-id: %016llx\r\n",
                       &high, &low, &span_id, &parent_span_id);
    
    if (parsed == 4) {
        context->trace_id.high = (uint64_t)high;
        context->trace_id.low = (uint64_t)low;
        context->span_id = (uint64_t)span_id;
        context->parent_span_id = (uint64_t)parent_span_id;
        return context;
    }
    
    free(context);
    return NULL;
}

// Export traces to backend
int export_traces_to_backend() {
    if (!tracing_initialized || global_tracing_config.backend_type == TRACING_BACKEND_NONE) {
        return 0;
    }
    
    pthread_mutex_lock(&registry_mutex);
    
    // Export completed traces
    trace_context_t *trace = global_trace_registry->completed_traces;
    while (trace) {
        export_single_trace(trace);
        trace = trace->next;
    }
    
    // Clear completed traces
    clear_completed_traces();
    
    pthread_mutex_unlock(&registry_mutex);
    
    return 0;
}

// Get tracing statistics
tracing_stats_t get_tracing_stats() {
    tracing_stats_t stats = {0};
    
    if (!tracing_initialized) {
        return stats;
    }
    
    pthread_mutex_lock(&registry_mutex);
    
    stats.total_traces = global_trace_registry->trace_count;
    stats.active_traces = count_active_traces();
    stats.completed_traces = count_completed_traces();
    stats.dropped_traces = global_trace_registry->dropped_traces;
    stats.sampling_rate = global_tracing_config.sampling_rate;
    
    pthread_mutex_unlock(&registry_mutex);
    
    return stats;
}

// Internal functions

void add_trace_to_registry(trace_context_t *context) {
    if (!context) return;
    
    context->next = global_trace_registry->active_traces;
    global_trace_registry->active_traces = context;
    global_trace_registry->trace_count++;
    
    // Check limits
    if (global_trace_registry->trace_count > global_tracing_config.max_spans_per_trace) {
        // Drop oldest traces
        drop_oldest_traces();
    }
}

void move_trace_to_completed(trace_context_t *context) {
    if (!context) return;
    
    // Remove from active traces
    trace_context_t **prev = &global_trace_registry->active_traces;
    while (*prev) {
        if (*prev == context) {
            *prev = context->next;
            break;
        }
        prev = &(*prev)->next;
    }
    
    // Add to completed traces
    context->next = global_trace_registry->completed_traces;
    global_trace_registry->completed_traces = context;
}

void export_single_trace(trace_context_t *trace) {
    // In a real implementation, this would send traces to the configured backend
    // (Jaeger, Zipkin, OpenTelemetry collector, etc.)
    
    switch (global_tracing_config.backend_type) {
        case TRACING_BACKEND_JAEGER:
            // Export to Jaeger
            break;
        case TRACING_BACKEND_ZIPKIN:
            // Export to Zipkin
            break;
        case TRACING_BACKEND_OTLP:
            // Export to OpenTelemetry
            break;
        default:
            break;
    }
}

void clear_completed_traces() {
    trace_context_t *trace = global_trace_registry->completed_traces;
    while (trace) {
        trace_context_t *next = trace->next;
        free_trace_context(trace);
        trace = next;
    }
    global_trace_registry->completed_traces = NULL;
}

int count_active_traces() {
    int count = 0;
    trace_context_t *trace = global_trace_registry->active_traces;
    while (trace) {
        count++;
        trace = trace->next;
    }
    return count;
}

int count_completed_traces() {
    int count = 0;
    trace_context_t *trace = global_trace_registry->completed_traces;
    while (trace) {
        count++;
        trace = trace->next;
    }
    return count;
}

void drop_oldest_traces() {
    // Drop oldest active traces when limit is exceeded
    trace_context_t *trace = global_trace_registry->active_traces;
    trace_context_t *prev = NULL;
    
    // Find the oldest traces (simplified - in production use proper age tracking)
    int to_drop = global_trace_registry->trace_count - global_tracing_config.max_spans_per_trace;
    for (int i = 0; i < to_drop && trace; i++) {
        trace_context_t *next = trace->next;
        if (prev) {
            prev->next = next;
        } else {
            global_trace_registry->active_traces = next;
        }
        free_trace_context(trace);
        trace = next;
        global_trace_registry->dropped_traces++;
        global_trace_registry->trace_count--;
    }
}

void free_trace_context(trace_context_t *context) {
    if (!context) return;
    
    // Free associated spans
    trace_span_t *span = context->spans;
    while (span) {
        trace_span_t *next = span->next;
        free(span);
        span = next;
    }
    
    free(context);
}

// Cleanup tracing system
void cleanup_distributed_tracing() {
    if (!tracing_initialized) {
        return;
    }
    
    pthread_mutex_lock(&registry_mutex);
    
    // Free all traces
    trace_context_t *trace = global_trace_registry->active_traces;
    while (trace) {
        trace_context_t *next = trace->next;
        free_trace_context(trace);
        trace = next;
    }
    
    trace = global_trace_registry->completed_traces;
    while (trace) {
        trace_context_t *next = trace->next;
        free_trace_context(trace);
        trace = next;
    }
    
    free(global_trace_registry);
    global_trace_registry = NULL;
    tracing_initialized = 0;
    
    pthread_mutex_unlock(&registry_mutex);
}