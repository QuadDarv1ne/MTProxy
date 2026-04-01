/*
 * failure-predictor.h
 * Intelligent Failure Prediction and Prevention System
 *
 * This system predicts system failures before they occur and implements
 * preventive measures to maintain system reliability and uptime.
 */

#ifndef FAILURE_PREDICTOR_H
#define FAILURE_PREDICTOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Failure types
typedef enum {
    FAILURE_TYPE_UNKNOWN = 0,
    FAILURE_TYPE_MEMORY_LEAK,
    FAILURE_TYPE_RESOURCE_EXHAUSTION,
    FAILURE_TYPE_NETWORK_DISCONNECT,
    FAILURE_TYPE_CRYPTO_FAILURE,
    FAILURE_TYPE_CONNECTION_TIMEOUT,
    FAILURE_TYPE_BUFFER_OVERFLOW,
    FAILURE_TYPE_DEADLOCK,
    FAILURE_TYPE_PERFORMANCE_DEGRADATION,
    FAILURE_TYPE_SECURITY_BREACH
} failure_type_t;

// Failure severity levels
typedef enum {
    FAILURE_SEVERITY_LOW = 0,
    FAILURE_SEVERITY_MEDIUM,
    FAILURE_SEVERITY_HIGH,
    FAILURE_SEVERITY_CRITICAL,
    FAILURE_SEVERITY_CATASTROPHIC
} failure_severity_t;

// Prevention actions
typedef enum {
    PREVENTION_ACTION_NONE = 0,
    PREVENTION_ACTION_RESTART_COMPONENT,
    PREVENTION_ACTION_REALLOCATE_RESOURCES,
    PREVENTION_ACTION_CLEANUP_MEMORY,
    PREVENTION_ACTION_RECONNECT_NETWORK,
    PREVENTION_ACTION_REINITIALIZE_CRYPTO,
    PREVENTION_ACTION_THROTTLE_CONNECTIONS,
    PREVENTION_ACTION_ISOLATE_FAULTY_COMPONENT,
    PREVENTION_ACTION_TRIGGER_FAILOVER,
    PREVENTION_ACTION_ENHANCE_MONITORING
} prevention_action_t;

// System component types
typedef enum {
    COMPONENT_TYPE_NETWORK = 0,
    COMPONENT_TYPE_CRYPTO,
    COMPONENT_TYPE_MEMORY,
    COMPONENT_TYPE_CONNECTION,
    COMPONENT_TYPE_THREAD,
    COMPONENT_TYPE_STORAGE,
    COMPONENT_TYPE_SECURITY,
    COMPONENT_TYPE_MONITORING
} component_type_t;

// Failure prediction structure
typedef struct {
    uint64_t prediction_id;
    failure_type_t predicted_failure;
    failure_severity_t severity;
    component_type_t affected_component;
    uint64_t predicted_time_to_failure_ms;
    double confidence_score;  // 0.0 - 100.0
    uint64_t prediction_timestamp;
    prevention_action_t recommended_action;
    char action_description[256];
    double prevention_effectiveness;  // Expected success rate 0.0 - 100.0
    bool action_executed;
    uint64_t execution_time;
    char failure_indicators[512];
} failure_prediction_t;

// Component health structure
typedef struct {
    component_type_t component_type;
    char component_name[64];
    double health_score;  // 0.0 - 100.0
    uint64_t last_check_time;
    uint64_t failure_count;
    uint64_t recovery_count;
    double uptime_percentage;
    uint64_t error_rate;
    bool is_healthy;
    bool is_degraded;
    bool requires_attention;
    char health_status[256];
    void* component_context;
} component_health_t;

// Failure pattern structure
typedef struct {
    failure_type_t failure_type;
    component_type_t component_type;
    char pattern_signature[128];
    int occurrence_count;
    uint64_t first_occurrence_time;
    uint64_t last_occurrence_time;
    uint64_t average_time_between_failures;
    bool is_recurring;
    double recurrence_probability;
    char root_cause_analysis[256];
} failure_pattern_t;

// Prevention strategy structure
typedef struct {
    char strategy_name[64];
    prevention_action_t primary_action;
    prevention_action_t fallback_action;
    double success_probability;
    double cost_impact;
    double time_to_execute_ms;
    bool requires_downtime;
    int priority_level;  // 1-10
    char applicable_failures[256];
    bool is_active;
} prevention_strategy_t;

// Failure predictor configuration
typedef struct {
    int enable_failure_prediction;
    int prediction_window_seconds;
    int pattern_analysis_window_hours;
    double failure_threshold_confidence;
    int min_occurrences_for_pattern;
    bool enable_automatic_prevention;
    int prevention_timeout_seconds;
    int health_check_interval_seconds;
    double critical_health_threshold;
    int max_predictions_to_keep;
    bool enable_root_cause_analysis;
    int analysis_depth;
    bool enable_prevention_learning;
    int learning_window_days;
    bool enable_component_isolation;
    double isolation_threshold;
} failure_config_t;

