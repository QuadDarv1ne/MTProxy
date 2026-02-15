/*
 * routing-optimizer.h
 * Intelligent Routing Optimization System
 *
 * This system provides intelligent routing decisions based on real-time
 * network conditions, performance metrics, and predictive analytics.
 */

#ifndef ROUTING_OPTIMIZER_H
#define ROUTING_OPTIMIZER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Routing strategies
typedef enum {
    ROUTING_STRATEGY_LATENCY = 0,
    ROUTING_STRATEGY_BANDWIDTH,
    ROUTING_STRATEGY_RELIABILITY,
    ROUTING_STRATEGY_COST,
    ROUTING_STRATEGY_BALANCED,
    ROUTING_STRATEGY_CUSTOM
} routing_strategy_t;

// Route types
typedef enum {
    ROUTE_TYPE_DIRECT = 0,
    ROUTE_TYPE_VIA_PROXY,
    ROUTE_TYPE_VIA_RELAY,
    ROUTE_TYPE_VIA_CDN,
    ROUTE_TYPE_MULTIPATH
} route_type_t;

// Network path structure
typedef struct {
    uint32_t path_id;
    char destination_ip[46];
    uint16_t destination_port;
    route_type_t route_type;
    char next_hop_ip[46];
    uint16_t next_hop_port;
    double latency_ms;
    double bandwidth_mbps;
    double packet_loss_rate;
    double jitter_ms;
    int hop_count;
    uint64_t last_updated;
    bool is_active;
    int priority;
    double health_score;  // 0.0 - 100.0
} network_path_t;

// Routing decision structure
typedef struct {
    uint32_t decision_id;
    uint32_t source_ip;
    uint32_t destination_ip;
    uint16_t destination_port;
    uint32_t selected_path_id;
    routing_strategy_t strategy_used;
    double expected_latency_ms;
    double expected_throughput_mbps;
    double confidence_score;  // 0.0 - 1.0
    uint64_t decision_timestamp;
    char reason[256];
} routing_decision_t;

// Route performance metrics
typedef struct {
    uint64_t total_routes_evaluated;
    uint64_t optimal_routes_selected;
    uint64_t failed_routes;
    uint64_t route_changes;
    double average_decision_time_ms;
    double routing_accuracy;
    double average_latency_improvement_ms;
    double average_throughput_improvement_mbps;
    uint64_t last_optimization_time;
} routing_stats_t;

// Routing optimizer configuration
typedef struct {
    int enable_intelligent_routing;
    routing_strategy_t default_strategy;
    int path_evaluation_interval_ms;
    int reoptimization_interval_ms;
    double latency_threshold_ms;
    double bandwidth_threshold_mbps;
    double packet_loss_threshold_percent;
    int max_paths_per_destination;
    bool enable_predictive_routing;
    int prediction_window_seconds;
    bool enable_load_balancing;
    int load_balancing_weight_factor;
    bool enable_failover;
    int failover_timeout_ms;
    bool enable_route_caching;
    int route_cache_ttl_seconds;
} routing_config_t;

// Path health structure
typedef struct {
    uint32_t path_id;
    double current_latency_ms;
    double current_bandwidth_mbps;
    double current_packet_loss_rate;
    double current_jitter_ms;
    double historical_latency_avg_ms;
    double historical_bandwidth_avg_mbps;
    double health_trend;  // Positive = improving, Negative = degrading
    uint64_t last_health_check;
    int consecutive_failures;
    bool is_healthy;
} path_health_t;

// Routing optimizer context
typedef struct {
    routing_config_t config;
    routing_stats_t stats;
    network_path_t* available_paths;
    int path_count;
    routing_decision_t* decision_history;
    int decision_history_count;
    path_health_t* path_health;
    int health_count;
    uint64_t last_evaluation_time;
    uint64_t last_reoptimization_time;
    bool is_optimizing;
    routing_strategy_t current_strategy;
    void* routing_algorithms[6];  // Pointers to different routing algorithm instances
    int active_algorithm_index;
} routing_optimizer_ctx_t;

// Network conditions structure
typedef struct {
    uint32_t source_ip;
    uint32_t destination_ip;
    uint16_t destination_port;
    double available_bandwidth_mbps;
    double current_latency_ms;
    double packet_loss_rate_percent;
    double jitter_ms;
    int concurrent_connections;
    uint64_t timestamp;
    char network_type[32];  // "wifi", "cellular", "ethernet", etc.
    int signal_strength;    // -100 to 0 dBm for wireless
} network_conditions_t;

// Callback function types
typedef void (*routing_decision_callback_t)(const routing_decision_t* decision);
typedef void (*path_health_callback_t)(const path_health_t* health);
typedef void (*routing_stats_callback_t)(const routing_stats_t* stats);
typedef void (*route_change_callback_t)(uint32_t old_path_id, uint32_t new_path_id);

// Function prototypes

// Initialization and cleanup
int init_routing_optimizer(routing_optimizer_ctx_t* ctx);
int init_routing_optimizer_with_config(routing_optimizer_ctx_t* ctx, const routing_config_t* config);
void cleanup_routing_optimizer(routing_optimizer_ctx_t* ctx);

