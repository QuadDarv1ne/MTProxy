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

#include "advanced-metrics.h"

// Global metrics registry
static metrics_registry_t *global_registry = NULL;
static pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;
static int metrics_initialized = 0;

// Histogram buckets for common metrics
static double default_histogram_buckets[] = {
    0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0
};

// Initialize metrics registry
int init_metrics_registry(const char *namespace_prefix) {
    if (metrics_initialized) {
        return 0; // Already initialized
    }

    pthread_mutex_lock(&registry_mutex);
    
    global_registry = calloc(1, sizeof(metrics_registry_t));
    if (!global_registry) {
        pthread_mutex_unlock(&registry_mutex);
        return -1;
    }

    global_registry->ns_prefix = namespace_prefix ? strdup(namespace_prefix) : strdup("mtproxy");
    global_registry->metric_count = 0;
    global_registry->metrics = NULL;
    global_registry->start_time = (long)time(NULL);
    
    // Register default metrics
    register_default_metrics();
    
    metrics_initialized = 1;
    pthread_mutex_unlock(&registry_mutex);
    
    return 0;
}

// Register a counter metric
metric_entry_t* register_counter(const char *name, const char *help, const char *labels) {
    return register_metric(name, help, METRIC_COUNTER, labels);
}

// Register a gauge metric
metric_entry_t* register_gauge(const char *name, const char *help, const char *labels) {
    return register_metric(name, help, METRIC_GAUGE, labels);
}

// Register a histogram metric
metric_entry_t* register_histogram(const char *name, const char *help, 
                                 const char *labels, double *buckets, int bucket_count) {
    metric_entry_t *metric = register_metric(name, help, METRIC_HISTOGRAM, labels);
    if (metric && buckets && bucket_count > 0) {
        metric->histogram_buckets = malloc(sizeof(histogram_bucket_t) * bucket_count);
        if (metric->histogram_buckets) {
            for (int i = 0; i < bucket_count; i++) {
                metric->histogram_buckets[i].upper_bound = buckets[i];
                metric->histogram_buckets[i].count = 0;
            }
            metric->bucket_count = bucket_count;
        }
    }
    return metric;
}

// Register a summary metric
metric_entry_t* register_summary(const char *name, const char *help, const char *labels) {
    return register_metric(name, help, METRIC_SUMMARY, labels);
}

// Internal function to register any metric type
metric_entry_t* register_metric(const char *name, const char *help, 
                               metric_type_t type, const char *labels) {
    if (!metrics_initialized || !name) {
        return NULL;
    }

    pthread_mutex_lock(&registry_mutex);
    
    // Check if metric already exists
    metric_entry_t *existing = find_metric(name, labels);
    if (existing) {
        pthread_mutex_unlock(&registry_mutex);
        return existing;
    }

    // Create new metric
    metric_entry_t *metric = calloc(1, sizeof(metric_entry_t));
    if (!metric) {
        pthread_mutex_unlock(&registry_mutex);
        return NULL;
    }

    metric->name = strdup(name);
    metric->help = help ? strdup(help) : NULL;
    metric->type = type;
    metric->value = 0.0;
    metric->labels = labels ? strdup(labels) : NULL;
    metric->timestamp_ms = get_current_timestamp_ms();
    metric->next = global_registry->metrics;
    
    // Add to registry
    global_registry->metrics = metric;
    global_registry->metric_count++;
    
    pthread_mutex_unlock(&registry_mutex);
    return metric;
}

// Find existing metric
metric_entry_t* find_metric(const char *name, const char *labels) {
    metric_entry_t *current = global_registry->metrics;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            if ((!labels && !current->labels) || 
                (labels && current->labels && strcmp(current->labels, labels) == 0)) {
                return current;
            }
        }
        current = current->next;
    }
    return NULL;
}

// Increment counter
int increment_counter(const char *name, const char *labels, double value) {
    metric_entry_t *metric = find_metric(name, labels);
    if (!metric || metric->type != METRIC_COUNTER) {
        return -1;
    }
    
    pthread_mutex_lock(&registry_mutex);
    metric->value += value;
    metric->timestamp_ms = get_current_timestamp_ms();
    pthread_mutex_unlock(&registry_mutex);
    
    return 0;
}