// Failure statistics
typedef struct {
    uint64_t total_predictions_made;
    uint64_t accurate_predictions;
    uint64_t false_positives;
    uint64_t missed_failures;
    uint64_t preventive_actions_taken;
    uint64_t successful_preventions;
    uint64_t total_failures_detected;
    uint64_t total_failures_prevented;
    double prediction_accuracy_rate;
    double prevention_success_rate;
    double average_time_to_failure_detection_ms;
    double average_prevention_lead_time_ms;
    uint64_t last_analysis_time;
    uint64_t next_analysis_time;
    double system_reliability_score;
} failure_stats_t;

// Failure predictor context
typedef struct {
    failure_config_t config;
    failure_stats_t stats;
    component_health_t* component_health;
    int component_count;
    failure_prediction_t* prediction_history;
    int prediction_count;
    failure_pattern_t* failure_patterns;
    int pattern_count;
    prevention_strategy_t* prevention_strategies;
    int strategy_count;
    uint64_t last_prediction_time;
    uint64_t last_health_check_time;
    uint64_t last_pattern_analysis_time;
    uint64_t last_prevention_time;
    bool is_analyzing;
    bool is_predicting;
    bool is_preventing;
    void* prediction_models[8];  // Pointers to different prediction models
    int active_model_index;
    double component_reliability_scores[8];
    int reliability_history_index;
} failure_predictor_ctx_t;

// System metrics for failure analysis
typedef struct {
    uint64_t timestamp;
    double memory_usage_percent;
    double cpu_usage_percent;
    uint64_t active_connections;
    uint64_t pending_connections;
    double network_latency_ms;
    double packet_loss_rate;
    uint64_t buffer_overflow_count;
    uint64_t timeout_count;
    uint64_t error_count;
    uint64_t retry_count;
    double crypto_failure_rate;
    uint64_t deadlock_count;
    double thread_utilization_percent;
    uint64_t disk_io_errors;
    char system_state[128];
} system_metrics_t;

// Alert structure
typedef struct {
    uint64_t alert_id;
    failure_type_t failure_type;
    failure_severity_t severity;
    component_type_t component;
    uint64_t alert_timestamp;
    char alert_message[256];
    bool is_acknowledged;
    uint64_t acknowledgment_time;
    char acknowledged_by[64];
    bool requires_immediate_action;
    uint64_t escalation_level;
} failure_alert_t;

// Recovery procedure structure
typedef struct {
    char procedure_name[64];
    failure_type_t target_failure;
    component_type_t target_component;
    int (*recovery_function)(void* context);
    int estimated_recovery_time_seconds;
    double success_probability;
    char required_resources[256];
    bool is_automated;
    int priority;
    void* recovery_context;
} recovery_procedure_t;

// Callback function types
typedef void (*failure_prediction_callback_t)(const failure_prediction_t* prediction);
typedef void (*component_health_callback_t)(const component_health_t* health);
typedef void (*failure_alert_callback_t)(const failure_alert_t* alert);
typedef void (*prevention_action_callback_t)(const failure_prediction_t* prediction, bool success);
typedef void (*recovery_callback_t)(const recovery_procedure_t* procedure, bool success);

// Function prototypes

// Initialization and cleanup
int init_failure_predictor(failure_predictor_ctx_t* ctx);
int init_failure_predictor_with_config(failure_predictor_ctx_t* ctx, const failure_config_t* config);
void cleanup_failure_predictor(failure_predictor_ctx_t* ctx);

// Configuration management
void get_failure_config(failure_predictor_ctx_t* ctx, failure_config_t* config);
int set_failure_config(failure_predictor_ctx_t* ctx, const failure_config_t* config);

// Component health management
int register_component(failure_predictor_ctx_t* ctx, component_type_t type, const char* name, void* context);
int unregister_component(failure_predictor_ctx_t* ctx, const char* name);
int update_component_health(failure_predictor_ctx_t* ctx, const char* name, const component_health_t* health);
component_health_t* get_component_health(failure_predictor_ctx_t* ctx, const char* name);
component_health_t* get_all_component_health(failure_predictor_ctx_t* ctx, int* component_count);
bool is_component_healthy(failure_predictor_ctx_t* ctx, const char* name);
double get_component_reliability(failure_predictor_ctx_t* ctx, const char* name);

// Failure prediction
failure_prediction_t predict_system_failure(failure_predictor_ctx_t* ctx);
int analyze_failure_patterns(failure_predictor_ctx_t* ctx);
failure_pattern_t* detect_failure_patterns(failure_predictor_ctx_t* ctx, int* pattern_count);
bool is_failure_imminent(failure_predictor_ctx_t* ctx, failure_type_t* failure_type);
int get_failure_probability(failure_predictor_ctx_t* ctx, failure_type_t type, component_type_t component);

