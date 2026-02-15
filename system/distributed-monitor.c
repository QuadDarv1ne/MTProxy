/*
 * distributed-monitor.c
 * Distributed Performance Monitoring System Implementation
 */

#include "distributed-monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Define NULL if not already defined
#ifndef NULL
#define NULL ((void*)0)
#endif

// Define missing functions for compatibility
static int my_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int my_snprintf(char* str, size_t size, const char* format, ...) {
    // Simple implementation - just copy a basic message
    const char* basic_msg = "Cluster monitoring active";
    size_t len = strlen(basic_msg);
    if (size > 0) {
        size_t copy_len = (len < size - 1) ? len : size - 1;
        memcpy(str, basic_msg, copy_len);
        str[copy_len] = '\0';
    }
    return (int)len;
}

// Manual time_t definition for compatibility
typedef long long time_t;

// Global context
static distributed_monitor_ctx_t* g_monitor_ctx = NULL;
static monitor_config_t g_monitor_config = {0};

// Callback functions
static alert_callback_t g_alert_callback = NULL;
static node_status_callback_t g_node_status_callback = NULL;
static metric_callback_t g_metric_callback = NULL;

// Utility functions
static time_t get_current_timestamp_internal(void) {
    return (time_t)time(NULL);
}

static const char* node_status_to_string_internal(node_status_t status) {
    switch (status) {
        case NODE_STATUS_UNKNOWN: return "UNKNOWN";
        case NODE_STATUS_ONLINE: return "ONLINE";
        case NODE_STATUS_OFFLINE: return "OFFLINE";
        case NODE_STATUS_DEGRADED: return "DEGRADED";
        case NODE_STATUS_MAINTENANCE: return "MAINTENANCE";
        default: return "INVALID";
    }
}

