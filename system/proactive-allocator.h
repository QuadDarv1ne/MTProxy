/*
 * proactive-allocator.h
 * Proactive Resource Allocation System
 *
 * This system proactively allocates and manages system resources based on
 * predicted demand, performance patterns, and optimization goals.
 */

#ifndef PROACTIVE_ALLOCATOR_H
#define PROACTIVE_ALLOCATOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Resource types
typedef enum {
    RESOURCE_TYPE_CPU = 0,
    RESOURCE_TYPE_MEMORY,
    RESOURCE_TYPE_NETWORK_BANDWIDTH,
    RESOURCE_TYPE_DISK_IO,
    RESOURCE_TYPE_CONNECTIONS,
    RESOURCE_TYPE_THREADS,
    RESOURCE_TYPE_CRYPTO_BUFFERS,
    RESOURCE_TYPE_CACHE_MEMORY
} resource_type_t;

// Allocation strategies
typedef enum {
    STRATEGY_CONSERVATIVE = 0,
    STRATEGY_AGGRESSIVE,
    STRATEGY_BALANCED,
    STRATEGY_PREDICTIVE,
    STRATEGY_ADAPTIVE
} allocation_strategy_t;

// Resource request structure
typedef struct {
    uint64_t request_id;
    resource_type_t resource_type;
    uint64_t requested_amount;
    uint64_t minimum_required;
    uint64_t maximum_acceptable;
    uint64_t priority;  // 1-100 scale
    uint64_t deadline_ms;
    bool is_preemptible;
    char requester_id[64];
    char purpose[128];
    uint64_t request_time;
} resource_request_t;

// Resource allocation structure
typedef struct {
    uint64_t allocation_id;
    resource_request_t request;
    uint64_t allocated_amount;
    uint64_t allocation_time;
    uint64_t expiration_time;
    bool is_active;
    double utilization_efficiency;  // 0.0 - 100.0
    uint64_t usage_count;
    uint64_t last_access_time;
} resource_allocation_t;

// Resource pool structure
typedef struct {
    resource_type_t type;
    uint64_t total_capacity;
    uint64_t currently_allocated;
    uint64_t available_capacity;
    uint64_t reserved_capacity;
    double utilization_percentage;
    uint64_t allocation_count;
    uint64_t deallocation_count;
    uint64_t failed_allocations;
    double average_allocation_time_ms;
    double average_utilization_rate;
    uint64_t last_update_time;
} resource_pool_t;

// Proactive allocation configuration
typedef struct {
    int enable_proactive_allocation;
    allocation_strategy_t default_strategy;
    int prediction_horizon_seconds;
    double safety_margin_percent;
    int reallocation_interval_seconds;
    int max_concurrent_allocations;
    uint64_t minimum_allocation_unit;
    uint64_t maximum_allocation_unit;
    bool enable_overcommit;
    double overcommit_ratio;
    int garbage_collection_interval_seconds;
    double garbage_collection_threshold;
    bool enable_resource_sharing;
    int sharing_efficiency_target;
    bool enable_priority_scheduling;
    int high_priority_threshold;
} proactive_config_t;

// Resource demand forecast
typedef struct {
    resource_type_t resource_type;
    uint64_t forecasted_demand;
    uint64_t confidence_interval_min;
    uint64_t confidence_interval_max;
    double confidence_level;  // 0.0 - 1.0
    uint64_t forecast_time;
    uint64_t validity_period_seconds;
    char forecast_method[64];
    double trend_slope;
    bool is_increasing_trend;
} demand_forecast_t;

// Allocation optimization metrics
typedef struct {
    uint64_t total_allocations;
    uint64_t successful_allocations;
    uint64_t failed_allocations;
    uint64_t preempted_allocations;
    uint64_t reallocated_resources;
    double allocation_success_rate;
    double average_resource_utilization;
    double resource_efficiency_score;
    double cost_effectiveness_ratio;
    uint64_t peak_allocation_time;
    uint64_t last_optimization_time;
    double optimization_gain_percent;
} allocation_stats_t;

