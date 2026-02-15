/*
 * traffic-optimizer.h
 * Machine Learning-Based Traffic Optimization System
 *
 * This system uses machine learning algorithms to analyze traffic patterns
 * and optimize routing, resource allocation, and performance automatically.
 */

#ifndef TRAFFIC_OPTIMIZER_H
#define TRAFFIC_OPTIMIZER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Traffic pattern types
typedef enum {
    TRAFFIC_PATTERN_UNKNOWN = 0,
    TRAFFIC_PATTERN_STEADY,
    TRAFFIC_PATTERN_BURSTY,
    TRAFFIC_PATTERN_PERIODIC,
    TRAFFIC_PATTERN_SPIKE,
    TRAFFIC_PATTERN_DECLINING
} traffic_pattern_t;

// Optimization strategies
typedef enum {
    OPT_STRATEGY_LATENCY = 0,
    OPT_STRATEGY_THROUGHPUT,
    OPT_STRATEGY_BALANCED,
    OPT_STRATEGY_COST_EFFICIENT,
    OPT_STRATEGY_RELIABILITY
} optimization_strategy_t;

// ML model types
typedef enum {
    MODEL_TYPE_LINEAR_REGRESSION = 0,
    MODEL_TYPE_DECISION_TREE,
    MODEL_TYPE_NEURAL_NETWORK,
    MODEL_TYPE_RANDOM_FOREST,
    MODEL_TYPE_GRADIENT_BOOSTING
} ml_model_type_t;

// Traffic features structure
typedef struct {
    uint64_t timestamp;
    uint64_t bytes_transferred;
    uint64_t packet_count;
    uint32_t connection_count;
    double avg_packet_size;
    double bytes_per_second;
    double packets_per_second;
    uint32_t source_ip_hash;
    uint16_t destination_port;
    uint8_t protocol_type;
    int8_t geolocation_id;
    bool is_encrypted;
    double latency_ms;
    double packet_loss_rate;
    double jitter_ms;
} traffic_features_t;

// Prediction result structure
typedef struct {
    double predicted_latency;
    double predicted_throughput;
    double predicted_packet_loss;
    traffic_pattern_t predicted_pattern;
    int confidence_score;  // 0-100
    uint64_t prediction_timestamp;
    optimization_strategy_t recommended_strategy;
} prediction_result_t;

// Traffic optimizer configuration
typedef struct {
    int enable_ml_optimization;
    ml_model_type_t primary_model_type;
    int training_window_seconds;
    int prediction_horizon_seconds;
    int retraining_interval_seconds;
    double learning_rate;
    int max_training_samples;
    bool enable_online_learning;
    bool enable_ensemble_methods;
    int feature_selection_threshold;
    optimization_strategy_t default_strategy;
    int adaptation_threshold_percent;
    bool enable_auto_tuning;
} traffic_optimizer_config_t;

// Model statistics
typedef struct {
    uint64_t total_predictions;
    uint64_t accurate_predictions;
    uint64_t model_updates;
    uint64_t training_samples_processed;
    double model_accuracy;
    double average_prediction_error;
    uint64_t last_training_time;
    uint64_t next_retraining_time;
} model_stats_t;

// Traffic optimizer context
typedef struct {
    traffic_optimizer_config_t config;
    model_stats_t stats;
    traffic_features_t* training_data;
    int training_data_count;
    prediction_result_t* prediction_history;
    int prediction_history_count;
    void* ml_models[5];  // Pointers to different model instances
    int active_model_index;
    uint64_t last_prediction_time;
    uint64_t last_training_time;
    bool is_training_in_progress;
    optimization_strategy_t current_strategy;
    double current_performance_score;
} traffic_optimizer_ctx_t;

// Callback function types
typedef void (*prediction_callback_t)(const prediction_result_t* prediction);
typedef void (*strategy_change_callback_t)(optimization_strategy_t old_strategy, optimization_strategy_t new_strategy);
typedef void (*model_update_callback_t)(const model_stats_t* stats);

// Function prototypes

// Initialization and cleanup
int init_traffic_optimizer(traffic_optimizer_ctx_t* ctx);
int init_traffic_optimizer_with_config(traffic_optimizer_ctx_t* ctx, const traffic_optimizer_config_t* config);
void cleanup_traffic_optimizer(traffic_optimizer_ctx_t* ctx);

// Configuration management
void get_traffic_optimizer_config(traffic_optimizer_ctx_t* ctx, traffic_optimizer_config_t* config);
int set_traffic_optimizer_config(traffic_optimizer_ctx_t* ctx, const traffic_optimizer_config_t* config);

// Data collection and training
int add_traffic_sample(traffic_optimizer_ctx_t* ctx, const traffic_features_t* features);
int train_models(traffic_optimizer_ctx_t* ctx);
int retrain_models_if_needed(traffic_optimizer_ctx_t* ctx);
bool is_retraining_needed(traffic_optimizer_ctx_t* ctx);

// Prediction and optimization
prediction_result_t predict_traffic_behavior(traffic_optimizer_ctx_t* ctx, const traffic_features_t* current_features);
optimization_strategy_t recommend_optimization_strategy(traffic_optimizer_ctx_t* ctx, const prediction_result_t* prediction);
int apply_optimization_strategy(traffic_optimizer_ctx_t* ctx, optimization_strategy_t strategy);
int update_performance_metrics(traffic_optimizer_ctx_t* ctx, double actual_latency, double actual_throughput, double actual_loss);

// Model management
int load_ml_models(traffic_optimizer_ctx_t* ctx, const char* model_path);
int save_ml_models(traffic_optimizer_ctx_t* ctx, const char* model_path);
int select_best_model(traffic_optimizer_ctx_t* ctx);
double evaluate_model_performance(traffic_optimizer_ctx_t* ctx, int model_index);

// Feature engineering
traffic_features_t extract_traffic_features(const uint8_t* packet_data, size_t packet_size, uint32_t source_ip, uint16_t dest_port);
int normalize_features(traffic_features_t* features);
int calculate_feature_importance(traffic_optimizer_ctx_t* ctx, double* importance_scores);

// Strategy management
optimization_strategy_t get_current_strategy(traffic_optimizer_ctx_t* ctx);
int set_optimization_strategy(traffic_optimizer_ctx_t* ctx, optimization_strategy_t strategy);
double calculate_strategy_effectiveness(traffic_optimizer_ctx_t* ctx, optimization_strategy_t strategy);

// Statistics and reporting
model_stats_t get_model_statistics(traffic_optimizer_ctx_t* ctx);
void reset_model_statistics(traffic_optimizer_ctx_t* ctx);
void print_optimization_report(traffic_optimizer_ctx_t* ctx);

// Utility functions
const char* traffic_pattern_to_string(traffic_pattern_t pattern);
const char* optimization_strategy_to_string(optimization_strategy_t strategy);
const char* ml_model_type_to_string(ml_model_type_t model_type);
double calculate_traffic_utilization(const traffic_features_t* features);
bool is_traffic_anomalous(traffic_optimizer_ctx_t* ctx, const traffic_features_t* features);

// Callback registration
void register_prediction_callback(prediction_callback_t callback);
void register_strategy_change_callback(strategy_change_callback_t callback);
void register_model_update_callback(model_update_callback_t callback);

// Integration functions
int integrate_with_network_layer(traffic_optimizer_ctx_t* ctx);
int integrate_with_performance_monitor(traffic_optimizer_ctx_t* ctx);
int apply_traffic_optimizations(traffic_optimizer_ctx_t* ctx);
int verify_optimization_effectiveness(traffic_optimizer_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // TRAFFIC_OPTIMIZER_H