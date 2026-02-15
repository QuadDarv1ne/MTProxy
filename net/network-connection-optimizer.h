/*
 * Network Connection Optimizer Header for MTProxy
 * Provides optimized network connection management with pooling and load balancing
 */

#ifndef NETWORK_CONNECTION_OPTIMIZER_H
#define NETWORK_CONNECTION_OPTIMIZER_H

#include <stddef.h>

// Connection states
typedef enum {
    CONN_STATE_IDLE = 0,
    CONN_STATE_ACTIVE,
    CONN_STATE_BUSY,
    CONN_STATE_ERROR,
    CONN_STATE_CLOSED
} connection_state_t;

// Connection entry structure
typedef struct connection_entry {
    int connection_id;
    void *socket_handle;
    connection_state_t state;
    unsigned long long last_used;
    unsigned long long created_time;
    int use_count;
    struct connection_entry *next;
    struct connection_entry *prev;
    // Performance metrics
    double avg_response_time;
    long long total_requests;
    long long failed_requests;
} connection_entry_t;

// Connection pool structure
typedef struct {
    connection_entry_t *free_list;
    connection_entry_t *active_list;
    connection_entry_t *busy_list;
    int max_connections;
    int current_connections;
    int idle_connections;
    int active_connections;
    int busy_connections;
    // Pool statistics
    long long total_acquired;
    long long total_released;
    long long total_created;
    long long total_closed;
    long long cache_hits;
    long long cache_misses;
} connection_pool_t;

// Load balancer structure
typedef struct {
    connection_pool_t **pools;
    int pool_count;
    int current_pool_index;
    // Load balancing strategy
    int strategy; // 0 = round-robin, 1 = least connections, 2 = weighted
    // Statistics
    long long total_requests_routed;
    long long failed_routings;
} load_balancer_t;

// Network optimizer configuration
typedef struct {
    int enable_connection_pooling;
    int enable_load_balancing;
    int max_pool_connections;
    int min_idle_connections;
    int connection_timeout_ms;
    int enable_keepalive;
    int keepalive_interval_ms;
    int enable_compression;
    double performance_threshold;
} network_opt_config_t;

// Performance statistics
typedef struct {
    long long total_connections;
    long long active_connections;
    long long idle_connections;
    long long connection_reuse_count;
    long long new_connection_count;
    double avg_connection_time;
    double avg_request_time;
    long long total_bytes_sent;
    long long total_bytes_received;
    long long connection_errors;
} network_perf_stats_t;

// Main network optimizer structure
typedef struct {
    network_opt_config_t config;
    connection_pool_t *connection_pool;
    load_balancer_t *load_balancer;
    network_perf_stats_t stats;
    int is_initialized;
    unsigned long long operation_count;
} network_optimizer_t;

// Function declarations
network_optimizer_t* network_optimizer_init(network_opt_config_t *config);
connection_entry_t* network_acquire_connection();
int network_release_connection(connection_entry_t *conn);
void get_connection_pool_stats(long long *total_acquired, long long *total_released,
                              long long *total_created, long long *cache_hits,
                              int *current_connections, int *idle_connections);
void get_network_performance_stats(network_perf_stats_t *stats);
void cleanup_network_optimizer();
int init_global_network_optimizer();

// Convenience macros
#define ACQUIRE_CONNECTION() network_acquire_connection()
#define RELEASE_CONNECTION(conn) network_release_connection(conn)

#endif // NETWORK_CONNECTION_OPTIMIZER_H