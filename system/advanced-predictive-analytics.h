/*
 * advanced-predictive-analytics.h
 * Advanced Predictive Analytics System for MTProxy
 *
 * Implements sophisticated machine learning-based predictive analytics for
 * performance forecasting, anomaly detection, and proactive optimization.
 */

#ifndef ADVANCED_PREDICTIVE_ANALYTICS_H
#define ADVANCED_PREDICTIVE_ANALYTICS_H

#include <stdint.h>
#include <stdbool.h>

// Data types for predictive analytics
typedef enum {
    DATA_TYPE_CPU_UTILIZATION = 0,
    DATA_TYPE_MEMORY_USAGE = 1,
    DATA_TYPE_NETWORK_LATENCY = 2,
    DATA_TYPE_THROUGHPUT = 3,
    DATA_TYPE_CONNECTION_COUNT = 4,
    DATA_TYPE_ERROR_RATE = 5,
    DATA_TYPE_BANDWIDTH_USAGE = 6,
    DATA_TYPE_CRYPTO_PERFORMANCE = 7,
    DATA_TYPE_CACHE_HIT_RATIO = 8,
    DATA_TYPE_QUEUE_DEPTH = 9
} analytics_data_type_t;

// Prediction models
typedef enum {
    MODEL_TYPE_LINEAR_REGRESSION = 0,
    MODEL_TYPE_RANDOM_FOREST = 1,
    MODEL_TYPE_NEURAL_NETWORK = 2,
    MODEL_TYPE_ARIMA = 3,
    MODEL_TYPE_LSTM = 4,
    MODEL_TYPE_GRADIENT_BOOSTING = 5,
    MODEL_TYPE_SUPPORT_VECTOR_MACHINE = 6
} prediction_model_type_t;

// Anomaly types
typedef enum {
    ANOMALY_TYPE_PERFORMANCE_DEGRADATION = 0,
    ANOMALY_TYPE_RESOURCE_EXHAUSTION = 1,
    ANOMALY_TYPE_SECURITY_THREAT = 2,
    ANOMALY_TYPE_NETWORK_ISSUE = 3,
    ANOMALY_TYPE_CONFIGURATION_PROBLEM = 4,
    ANOMALY_TYPE_HARDWARE_FAILURE = 5
} anomaly_type_t;

// Confidence levels
typedef enum {
    CONFIDENCE_VERY_LOW = 0,
    CONFIDENCE_LOW = 1,
    CONFIDENCE_MEDIUM = 2,
    CONFIDENCE_HIGH = 3,
    CONFIDENCE_VERY_HIGH = 4
} confidence_level_t;

// Time series data point
typedef struct {
    uint64_t timestamp;
    double value;
    double confidence_interval_lower;
    double confidence_interval_upper;
    bool is_anomaly;
    anomaly_type_t anomaly_type;
    char anomaly_description[128];
} time_series_point_t;

// Feature vector for ML models
typedef struct {
    double features[32];  // Up to 32 features
    int feature_count;
    double target_value;
    uint64_t timestamp;
    char feature_names[32][32];  // Names for each feature
} feature_vector_t;

// Prediction result
typedef struct {
    uint64_t prediction_id;
    analytics_data_type_t data_type;
    prediction_model_type_t model_used;
    uint64_t prediction_timestamp;
    double predicted_value;
    double confidence_interval_lower;
    double confidence_interval_upper;
    confidence_level_t confidence_level;
    double accuracy_score;  // 0.0 - 1.0
    uint64_t time_horizon_seconds;
    char model_version[32];
    bool is_anomaly_predicted;
    anomaly_type_t predicted_anomaly_type;
    double anomaly_probability;  // 0.0 - 1.0
    char prediction_description[256];
} prediction_result_t;

// Model performance metrics
typedef struct {
    double mean_absolute_error;
    double mean_squared_error;
    double root_mean_squared_error;
    double mean_absolute_percentage_error;
    double r_squared;
    double adjusted_r_squared;
    uint64_t training_samples;
    uint64_t validation_samples;
    uint64_t test_samples;
    double training_time_seconds;
    uint64_t model_size_bytes;
    char last_training_timestamp[32];
} model_performance_t;

// Anomaly detection result
typedef struct {
    uint64_t detection_id;
    anomaly_type_t anomaly_type;
    analytics_data_type_t affected_metric;
    uint64_t detection_timestamp;
    double anomaly_score;  // 0.0 - 100.0
    confidence_level_t confidence_level;
    double baseline_value;
    double current_value;
    double deviation_percentage;
    char anomaly_description[256];
    char recommended_action[256];
    bool requires_immediate_attention;
    uint64_t estimated_impact_duration_seconds;
} anomaly_detection_result_t;