// Set gauge value
int set_gauge(const char *name, const char *labels, double value) {
    metric_entry_t *metric = find_metric(name, labels);
    if (!metric || metric->type != METRIC_GAUGE) {
        return -1;
    }
    
    pthread_mutex_lock(&registry_mutex);
    metric->value = value;
    metric->timestamp_ms = get_current_timestamp_ms();
    pthread_mutex_unlock(&registry_mutex);
    
    return 0;
}

// Record histogram observation
int observe_histogram(const char *name, const char *labels, double value) {
    metric_entry_t *metric = find_metric(name, labels);
    if (!metric || metric->type != METRIC_HISTOGRAM) {
        return -1;
    }
    
    pthread_mutex_lock(&registry_mutex);
    
    // Update histogram buckets
    for (int i = 0; i < metric->bucket_count; i++) {
        if (value <= metric->histogram_buckets[i].upper_bound) {
            metric->histogram_buckets[i].count++;
        }
    }
    
    // Update sum and count
    metric->histogram_sum += value;
    metric->histogram_count++;
    metric->timestamp_ms = get_current_timestamp_ms();
    
    pthread_mutex_unlock(&registry_mutex);
    
    return 0;
}

// Update summary
int update_summary(const char *name, const char *labels, double value) {
    metric_entry_t *metric = find_metric(name, labels);
    if (!metric || metric->type != METRIC_SUMMARY) {
        return -1;
    }
    
    pthread_mutex_lock(&registry_mutex);
    
    // Simple quantile approximation (for production use, consider more sophisticated algorithms)
    metric->summary_sum += value;
    metric->summary_count++;
    
    // Keep track of min/max for basic quantiles
    if (metric->summary_count == 1) {
        metric->summary_min = value;
        metric->summary_max = value;
    } else {
        if (value < metric->summary_min) metric->summary_min = value;
        if (value > metric->summary_max) metric->summary_max = value;
    }
    
    metric->timestamp_ms = get_current_timestamp_ms();
    pthread_mutex_unlock(&registry_mutex);
    
    return 0;
}

// Get current metrics registry
metrics_registry_t* get_metrics_registry() {
    return global_registry;
}

// Export metrics in Prometheus format
int export_prometheus_format(char *buffer, size_t buffer_size) {
    if (!metrics_initialized || !buffer || buffer_size == 0) {
        return -1;
    }
    
    size_t offset = 0;
    pthread_mutex_lock(&registry_mutex);
    
    // Add header with timestamp
    offset += snprintf(buffer + offset, buffer_size - offset, 
                      "# HELP %s_build_info MTProxy build information\n", 
                      global_registry->ns_prefix);
    offset += snprintf(buffer + offset, buffer_size - offset,
                      "# TYPE %s_build_info gauge\n", 
                      global_registry->ns_prefix);
    offset += snprintf(buffer + offset, buffer_size - offset,
                      "%s_build_info{version=\"%s\",commit=\"%s\"} 1\n\n",
                      global_registry->ns_prefix, "1.0.0", "unknown");
    
    // Export each metric
    metric_entry_t *current = global_registry->metrics;
    while (current && offset < buffer_size - 1) {
        offset += export_metric(buffer + offset, buffer_size - offset, current);
        current = current->next;
    }
    
    pthread_mutex_unlock(&registry_mutex);
    return (int)offset;
}