// Proactive allocator context
typedef struct {
    proactive_config_t config;
    allocation_stats_t stats;
    resource_pool_t resource_pools[8];
    int pool_count;
    resource_request_t* pending_requests;
    int pending_request_count;
    resource_allocation_t* active_allocations;
    int active_allocation_count;
    demand_forecast_t* demand_forecasts;
    int forecast_count;
    uint64_t last_allocation_time;
    uint64_t last_reallocation_time;
    uint64_t last_garbage_collection_time;
    bool is_optimizing;
    allocation_strategy_t current_strategy;
    void* allocation_algorithms[5];  // Pointers to different allocation algorithms
    int active_algorithm_index;
    double resource_efficiency_history[1000];
    int efficiency_history_index;
} proactive_allocator_ctx_t;

// Resource pressure indicators
typedef struct {
    resource_type_t resource_type;
    double current_pressure;  // 0.0 - 100.0
    double predicted_pressure;
    double pressure_trend;  // Positive = increasing, Negative = decreasing
    uint64_t pressure_timestamp;
    bool is_critical;
    char pressure_description[256];
    double mitigation_recommendation_score;
} resource_pressure_t;

// Allocation policy structure
typedef struct {
    char policy_name[64];
    allocation_strategy_t strategy;
    double priority_weight;
    double efficiency_weight;
    double cost_weight;
    double reliability_weight;
    bool enable_preemption;
    int preemption_grace_period_seconds;
    bool enable_resource_pinning;
    int pinning_duration_seconds;
    char resource_constraints[256];
} allocation_policy_t;

// Callback function types
typedef void (*allocation_callback_t)(const resource_allocation_t* allocation);
typedef void (*deallocation_callback_t)(const resource_allocation_t* allocation);
typedef void (*resource_pressure_callback_t)(const resource_pressure_t* pressure);
typedef void (*allocation_stats_callback_t)(const allocation_stats_t* stats);
typedef int (*resource_availability_callback_t)(resource_type_t type, uint64_t amount);

// Function prototypes

// Initialization and cleanup
int init_proactive_allocator(proactive_allocator_ctx_t* ctx);
int init_proactive_allocator_with_config(proactive_allocator_ctx_t* ctx, const proactive_config_t* config);
void cleanup_proactive_allocator(proactive_allocator_ctx_t* ctx);

// Configuration management
void get_proactive_config(proactive_allocator_ctx_t* ctx, proactive_config_t* config);
int set_proactive_config(proactive_allocator_ctx_t* ctx, const proactive_config_t* config);

// Resource pool management
int create_resource_pool(proactive_allocator_ctx_t* ctx, resource_type_t type, uint64_t capacity);
int update_resource_pool_capacity(proactive_allocator_ctx_t* ctx, resource_type_t type, uint64_t new_capacity);
int get_resource_pool_info(proactive_allocator_ctx_t* ctx, resource_type_t type, resource_pool_t* pool_info);
resource_pool_t* get_all_resource_pools(proactive_allocator_ctx_t* ctx, int* pool_count);
uint64_t get_available_resources(proactive_allocator_ctx_t* ctx, resource_type_t type);
uint64_t get_total_resources(proactive_allocator_ctx_t* ctx, resource_type_t type);

// Resource request management
uint64_t request_resources(proactive_allocator_ctx_t* ctx, const resource_request_t* request);
int cancel_resource_request(proactive_allocator_ctx_t* ctx, uint64_t request_id);
int modify_resource_request(proactive_allocator_ctx_t* ctx, uint64_t request_id, const resource_request_t* new_request);
resource_request_t* get_pending_requests(proactive_allocator_ctx_t* ctx, int* request_count);
int prioritize_requests(proactive_allocator_ctx_t* ctx);

// Resource allocation functions
int allocate_resources(proactive_allocator_ctx_t* ctx, uint64_t request_id);
int deallocate_resources(proactive_allocator_ctx_t* ctx, uint64_t allocation_id);
int reallocate_resources(proactive_allocator_ctx_t* ctx);
int optimize_resource_allocation(proactive_allocator_ctx_t* ctx);
resource_allocation_t* get_active_allocations(proactive_allocator_ctx_t* ctx, int* allocation_count);
resource_allocation_t* find_allocation_by_id(proactive_allocator_ctx_t* ctx, uint64_t allocation_id);

