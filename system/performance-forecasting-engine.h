/*
 * performance-forecasting-engine.h
 * Performance Forecasting Engine for MTProxy
 *
 * Advanced system for forecasting performance metrics, resource utilization,
 * and system behavior based on historical data and predictive models.
 */

#ifndef PERFORMANCE_FORECASTING_ENGINE_H
#define PERFORMANCE_FORECASTING_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

// Forecast types
typedef enum {
    FORECAST_TYPE_CPU_UTILIZATION = 0,
    FORECAST_TYPE_MEMORY_USAGE = 1,
    FORECAST_TYPE_NETWORK_TRAFFIC = 2,
    FORECAST_TYPE_CONNECTION_RATE = 3,
    FORECAST_TYPE_LATENCY = 4,
    FORECAST_TYPE_THROUGHPUT = 5,
    FORECAST_TYPE_ERROR_RATE = 6,
    FORECAST_TYPE_BANDWIDTH = 7,
    FORECAST_TYPE_QUEUE_LENGTH = 8,
    FORECAST_TYPE_RESPONSE_TIME = 9,
    FORECAST_TYPE_SYSTEM_LOAD = 10,
    FORECAST_TYPE_RESOURCE_PRESSURE = 11
} forecast_type_t;

// Forecast horizons
typedef enum {
    FORECAST_HORIZON_SHORT = 0,    // 1-10 minutes
    FORECAST_HORIZON_MEDIUM = 1,   // 10-60 minutes
    FORECAST_HORIZON_LONG = 2,     // 1-24 hours
    FORECAST_HORIZON_VERY_LONG = 3 // 1-7 days
} forecast_horizon_t;

// Seasonality patterns
typedef enum {
    SEASONALITY_NONE = 0,
    SEASONALITY_DAILY = 1,
    SEASONALITY_WEEKLY = 2,
    SEASONALITY_MONTHLY = 3,
    SEASONALITY_YEARLY = 4
} seasonality_type_t;

// Trend patterns
typedef enum {
    TREND_STABLE = 0,
    TREND_INCREASING = 1,
    TREND_DECREASING = 2,
    TREND_VOLATILE = 3
} trend_type_t;

// Confidence intervals
typedef enum {
    CONFIDENCE_68_PERCENT = 0,  // 1 standard deviation
    CONFIDENCE_95_PERCENT = 1,  // 2 standard deviations
    CONFIDENCE_99_PERCENT = 2   // 3 standard deviations
} confidence_interval_t;

// Forecast data point
typedef struct {
    uint64_t timestamp;
    double forecasted_value;
    double lower_bound;
    double upper_bound;
    double actual_value;  // Set when available
    double error_percentage;
    confidence_interval_t confidence_level;
    bool is_valid;
    char forecast_source[64];
    uint64_t forecast_generation_time;
} forecast_point_t;

// Forecast model
typedef struct {
    forecast_type_t forecast_type;
    forecast_horizon_t horizon;
    void* model_data;
    double model_accuracy;
    double last_training_score;
    uint64_t last_training_time;
    uint64_t next_retraining_time;
    char model_name[64];
    char model_version[32];
    bool is_active;
    bool needs_retraining;
    int prediction_count;
    int accurate_predictions;
} forecast_model_t;

// Historical pattern
typedef struct {
    forecast_type_t pattern_type;
    seasonality_type_t seasonality;
    trend_type_t trend;
    double trend_slope;
    double seasonal_amplitude;
    double seasonal_period;
    double noise_level;
    uint64_t pattern_start_time;
    uint64_t pattern_end_time;
    double confidence_score;
    char pattern_description[128];
    bool is_recurring;
} historical_pattern_t;

// Forecast result
typedef struct {
    uint64_t forecast_id;
    forecast_type_t forecast_type;
    forecast_horizon_t horizon;
    uint64_t forecast_start_time;
    uint64_t forecast_end_time;
    int point_count;
    forecast_point_t* forecast_points;
    double overall_confidence;
    double mean_absolute_error;
    double mean_squared_error;
    historical_pattern_t detected_pattern;
    trend_type_t overall_trend;
    seasonality_type_t overall_seasonality;
    char forecast_summary[256];
    char recommendations[512];
    bool is_emergency_forecast;  // Critical system issues predicted
    double critical_threshold;   // Threshold that triggers emergency
    uint64_t estimated_impact_time_seconds;
} forecast_result_t;

