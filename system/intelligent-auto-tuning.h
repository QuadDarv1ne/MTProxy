/*
 * Intelligent Auto-Tuning System with Machine Learning for MTProxy
 * Automatically optimizes system parameters based on performance analysis
 */

#ifndef _INTELLIGENT_AUTO_TUNING_H_
#define _INTELLIGENT_AUTO_TUNING_H_

#include <stdint.h>
#include <stddef.h>

// Tuning parameters
typedef enum {
    TUNING_PARAM_THREAD_COUNT = 0,         // Number of worker threads
    TUNING_PARAM_BUFFER_SIZE = 1,          // Buffer sizes for network I/O
    TUNING_PARAM_CONNECTION_POOL = 2,      // Connection pool sizes
    TUNING_PARAM_MEMORY_CACHE = 3,         // Memory cache sizes
    TUNING_PARAM_CRYPTO_BATCH = 4,         // Cryptographic batch sizes
    TUNING_PARAM_NETWORK_TIMEOUT = 5,      // Network timeout values
    TUNING_PARAM_RETRY_COUNT = 6,          // Retry attempt counts
    TUNING_PARAM_LOAD_BALANCING = 7,       // Load balancing algorithms
    TUNING_PARAM_COMPRESSION_LEVEL = 8,    // Compression levels
    TUNING_PARAM_LOG_LEVEL = 9             // Logging verbosity levels
} tuning_parameter_t;

// Performance metrics types
typedef enum {
    METRIC_TYPE_LATENCY = 0,               // Response time metrics
    METRIC_TYPE_THROUGHPUT = 1,            // Request processing rate
    METRIC_TYPE_CPU_USAGE = 2,             // CPU utilization
    METRIC_TYPE_MEMORY_USAGE = 3,          // Memory consumption
    METRIC_TYPE_CONNECTION_COUNT = 4,      // Active connections
    METRIC_TYPE_ERROR_RATE = 5,            // Error occurrence rate
    METRIC_TYPE_BANDWIDTH = 6,             // Network bandwidth usage
    METRIC_TYPE_QUEUE_DEPTH = 7            // Queue depths
} metric_type_t;

// Tuning strategies
typedef enum {
    STRATEGY_CONSERVATIVE = 0,             // Minimal changes, stable operation
    STRATEGY_BALANCED = 1,                 // Balanced performance and stability
    STRATEGY_AGGRESSIVE = 2,               // Maximum performance, higher risk
    STRATEGY_ADAPTIVE = 3,                 // Learn from performance patterns
    STRATEGY_PREDICTIVE = 4                // Predict optimal settings
} tuning_strategy_t;

// Parameter configuration
typedef struct {
    tuning_parameter_t parameter;
    long long current_value;
    long long min_value;
    long long max_value;
    long long optimal_value;
    long long step_size;
    int is_tunable;
    double sensitivity;                    // How sensitive to changes
    long long last_adjustment_time;
    int adjustment_count;
    double performance_impact;             // Measured impact of last change
} parameter_config_t;

// Performance sample
typedef struct {
    long long timestamp;
    double metrics[8];                     // Values for each metric type
    int metric_count;
    long long parameter_values[10];        // Current parameter values
    int parameter_count;
    double overall_performance_score;      // Composite performance score
    int is_optimal;                        // Whether this represents optimal state
} performance_sample_t;

// ML Model for tuning
typedef struct {
    double weights[10][8];                 // Parameter-to-metric relationships
    double bias[8];                        // Bias terms for each metric
    double learning_rate;
    int trained;
    long long training_samples;
    double model_accuracy;
    long long last_training_time;
} ml_tuning_model_t;

// Auto-tuning configuration
typedef struct {
    tuning_strategy_t strategy;
    int enable_ml_tuning;
    int max_training_samples;
    int sample_history_size;
    double learning_rate;
    double convergence_threshold;
    int auto_tuning_interval_seconds;
    int enable_performance_prediction;
    double prediction_horizon_minutes;
    int enable_safe_mode;
    double safe_mode_threshold;
    int enable_logging;
    char log_file[256];
} auto_tuning_config_t;

