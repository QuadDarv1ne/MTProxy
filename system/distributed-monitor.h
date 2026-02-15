/*
 * distributed-monitor.h
 * Distributed Performance Monitoring System for MTProxy
 *
 * This system provides distributed monitoring capabilities for MTProxy clusters,
 * allowing real-time performance tracking across multiple proxy instances.
 */

#ifndef DISTRIBUTED_MONITOR_H
#define DISTRIBUTED_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "common/platform_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of nodes in the cluster
#define MAX_CLUSTER_NODES 256
#define MAX_METRIC_NAME_LEN 64
#define MAX_NODE_NAME_LEN 32
#define METRICS_HISTORY_SIZE 1000

// Node status enumeration
typedef enum {
    NODE_STATUS_UNKNOWN = 0,
    NODE_STATUS_ONLINE,
    NODE_STATUS_OFFLINE,
    NODE_STATUS_DEGRADED,
    NODE_STATUS_MAINTENANCE
} node_status_t;

// Metric types
typedef enum {
    METRIC_TYPE_COUNTER,
    METRIC_TYPE_GAUGE,
    METRIC_TYPE_HISTOGRAM,
    METRIC_TYPE_SUMMARY
} metric_type_t;

// Alert severity levels
typedef enum {
    ALERT_SEVERITY_INFO = 0,
    ALERT_SEVERITY_WARNING,
    ALERT_SEVERITY_ERROR,
    ALERT_SEVERITY_CRITICAL
} alert_severity_t;

// Performance metrics structure
typedef struct {
    char name[MAX_METRIC_NAME_LEN];
    metric_type_t type;
    double value;
    uint64_t timestamp;
    char unit[16];
} performance_metric_t;

// Node information structure
typedef struct {
    char node_name[MAX_NODE_NAME_LEN];
    char ip_address[46];  // IPv6 max length
    uint16_t port;
    node_status_t status;
    time_t last_heartbeat;
    uint32_t uptime_seconds;
    double cpu_usage_percent;
    uint64_t memory_usage_bytes;
    uint64_t network_throughput_bps;
    uint64_t connections_count;
    performance_metric_t metrics[32];
    int metric_count;
} cluster_node_t;

// Alert structure
typedef struct {
    uint64_t alert_id;
    alert_severity_t severity;
    char message[256];
    char source_node[MAX_NODE_NAME_LEN];
    uint64_t timestamp;
    bool acknowledged;
} cluster_alert_t;

// Distributed monitoring context
typedef struct {
    cluster_node_t nodes[MAX_CLUSTER_NODES];
    int node_count;
    cluster_alert_t alerts[1000];
    int alert_count;
    time_t last_update;
    bool monitoring_enabled;
    char cluster_name[64];
    uint16_t monitoring_port;
} distributed_monitor_ctx_t;

// Callback function types
typedef void (*alert_callback_t)(const cluster_alert_t* alert);
typedef void (*node_status_callback_t)(const cluster_node_t* node, node_status_t old_status);
typedef void (*metric_callback_t)(const cluster_node_t* node, const performance_metric_t* metric);

// Function prototypes

// Initialization and cleanup
int init_distributed_monitor(distributed_monitor_ctx_t* ctx, const char* cluster_name, uint16_t port);
void cleanup_distributed_monitor(distributed_monitor_ctx_t* ctx);

// Node management
int add_cluster_node(distributed_monitor_ctx_t* ctx, const char* node_name, const char* ip, uint16_t port);
int remove_cluster_node(distributed_monitor_ctx_t* ctx, const char* node_name);
int update_node_status(distributed_monitor_ctx_t* ctx, const char* node_name, node_status_t status);
cluster_node_t* find_node_by_name(distributed_monitor_ctx_t* ctx, const char* node_name);
cluster_node_t* find_node_by_ip(distributed_monitor_ctx_t* ctx, const char* ip_address);

// Metric collection and reporting
int report_node_metrics(distributed_monitor_ctx_t* ctx, const char* node_name, 
                       const performance_metric_t* metrics, int count);
int add_node_metric(cluster_node_t* node, const performance_metric_t* metric);
double get_average_metric(distributed_monitor_ctx_t* ctx, const char* metric_name);
double get_metric_for_node(distributed_monitor_ctx_t* ctx, const char* node_name, const char* metric_name);

// Alert management
int generate_alert(distributed_monitor_ctx_t* ctx, alert_severity_t severity, 
                  const char* message, const char* source_node);
int acknowledge_alert(distributed_monitor_ctx_t* ctx, uint64_t alert_id);
cluster_alert_t* get_unacknowledged_alerts(distributed_monitor_ctx_t* ctx, int* count);
void clear_old_alerts(distributed_monitor_ctx_t* ctx, time_t older_than);

// Health checking
bool perform_health_check(distributed_monitor_ctx_t* ctx, const char* node_name);
void perform_cluster_health_check(distributed_monitor_ctx_t* ctx);
node_status_t calculate_node_status(const cluster_node_t* node);

// Network communication
int start_monitoring_server(distributed_monitor_ctx_t* ctx);
int stop_monitoring_server(distributed_monitor_ctx_t* ctx);
int send_metrics_to_coordinator(distributed_monitor_ctx_t* ctx, const char* coordinator_ip, uint16_t port);

// Data aggregation and analysis
double calculate_cluster_throughput(distributed_monitor_ctx_t* ctx);
double calculate_cluster_cpu_usage(distributed_monitor_ctx_t* ctx);
uint64_t calculate_total_connections(distributed_monitor_ctx_t* ctx);
void generate_cluster_report(distributed_monitor_ctx_t* ctx, char* report_buffer, size_t buffer_size);

// Callback registration
void register_alert_callback(alert_callback_t callback);
void register_node_status_callback(node_status_callback_t callback);
void register_metric_callback(metric_callback_t callback);

// Utility functions
const char* node_status_to_string(node_status_t status);
const char* alert_severity_to_string(alert_severity_t severity);
void print_cluster_status(distributed_monitor_ctx_t* ctx);
time_t get_current_timestamp(void);

// Configuration
typedef struct {
    int heartbeat_interval_seconds;
    int alert_threshold_cpu_percent;
    int alert_threshold_memory_percent;
    uint64_t alert_threshold_throughput_bps;
    int max_offline_time_seconds;
    bool auto_node_discovery;
    char coordinator_ip[46];
    uint16_t coordinator_port;
} monitor_config_t;

int load_monitor_config(monitor_config_t* config, const char* config_file);
int save_monitor_config(const monitor_config_t* config, const char* config_file);
void set_monitor_config(distributed_monitor_ctx_t* ctx, const monitor_config_t* config);

#ifdef __cplusplus
}
#endif

#endif // DISTRIBUTED_MONITOR_H