// Predictive analytics configuration
typedef struct {
    bool enable_predictions;
    bool enable_anomaly_detection;
    bool enable_auto_model_selection;
    bool enable_online_learning;
    int prediction_horizon_seconds;
    int data_collection_interval_seconds;
    int model_retraining_interval_hours;
    double anomaly_threshold;  // 0.0 - 100.0
    int max_historical_data_points;
    int ensemble_model_count;
    prediction_model_type_t primary_model_type;
    prediction_model_type_t fallback_model_type;
    double minimum_confidence_threshold;
    bool enable_feature_selection;
    int feature_selection_threshold;
    bool enable_cross_validation;
    int cross_validation_folds;
    bool enable_ensemble_voting;
    double ensemble_voting_threshold;
    bool enable_drift_detection;
    double concept_drift_threshold;
    bool enable_explainable_ai;
    int max_explanation_features;
} predictive_analytics_config_t;

// Time series dataset
typedef struct {
    analytics_data_type_t data_type;
    time_series_point_t* data_points;
    int point_count;
    int max_points;
    uint64_t first_timestamp;
    uint64_t last_timestamp;
    double mean_value;
    double standard_deviation;
    double min_value;
    double max_value;
    bool is_stationary;
    double trend_slope;
    double seasonality_strength;
} time_series_dataset_t;

// ML Model context
typedef struct {
    prediction_model_type_t model_type;
    void* model_data;
    model_performance_t performance;
    feature_vector_t* training_data;
    int training_sample_count;
    int max_training_samples;
    bool is_trained;
    bool is_active;
    char model_name[64];
    char model_version[32];
    uint64_t last_training_timestamp;
    uint64_t next_retraining_timestamp;
    double current_accuracy;
    int prediction_count;
    int correct_predictions;
} ml_model_context_t;

// Ensemble model manager
typedef struct {
    ml_model_context_t* models;
    int model_count;
    int max_models;
    prediction_model_type_t ensemble_strategy;
    double* model_weights;
    bool enable_weighted_voting;
    bool enable_dynamic_weighting;
    double consensus_threshold;
    int successful_ensemble_predictions;
    int total_ensemble_predictions;
} ensemble_model_manager_t;

// Predictive analytics context
typedef struct {
    // Configuration
    predictive_analytics_config_t config;
    
    // Data management
    time_series_dataset_t datasets[16];  // One per data type
    int dataset_count;
    
    // ML Models
    ensemble_model_manager_t ensemble_manager;
    ml_model_context_t primary_models[16];  // One per data type
    int primary_model_count;
    
    // Current state
    feature_vector_t current_features;
    uint64_t last_prediction_timestamp;
    uint64_t last_anomaly_detection_timestamp;
    uint64_t last_model_retraining_timestamp;
    bool analytics_enabled;
    bool learning_mode;
    
    // Results storage
    prediction_result_t* prediction_history;
    int prediction_history_count;
    int max_prediction_history;
    anomaly_detection_result_t* anomaly_history;
    int anomaly_history_count;
    int max_anomaly_history;
    
    // Performance tracking
    uint64_t total_predictions;
    uint64_t accurate_predictions;
    uint64_t total_anomalies_detected;
    uint64_t true_positives;
    uint64_t false_positives;
    uint64_t false_negatives;
    double overall_accuracy;
    double anomaly_detection_rate;
    
    // Resource management
    uint64_t memory_usage_bytes;
    int active_threads;
    bool resource_constrained;
    
    // State management
    int initialized;
    int active;
    uint64_t start_time;
    char system_id[64];
} predictive_analytics_context_t;

// Callback function types
typedef void (*prediction_callback_t)(const prediction_result_t* prediction);
typedef void (*anomaly_callback_t)(const anomaly_detection_result_t* anomaly);
typedef void (*model_update_callback_t)(prediction_model_type_t model_type, 
                                      const model_performance_t* performance);
typedef void (*drift_detection_callback_t)(analytics_data_type_t data_type, 
                                         double drift_score, const char* description);

// Function declarations

// Initialization and cleanup
int init_predictive_analytics(predictive_analytics_context_t* ctx);
int init_predictive_analytics_with_config(predictive_analytics_context_t* ctx, 
                                        const predictive_analytics_config_t* config);
void cleanup_predictive_analytics(predictive_analytics_context_t* ctx);

// Configuration management
void get_predictive_config(predictive_analytics_context_t* ctx, predictive_analytics_config_t* config);
int set_predictive_config(predictive_analytics_context_t* ctx, const predictive_analytics_config_t* config);
int enable_predictive_analytics(predictive_analytics_context_t* ctx, bool enable);
int set_anomaly_threshold(predictive_analytics_context_t* ctx, double threshold);

// Data collection and management
int add_data_point(predictive_analytics_context_t* ctx, analytics_data_type_t data_type,
                  uint64_t timestamp, double value);