// Proactive allocation
int enable_proactive_allocation(proactive_allocator_ctx_t* ctx);
int disable_proactive_allocation(proactive_allocator_ctx_t* ctx);
bool is_proactive_allocation_enabled(proactive_allocator_ctx_t* ctx);
int perform_proactive_allocation_cycle(proactive_allocator_ctx_t* ctx);
demand_forecast_t* generate_demand_forecast(proactive_allocator_ctx_t* ctx, resource_type_t type, int horizon_seconds);
int update_demand_forecasts(proactive_allocator_ctx_t* ctx);

// Resource pressure monitoring
resource_pressure_t* monitor_resource_pressure(proactive_allocator_ctx_t* ctx, resource_type_t type);
int get_all_resource_pressures(proactive_allocator_ctx_t* ctx, resource_pressure_t* pressures, int max_pressures);
bool is_resource_critical(proactive_allocator_ctx_t* ctx, resource_type_t type);
double get_resource_pressure_score(proactive_allocator_ctx_t* ctx, resource_type_t type);
int mitigate_resource_pressure(proactive_allocator_ctx_t* ctx, resource_type_t type);

// Allocation policy management
int register_allocation_policy(proactive_allocator_ctx_t* ctx, const allocation_policy_t* policy);
int unregister_allocation_policy(proactive_allocator_ctx_t* ctx, const char* policy_name);
allocation_policy_t* get_allocation_policy(proactive_allocator_ctx_t* ctx, const char* policy_name);
int set_active_allocation_policy(proactive_allocator_ctx_t* ctx, const char* policy_name);
allocation_strategy_t determine_optimal_strategy(proactive_allocator_ctx_t* ctx);

// Garbage collection
int enable_garbage_collection(proactive_allocator_ctx_t* ctx);
int disable_garbage_collection(proactive_allocator_ctx_t* ctx);
int perform_garbage_collection(proactive_allocator_ctx_t* ctx);
int force_garbage_collection(proactive_allocator_ctx_t* ctx);
uint64_t get_garbage_collection_stats(proactive_allocator_ctx_t* ctx);

// Performance optimization
int optimize_allocation_efficiency(proactive_allocator_ctx_t* ctx);
double get_current_efficiency_score(proactive_allocator_ctx_t* ctx);
double get_average_efficiency_score(proactive_allocator_ctx_t* ctx, int window_size);
int get_efficiency_history(proactive_allocator_ctx_t* ctx, double* history, int max_samples);
double calculate_cost_effectiveness(proactive_allocator_ctx_t* ctx);

// Statistics and reporting
allocation_stats_t get_allocation_statistics(proactive_allocator_ctx_t* ctx);
void reset_allocation_statistics(proactive_allocator_ctx_t* ctx);
void print_allocation_report(proactive_allocator_ctx_t* ctx);
int export_allocation_data(proactive_allocator_ctx_t* ctx, const char* filename);

// Utility functions
const char* resource_type_to_string(resource_type_t type);
const char* allocation_strategy_to_string(allocation_strategy_t strategy);
uint64_t calculate_optimal_allocation_size(proactive_allocator_ctx_t* ctx, resource_type_t type, uint64_t demand);
bool is_allocation_feasible(proactive_allocator_ctx_t* ctx, const resource_request_t* request);
double calculate_allocation_priority(proactive_allocator_ctx_t* ctx, const resource_request_t* request);
uint64_t estimate_allocation_time(proactive_allocator_ctx_t* ctx, resource_type_t type, uint64_t amount);

// Callback registration
void register_allocation_callback(allocation_callback_t callback);
void register_deallocation_callback(deallocation_callback_t callback);
void register_resource_pressure_callback(resource_pressure_callback_t callback);
void register_allocation_stats_callback(allocation_stats_callback_t callback);
void register_resource_availability_callback(resource_availability_callback_t callback);

// Integration functions
int integrate_with_predictive_optimizer(proactive_allocator_ctx_t* ctx);
int integrate_with_auto_scaler(proactive_allocator_ctx_t* ctx);
int integrate_with_memory_manager(proactive_allocator_ctx_t* ctx);
int apply_proactive_allocations(proactive_allocator_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // PROACTIVE_ALLOCATOR_H