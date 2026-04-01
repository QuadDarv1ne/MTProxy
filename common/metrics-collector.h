/*
 * Metrics collector for MTProxy
 *
 * This file defines the interface for collecting and exporting metrics
 * in Prometheus format for monitoring and observability.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Metric types */
typedef enum {
    METRIC_COUNTER,
    METRIC_GAUGE,
    METRIC_HISTOGRAM,
    METRIC_SUMMARY
} metric_type_t;

/* Metric definition structure */
typedef struct metric_entry {
    char *name;                     /* Metric name */
    char *help;                     /* Help text */
    metric_type_t type;            /* Type of metric */
    double value;                  /* Current value for gauge/counter */
    struct metric_entry *next;     /* Next metric in list */
    char *labels;                  /* Label key-value pairs */
    uint64_t timestamp_ms;         /* Timestamp in milliseconds */
} metric_entry_t;

/* Histogram bucket definition */
typedef struct {
    double upper_bound;            /* Upper bound of bucket */
    uint64_t count;                /* Count of observations in bucket */
} histogram_bucket_t;

/* Metrics registry */
typedef struct {
    metric_entry_t *metrics;       /* List of registered metrics */
    uint32_t metric_count;         /* Number of registered metrics */
    char *ns_prefix;              /* Metric namespace prefix */
    char *subsystem;              /* Subsystem prefix */
} metrics_registry_t;

/* Initialize metrics collection system */
int init_metrics_system(const char *ns_prefix, const char *subsystem);

/* Register a new metric */
metric_entry_t* register_metric(const char *name, metric_type_t type, const char *help);

/* Update a counter metric */
int update_counter(const char *name, double increment, const char *labels);

/* Set a gauge metric */
int set_gauge(const char *name, double value, const char *labels);

/* Observe a value for histogram/summary */
int observe_histogram(const char *name, double value, const char *labels);

/* Get metrics in Prometheus text format */
char* get_prometheus_metrics(void);

/* Export metrics in JSON format */
char* get_json_metrics(void);

/* Record request duration */
int record_request_duration(const char *operation, double duration_ms);

/* Increment connection counter */
int increment_connection_count(void);

/* Decrement active connection count */
int decrement_connection_count(void);

/* Record bytes transferred */
int record_bytes_transferred(size_t bytes, int is_upload);

/* Update encryption operation metrics */
int update_encryption_metrics(const char *algorithm, double duration_ms);

/* Update authentication metrics */
int update_auth_metrics(int success, double duration_ms);

/* Get metrics registry instance */
metrics_registry_t* get_metrics_registry(void);

/* Cleanup metrics system */
void destroy_metrics_system(void);

/* Format metrics for HTTP response */
int format_metrics_http_response(char *buffer, size_t buf_size);

#ifdef __cplusplus
}
#endif