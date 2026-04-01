/*
 * ML-based Performance Predictor for MTProxy
 * Uses machine learning to predict system performance and optimize resource usage
 */

#ifndef _ML_PERFORMANCE_PREDICTOR_H_
#define _ML_PERFORMANCE_PREDICTOR_H_

#include <stdint.h>
#include <stddef.h>

// Feature types for ML model
typedef enum {
    FEATURE_TYPE_CPU_USAGE = 0,
    FEATURE_TYPE_MEMORY_USAGE = 1,
    FEATURE_TYPE_NETWORK_THROUGHPUT = 2,
    FEATURE_TYPE_CONNECTION_COUNT = 3,
    FEATURE_TYPE_REQUEST_RATE = 4,
    FEATURE_TYPE_RESPONSE_TIME = 5,
    FEATURE_TYPE_ERROR_RATE = 6,
    FEATURE_TYPE_THREAD_COUNT = 7,
    FEATURE_TYPE_BUFFER_UTILIZATION = 8,
    FEATURE_TYPE_CRYPTO_LOAD = 9
} feature_type_t;

// Prediction targets
typedef enum {
    TARGET_LATENCY = 0,
    TARGET_THROUGHPUT = 1,
    TARGET_RESOURCE_UTILIZATION = 2,
    TARGET_ERROR_RATE = 3,
    TARGET_SYSTEM_STABILITY = 4
} prediction_target_t;

// Model types
typedef enum {
    MODEL_TYPE_LINEAR_REGRESSION = 0,
    MODEL_TYPE_DECISION_TREE = 1,
    MODEL_TYPE_NEURAL_NETWORK = 2,
    MODEL_TYPE_ENSEMBLE = 3
} model_type_t;

// Feature vector
typedef struct {
    double features[10];              // Feature values for different metrics
    int feature_count;
    long long timestamp;
} feature_vector_t;

// Training sample
typedef struct {
    feature_vector_t input_features;
    double target_values[5];          // Predicted values for different targets
    double actual_values[5];          // Actual values for training
    int target_count;
} training_sample_t;

// Prediction result
typedef struct {
    prediction_target_t target;
    double predicted_value;
    double confidence;                // 0.0-1.0 confidence in prediction
    double lower_bound;               // Lower confidence interval
    double upper_bound;               // Upper confidence interval
    long long prediction_timestamp;
    long long validity_period_seconds; // How long prediction is valid
} prediction_result_t;

// Model performance metrics
typedef struct {
    double accuracy;
    double precision;
    double recall;
    double f1_score;
    double mean_squared_error;
    double mean_absolute_error;
    long long training_samples_used;
    long long predictions_made;
    long long accurate_predictions;
} model_metrics_t;

// ML Predictor configuration
typedef struct {
    model_type_t model_type;
    int max_training_samples;
    int feature_window_size;
    double learning_rate;
    int retraining_interval_seconds;
    double accuracy_threshold;
    int enable_online_learning;
    int prediction_horizon_seconds;
} ml_predictor_config_t;

// ML Predictor context
typedef struct {
    // Configuration
    ml_predictor_config_t config;
    
    // Model data
    training_sample_t *training_data;
    int training_sample_count;
    int max_training_samples;
    
    // Current model state
    double model_weights[10][5];      // Weights for each feature-target combination
    double model_bias[5];             // Bias terms for each target
    model_metrics_t current_metrics;
    
    // Feature history
    feature_vector_t *feature_history;
    int history_size;
    int history_index;
    
    // Prediction cache
    prediction_result_t *prediction_cache;
    int cache_size;
    int cache_index;
    
    // Statistics
    long long total_predictions;
    long long accurate_predictions;
    long long training_sessions;
    double average_prediction_error;
    
    // Adaptive parameters
    double adaptation_rate;
    double model_confidence_threshold;
    
    // State
    int initialized;
    int active;
    int model_trained;
    long long last_training_time;
    long long last_prediction_time;
} ml_performance_predictor_t;

// Performance pattern
typedef struct {
    long long pattern_id;
    char pattern_name[64];
    double feature_signature[10];
    double performance_impact;
    int frequency;                    // How often this pattern occurs
    long long first_seen;
    long long last_seen;
} performance_pattern_t;

// Anomaly detection
typedef struct {
    int is_anomaly;
    double anomaly_score;             // 0.0-1.0, higher means more anomalous
    char anomaly_description[256];
    long long detection_time;
    feature_vector_t anomalous_features;
} anomaly_detection_result_t;

// Initialize the ML predictor
int ml_predictor_init(ml_performance_predictor_t *predictor,
                     const ml_predictor_config_t *config);

// Cleanup the ML predictor
void ml_predictor_cleanup(ml_performance_predictor_t *predictor);

// Add training data
int ml_predictor_add_training_sample(ml_performance_predictor_t *predictor,
                                   const feature_vector_t *features,
                                   const double *actual_values,
                                   int value_count);

// Train the model
int ml_predictor_train_model(ml_performance_predictor_t *predictor);

// Make performance prediction
prediction_result_t* ml_predictor_predict(ml_performance_predictor_t *predictor,
                                        const feature_vector_t *current_features,
                                        prediction_target_t target);

// Add feature vector to history
int ml_predictor_add_features(ml_performance_predictor_t *predictor,
                            const feature_vector_t *features);

// Detect performance anomalies
anomaly_detection_result_t* ml_predictor_detect_anomalies(ml_performance_predictor_t *predictor,
                                                        const feature_vector_t *features);

// Get model performance metrics
void ml_predictor_get_metrics(ml_performance_predictor_t *predictor,
                            model_metrics_t *metrics);

// Get prediction accuracy
double ml_predictor_get_accuracy(ml_performance_predictor_t *predictor);

// Get optimization recommendations based on predictions
int ml_predictor_get_recommendations(ml_performance_predictor_t *predictor,
                                   char *recommendations_buffer,
                                   size_t buffer_size);

// Retrain model if needed
int ml_predictor_retrain_if_needed(ml_performance_predictor_t *predictor);

// Enable/disable the predictor
int ml_predictor_enable(ml_performance_predictor_t *predictor);
int ml_predictor_disable(ml_performance_predictor_t *predictor);

// Reset statistics
void ml_predictor_reset_stats(ml_performance_predictor_t *predictor);

// Get global instance
ml_performance_predictor_t* get_global_ml_predictor(void);

#endif // _ML_PERFORMANCE_PREDICTOR_H_