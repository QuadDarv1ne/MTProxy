/*
 * predictive-optimizer.h
 * Predictive Performance Optimization System
 *
 * This system uses machine learning and statistical analysis to predict
 * performance degradation before it occurs and automatically take
 * preventive actions to maintain optimal performance.
 */

#ifndef PREDICTIVE_OPTIMIZER_H
#define PREDICTIVE_OPTIMIZER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Degradation prediction types
typedef enum {
    DEGRADATION_TYPE_UNKNOWN = 0,
    DEGRADATION_TYPE_CPU_PRESSURE,
    DEGRADATION_TYPE_MEMORY_PRESSURE,
    DEGRADATION_TYPE_NETWORK_LATENCY,
    DEGRADATION_TYPE_DISK_IO,
    DEGRADATION_TYPE_CONNECTION_SATURATION,
    DEGRADATION_TYPE_CRYPTO_PERFORMANCE,
    DEGRADATION_TYPE_BANDWIDTH_LIMITATION
} degradation_type_t;

// Prediction confidence levels
typedef enum {
    CONFIDENCE_LOW = 0,
    CONFIDENCE_MEDIUM,
    CONFIDENCE_HIGH,
    CONFIDENCE_CRITICAL
} prediction_confidence_t;

// Preventive action types
typedef enum {
    ACTION_NONE = 0,
    ACTION_SCALE_RESOURCES,
    ACTION_REBALANCE_LOAD,
    ACTION_PREALLOCATE_MEMORY,
    ACTION_OPTIMIZE_CONNECTIONS,
    ACTION_ADJUST_CRYPTO_PARAMETERS,
    ACTION_ENABLE_CACHING,
    ACTION_THROTTLE_NON_CRITICAL,
    ACTION_REDIRECT_TRAFFIC,
    ACTION_PREEMPTIVE_CLEANUP
} preventive_action_t;

// Performance metrics structure
typedef struct {
    uint64_t timestamp;
    double cpu_utilization_percent;
    double memory_utilization_percent;
    double network_latency_ms;
    double disk_io_wait_time_ms;
    uint64_t active_connections;
    uint64_t pending_connections;
    double throughput_mbps;
    double crypto_operation_time_ms;
    double cache_hit_ratio;
    double error_rate_percent;
    uint64_t queue_depth;
    double bandwidth_utilization_percent;
    uint64_t memory_pressure_score;  // 0-100 scale
    uint64_t cpu_pressure_score;     // 0-100 scale
} performance_metrics_t;

// Prediction result structure
typedef struct {
    uint64_t prediction_id;
    degradation_type_t predicted_degradation;
    prediction_confidence_t confidence_level;
    uint64_t predicted_time_to_degradation_ms;
    double severity_score;  // 0.0 - 100.0
    uint64_t prediction_timestamp;
    preventive_action_t recommended_action;
    char action_description[256];
    double prevention_effectiveness_score;  // Expected improvement 0.0 - 100.0
    bool action_executed;
    uint64_t execution_time;
} prediction_result_t;

// System health structure
typedef struct {
    double overall_health_score;  // 0.0 - 100.0
    double cpu_health_score;
    double memory_health_score;
    double network_health_score;
    double storage_health_score;
    double crypto_health_score;
    uint64_t last_health_check_time;
    bool is_stable;
    bool requires_attention;
    char health_status_message[256];
} system_health_t;

// Predictive optimizer configuration
typedef struct {
    int enable_predictive_optimization;
    int prediction_window_seconds;
    int retraining_interval_seconds;
    double degradation_threshold_percent;
    int min_confidence_level;
    bool enable_automatic_prevention;
    int preventive_action_timeout_seconds;
    int health_check_interval_seconds;
    double critical_health_threshold;
    int max_predictions_to_keep;
    bool enable_performance_forecasting;
    int forecasting_horizon_seconds;
    bool enable_resource_preallocation;
    double preallocation_threshold_percent;
    bool enable_adaptive_tuning;
    int tuning_interval_seconds;
} predictive_config_t;

// Degradation model statistics
typedef struct {
    uint64_t total_predictions_made;
    uint64_t accurate_predictions;
    uint64_t false_positives;
    uint64_t missed_degradations;
    uint64_t preventive_actions_taken;
    uint64_t successful_preventions;
    double prediction_accuracy_rate;
    double average_prediction_lead_time_ms;
    double average_prevention_effectiveness;
    uint64_t last_model_training_time;
    uint64_t next_training_time;
    double model_confidence_score;
} degradation_stats_t;

// Predictive optimizer context
typedef struct {
    predictive_config_t config;
    degradation_stats_t stats;
    performance_metrics_t* metrics_history;
    int metrics_count;
    prediction_result_t* prediction_history;
    int prediction_count;
    system_health_t current_health;
    void* ml_models[8];  // Pointers to different prediction models
    int active_model_index;
    uint64_t last_prediction_time;
    uint64_t last_health_check_time;
    uint64_t last_training_time;
    bool is_training_in_progress;
    bool is_predicting;
    double baseline_performance[10];  // Performance baselines for different metrics
    void* prevention_handlers[10];    // Handlers for different preventive actions
} predictive_optimizer_ctx_t;

// Degradation pattern structure
typedef struct {
    degradation_type_t type;
    double correlation_coefficient;
    int pattern_length_samples;
    uint64_t average_time_to_degradation_ms;
    char pattern_signature[128];
    bool is_recurring;
    int recurrence_count;
} degradation_pattern_t;