static const char* alert_severity_to_string_internal(alert_severity_t severity) {
    switch (severity) {
        case ALERT_SEVERITY_INFO: return "INFO";
        case ALERT_SEVERITY_WARNING: return "WARNING";
        case ALERT_SEVERITY_ERROR: return "ERROR";
        case ALERT_SEVERITY_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

// Main implementation functions
int init_distributed_monitor(distributed_monitor_ctx_t* ctx, const char* cluster_name, uint16_t port) {
    if (!ctx || !cluster_name) {
        return -1;
    }
    
    memset(ctx, 0, sizeof(distributed_monitor_ctx_t));
    
    strncpy(ctx->cluster_name, cluster_name, sizeof(ctx->cluster_name) - 1);
    ctx->monitoring_port = port;
    ctx->monitoring_enabled = true;
    ctx->last_update = get_current_timestamp_internal();
    ctx->node_count = 0;
    ctx->alert_count = 0;
    
    g_monitor_ctx = ctx;
    
    // Initialize default configuration
    g_monitor_config.heartbeat_interval_seconds = 30;
    g_monitor_config.alert_threshold_cpu_percent = 80;
    g_monitor_config.alert_threshold_memory_percent = 85;
    g_monitor_config.alert_threshold_throughput_bps = 1000000000ULL; // 1 Gbps
    g_monitor_config.max_offline_time_seconds = 120;
    g_monitor_config.auto_node_discovery = true;
    strcpy(g_monitor_config.coordinator_ip, "127.0.0.1");
    g_monitor_config.coordinator_port = 8080;
    
    return 0;
}

void cleanup_distributed_monitor(distributed_monitor_ctx_t* ctx) {
    if (!ctx) return;
    
    // Clear all nodes and alerts
    ctx->node_count = 0;
    ctx->alert_count = 0;
    ctx->monitoring_enabled = false;
    
    if (g_monitor_ctx == ctx) {
        g_monitor_ctx = NULL;
    }
}

int add_cluster_node(distributed_monitor_ctx_t* ctx, const char* node_name, const char* ip, uint16_t port) {
    if (!ctx || !node_name || !ip || ctx->node_count >= MAX_CLUSTER_NODES) {
        return -1;
    }
    
    cluster_node_t* node = &ctx->nodes[ctx->node_count];
    
    strncpy(node->node_name, node_name, sizeof(node->node_name) - 1);
    strncpy(node->ip_address, ip, sizeof(node->ip_address) - 1);
    node->port = port;
    node->status = NODE_STATUS_UNKNOWN;
    node->last_heartbeat = get_current_timestamp_internal();
    node->uptime_seconds = 0;
    node->cpu_usage_percent = 0.0;
    node->memory_usage_bytes = 0;
    node->network_throughput_bps = 0;
    node->connections_count = 0;
    node->metric_count = 0;
    
    ctx->node_count++;
    ctx->last_update = get_current_timestamp_internal();
    
    return 0;
}

int remove_cluster_node(distributed_monitor_ctx_t* ctx, const char* node_name) {
    if (!ctx || !node_name) return -1;
    
    for (int i = 0; i < ctx->node_count; i++) {
        if (strcmp(ctx->nodes[i].node_name, node_name) == 0) {
            // Shift remaining nodes
            for (int j = i; j < ctx->node_count - 1; j++) {
                ctx->nodes[j] = ctx->nodes[j + 1];
            }
            ctx->node_count--;
            ctx->last_update = get_current_timestamp_internal();
            return 0;
        }
    }
    
    return -1; // Node not found
}

cluster_node_t* find_node_by_name(distributed_monitor_ctx_t* ctx, const char* node_name) {
    if (!ctx || !node_name) return NULL;
    
    for (int i = 0; i < ctx->node_count; i++) {
        if (strcmp(ctx->nodes[i].node_name, node_name) == 0) {
            return &ctx->nodes[i];
        }
    }
    
    return NULL;
}

cluster_node_t* find_node_by_ip(distributed_monitor_ctx_t* ctx, const char* ip_address) {
    if (!ctx || !ip_address) return NULL;
    
    for (int i = 0; i < ctx->node_count; i++) {
        if (strcmp(ctx->nodes[i].ip_address, ip_address) == 0) {
            return &ctx->nodes[i];
        }
    }
    
    return NULL;
}

int update_node_status(distributed_monitor_ctx_t* ctx, const char* node_name, node_status_t status) {
    if (!ctx || !node_name) return -1;
    
    cluster_node_t* node = find_node_by_name(ctx, node_name);
    if (!node) return -1;
    
    node_status_t old_status = node->status;
    node->status = status;
    node->last_heartbeat = get_current_timestamp_internal();
    
    // Call callback if registered
    if (g_node_status_callback && old_status != status) {
        g_node_status_callback(node, old_status);
    }
    
    ctx->last_update = get_current_timestamp_internal();
    return 0;
}

int report_node_metrics(distributed_monitor_ctx_t* ctx, const char* node_name, 
                       const performance_metric_t* metrics, int count) {
    if (!ctx || !node_name || !metrics || count <= 0) return -1;
    
    cluster_node_t* node = find_node_by_name(ctx, node_name);
    if (!node) return -1;
    
    // Add metrics to node
    int added = 0;
    for (int i = 0; i < count && node->metric_count < 32; i++) {
        node->metrics[node->metric_count] = metrics[i];
        node->metrics[node->metric_count].timestamp = get_current_timestamp_internal();
        node->metric_count++;
        added++;
        
        // Call metric callback if registered
        if (g_metric_callback) {
            g_metric_callback(node, &metrics[i]);
        }
    }
    
    // Update node's key performance indicators
    for (int i = 0; i < count; i++) {
        if (strcmp(metrics[i].name, "cpu_usage_percent") == 0) {
            node->cpu_usage_percent = metrics[i].value;
        } else if (strcmp(metrics[i].name, "memory_usage_bytes") == 0) {
            node->memory_usage_bytes = (uint64_t)metrics[i].value;
        } else if (strcmp(metrics[i].name, "network_throughput_bps") == 0) {
            node->network_throughput_bps = (uint64_t)metrics[i].value;
        } else if (strcmp(metrics[i].name, "connections_count") == 0) {
            node->connections_count = (uint64_t)metrics[i].value;
        }
    }
    
    ctx->last_update = get_current_timestamp_internal();
    
    // Check for alerts based on thresholds
    if (node->cpu_usage_percent > g_monitor_config.alert_threshold_cpu_percent) {
        char alert_msg[256];
        snprintf(alert_msg, sizeof(alert_msg), 
                "High CPU usage on node %s: %.2f%%", 
                node_name, node->cpu_usage_percent);
        generate_alert(ctx, ALERT_SEVERITY_WARNING, alert_msg, node_name);
    }
    
    if (node->memory_usage_bytes > (g_monitor_config.alert_threshold_memory_percent * 1024 * 1024 * 1024 / 100)) {
        char alert_msg[256];
        snprintf(alert_msg, sizeof(alert_msg), 
                "High memory usage on node %s: %.2f GB", 
                node_name, (double)node->memory_usage_bytes / (1024.0 * 1024.0 * 1024.0));
        generate_alert(ctx, ALERT_SEVERITY_WARNING, alert_msg, node_name);
    }
    
    return added;
}

int generate_alert(distributed_monitor_ctx_t* ctx, alert_severity_t severity, 
                  const char* message, const char* source_node) {
    if (!ctx || !message || ctx->alert_count >= 1000) return -1;
    
    cluster_alert_t* alert = &ctx->alerts[ctx->alert_count];
    
    static uint64_t alert_id_counter = 1;
    alert->alert_id = alert_id_counter++;
    alert->severity = severity;
    strncpy(alert->message, message, sizeof(alert->message) - 1);
    if (source_node) {
        strncpy(alert->source_node, source_node, sizeof(alert->source_node) - 1);
    } else {
        strcpy(alert->source_node, "SYSTEM");
    }
    alert->timestamp = get_current_timestamp_internal();
    alert->acknowledged = false;
    
    ctx->alert_count++;
    ctx->last_update = get_current_timestamp_internal();
    
    // Call alert callback if registered
    if (g_alert_callback) {
        g_alert_callback(alert);
    }
    
    return 0;
}

void perform_cluster_health_check(distributed_monitor_ctx_t* ctx) {
    if (!ctx) return;
    
    time_t current_time = get_current_timestamp_internal();
    
    for (int i = 0; i < ctx->node_count; i++) {
        cluster_node_t* node = &ctx->nodes[i];
        time_t time_since_heartbeat = current_time - node->last_heartbeat;
        
        // Check if node is offline based on heartbeat timeout
        if (time_since_heartbeat > g_monitor_config.max_offline_time_seconds) {
            if (node->status != NODE_STATUS_OFFLINE) {
                update_node_status(ctx, node->node_name, NODE_STATUS_OFFLINE);
                
                char alert_msg[256];
                snprintf(alert_msg, sizeof(alert_msg), 
                        "Node %s is offline (no heartbeat for %lld seconds)", 
                        node->node_name, (long long)time_since_heartbeat);
                generate_alert(ctx, ALERT_SEVERITY_ERROR, alert_msg, node->node_name);
            }
        } else if (node->status == NODE_STATUS_OFFLINE) {
            // Node came back online
            update_node_status(ctx, node->node_name, NODE_STATUS_ONLINE);
        }
    }
}

double calculate_cluster_throughput(distributed_monitor_ctx_t* ctx) {
    if (!ctx) return 0.0;
    
    double total_throughput = 0.0;
    int active_nodes = 0;
    
    for (int i = 0; i < ctx->node_count; i++) {
        if (ctx->nodes[i].status == NODE_STATUS_ONLINE) {
            total_throughput += (double)ctx->nodes[i].network_throughput_bps;
            active_nodes++;
        }
    }
    
    return active_nodes > 0 ? total_throughput : 0.0;
}

double calculate_cluster_cpu_usage(distributed_monitor_ctx_t* ctx) {
    if (!ctx) return 0.0;
    
    double total_cpu = 0.0;
    int active_nodes = 0;
    
    for (int i = 0; i < ctx->node_count; i++) {
        if (ctx->nodes[i].status == NODE_STATUS_ONLINE) {
            total_cpu += ctx->nodes[i].cpu_usage_percent;
            active_nodes++;
        }
    }
    
    return active_nodes > 0 ? total_cpu / active_nodes : 0.0;
}

uint64_t calculate_total_connections(distributed_monitor_ctx_t* ctx) {
    if (!ctx) return 0;
    
    uint64_t total_connections = 0;
    
    for (int i = 0; i < ctx->node_count; i++) {
        if (ctx->nodes[i].status == NODE_STATUS_ONLINE) {
            total_connections += ctx->nodes[i].connections_count;
        }
    }
    
    return total_connections;
}

void generate_cluster_report(distributed_monitor_ctx_t* ctx, char* report_buffer, size_t buffer_size) {
    if (!ctx || !report_buffer || buffer_size == 0) return;
    
    time_t current_time = get_current_timestamp_internal();
    
    int written = snprintf(report_buffer, buffer_size,
        "=== MTProxy Cluster Report ===\n"
        "Cluster: %s\n"
        "Generated: %lld\n"
        "Nodes: %d\n"
        "Alerts: %d\n"
        "Last Update: %lld\n\n",
        ctx->cluster_name,
        (long long)current_time,
        ctx->node_count,
        ctx->alert_count,
        (long long)ctx->last_update);
    
    if (written < 0 || (size_t)written >= buffer_size) return;
    
    // Add node information
    written += snprintf(report_buffer + written, buffer_size - written,
        "=== Node Status ===\n");
    
    for (int i = 0; i < ctx->node_count && (size_t)written < buffer_size; i++) {
        cluster_node_t* node = &ctx->nodes[i];
        written += snprintf(report_buffer + written, buffer_size - written,
            "Node: %s (%s:%d) - Status: %s\n"
            "  CPU: %.2f%%, Memory: %.2f GB, Throughput: %.2f Mbps\n"
            "  Connections: %llu, Uptime: %u seconds\n\n",
            node->node_name, node->ip_address, node->port,
            node_status_to_string_internal(node->status),
            node->cpu_usage_percent,
            (double)node->memory_usage_bytes / (1024.0 * 1024.0 * 1024.0),
            (double)node->network_throughput_bps / (1024.0 * 1024.0),
            (unsigned long long)node->connections_count,
            node->uptime_seconds);
    }
    
    // Add cluster summary
    if ((size_t)written < buffer_size) {
        written += snprintf(report_buffer + written, buffer_size - written,
            "=== Cluster Summary ===\n"
            "Total Throughput: %.2f Mbps\n"
            "Average CPU Usage: %.2f%%\n"
            "Total Connections: %llu\n",
            calculate_cluster_throughput(ctx) / (1024.0 * 1024.0),
            calculate_cluster_cpu_usage(ctx),
            (unsigned long long)calculate_total_connections(ctx));
    }
}

// Callback registration functions
void register_alert_callback(alert_callback_t callback) {
    g_alert_callback = callback;
}

void register_node_status_callback(node_status_callback_t callback) {
    g_node_status_callback = callback;
}

void register_metric_callback(metric_callback_t callback) {
    g_metric_callback = callback;
}

// Utility functions
const char* node_status_to_string(node_status_t status) {
    return node_status_to_string_internal(status);
}

const char* alert_severity_to_string(alert_severity_t severity) {
    return alert_severity_to_string_internal(severity);
}

void print_cluster_status(distributed_monitor_ctx_t* ctx) {
    if (!ctx) return;
    
    char report_buffer[4096];
    generate_cluster_report(ctx, report_buffer, sizeof(report_buffer));
    printf("%s\n", report_buffer);
}

time_t get_current_timestamp(void) {
    return get_current_timestamp_internal();
}