// Resource projection
typedef struct {
    forecast_type_t resource_type;
    uint64_t timestamp;
    double current_value;
    double projected_value;
    double required_capacity;
    double available_capacity;
    double utilization_percentage;
    double pressure_score;  // 0-100 scale
    bool is_bottleneck;
    char bottleneck_cause[128];
    double bottleneck_impact_score;
    bool requires_scaling;
    int recommended_scaling_factor;  // 100 = 1x, 150 = 1.5x, etc.
    char scaling_recommendation[256];
} resource_projection_t;

// Forecasting engine configuration
typedef struct {
    bool enable_forecasting;
    bool enable_auto_retraining;
    bool enable_pattern_detection;
    bool enable_anomaly_forecasting;
    bool enable_resource_planning;
    bool enable_emergency_alerts;
    int forecast_points_per_horizon;
    int data_retention_days;
    int pattern_memory_size;
    double confidence_threshold;  // 0.0 - 100.0
    int minimum_data_points_required;
    int max_forecast_models;
    int model_selection_timeout_seconds;
    bool enable_cross_validation;
    int validation_window_size;
    bool enable_ensemble_methods;
    double ensemble_consensus_threshold;
    double anomaly_forecast_threshold;
    int resource_pressure_threshold_percent;
    int critical_alert_threshold_percent;
    bool enable_predictive_scaling;
    double scaling_confidence_threshold;
    int max_scaling_recommendation_factor;
    bool enable_trend_analysis;
    bool enable_seasonality_detection;
} forecasting_config_t;

// Performance forecasting context
typedef struct {
    // Configuration
    forecasting_config_t config;
    
    // Forecasting models
    forecast_model_t* models;
    int model_count;
    int max_models;
    
    // Historical data
    historical_pattern_t* patterns;
    int pattern_count;
    int max_patterns;
    double historical_data[16][86400];  // 16 metrics, 24 hours @ 1sec resolution
    int data_point_count[16];
    uint64_t data_collection_times[16][86400];
    int data_index[16];  // Current index for circular buffer
    
    // Resource tracking
    resource_projection_t* resource_projections;
    int resource_count;
    int max_resources;
    double baseline_capacity[16];  // Baseline capacity for each resource type
    double current_capacity[16];   // Current capacity
    
    // Pattern recognition
    bool pattern_detection_enabled;
    int minimum_pattern_length;
    double pattern_similarity_threshold;
    double correlation_threshold;
    
    // Current state
    uint64_t last_forecast_generation;
    uint64_t last_model_retraining;
    uint64_t last_emergency_forecast;
    bool engine_active;
    bool emergency_mode;
    bool performance_degradation_detected;
    int critical_alerts_count;
    
    // Statistical counters
    uint64_t total_forecasts_generated;
    uint64_t accurate_forecasts;
    uint64_t inaccurate_forecasts;
    uint64_t patterns_discovered;
    uint64_t false_alarm_predictions;
    double forecast_accuracy_rate;
    double average_forecast_confidence;
    uint64_t anomalous_behavior_detected;
    double emergency_alert_precision_rate;
    
    // Alert management
    struct {
        int enabled_alert_types;
        uint64_t last_alert_times[32];
        double alert_thresholds[32];
        int alert_severities[32];
        int max_alert_rate;
    } alerts_system;
    
    // Output management
    forecast_result_t* forecast_history;
    int forecast_history_count;
    int max_forecast_history;
    forecast_result_t* pending_forecasts;
    int pending_forecast_count;
    int max_pending_forecasts;
    
    // Performance monitoring
    uint64_t last_performance_check;
    double current_performance_score;
    double predicted_performance_score;
    double performance_degradation_trend;
    bool performance_recovery_in_progress;
    
    // State management
    int initialized;
    int active;
    uint64_t start_time;
    char engine_id[64];
    char version_string[32];
} forecasting_engine_context_t;

