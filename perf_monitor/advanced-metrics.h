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

#ifndef __ADVANCED_METRICS_H__
#define __ADVANCED_METRICS_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct metric_entry;
struct metrics_registry;

/* Metric types */
typedef enum {
    METRIC_COUNTER,
    METRIC_GAUGE,
    METRIC_HISTOGRAM,
    METRIC_SUMMARY
} metric_type_t;

/* Histogram bucket definition */
typedef struct {
    double upper_bound;            /* Upper bound of bucket */
    uint64_t count;                /* Count of observations in bucket */
} histogram_bucket_t;

/* Summary statistics */
typedef struct {
    double sum;                    /* Sum of all observations */
    uint64_t count;                /* Count of observations */
    double min;                    /* Minimum value */
    double max;                    /* Maximum value */
    double *quantiles;             /* Quantile values */
    size_t quantile_count;         /* Number of quantiles */
} summary_stats_t;

/* Metric definition structure */
typedef struct metric_entry {
    char *name;                     /* Metric name */
    char *help;                     /* Help text */
    metric_type_t type;            /* Type of metric */
    double value;                  /* Current value for gauge/counter */
    struct metric_entry *next;     /* Next metric in list */
    char *labels;                  /* Label key-value pairs */
    uint64_t timestamp_ms;         /* Timestamp in milliseconds */
    
    /* Histogram specific fields */
    histogram_bucket_t *histogram_buckets;
    int bucket_count;
    double histogram_sum;
    uint64_t histogram_count;
    
    /* Summary specific fields */
    double summary_sum;
    uint64_t summary_count;
    double summary_min;
    double summary_max;
} metric_entry_t;

/* Metrics registry */
typedef struct metrics_registry {
    metric_entry_t *metrics;       /* List of registered metrics */
    uint32_t metric_count;         /* Number of registered metrics */
    char *ns_prefix;              /* Metric namespace prefix */
    long start_time;            /* Registry start time */
    long last_update;           /* Last update time */
} metrics_registry_t;

/* Function prototypes */

/**
 * Initialize metrics registry
 * @param namespace_prefix: Prefix for all metric names (default: "mtproxy")
 * @return: 0 on success, -1 on error
 */
int init_metrics_registry(const char *namespace_prefix);

/**
 * Register a counter metric
 * @param name: Metric name
 * @param help: Help text
 * @param labels: Label key-value pairs (optional)
 * @return: Pointer to metric entry or NULL on error
 */
metric_entry_t* register_counter(const char *name, const char *help, const char *labels);

/**
 * Register a gauge metric
 * @param name: Metric name
 * @param help: Help text
 * @param labels: Label key-value pairs (optional)
 * @return: Pointer to metric entry or NULL on error
 */
metric_entry_t* register_gauge(const char *name, const char *help, const char *labels);

/**
 * Register a histogram metric
 * @param name: Metric name
 * @param help: Help text
 * @param labels: Label key-value pairs (optional)
 * @param buckets: Array of bucket boundaries
 * @param bucket_count: Number of buckets
 * @return: Pointer to metric entry or NULL on error
 */
metric_entry_t* register_histogram(const char *name, const char *help, 
                                 const char *labels, double *buckets, int bucket_count);

/**
 * Register a summary metric
 * @param name: Metric name
 * @param help: Help text
 * @param labels: Label key-value pairs (optional)
 * @return: Pointer to metric entry or NULL on error
 */
metric_entry_t* register_summary(const char *name, const char *help, const char *labels);

/**
 * Internal function to register any metric type
 * @param name: Metric name
 * @param help: Help text
 * @param type: Metric type
 * @param labels: Label key-value pairs (optional)
 * @return: Pointer to metric entry or NULL on error
 */
metric_entry_t* register_metric(const char *name, const char *help, 
                               metric_type_t type, const char *labels);

/**
 * Find existing metric
 * @param name: Metric name
 * @param labels: Label key-value pairs (optional)
 * @return: Pointer to metric entry or NULL if not found
 */
metric_entry_t* find_metric(const char *name, const char *labels);

/**
 * Increment counter value
 * @param name: Counter name
 * @param labels: Label key-value pairs (optional)
 * @param value: Value to increment by
 * @return: 0 on success, -1 on error
 */
int increment_counter(const char *name, const char *labels, double value);

/**
 * Set gauge value
 * @param name: Gauge name
 * @param labels: Label key-value pairs (optional)
 * @param value: Value to set
 * @return: 0 on success, -1 on error
 */
int set_gauge(const char *name, const char *labels, double value);

/**
 * Record histogram observation
 * @param name: Histogram name
 * @param labels: Label key-value pairs (optional)
 * @param value: Observation value
 * @return: 0 on success, -1 on error
 */
int observe_histogram(const char *name, const char *labels, double value);

/**
 * Update summary with new observation
 * @param name: Summary name
 * @param labels: Label key-value pairs (optional)
 * @param value: Observation value
 * @return: 0 on success, -1 on error
 */
int update_summary(const char *name, const char *labels, double value);

/**
 * Get current metrics registry
 * @return: Pointer to metrics registry
 */
metrics_registry_t* get_metrics_registry();

/**
 * Export metrics in Prometheus format
 * @param buffer: Output buffer
 * @param buffer_size: Size of output buffer
 * @return: Number of bytes written, -1 on error
 */
int export_prometheus_format(char *buffer, size_t buffer_size);

/**
 * Export single metric in Prometheus format
 * @param buffer: Output buffer
 * @param buffer_size: Size of output buffer
 * @param metric: Metric to export
 * @return: Number of bytes written
 */
size_t export_metric(char *buffer, size_t buffer_size, metric_entry_t *metric);

/**
 * Export histogram metric
 * @param buffer: Output buffer
 * @param buffer_size: Size of output buffer
 * @param metric: Histogram metric to export
 * @return: Number of bytes written
 */
size_t export_histogram_metric(char *buffer, size_t buffer_size, metric_entry_t *metric);

/**
 * Export summary metric
 * @param buffer: Output buffer
 * @param buffer_size: Size of output buffer
 * @param metric: Summary metric to export
 * @return: Number of bytes written
 */
size_t export_summary_metric(char *buffer, size_t buffer_size, metric_entry_t *metric);

/**
 * Register default MTProxy metrics
 */
void register_default_metrics();

/**
 * Get current timestamp in milliseconds
 * @return: Current timestamp in milliseconds
 */
uint64_t get_current_timestamp_ms();

/**
 * Get metric type as string
 * @param type: Metric type
 * @return: String representation of metric type
 */
const char* get_metric_type_string(metric_type_t type);

/**
 * Cleanup metrics registry
 */
void cleanup_metrics_registry();

#ifdef __cplusplus
}
#endif

#endif /* __ADVANCED_METRICS_H__ */