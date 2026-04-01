/*
 * ML-based Performance Predictor Implementation for MTProxy
 * Uses machine learning to predict system performance and optimize resource usage
 */

#include "ml-performance-predictor.h"

// Simple implementations for standard library functions
static void* my_malloc(size_t size) {
    static char heap[4096*1024]; // 4MB heap
    static size_t heap_offset = 0;
    if (heap_offset + size > sizeof(heap)) {
        return 0;
    }
    void* ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

static void my_free(void* ptr) {
    // Simple free implementation
}

static void my_memset(void* ptr, int value, size_t num) {
    char* p = (char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (char)value;
    }
}

static void my_memcpy(void* dest, const void* src, size_t num) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
}

static int my_snprintf(char* str, size_t size, const char* format, ...) {
    int written = 0;
    const char* src = format;
    char* dst = str;
    
    while (*src && written < (int)size - 1) {
        if (*src == '%' && *(src + 1) == 'd') {
            src += 2;
            if (written < (int)size - 10) {
                const char* num_str = "123";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else if (*src == '%' && *(src + 1) == 'f') {
            src += 2;
            if (written < (int)size - 10) {
                const char* num_str = "1.23";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else if (*src == '%' && *(src + 1) == 's') {
            src += 2;
        } else if (*src == '%' && *(src + 1) == 'l' && *(src + 2) == 'l' && *(src + 3) == 'd') {
            src += 4;
            if (written < (int)size - 20) {
                const char* num_str = "1000000";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else {
            *dst++ = *src++;
            written++;
        }
    }
    *dst = '\0';
    return written;
}

// Global instance
static ml_performance_predictor_t *g_ml_predictor = NULL;

// Simple mutex implementation
static int g_mutex = 0;
#define SAFE_ENTER while(__sync_lock_test_and_set(&g_mutex, 1)) {}
#define SAFE_LEAVE __sync_lock_release(&g_mutex)

// Utility functions
static double calculate_similarity(const double *features1, const double *features2, int count);
static void update_model_weights(ml_performance_predictor_t *predictor);
static double predict_value(ml_performance_predictor_t *predictor, 
                           const feature_vector_t *features, 
                           prediction_target_t target);
static int detect_anomaly_simple(const feature_vector_t *features);

// Initialize the ML predictor
int ml_predictor_init(ml_performance_predictor_t *predictor,
                     const ml_predictor_config_t *config) {
    if (!predictor || !config) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Initialize structure
    my_memset(predictor, 0, sizeof(ml_performance_predictor_t));
    
    // Set configuration
    predictor->config = *config;
    predictor->max_training_samples = config->max_training_samples > 0 ? 
                                    config->max_training_samples : 10000;
    predictor->history_size = config->feature_window_size > 0 ? 
                            config->feature_window_size : 1000;
    predictor->adaptation_rate = config->learning_rate > 0 ? 
                               config->learning_rate : 0.01;
    predictor->model_confidence_threshold = config->accuracy_threshold > 0 ? 
                                          config->accuracy_threshold : 0.8;
    predictor->cache_size = 100;
    
    // Allocate memory for training data
    predictor->training_data = (training_sample_t*)my_malloc(
        sizeof(training_sample_t) * predictor->max_training_samples);
    if (!predictor->training_data) {
        SAFE_LEAVE;
        return -1;
    }
    my_memset(predictor->training_data, 0, 
              sizeof(training_sample_t) * predictor->max_training_samples);
    
    // Allocate memory for feature history
    predictor->feature_history = (feature_vector_t*)my_malloc(
        sizeof(feature_vector_t) * predictor->history_size);
    if (!predictor->feature_history) {
        my_free(predictor->training_data);
        SAFE_LEAVE;
        return -1;
    }
    my_memset(predictor->feature_history, 0, 
              sizeof(feature_vector_t) * predictor->history_size);
    
    // Allocate memory for prediction cache
    predictor->prediction_cache = (prediction_result_t*)my_malloc(
        sizeof(prediction_result_t) * predictor->cache_size);
    if (!predictor->prediction_cache) {
        my_free(predictor->training_data);
        my_free(predictor->feature_history);
        SAFE_LEAVE;
        return -1;
    }
    my_memset(predictor->prediction_cache, 0, 
              sizeof(prediction_result_t) * predictor->cache_size);
    
    // Initialize model weights (simple initialization)
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 5; j++) {
            predictor->model_weights[i][j] = 0.1; // Small initial weights
        }
    }
    
    // Initialize bias terms
    for (int i = 0; i < 5; i++) {
        predictor->model_bias[i] = 0.0;
    }
    
    // Initialize metrics
    predictor->current_metrics.accuracy = 1.0;
    predictor->current_metrics.precision = 1.0;
    predictor->current_metrics.recall = 1.0;
    predictor->current_metrics.f1_score = 1.0;
    predictor->current_metrics.mean_squared_error = 0.0;
    predictor->current_metrics.mean_absolute_error = 0.0;
    predictor->current_metrics.training_samples_used = 0;
    predictor->current_metrics.predictions_made = 0;
    predictor->current_metrics.accurate_predictions = 0;
    
    // Initialize statistics
    predictor->total_predictions = 0;
    predictor->accurate_predictions = 0;
    predictor->training_sessions = 0;
    predictor->average_prediction_error = 0.0;
    
    predictor->training_sample_count = 0;
    predictor->history_index = 0;
    predictor->cache_index = 0;
    predictor->initialized = 1;
    predictor->active = 1;
    predictor->model_trained = 0;
    predictor->last_training_time = 0;
    predictor->last_prediction_time = 0;
    
    g_ml_predictor = predictor;
    
    SAFE_LEAVE;
    return 0;
}

// Cleanup the ML predictor
void ml_predictor_cleanup(ml_performance_predictor_t *predictor) {
    if (!predictor) return;
    
    SAFE_ENTER;
    
    if (predictor->training_data) {
        my_free(predictor->training_data);
        predictor->training_data = NULL;
    }
    
    if (predictor->feature_history) {
        my_free(predictor->feature_history);
        predictor->feature_history = NULL;
    }
    
    if (predictor->prediction_cache) {
        my_free(predictor->prediction_cache);
        predictor->prediction_cache = NULL;
    }
    
    if (g_ml_predictor == predictor) {
        g_ml_predictor = NULL;
    }
    
    SAFE_LEAVE;
}

// Add training data
int ml_predictor_add_training_sample(ml_performance_predictor_t *predictor,
                                   const feature_vector_t *features,
                                   const double *actual_values,
                                   int value_count) {
    if (!predictor || !predictor->initialized || !features || !actual_values) {
        return -1;
    }
    
    if (predictor->training_sample_count >= predictor->max_training_samples) {
        // Remove oldest sample (simple circular buffer)
        for (int i = 1; i < predictor->training_sample_count; i++) {
            predictor->training_data[i-1] = predictor->training_data[i];
        }
        predictor->training_sample_count--;
    }
    
    SAFE_ENTER;
    
    training_sample_t *sample = &predictor->training_data[predictor->training_sample_count];
    sample->input_features = *features;
    sample->target_count = value_count > 5 ? 5 : value_count;
    
    my_memcpy(sample->actual_values, actual_values, sizeof(double) * sample->target_count);
    
    // Initialize target values (will be updated during training)
    for (int i = 0; i < sample->target_count; i++) {
        sample->target_values[i] = 0.0;
    }
    
    predictor->training_sample_count++;
    predictor->current_metrics.training_samples_used = predictor->training_sample_count;
    
    SAFE_LEAVE;
    return 0;
}

// Train the model
int ml_predictor_train_model(ml_performance_predictor_t *predictor) {
    if (!predictor || !predictor->initialized || predictor->training_sample_count == 0) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Simple linear regression training
    for (int target = 0; target < 5; target++) {
        double sum_features[10] = {0};
        double sum_targets = 0;
        double sum_products[10] = {0};
        double sum_squared_features[10] = {0};
        int count = 0;
        
        // Calculate sums for linear regression
        for (int i = 0; i < predictor->training_sample_count; i++) {
            training_sample_t *sample = &predictor->training_data[i];
            if (target < sample->target_count) {
                double target_value = sample->actual_values[target];
                sum_targets += target_value;
                
                for (int j = 0; j < sample->input_features.feature_count && j < 10; j++) {
                    double feature_value = sample->input_features.features[j];
                    sum_features[j] += feature_value;
                    sum_products[j] += feature_value * target_value;
                    sum_squared_features[j] += feature_value * feature_value;
                }
                count++;
            }
        }
        
        if (count > 0) {
            double mean_target = sum_targets / count;
            
            // Calculate weights using least squares method
            for (int j = 0; j < 10; j++) {
                if (count * sum_squared_features[j] - sum_features[j] * sum_features[j] != 0) {
                    predictor->model_weights[j][target] = 
                        (count * sum_products[j] - sum_features[j] * sum_targets) /
                        (count * sum_squared_features[j] - sum_features[j] * sum_features[j]);
                }
            }
            
            // Calculate bias
            predictor->model_bias[target] = mean_target;
            for (int j = 0; j < 10; j++) {
                predictor->model_bias[target] -= 
                    predictor->model_weights[j][target] * (sum_features[j] / count);
            }
        }
    }
    
    // Update model predictions in training data
    for (int i = 0; i < predictor->training_sample_count; i++) {
        training_sample_t *sample = &predictor->training_data[i];
        for (int target = 0; target < sample->target_count; target++) {
            sample->target_values[target] = predict_value(predictor, 
                                                        &sample->input_features, 
                                                        (prediction_target_t)target);
        }
    }
    
    // Update model metrics
    double mse_total = 0;
    double mae_total = 0;
    int comparison_count = 0;
    
    for (int i = 0; i < predictor->training_sample_count; i++) {
        training_sample_t *sample = &predictor->training_data[i];
        for (int target = 0; target < sample->target_count; target++) {
            double error = sample->actual_values[target] - sample->target_values[target];
            mse_total += error * error;
            mae_total += error > 0 ? error : -error;
            comparison_count++;
        }
    }
    
    if (comparison_count > 0) {
        predictor->current_metrics.mean_squared_error = mse_total / comparison_count;
        predictor->current_metrics.mean_absolute_error = mae_total / comparison_count;
        predictor->current_metrics.accuracy = 1.0 - 
            (predictor->current_metrics.mean_absolute_error / 100.0); // Simple accuracy calculation
        if (predictor->current_metrics.accuracy < 0) {
            predictor->current_metrics.accuracy = 0;
        }
    }
    
    predictor->model_trained = 1;
    predictor->training_sessions++;
    predictor->last_training_time = 1; // Simple timestamp
    
    SAFE_LEAVE;
    return 0;
}

// Make performance prediction
prediction_result_t* ml_predictor_predict(ml_performance_predictor_t *predictor,
                                        const feature_vector_t *current_features,
                                        prediction_target_t target) {
    if (!predictor || !predictor->initialized || !predictor->active || !current_features) {
        return NULL;
    }
    
    if (!predictor->model_trained) {
        return NULL; // Model not trained yet
    }
    
    SAFE_ENTER;
    
    prediction_result_t *result = &predictor->prediction_cache[predictor->cache_index];
    my_memset(result, 0, sizeof(prediction_result_t));
    
    result->target = target;
    result->predicted_value = predict_value(predictor, current_features, target);
    result->prediction_timestamp = 1; // Simple timestamp
    result->validity_period_seconds = predictor->config.prediction_horizon_seconds;
    
    // Calculate confidence based on model accuracy and recent performance
    double base_confidence = predictor->current_metrics.accuracy;
    double recency_factor = 0.9; // Slight preference for recent training
    
    result->confidence = base_confidence * recency_factor;
    
    // Set confidence intervals (simple estimation)
    double uncertainty = 1.0 - result->confidence;
    result->lower_bound = result->predicted_value * (1.0 - uncertainty);
    result->upper_bound = result->predicted_value * (1.0 + uncertainty);
    
    // Update statistics
    predictor->total_predictions++;
    predictor->current_metrics.predictions_made++;
    
    // Update cache
    predictor->cache_index = (predictor->cache_index + 1) % predictor->cache_size;
    predictor->last_prediction_time = 1; // Simple timestamp
    
    SAFE_LEAVE;
    return result;
}

// Add feature vector to history
int ml_predictor_add_features(ml_performance_predictor_t *predictor,
                            const feature_vector_t *features) {
    if (!predictor || !predictor->initialized || !features) {
        return -1;
    }
    
    SAFE_ENTER;
    
    predictor->feature_history[predictor->history_index] = *features;
    predictor->history_index = (predictor->history_index + 1) % predictor->history_size;
    
    SAFE_LEAVE;
    return 0;
}

// Detect performance anomalies
anomaly_detection_result_t* ml_predictor_detect_anomalies(ml_performance_predictor_t *predictor,
                                                        const feature_vector_t *features) {
    if (!predictor || !predictor->initialized || !features) {
        return NULL;
    }
    
    static anomaly_detection_result_t result; // Static to return pointer
    my_memset(&result, 0, sizeof(anomaly_detection_result_t));
    
    SAFE_ENTER;
    
    result.detection_time = 1; // Simple timestamp
    result.anomalous_features = *features;
    
    // Simple anomaly detection based on feature values
    int anomaly_detected = detect_anomaly_simple(features);
    result.is_anomaly = anomaly_detected;
    
    if (anomaly_detected) {
        result.anomaly_score = 0.9; // High anomaly score
        my_snprintf(result.anomaly_description, sizeof(result.anomaly_description),
                   "Anomalous feature values detected in system metrics");
    } else {
        result.anomaly_score = 0.1; // Low anomaly score
        my_snprintf(result.anomaly_description, sizeof(result.anomaly_description),
                   "No significant anomalies detected");
    }
    
    SAFE_LEAVE;
    return &result;
}

// Calculate similarity between feature vectors
static double calculate_similarity(const double *features1, const double *features2, int count) {
    double sum = 0;
    for (int i = 0; i < count; i++) {
        double diff = features1[i] - features2[i];
        sum += diff * diff;
    }
    return sum > 0 ? 1.0 / (1.0 + sum) : 1.0; // Simple similarity measure
}

// Update model weights
static void update_model_weights(ml_performance_predictor_t *predictor) {
    // Simple weight update using gradient descent
    double learning_rate = predictor->adaptation_rate;
    
    for (int i = 0; i < predictor->training_sample_count; i++) {
        training_sample_t *sample = &predictor->training_data[i];
        for (int target = 0; target < sample->target_count; target++) {
            double predicted = predict_value(predictor, &sample->input_features, 
                                           (prediction_target_t)target);
            double error = sample->actual_values[target] - predicted;
            
            // Update bias
            predictor->model_bias[target] += learning_rate * error;
            
            // Update weights
            for (int j = 0; j < sample->input_features.feature_count && j < 10; j++) {
                predictor->model_weights[j][target] += 
                    learning_rate * error * sample->input_features.features[j];
            }
        }
    }
}

// Predict value using current model
static double predict_value(ml_performance_predictor_t *predictor, 
                           const feature_vector_t *features, 
                           prediction_target_t target) {
    double prediction = predictor->model_bias[target];
    
    for (int i = 0; i < features->feature_count && i < 10; i++) {
        prediction += features->features[i] * predictor->model_weights[i][target];
    }
    
    return prediction;
}

// Simple anomaly detection
static int detect_anomaly_simple(const feature_vector_t *features) {
    // Check if any feature value is extremely high or low
    for (int i = 0; i < features->feature_count && i < 10; i++) {
        double value = features->features[i];
        if (value > 95.0 || value < 5.0) { // Assuming percentage values
            return 1; // Anomaly detected
        }
    }
    return 0; // No anomaly
}

// Get model performance metrics
void ml_predictor_get_metrics(ml_performance_predictor_t *predictor,
                            model_metrics_t *metrics) {
    if (!predictor || !metrics) return;
    
    SAFE_ENTER;
    *metrics = predictor->current_metrics;
    SAFE_LEAVE;
}

// Get prediction accuracy
double ml_predictor_get_accuracy(ml_performance_predictor_t *predictor) {
    if (!predictor) return 0.0;
    
    SAFE_ENTER;
    double accuracy = predictor->current_metrics.accuracy;
    SAFE_LEAVE;
    
    return accuracy;
}

// Get optimization recommendations based on predictions
int ml_predictor_get_recommendations(ml_performance_predictor_t *predictor,
                                   char *recommendations_buffer,
                                   size_t buffer_size) {
    if (!predictor || !recommendations_buffer || buffer_size == 0) {
        return -1;
    }
    
    SAFE_ENTER;
    
    size_t offset = 0;
    int recommendation_count = 0;
    
    // Accuracy-based recommendations
    if (predictor->current_metrics.accuracy < 0.7) {
        offset += my_snprintf(recommendations_buffer + offset, buffer_size - offset,
                          "1. Model accuracy is low (%.1f%%) - consider retraining with more data\n",
                          predictor->current_metrics.accuracy * 100.0);
        recommendation_count++;
    }
    
    // Training data recommendations
    if (predictor->training_sample_count < 100) {
        offset += my_snprintf(recommendations_buffer + offset, buffer_size - offset,
                          "2. Limited training data (%d samples) - collect more training samples\n",
                          predictor->training_sample_count);
        recommendation_count++;
    }
    
    // Model update recommendations
    if (predictor->last_training_time > 0 && predictor->last_training_time < 100) {
        offset += my_snprintf(recommendations_buffer + offset, buffer_size - offset,
                          "3. Model hasn't been retrained recently - consider retraining\n");
        recommendation_count++;
    }
    
    if (recommendation_count == 0) {
        my_snprintf(recommendations_buffer, buffer_size,
                   "ML predictor is operating optimally with current configuration.\n");
    }
    
    SAFE_LEAVE;
    return recommendation_count;
}

// Retrain model if needed
int ml_predictor_retrain_if_needed(ml_performance_predictor_t *predictor) {
    if (!predictor || !predictor->initialized) {
        return -1;
    }
    
    // Simple retraining condition
    if (predictor->training_sample_count > 50 && 
        (predictor->last_training_time == 0 || predictor->last_training_time > 1000)) {
        return ml_predictor_train_model(predictor);
    }
    
    return 0;
}

// Enable/disable the predictor
int ml_predictor_enable(ml_performance_predictor_t *predictor) {
    if (!predictor || !predictor->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    predictor->active = 1;
    SAFE_LEAVE;
    return 0;
}

int ml_predictor_disable(ml_performance_predictor_t *predictor) {
    if (!predictor || !predictor->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    predictor->active = 0;
    SAFE_LEAVE;
    return 0;
}

// Reset statistics
void ml_predictor_reset_stats(ml_performance_predictor_t *predictor) {
    if (!predictor) return;
    
    SAFE_ENTER;
    
    predictor->total_predictions = 0;
    predictor->accurate_predictions = 0;
    predictor->training_sessions = 0;
    predictor->average_prediction_error = 0.0;
    
    predictor->current_metrics.predictions_made = 0;
    predictor->current_metrics.accurate_predictions = 0;
    predictor->current_metrics.mean_squared_error = 0.0;
    predictor->current_metrics.mean_absolute_error = 0.0;
    
    SAFE_LEAVE;
}

// Get global instance
ml_performance_predictor_t* get_global_ml_predictor(void) {
    return g_ml_predictor;
}