// Alert types
typedef enum {
    ALERT_TYPE_PERFORMANCE_DEGRADATION = 0,
    ALERT_TYPE_RESOURCE_EXHAUSTION = 1,
    ALERT_TYPE_CAPACITY_BOTTLENECK = 2,
    ALERT_TYPE_ANOMALOUS_BEHAVIOR = 3,
    ALERT_TYPE_CRITICAL_THRESHOLD = 4,
    ALERT_TYPE_EMERGENCY_SITUATION = 5,
    ALERT_TYPE_FORECAST_INACCURACY = 6,
    ALERT_TYPE_MODEL_DEGRADATION = 7
} alert_type_t;

// Alert severity levels
typedef enum {
    ALERT_SEVERITY_INFO = 0,
    ALERT_SEVERITY_WARNING = 1,
    ALERT_SEVERITY_ERROR = 2,
    ALERT_SEVERITY_CRITICAL = 3,
    ALERT_SEVERITY_EMERGENCY = 4
} alert_severity_t;

// Alert structure
typedef struct {
    uint64_t alert_id;
    alert_type_t alert_type;
    alert_severity_t severity;
    uint64_t timestamp;
    forecast_type_t affected_metric;
    double current_value;
    double predicted_value;
    double threshold_value;
    double deviation_percentage;
    char alert_message[256];
    char recommended_action[256];
    bool requires_immediate_action;
    uint64_t estimated_resolution_time_seconds;
    bool alert_acknowledged;
    uint64_t acknowledgment_time;
    char acknowledged_by[64];
} forecast_alert_t;

// Callback function types
typedef void (*forecast_generation_callback_t)(const forecast_result_t* forecast);
typedef void (*forecast_alert_callback_t)(const forecast_alert_t* alert);
typedef void (*pattern_detection_callback_t)(const historical_pattern_t* pattern);
typedef void (*resource_projection_callback_t)(const resource_projection_t* projection);
typedef void (*forecast_accuracy_callback_t)(forecast_type_t type, double accuracy, 
                                           const char* model_name);

// Function declarations

// Initialization and cleanup
int init_forecasting_engine(forecasting_engine_context_t* ctx);
int init_forecasting_engine_with_config(forecasting_engine_context_t* ctx, 
                                      const forecasting_config_t* config);
void cleanup_forecasting_engine(forecasting_engine_context_t* ctx);

// Configuration management
void get_forecasting_config(forecasting_engine_context_t* ctx, forecasting_config_t* config);
int set_forecasting_config(forecasting_engine_context_t* ctx, const forecasting_config_t* config);
int enable_forecasting(forecasting_engine_context_t* ctx, bool enable);
int set_confidence_threshold(forecasting_engine_context_t* ctx, double threshold);

// Data collection and management
int add_historical_data(forecasting_engine_context_t* ctx, forecast_type_t type,
                       uint64_t timestamp, double value);
int collect_current_metrics(forecasting_engine_context_t* ctx);
int update_resource_capacity(forecasting_engine_context_t* ctx, forecast_type_t resource_type,
                           double current_capacity, double baseline_capacity);
int preprocess_historical_data(forecasting_engine_context_t* ctx, forecast_type_t type);

// Forecast generation
forecast_result_t generate_forecast(forecasting_engine_context_t* ctx, 
                                  forecast_type_t type, forecast_horizon_t horizon);
int generate_multiple_forecasts(forecasting_engine_context_t* ctx, 
                              forecast_result_t* results, int max_results);
int generate_comprehensive_forecast(forecasting_engine_context_t* ctx, 
                                  forecast_result_t* comprehensive_result);
forecast_result_t generate_emergency_forecast(forecasting_engine_context_t* ctx,
                                            forecast_type_t critical_type);

// Pattern detection and analysis
historical_pattern_t detect_historical_pattern(forecasting_engine_context_t* ctx, 
                                             forecast_type_t type);
int detect_all_patterns(forecasting_engine_context_t* ctx);
int correlate_patterns(forecasting_engine_context_t* ctx, forecast_type_t type1, 
                      forecast_type_t type2, double* correlation_coefficient);