int add_feature_vector(predictive_analytics_context_t* ctx, const feature_vector_t* features);
int collect_current_metrics(predictive_analytics_context_t* ctx);
time_series_dataset_t* get_time_series_dataset(predictive_analytics_context_t* ctx, 
                                             analytics_data_type_t data_type);
int preprocess_data(predictive_analytics_context_t* ctx, analytics_data_type_t data_type);

// Model management
int initialize_ml_model(predictive_analytics_context_t* ctx, prediction_model_type_t model_type,
                       analytics_data_type_t data_type);
int train_model(predictive_analytics_context_t* ctx, prediction_model_type_t model_type,
               analytics_data_type_t data_type);
int retrain_models(predictive_analytics_context_t* ctx);
model_performance_t get_model_performance(predictive_analytics_context_t* ctx, 
                                        prediction_model_type_t model_type,
                                        analytics_data_type_t data_type);
int select_best_model(predictive_analytics_context_t* ctx, analytics_data_type_t data_type);

// Prediction functions
prediction_result_t predict_future_value(predictive_analytics_context_t* ctx, 
                                       analytics_data_type_t data_type,
                                       uint64_t time_horizon_seconds);
int predict_multiple_metrics(predictive_analytics_context_t* ctx, 
                           prediction_result_t* results, int max_results);
double get_prediction_confidence(predictive_analytics_context_t* ctx, 
                               analytics_data_type_t data_type);
int validate_prediction(predictive_analytics_context_t* ctx, 
                       const prediction_result_t* prediction, double actual_value);

// Anomaly detection
anomaly_detection_result_t detect_anomalies(predictive_analytics_context_t* ctx,
                                          analytics_data_type_t data_type);
int detect_anomalies_in_all_metrics(predictive_analytics_context_t* ctx);
int update_anomaly_baseline(predictive_analytics_context_t* ctx, 
                           analytics_data_type_t data_type);
double calculate_anomaly_score(predictive_analytics_context_t* ctx, 
                             analytics_data_type_t data_type, double value);
int get_anomaly_threshold(predictive_analytics_context_t* ctx, analytics_data_type_t data_type);

// Ensemble methods
int initialize_ensemble(predictive_analytics_context_t* ctx);
int add_model_to_ensemble(predictive_analytics_context_t* ctx, 
                         prediction_model_type_t model_type);
prediction_result_t ensemble_predict(predictive_analytics_context_t* ctx, 
                                   analytics_data_type_t data_type,
                                   uint64_t time_horizon_seconds);
int update_ensemble_weights(predictive_analytics_context_t* ctx);
double get_ensemble_confidence(predictive_analytics_context_t* ctx, 
                             analytics_data_type_t data_type);

// Advanced analytics
int detect_concept_drift(predictive_analytics_context_t* ctx, 
                        analytics_data_type_t data_type);
int perform_feature_selection(predictive_analytics_context_t* ctx, 
                             analytics_data_type_t data_type);
int explain_prediction(predictive_analytics_context_t* ctx, 
                      const prediction_result_t* prediction, char* explanation, int max_length);
int identify_performance_patterns(predictive_analytics_context_t* ctx);

// Statistics and reporting
void get_analytics_statistics(predictive_analytics_context_t* ctx, 
                             uint64_t* total_predictions, uint64_t* accurate_predictions,
                             double* accuracy_rate, uint64_t* anomalies_detected);
int get_prediction_history(predictive_analytics_context_t* ctx, 
                          prediction_result_t* history, int max_count);
int get_anomaly_history(predictive_analytics_context_t* ctx, 
                       anomaly_detection_result_t* history, int max_count);
double get_system_health_score(predictive_analytics_context_t* ctx);

// Callback registration
void register_prediction_callback(prediction_callback_t callback);
void register_anomaly_callback(anomaly_callback_t callback);
void register_model_update_callback(model_update_callback_t callback);
void register_drift_detection_callback(drift_detection_callback_t callback);

// Integration functions
int integrate_with_performance_monitor(predictive_analytics_context_t* ctx);
int integrate_with_adaptive_protocol_manager(predictive_analytics_context_t* ctx);
int apply_predictive_optimizations(predictive_analytics_context_t* ctx);
int verify_analytics_integrity(predictive_analytics_context_t* ctx);

// Utility functions
const char* data_type_to_string(analytics_data_type_t data_type);
const char* model_type_to_string(prediction_model_type_t model_type);
const char* anomaly_type_to_string(anomaly_type_t anomaly_type);
const char* confidence_level_to_string(confidence_level_t confidence);
analytics_data_type_t string_to_data_type(const char* str);
prediction_model_type_t string_to_model_type(const char* str);
double calculate_correlation(const double* series1, const double* series2, int length);
int normalize_time_series(double* series, int length, double* normalized);
int detect_seasonality(const double* series, int length, double* seasonality_score);

#endif // ADVANCED_PREDICTIVE_ANALYTICS_H