// Preventive action handler
typedef struct {
    preventive_action_t action_type;
    int (*execute_action)(predictive_optimizer_ctx_t* ctx, const prediction_result_t* prediction);
    int (*validate_action)(predictive_optimizer_ctx_t* ctx, const prediction_result_t* prediction);
    double (*estimate_effectiveness)(predictive_optimizer_ctx_t* ctx, const prediction_result_t* prediction);
    char action_name[64];
    bool is_enabled;
} prevention_handler_t;

// Callback function types
typedef void (*prediction_callback_t)(const prediction_result_t* prediction);
typedef void (*degradation_alert_callback_t)(degradation_type_t type, double severity, const char* message);
typedef void (*health_status_callback_t)(const system_health_t* health);
typedef void (*prevention_action_callback_t)(const prediction_result_t* prediction, bool success);

// Function prototypes

// Initialization and cleanup
int init_predictive_optimizer(predictive_optimizer_ctx_t* ctx);
int init_predictive_optimizer_with_config(predictive_optimizer_ctx_t* ctx, const predictive_config_t* config);
void cleanup_predictive_optimizer(predictive_optimizer_ctx_t* ctx);

// Configuration management
void get_predictive_config(predictive_optimizer_ctx_t* ctx, predictive_config_t* config);
int set_predictive_config(predictive_optimizer_ctx_t* ctx, const predictive_config_t* config);

// Performance monitoring and data collection
int add_performance_metrics(predictive_optimizer_ctx_t* ctx, const performance_metrics_t* metrics);
int collect_current_metrics(predictive_optimizer_ctx_t* ctx, performance_metrics_t* metrics);
performance_metrics_t get_latest_metrics(predictive_optimizer_ctx_t* ctx);
int get_metrics_history(predictive_optimizer_ctx_t* ctx, performance_metrics_t* metrics, int max_count);

// Prediction and analysis
prediction_result_t predict_performance_degradation(predictive_optimizer_ctx_t* ctx);
int retrain_prediction_models(predictive_optimizer_ctx_t* ctx);
bool is_degradation_imminent(predictive_optimizer_ctx_t* ctx, degradation_type_t* degradation_type);
degradation_pattern_t* detect_degradation_patterns(predictive_optimizer_ctx_t* ctx, int* pattern_count);
double calculate_performance_trend(predictive_optimizer_ctx_t* ctx, int metric_index, int time_window_seconds);

// System health monitoring
system_health_t assess_system_health(predictive_optimizer_ctx_t* ctx);
int perform_health_check(predictive_optimizer_ctx_t* ctx);
bool is_system_healthy(predictive_optimizer_ctx_t* ctx);
double get_health_score(predictive_optimizer_ctx_t* ctx);
int get_health_history(predictive_optimizer_ctx_t* ctx, system_health_t* health_history, int max_count);

// Preventive action management
int register_prevention_handler(predictive_optimizer_ctx_t* ctx, const prevention_handler_t* handler);
int execute_preventive_action(predictive_optimizer_ctx_t* ctx, const prediction_result_t* prediction);
int validate_prevention_effectiveness(predictive_optimizer_ctx_t* ctx, const prediction_result_t* prediction);
preventive_action_t suggest_preventive_action(predictive_optimizer_ctx_t* ctx, const prediction_result_t* prediction);

// Resource management
int preallocate_resources(predictive_optimizer_ctx_t* ctx, double utilization_threshold);
int rebalance_system_load(predictive_optimizer_ctx_t* ctx);
int optimize_memory_usage(predictive_optimizer_ctx_t* ctx);
int adjust_connection_limits(predictive_optimizer_ctx_t* ctx);
int tune_crypto_parameters(predictive_optimizer_ctx_t* ctx);

// Adaptive tuning
int enable_adaptive_tuning(predictive_optimizer_ctx_t* ctx);
int disable_adaptive_tuning(predictive_optimizer_ctx_t* ctx);
int apply_adaptive_optimizations(predictive_optimizer_ctx_t* ctx);
double get_current_tuning_score(predictive_optimizer_ctx_t* ctx);

// Model management
int load_degradation_models(predictive_optimizer_ctx_t* ctx, const char* model_path);
int save_degradation_models(predictive_optimizer_ctx_t* ctx, const char* model_path);
int update_performance_baselines(predictive_optimizer_ctx_t* ctx);
double evaluate_model_performance(predictive_optimizer_ctx_t* ctx);

// Statistics and reporting
degradation_stats_t get_degradation_statistics(predictive_optimizer_ctx_t* ctx);
void reset_degradation_statistics(predictive_optimizer_ctx_t* ctx);
void print_predictive_report(predictive_optimizer_ctx_t* ctx);
int export_prediction_data(predictive_optimizer_ctx_t* ctx, const char* filename);

// Utility functions
const char* degradation_type_to_string(degradation_type_t type);
const char* confidence_level_to_string(prediction_confidence_t confidence);
const char* preventive_action_to_string(preventive_action_t action);
double calculate_degradation_severity(const performance_metrics_t* current, const performance_metrics_t* baseline);
bool is_metric_degrading(double current_value, double baseline_value, double threshold_percent);
uint64_t estimate_time_to_degradation(double current_trend, double threshold, double current_value);

// Callback registration
void register_prediction_callback(prediction_callback_t callback);
void register_degradation_alert_callback(degradation_alert_callback_t callback);
void register_health_status_callback(health_status_callback_t callback);
void register_prevention_action_callback(prevention_action_callback_t callback);

// Integration functions
int integrate_with_performance_monitor(predictive_optimizer_ctx_t* ctx);
int integrate_with_auto_scaler(predictive_optimizer_ctx_t* ctx);
int integrate_with_compression_optimizer(predictive_optimizer_ctx_t* ctx);
int apply_predictive_optimizations(predictive_optimizer_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // PREDICTIVE_OPTIMIZER_H