// System metrics collection
int add_system_metrics(failure_predictor_ctx_t* ctx, const system_metrics_t* metrics);
system_metrics_t get_latest_system_metrics(failure_predictor_ctx_t* ctx);
int get_metrics_history(failure_predictor_ctx_t* ctx, system_metrics_t* metrics, int max_count);
double calculate_system_stability_score(failure_predictor_ctx_t* ctx);

// Prevention management
int register_prevention_strategy(failure_predictor_ctx_t* ctx, const prevention_strategy_t* strategy);
int unregister_prevention_strategy(failure_predictor_ctx_t* ctx, const char* strategy_name);
int execute_prevention_action(failure_predictor_ctx_t* ctx, const failure_prediction_t* prediction);
prevention_action_t recommend_prevention_action(failure_predictor_ctx_t* ctx, const failure_prediction_t* prediction);
int validate_prevention_effectiveness(failure_predictor_ctx_t* ctx, const failure_prediction_t* prediction);

// Recovery management
int register_recovery_procedure(failure_predictor_ctx_t* ctx, const recovery_procedure_t* procedure);
int unregister_recovery_procedure(failure_predictor_ctx_t* ctx, const char* procedure_name);
int execute_recovery_procedure(failure_predictor_ctx_t* ctx, const char* procedure_name);
int get_recovery_procedures(failure_predictor_ctx_t* ctx, recovery_procedure_t* procedures, int max_count);
int trigger_automatic_recovery(failure_predictor_ctx_t* ctx, failure_type_t failure_type);

// Alert management
int generate_failure_alert(failure_predictor_ctx_t* ctx, const failure_prediction_t* prediction);
failure_alert_t* get_unacknowledged_alerts(failure_predictor_ctx_t* ctx, int* alert_count);
int acknowledge_alert(failure_predictor_ctx_t* ctx, uint64_t alert_id, const char* acknowledged_by);
int escalate_alert(failure_predictor_ctx_t* ctx, uint64_t alert_id);
int get_alert_history(failure_predictor_ctx_t* ctx, failure_alert_t* alerts, int max_count);

// Root cause analysis
int enable_root_cause_analysis(failure_predictor_ctx_t* ctx);
int disable_root_cause_analysis(failure_predictor_ctx_t* ctx);
char* perform_root_cause_analysis(failure_predictor_ctx_t* ctx, failure_type_t failure_type);
int get_failure_correlation(failure_predictor_ctx_t* ctx, failure_type_t type1, failure_type_t type2);

// Learning and adaptation
int enable_prevention_learning(failure_predictor_ctx_t* ctx);
int disable_prevention_learning(failure_predictor_ctx_t* ctx);
int update_prediction_models(failure_predictor_ctx_t* ctx);
double get_model_confidence_score(failure_predictor_ctx_t* ctx);
int retrain_failure_models(failure_predictor_ctx_t* ctx);

// Component isolation
int enable_component_isolation(failure_predictor_ctx_t* ctx);
int disable_component_isolation(failure_predictor_ctx_t* ctx);
int isolate_faulty_component(failure_predictor_ctx_t* ctx, const char* component_name);
int restore_isolated_component(failure_predictor_ctx_t* ctx, const char* component_name);
bool is_component_isolated(failure_predictor_ctx_t* ctx, const char* component_name);

// Statistics and reporting
failure_stats_t get_failure_statistics(failure_predictor_ctx_t* ctx);
void reset_failure_statistics(failure_predictor_ctx_t* ctx);
void print_failure_report(failure_predictor_ctx_t* ctx);
int export_failure_data(failure_predictor_ctx_t* ctx, const char* filename);

// Utility functions
const char* failure_type_to_string(failure_type_t type);
const char* failure_severity_to_string(failure_severity_t severity);
const char* component_type_to_string(component_type_t type);
const char* prevention_action_to_string(prevention_action_t action);
double calculate_failure_probability(const system_metrics_t* current_metrics, const system_metrics_t* baseline);
bool is_system_stable(const system_metrics_t* metrics);
uint64_t estimate_time_to_failure(double degradation_rate, double current_state, double failure_threshold);

// Callback registration
void register_failure_prediction_callback(failure_prediction_callback_t callback);
void register_component_health_callback(component_health_callback_t callback);
void register_failure_alert_callback(failure_alert_callback_t callback);
void register_prevention_action_callback(prevention_action_callback_t callback);
void register_recovery_callback(recovery_callback_t callback);

// Integration functions
int integrate_with_system_monitor(failure_predictor_ctx_t* ctx);
int integrate_with_predictive_optimizer(failure_predictor_ctx_t* ctx);
int integrate_with_proactive_allocator(failure_predictor_ctx_t* ctx);
int apply_failure_preventions(failure_predictor_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // FAILURE_PREDICTOR_H