// Export single metric in Prometheus format
size_t export_metric(char *buffer, size_t buffer_size, metric_entry_t *metric) {
    if (!buffer || buffer_size == 0 || !metric) {
        return 0;
    }
    
    size_t offset = 0;
    
    // Add HELP line
    if (metric->help) {
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "# HELP %s_%s %s\n",
                          global_registry->ns_prefix, metric->name, metric->help);
    }
    
    // Add TYPE line
    const char *type_str = get_metric_type_string(metric->type);
    offset += snprintf(buffer + offset, buffer_size - offset,
                      "# TYPE %s_%s %s\n",
                      global_registry->ns_prefix, metric->name, type_str);
    
    // Add metric value
    if (metric->type == METRIC_HISTOGRAM) {
        offset += export_histogram_metric(buffer + offset, buffer_size - offset, metric);
    } else if (metric->type == METRIC_SUMMARY) {
        offset += export_summary_metric(buffer + offset, buffer_size - offset, metric);
    } else {
        // Simple gauge/counter
        if (metric->labels) {
            offset += snprintf(buffer + offset, buffer_size - offset,
                              "%s_%s{%s} %.6f %llu\n",
                              global_registry->ns_prefix, metric->name, metric->labels,
                              metric->value, metric->timestamp_ms);
        } else {
            offset += snprintf(buffer + offset, buffer_size - offset,
                              "%s_%s %.6f %llu\n",
                              global_registry->ns_prefix, metric->name,
                              metric->value, metric->timestamp_ms);
        }
    }
    
    offset += snprintf(buffer + offset, buffer_size - offset, "\n");
    return offset;
}

// Export histogram metric
size_t export_histogram_metric(char *buffer, size_t buffer_size, metric_entry_t *metric) {
    size_t offset = 0;
    char label_buf[256];
    
    // Export bucket counts
    for (int i = 0; i < metric->bucket_count; i++) {
        if (metric->labels) {
            snprintf(label_buf, sizeof(label_buf), "%s,le=\"%.3f\"", 
                    metric->labels, metric->histogram_buckets[i].upper_bound);
        } else {
            snprintf(label_buf, sizeof(label_buf), "le=\"%.3f\"", 
                    metric->histogram_buckets[i].upper_bound);
        }
        
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "%s_%s_bucket{%s} %llu %llu\n",
                          global_registry->ns_prefix, metric->name, label_buf,
                          metric->histogram_buckets[i].count, metric->timestamp_ms);
    }
    
    // Export +Inf bucket (total count)
    if (metric->labels) {
        snprintf(label_buf, sizeof(label_buf), "%s,le=\"+Inf\"", metric->labels);
    } else {
        snprintf(label_buf, sizeof(label_buf), "le=\"+Inf\"");
    }
    offset += snprintf(buffer + offset, buffer_size - offset,
                      "%s_%s_bucket{%s} %llu %llu\n",
                      global_registry->ns_prefix, metric->name, label_buf,
                      metric->histogram_count, metric->timestamp_ms);
    
    // Export sum and count
    if (metric->labels) {
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "%s_%s_sum{%s} %.6f %llu\n",
                          global_registry->ns_prefix, metric->name, metric->labels,
                          metric->histogram_sum, metric->timestamp_ms);
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "%s_%s_count{%s} %llu %llu\n",
                          global_registry->ns_prefix, metric->name, metric->labels,
                          metric->histogram_count, metric->timestamp_ms);
    } else {
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "%s_%s_sum %.6f %llu\n",
                          global_registry->ns_prefix, metric->name,
                          metric->histogram_sum, metric->timestamp_ms);
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "%s_%s_count %llu %llu\n",
                          global_registry->ns_prefix, metric->name,
                          metric->histogram_count, metric->timestamp_ms);
    }
    
    return offset;
}

