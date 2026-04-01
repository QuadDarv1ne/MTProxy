/*
 * Advanced Load Balancer Header for MTProxy
 * Implements sophisticated load distribution algorithms with health monitoring
 */

#ifndef ADVANCED_LOAD_BALANCER_H
#define ADVANCED_LOAD_BALANCER_H

// Load balancing algorithms
typedef enum {
    LB_ALGORITHM_ROUND_ROBIN = 0,
    LB_ALGORITHM_LEAST_CONNECTIONS = 1,
    LB_ALGORITHM_WEIGHTED_ROUND_ROBIN = 2,
    LB_ALGORITHM_IP_HASH = 3,
    LB_ALGORITHM_LEAST_RESPONSE_TIME = 4
} lb_algorithm_t;

// Server status
typedef enum {
    LB_SERVER_STATUS_HEALTHY = 0,
    LB_SERVER_STATUS_UNHEALTHY = 1,
    LB_SERVER_STATUS_MAINTENANCE = 2
} lb_server_status_t;

// Server structure
typedef struct {
    char address[46];              // IPv6 address max length
    int port;
    int weight;
    int max_connections;
    int current_connections;
    lb_server_status_t status;
    int failure_count;
    long long last_health_check;
    long long response_time_ms;
    long long total_requests;
    long long successful_requests;
} lb_server_t;

// Configuration structure
typedef struct {
    lb_algorithm_t algorithm;
    int enable_health_checks;
    long long health_check_interval_ms;
    int max_retries;
    long long connection_timeout_ms;
    int enable_weight_adjustment;
    long long weight_adjustment_interval_ms;
    int failover_enabled;
    int session_persistence;
    int max_servers;
    int enable_statistics;
} lb_config_t;

// Statistics structure
typedef struct {
    long long total_requests;
    long long successful_requests;
    long long failed_requests;
    long long health_check_failures;
    long long load_balancing_decisions;
    long long server_failovers;
    int current_active_servers;
    int peak_active_servers;
} lb_stats_t;

// Advanced load balancer structure
typedef struct {
    // Configuration
    lb_config_t config;
    
    // Statistics
    lb_stats_t stats;
    
    // Server pool
    lb_server_t servers[100];      // Fixed size for simplicity
    int server_count;
    int current_index;
    
    // State management
    long long last_health_check;
    long long last_weight_adjustment;
    int initialized;
} advanced_load_balancer_t;

// Function declarations
advanced_load_balancer_t* advanced_load_balancer_init(const lb_config_t *config);
int advanced_load_balancer_add_server(advanced_load_balancer_t *lb,
                                    const char *address,
                                    int port,
                                    int weight,
                                    int max_connections);
lb_server_t* advanced_load_balancer_select_server(advanced_load_balancer_t *lb,
                                                const char *client_ip);
int advanced_load_balancer_perform_health_checks(advanced_load_balancer_t *lb);
int advanced_load_balancer_adjust_weights(advanced_load_balancer_t *lb);
void advanced_load_balancer_report_success(advanced_load_balancer_t *lb,
                                         lb_server_t *server,
                                         long long response_time_ms);
void advanced_load_balancer_report_failure(advanced_load_balancer_t *lb,
                                         lb_server_t *server);
lb_stats_t advanced_load_balancer_get_stats(advanced_load_balancer_t *lb);
void advanced_load_balancer_reset_stats(advanced_load_balancer_t *lb);
void advanced_load_balancer_cleanup(advanced_load_balancer_t *lb);
advanced_load_balancer_t* get_global_load_balancer(void);

// Internal algorithm functions
static int advanced_load_balancer_round_robin(advanced_load_balancer_t *lb);
static int advanced_load_balancer_least_connections(advanced_load_balancer_t *lb);
static int advanced_load_balancer_weighted_round_robin(advanced_load_balancer_t *lb);
static int advanced_load_balancer_ip_hash(advanced_load_balancer_t *lb, const char *client_ip);
static int advanced_load_balancer_least_response_time(advanced_load_balancer_t *lb);

#endif // ADVANCED_LOAD_BALANCER_H