trend_type_t analyze_trend(forecasting_engine_context_t* ctx, forecast_type_t type);
seasonality_type_t detect_seasonality(forecasting_engine_context_t* ctx, forecast_type_t type);

// Resource projection
resource_projection_t project_resource_utilization(forecasting_engine_context_t* ctx,
                                                 forecast_type_t resource_type,
                                                 uint64_t time_horizon_seconds);
int project_all_resources(forecasting_engine_context_t* ctx, 
                         resource_projection_t* projections, int max_projections);
int identify_bottlenecks(forecasting_engine_context_t* ctx);
int recommend_scaling_actions(forecasting_engine_context_t* ctx);

// Alert management
forecast_alert_t generate_forecast_alert(forecasting_engine_context_t* ctx, 
                                       alert_type_t alert_type, forecast_type_t affected_metric,
                                       double current_value, double predicted_value);
int process_forecast_alerts(forecasting_engine_context_t* ctx);
int acknowledge_alert(forecasting_engine_context_t* ctx, uint64_t alert_id, 
                     const char* acknowledged_by);
int get_active_alerts(forecasting_engine_context_t* ctx, 
                     forecast_alert_t* alerts, int max_alerts);

// Model management
int initialize_forecast_model(forecasting_engine_context_t* ctx, forecast_type_t type,
                             forecast_horizon_t horizon);
int train_forecast_model(forecasting_engine_context_t* ctx, forecast_type_t type);
int retrain_all_models(forecasting_engine_context_t* ctx);
int select_best_model(forecasting_engine_context_t* ctx, forecast_type_t type);
double evaluate_model_accuracy(forecasting_engine_context_t* ctx, forecast_type_t type,
                              const forecast_result_t* actual_results);

// Performance monitoring
int check_performance_degradation(forecasting_engine_context_t* ctx);
double calculate_performance_score(forecasting_engine_context_t* ctx);
int detect_performance_anomalies(forecasting_engine_context_t* ctx);
int trigger_performance_recovery(forecasting_engine_context_t* ctx);

// Statistics and reporting
void get_forecasting_statistics(forecasting_engine_context_t* ctx,
                               uint64_t* total_forecasts, uint64_t* accurate_forecasts,
                               double* accuracy_rate, uint64_t* patterns_found);
int get_forecast_history(forecasting_engine_context_t* ctx, 
                        forecast_result_t* history, int max_count);
int get_pattern_history(forecasting_engine_context_t* ctx, 
                       historical_pattern_t* history, int max_count);
double get_forecast_confidence(forecasting_engine_context_t* ctx, forecast_type_t type);

// Callback registration
void register_forecast_generation_callback(forecast_generation_callback_t callback);
void register_forecast_alert_callback(forecast_alert_callback_t callback);
void register_pattern_detection_callback(pattern_detection_callback_t callback);
void register_resource_projection_callback(resource_projection_callback_t callback);
void register_forecast_accuracy_callback(forecast_accuracy_callback_t callback);

// Integration functions
int integrate_with_predictive_analytics(forecasting_engine_context_t* ctx);
int integrate_with_performance_optimizer(forecasting_engine_context_t* ctx);
int integrate_with_adaptive_protocol_manager(forecasting_engine_context_t* ctx);
int apply_forecasting_optimizations(forecasting_engine_context_t* ctx);
int verify_forecasting_integrity(forecasting_engine_context_t* ctx);

// Utility functions
const char* forecast_type_to_string(forecast_type_t type);
const char* forecast_horizon_to_string(forecast_horizon_t horizon);
const char* seasonality_type_to_string(seasonality_type_t seasonality);
const char* trend_type_to_string(trend_type_t trend);
const char* alert_type_to_string(alert_type_t alert_type);
const char* alert_severity_to_string(alert_severity_t severity);
forecast_type_t string_to_forecast_type(const char* str);
forecast_horizon_t string_to_forecast_horizon(const char* str);
double calculate_forecast_error(const forecast_result_t* forecast);
int validate_forecast_result(const forecast_result_t* forecast);
int export_forecast_data(forecasting_engine_context_t* ctx, const char* filename);

#endif // PERFORMANCE_FORECASTING_ENGINE_H