/*
 * Advanced Connection Pool Optimizer Header for MTProxy
 * Implements sophisticated connection management with predictive scaling
 * and intelligent resource allocation
 */

#ifndef ADVANCED_CONNECTION_OPTIMIZER_H
#define ADVANCED_CONNECTION_OPTIMIZER_H

#include <stdint.h>
#include <stddef.h>

#define PREDICTION_HISTORY_SIZE 1000

// Load balancing algorithms
typedef enum {
    LOAD_BALANCE_ROUND_ROBIN = 0,
    LOAD_BALANCE_LEAST_CONNECTIONS = 1,
    LOAD_BALANCE_WEIGHTED = 2,
    LOAD_BALANCE_IP_HASH = 3
} load_balancing_algorithm_t;

// Connection states
typedef enum {
    CONN_STATE_IDLE = 0,
    CONN_STATE_ACTIVE = 1,
    CONN_STATE_BUSY = 2,
    CONN_STATE_ERROR = 3,
    CONN_STATE_CLOSED = 4
} connection_state_t;

// Connection types
typedef enum {
    CONN_TYPE_CLIENT = 0,
    CONN_TYPE_SERVER = 1,
    CONN_TYPE_INTERNAL = 2
} connection_type_t;

// Connection information structure
typedef struct {
    int fd;
    connection_state_t state;
    connection_type_t type;
    long long creation_time;
    long long last_used_time;
    int is_active;
    int retry_count;
    char remote_addr[46];  // IPv6 address max length
    int remote_port;
    void *user_data;
} connection_info_t;

// Configuration structure
typedef struct {
    int enable_predictive_scaling;
    int enable_adaptive_timeout;
    int enable_connection_reuse;
    int enable_health_monitoring;
    int min_pool_size;
    int max_pool_size;
    int initial_pool_size;
    double scale_up_threshold;    // 0.0 - 1.0
    double scale_down_threshold;  // 0.0 - 1.0
    int max_scale_step;
    int prediction_window_size;
    long long health_check_interval_ms;
    long long connection_timeout_ms;
    long long idle_timeout_ms;
    int enable_load_balancing;
    load_balancing_algorithm_t load_balancing_algorithm;
} adv_conn_opt_config_t;

// Statistics structure
typedef struct {
    long long total_connections_created;
    long long total_connections_reused;
    long long total_connections_failed;
    long long scaling_events;
    long long health_check_failures;
    long long load_balancing_decisions;
    int current_pool_size;
    int peak_pool_size;
    double current_utilization;
} adv_conn_opt_stats_t;

// Advanced connection optimizer structure
typedef struct {
    // Configuration
    adv_conn_opt_config_t config;
    
    // Statistics
    adv_conn_opt_stats_t stats;
    
    // Prediction data
    double prediction_history[PREDICTION_HISTORY_SIZE];
    int prediction_history_size;
    int prediction_history_index;
    
    // Load balancing
    int current_lb_index;
    int active_connections;
    
    // State management
    long long last_scale_time;
    long long last_health_check;
    int initialized;
} advanced_conn_optimizer_t;

// Function declarations
advanced_conn_optimizer_t* advanced_conn_optimizer_init(const adv_conn_opt_config_t *config);
int advanced_conn_optimizer_perform_scaling(advanced_conn_optimizer_t *optimizer);
int advanced_conn_optimizer_get_connection(advanced_conn_optimizer_t *optimizer, 
                                        connection_info_t *conn_info);
int advanced_conn_optimizer_return_connection(advanced_conn_optimizer_t *optimizer, 
                                           int conn_fd);
int advanced_conn_optimizer_perform_health_check(advanced_conn_optimizer_t *optimizer);
adv_conn_opt_stats_t advanced_conn_optimizer_get_stats(advanced_conn_optimizer_t *optimizer);
void advanced_conn_optimizer_reset_stats(advanced_conn_optimizer_t *optimizer);
void advanced_conn_optimizer_cleanup(advanced_conn_optimizer_t *optimizer);
advanced_conn_optimizer_t* get_global_advanced_optimizer(void);

#endif // ADVANCED_CONNECTION_OPTIMIZER_H