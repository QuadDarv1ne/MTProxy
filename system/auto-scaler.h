/*
 * auto-scaler.h
 * Auto-Scaling Optimization Component for MTProxy
 *
 * This system provides automatic scaling capabilities based on system load,
 * connection count, and performance metrics.
 */

#ifndef AUTO_SCALER_H
#define AUTO_SCALER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Scaling policies
typedef enum {
    SCALING_POLICY_CONSERVATIVE = 0,
    SCALING_POLICY_AGGRESSIVE,
    SCALING_POLICY_ADAPTIVE,
    SCALING_POLICY_CUSTOM
} scaling_policy_t;

// Resource types that can be scaled
typedef enum {
    RESOURCE_TYPE_THREADS = 0,
    RESOURCE_TYPE_CONNECTIONS,
    RESOURCE_TYPE_MEMORY,
    RESOURCE_TYPE_BANDWIDTH,
    RESOURCE_TYPE_CPU
} resource_type_t;

// Scaling actions
typedef enum {
    SCALING_ACTION_NONE = 0,
    SCALING_ACTION_SCALE_UP,
    SCALING_ACTION_SCALE_DOWN,
    SCALING_ACTION_MAINTAIN
} scaling_action_t;

// Auto-scaler configuration
typedef struct {
    int enable_auto_scaling;
    scaling_policy_t policy;
    int min_resources[5];      // Minimum resources for each type
    int max_resources[5];      // Maximum resources for each type
    int target_utilization;    // Target utilization percentage (0-100)
    int scale_up_threshold;    // Threshold to trigger scale up (%)
    int scale_down_threshold;  // Threshold to trigger scale down (%)
    int cooldown_period_seconds;  // Cooldown period between scaling actions
    int evaluation_interval_seconds;  // How often to evaluate scaling needs
    double scale_up_multiplier;    // Multiplier for scale up operations
    double scale_down_multiplier;  // Multiplier for scale down operations
    int enable_predictive_scaling; // Enable predictive scaling based on trends
    int prediction_window_seconds; // Window for trend analysis
    bool enable_hysteresis;        // Enable hysteresis to prevent oscillation
    int hysteresis_threshold;      // Hysteresis threshold percentage
} auto_scaler_config_t;

// Resource metrics structure
typedef struct {
    resource_type_t type;
    int current_value;
    int target_value;
    int min_value;
    int max_value;
    double utilization_percent;
    uint64_t last_updated;
} resource_metrics_t;

// Scaling decision structure
typedef struct {
    scaling_action_t action;
    resource_type_t resource_type;
    int current_value;
    int new_value;
    int adjustment_amount;
    double confidence_score;  // 0.0 - 1.0
    uint64_t timestamp;
    char reason[256];
} scaling_decision_t;

// Auto-scaler statistics
typedef struct {
    uint64_t total_scaling_events;
    uint64_t scale_up_events;
    uint64_t scale_down_events;
    uint64_t no_action_events;
    uint64_t failed_scaling_attempts;
    double average_scaling_latency_ms;
    double scaling_accuracy;  // Percentage of correct scaling decisions
    uint64_t last_scaling_time;
    int current_resource_levels[5];
} auto_scaler_stats_t;

// Auto-scaler context
typedef struct {
    auto_scaler_config_t config;
    auto_scaler_stats_t stats;
    resource_metrics_t resources[5];
    scaling_decision_t decision_history[1000];
    int decision_history_index;
    uint64_t last_evaluation_time;
    uint64_t last_scaling_time;
    bool is_scaling_in_progress;
    int scaling_cooldown_counter;
    double utilization_trend[60];  // Last 60 utilization measurements
    int trend_index;
    void* resource_manager;  // Pointer to resource manager
} auto_scaler_ctx_t;

// Callback function types
typedef void (*scaling_decision_callback_t)(const scaling_decision_t* decision);
typedef int (*resource_query_callback_t)(resource_type_t type, int* current_value, int* max_value);
typedef int (*resource_adjust_callback_t)(resource_type_t type, int new_value);
typedef void (*scaling_event_callback_t)(const char* event_description);