// Configuration management
void get_routing_config(routing_optimizer_ctx_t* ctx, routing_config_t* config);
int set_routing_config(routing_optimizer_ctx_t* ctx, const routing_config_t* config);

// Path management
int add_network_path(routing_optimizer_ctx_t* ctx, const network_path_t* path);
int remove_network_path(routing_optimizer_ctx_t* ctx, uint32_t path_id);
int update_network_path(routing_optimizer_ctx_t* ctx, const network_path_t* path);
network_path_t* find_best_path(routing_optimizer_ctx_t* ctx, uint32_t destination_ip, uint16_t destination_port);
network_path_t* get_path_by_id(routing_optimizer_ctx_t* ctx, uint32_t path_id);
int get_available_paths(routing_optimizer_ctx_t* ctx, network_path_t* paths, int max_paths);

// Routing decision making
routing_decision_t make_routing_decision(routing_optimizer_ctx_t* ctx, 
                                       uint32_t source_ip,
                                       uint32_t destination_ip,
                                       uint16_t destination_port,
                                       const network_conditions_t* conditions);
int apply_routing_decision(routing_optimizer_ctx_t* ctx, const routing_decision_t* decision);
routing_strategy_t select_routing_strategy(routing_optimizer_ctx_t* ctx, const network_conditions_t* conditions);
double calculate_path_score(routing_optimizer_ctx_t* ctx, const network_path_t* path, routing_strategy_t strategy);

// Path health monitoring
int update_path_health(routing_optimizer_ctx_t* ctx, uint32_t path_id, const path_health_t* health);
path_health_t* get_path_health(routing_optimizer_ctx_t* ctx, uint32_t path_id);
bool is_path_healthy(routing_optimizer_ctx_t* ctx, uint32_t path_id);
int perform_health_check(routing_optimizer_ctx_t* ctx, uint32_t path_id);
void update_all_path_health(routing_optimizer_ctx_t* ctx);

// Predictive routing
int enable_predictive_routing(routing_optimizer_ctx_t* ctx);
int disable_predictive_routing(routing_optimizer_ctx_t* ctx);
bool is_predictive_routing_enabled(routing_optimizer_ctx_t* ctx);
network_path_t* predict_best_path(routing_optimizer_ctx_t* ctx, 
                                uint32_t destination_ip, 
                                uint16_t destination_port,
                                int prediction_horizon_seconds);

// Load balancing
int enable_load_balancing(routing_optimizer_ctx_t* ctx);
int disable_load_balancing(routing_optimizer_ctx_t* ctx);
network_path_t* select_load_balanced_path(routing_optimizer_ctx_t* ctx, 
                                        uint32_t destination_ip,
                                        uint16_t destination_port,
                                        const network_conditions_t* conditions);
double calculate_load_distribution(routing_optimizer_ctx_t* ctx, uint32_t path_id);

// Failover management
int enable_failover(routing_optimizer_ctx_t* ctx);
int disable_failover(routing_optimizer_ctx_t* ctx);
network_path_t* get_failover_path(routing_optimizer_ctx_t* ctx, uint32_t failed_path_id);
int handle_path_failure(routing_optimizer_ctx_t* ctx, uint32_t failed_path_id);
bool is_failover_enabled(routing_optimizer_ctx_t* ctx);

// Performance optimization
int optimize_routing_table(routing_optimizer_ctx_t* ctx);
int reoptimize_all_routes(routing_optimizer_ctx_t* ctx);
double get_current_routing_efficiency(routing_optimizer_ctx_t* ctx);
int update_routing_performance_metrics(routing_optimizer_ctx_t* ctx, 
                                     uint32_t path_id,
                                     double actual_latency,
                                     double actual_throughput,
                                     double actual_loss);

// Statistics and reporting
routing_stats_t get_routing_statistics(routing_optimizer_ctx_t* ctx);
void reset_routing_statistics(routing_optimizer_ctx_t* ctx);
void print_routing_report(routing_optimizer_ctx_t* ctx);
int get_top_performing_paths(routing_optimizer_ctx_t* ctx, network_path_t* paths, int max_paths);

// Utility functions
const char* routing_strategy_to_string(routing_strategy_t strategy);
const char* route_type_to_string(route_type_t type);
double calculate_network_distance(uint32_t ip1, uint32_t ip2);
bool is_network_condition_critical(const network_conditions_t* conditions);
int generate_path_hash(const network_path_t* path);

// Callback registration
void register_routing_decision_callback(routing_decision_callback_t callback);
void register_path_health_callback(path_health_callback_t callback);
void register_routing_stats_callback(routing_stats_callback_t callback);
void register_route_change_callback(route_change_callback_t callback);

// Integration functions
int integrate_with_network_layer(routing_optimizer_ctx_t* ctx);
int integrate_with_performance_monitor(routing_optimizer_ctx_t* ctx);
int apply_routing_optimizations(routing_optimizer_ctx_t* ctx);
int verify_routing_integrity(routing_optimizer_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // ROUTING_OPTIMIZER_H