// Export summary metric
size_t export_summary_metric(char *buffer, size_t buffer_size, metric_entry_t *metric) {
    size_t offset = 0;
    
    // Export quantiles (simplified - in production use proper quantile estimation)
    double quantiles[] = {0.5, 0.9, 0.99};
    double quantile_values[] = {
        metric->summary_min + (metric->summary_max - metric->summary_min) * 0.5,
        metric->summary_min + (metric->summary_max - metric->summary_min) * 0.9,
        metric->summary_min + (metric->summary_max - metric->summary_min) * 0.99
    };
    
    char label_buf[256];
    for (int i = 0; i < 3; i++) {
        if (metric->labels) {
            snprintf(label_buf, sizeof(label_buf), "%s,quantile=\"%.2f\"", 
                    metric->labels, quantiles[i]);
        } else {
            snprintf(label_buf, sizeof(label_buf), "quantile=\"%.2f\"", quantiles[i]);
        }
        
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "%s_%s{%s} %.6f %llu\n",
                          global_registry->ns_prefix, metric->name, label_buf,
                          quantile_values[i], metric->timestamp_ms);
    }
    
    // Export sum and count
    if (metric->labels) {
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "%s_%s_sum{%s} %.6f %llu\n",
                          global_registry->ns_prefix, metric->name, metric->labels,
                          metric->summary_sum, metric->timestamp_ms);
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "%s_%s_count{%s} %llu %llu\n",
                          global_registry->ns_prefix, metric->name, metric->labels,
                          metric->summary_count, metric->timestamp_ms);
    } else {
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "%s_%s_sum %.6f %llu\n",
                          global_registry->ns_prefix, metric->name,
                          metric->summary_sum, metric->timestamp_ms);
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "%s_%s_count %llu %llu\n",
                          global_registry->ns_prefix, metric->name,
                          metric->summary_count, metric->timestamp_ms);
    }
    
    return offset;
}

// Register default MTProxy metrics
void register_default_metrics() {
    // Connection metrics
    register_counter("connections_total", "Total number of connections", "type=\"inbound\"");
    register_counter("connections_total", "Total number of connections", "type=\"outbound\"");
    register_gauge("active_connections", "Current active connections", "type=\"inbound\"");
    register_gauge("active_connections", "Current active connections", "type=\"outbound\"");
    register_histogram("connection_duration_seconds", "Connection duration histogram", 
                      NULL, default_histogram_buckets, 
                      sizeof(default_histogram_buckets)/sizeof(default_histogram_buckets[0]));
    
    // Performance metrics
    register_counter("requests_total", "Total number of requests processed", NULL);
    register_histogram("request_duration_seconds", "Request processing time", 
                      NULL, default_histogram_buckets,
                      sizeof(default_histogram_buckets)/sizeof(default_histogram_buckets[0]));
    register_counter("bytes_transferred_total", "Total bytes transferred", "direction=\"sent\"");
    register_counter("bytes_transferred_total", "Total bytes transferred", "direction=\"received\"");
    
    // Security metrics
    register_counter("authentication_attempts_total", "Authentication attempts", "result=\"success\"");
    register_counter("authentication_attempts_total", "Authentication attempts", "result=\"failure\"");
    register_counter("blocked_connections_total", "Connections blocked by security", "reason=\"ddos\"");
    register_counter("rate_limited_requests_total", "Rate limited requests", NULL);
    
    // Resource metrics
    register_gauge("memory_usage_bytes", "Current memory usage", NULL);
    register_gauge("cpu_usage_percent", "Current CPU usage", NULL);
    register_gauge("file_descriptors", "Open file descriptors", NULL);
    register_gauge("active_threads", "Active threads", NULL);
}

// Get current timestamp in milliseconds
uint64_t get_current_timestamp_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// Get metric type as string
const char* get_metric_type_string(metric_type_t type) {
    switch (type) {
        case METRIC_COUNTER: return "counter";
        case METRIC_GAUGE: return "gauge";
        case METRIC_HISTOGRAM: return "histogram";
        case METRIC_SUMMARY: return "summary";
        default: return "untyped";
    }
}

// Cleanup metrics registry
void cleanup_metrics_registry() {
    if (!metrics_initialized) {
        return;
    }
    
    pthread_mutex_lock(&registry_mutex);
    
    metric_entry_t *current = global_registry->metrics;
    while (current) {
        metric_entry_t *next = current->next;
        
        free(current->name);
        free(current->help);
        free(current->labels);
        if (current->histogram_buckets) {
            free(current->histogram_buckets);
        }
        free(current);
        
        current = next;
    }
    
    free(global_registry->ns_prefix);
    free(global_registry);
    global_registry = NULL;
    metrics_initialized = 0;
    
    pthread_mutex_unlock(&registry_mutex);
}