// Function prototypes

// Initialization and cleanup
int init_auto_scaler(auto_scaler_ctx_t* ctx);
int init_auto_scaler_with_config(auto_scaler_ctx_t* ctx, const auto_scaler_config_t* config);
void cleanup_auto_scaler(auto_scaler_ctx_t* ctx);

// Configuration management
void get_auto_scaler_config(auto_scaler_ctx_t* ctx, auto_scaler_config_t* config);
int set_auto_scaler_config(auto_scaler_ctx_t* ctx, const auto_scaler_config_t* config);
int load_auto_scaler_config(auto_scaler_config_t* config, const char* config_file);
int save_auto_scaler_config(const auto_scaler_config_t* config, const char* config_file);

// Resource management
int register_resource_manager(auto_scaler_ctx_t* ctx, void* resource_manager);
int update_resource_metrics(auto_scaler_ctx_t* ctx, resource_type_t type, 
                           int current_value, int max_value);
int get_current_resource_level(auto_scaler_ctx_t* ctx, resource_type_t type);
int get_target_resource_level(auto_scaler_ctx_t* ctx, resource_type_t type);

// Scaling decision making
scaling_decision_t evaluate_scaling_needs(auto_scaler_ctx_t* ctx);
int execute_scaling_decision(auto_scaler_ctx_t* ctx, const scaling_decision_t* decision);
int apply_scaling_policy(auto_scaler_ctx_t* ctx);
scaling_action_t determine_scaling_action(auto_scaler_ctx_t* ctx, resource_type_t type);

// Predictive scaling
int enable_predictive_scaling(auto_scaler_ctx_t* ctx);
int disable_predictive_scaling(auto_scaler_ctx_t* ctx);
double predict_future_utilization(auto_scaler_ctx_t* ctx, int prediction_horizon_seconds);
int update_utilization_trend(auto_scaler_ctx_t* ctx, double current_utilization);

// Performance monitoring
double get_current_system_utilization(auto_scaler_ctx_t* ctx);
double get_average_utilization(auto_scaler_ctx_t* ctx, int window_size);
int get_utilization_trend(auto_scaler_ctx_t* ctx, double* trend_slope, double* trend_confidence);

// Statistics and reporting
auto_scaler_stats_t get_auto_scaler_statistics(auto_scaler_ctx_t* ctx);
void reset_auto_scaler_statistics(auto_scaler_ctx_t* ctx);
void print_scaling_report(auto_scaler_ctx_t* ctx);

// Control functions
int start_auto_scaling(auto_scaler_ctx_t* ctx);
int stop_auto_scaling(auto_scaler_ctx_t* ctx);
bool is_auto_scaling_active(auto_scaler_ctx_t* ctx);
int force_scaling_evaluation(auto_scaler_ctx_t* ctx);

// Manual scaling
int manual_scale_up(auto_scaler_ctx_t* ctx, resource_type_t type, int amount);
int manual_scale_down(auto_scaler_ctx_t* ctx, resource_type_t type, int amount);
int set_resource_level(auto_scaler_ctx_t* ctx, resource_type_t type, int target_level);

// Utility functions
const char* scaling_policy_to_string(scaling_policy_t policy);
const char* resource_type_to_string(resource_type_t type);
const char* scaling_action_to_string(scaling_action_t action);
double calculate_scaling_multiplier(auto_scaler_ctx_t* ctx, resource_type_t type);
int validate_scaling_decision(auto_scaler_ctx_t* ctx, const scaling_decision_t* decision);

// Callback registration
void register_scaling_decision_callback(scaling_decision_callback_t callback);
void register_resource_query_callback(resource_query_callback_t callback);
void register_resource_adjust_callback(resource_adjust_callback_t callback);
void register_scaling_event_callback(scaling_event_callback_t callback);

// Integration functions
int integrate_with_performance_monitor(auto_scaler_ctx_t* ctx);
int integrate_with_resource_manager(auto_scaler_ctx_t* ctx);
int apply_auto_scaling(auto_scaler_ctx_t* ctx);
int verify_scaling_operations(auto_scaler_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // AUTO_SCALER_H