// Auto-tuning system context
typedef struct {
    // Configuration
    auto_tuning_config_t config;
    
    // Parameter configurations
    parameter_config_t *parameters;
    int parameter_count;
    int max_parameters;
    
    // Performance samples
    performance_sample_t *samples;
    int sample_count;
    int max_samples;
    int sample_index;
    
    // ML Model
    ml_tuning_model_t ml_model;
    
    // Current state
    long long current_parameter_values[10];
    double current_performance_metrics[8];
    double baseline_performance_score;
    double current_performance_score;
    
    // Statistics
    long long total_tuning_operations;
    long long successful_tunings;
    long long failed_tunings;
    long long performance_improvements;
    long long performance_degradations;
    double average_improvement_percent;
    double tuning_success_rate;
    
    // Adaptive parameters
    double exploration_rate;               // Balance between exploration and exploitation
    double confidence_threshold;           // Confidence needed for parameter changes
    long long last_tuning_time;
    long long tuning_cooldown_period;
    
    // Safety mechanisms
    long long safe_baseline_values[10];
    double safe_performance_threshold;
    int safety_mode_active;
    long long safety_mode_start_time;
    
    // State
    int initialized;
    int active;
    int learning_phase;
    int model_trained;
} intelligent_auto_tuner_t;

// Tuning recommendation
typedef struct {
    tuning_parameter_t parameter;
    long long recommended_value;
    long long current_value;
    double expected_improvement_percent;
    double confidence_level;
    char reason[256];
    int is_safe_change;
    long long estimated_time_to_effect;
} tuning_recommendation_t;

// Auto-tuning statistics
typedef struct {
    long long total_parameters;
    long long tunable_parameters;
    long long samples_collected;
    long long model_accuracy_percent;
    long long successful_tunings;
    long long failed_tunings;
    double average_performance_improvement;
    double current_performance_score;
    double baseline_performance_score;
    long long learning_phase_progress;
    int safety_mode_active;
    long long recommendations_generated;
} auto_tuning_stats_t;

// Initialize auto-tuning system
int auto_tuner_init(intelligent_auto_tuner_t *tuner,
                   const auto_tuning_config_t *config);

// Cleanup auto-tuning system
void auto_tuner_cleanup(intelligent_auto_tuner_t *tuner);

// Add tunable parameter
int auto_tuner_add_parameter(intelligent_auto_tuner_t *tuner,
                           tuning_parameter_t parameter,
                           long long initial_value,
                           long long min_value,
                           long long max_value,
                           long long step_size,
                           double sensitivity);

// Remove tunable parameter
int auto_tuner_remove_parameter(intelligent_auto_tuner_t *tuner,
                              tuning_parameter_t parameter);

// Add performance sample
int auto_tuner_add_sample(intelligent_auto_tuner_t *tuner,
                         const double *metrics,
                         int metric_count,
                         const long long *parameter_values,
                         int parameter_count);

// Train ML model
int auto_tuner_train_model(intelligent_auto_tuner_t *tuner);

// Generate tuning recommendations
tuning_recommendation_t* auto_tuner_get_recommendations(intelligent_auto_tuner_t *tuner,
                                                      int *recommendation_count);

// Apply tuning recommendation
int auto_tuner_apply_recommendation(intelligent_auto_tuner_t *tuner,
                                   const tuning_recommendation_t *recommendation);

// Get current parameter values
int auto_tuner_get_current_parameters(intelligent_auto_tuner_t *tuner,
                                    long long *parameter_values,
                                    int max_parameters);

// Set parameter values
int auto_tuner_set_parameters(intelligent_auto_tuner_t *tuner,
                             const long long *parameter_values,
                             int parameter_count);

// Calculate performance score
double auto_tuner_calculate_performance_score(intelligent_auto_tuner_t *tuner,
                                            const double *metrics,
                                            int metric_count);

// Enable/disable auto-tuning
int auto_tuner_enable(intelligent_auto_tuner_t *tuner);
int auto_tuner_disable(intelligent_auto_tuner_t *tuner);

// Reset learning
int auto_tuner_reset_learning(intelligent_auto_tuner_t *tuner);

// Get auto-tuning statistics
void auto_tuner_get_stats(intelligent_auto_tuner_t *tuner,
                         auto_tuning_stats_t *stats);

// Export tuning data
int auto_tuner_export_data(intelligent_auto_tuner_t *tuner,
                          const char *filename);

// Import tuning data
int auto_tuner_import_data(intelligent_auto_tuner_t *tuner,
                          const char *filename);

// Reset statistics
void auto_tuner_reset_stats(intelligent_auto_tuner_t *tuner);

// Get global instance
intelligent_auto_tuner_t* get_global_auto_tuner(void);

#endif // _INTELLIGENT_AUTO_TUNING_H_