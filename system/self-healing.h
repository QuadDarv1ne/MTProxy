/*
 * self-healing.h
 * Advanced Self-Healing System
 *
 * This system provides autonomous healing capabilities that automatically
 * detect, diagnose, and repair system issues without human intervention.
 */

#ifndef SELF_HEALING_H
#define SELF_HEALING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Healing types
typedef enum {
    HEALING_TYPE_UNKNOWN = 0,
    HEALING_TYPE_PERFORMANCE_RESTORE,
    HEALING_TYPE_FAILURE_RECOVERY,
    HEALING_TYPE_RESOURCE_REBALANCE,
    HEALING_TYPE_CONFIGURATION_ADJUST,
    HEALING_TYPE_SECURITY_PATCH,
    HEALING_TYPE_LOAD_BALANCE,
    HEALING_TYPE_COMPONENT_RESTART
} healing_type_t;

// Healing complexity levels
typedef enum {
    COMPLEXITY_LOW = 0,
    COMPLEXITY_MEDIUM,
    COMPLEXITY_HIGH,
    COMPLEXITY_CRITICAL
} complexity_level_t;

// Self-healing strategies
typedef enum {
    STRATEGY_AUTOMATIC = 0,
    STRATEGY_SUPERVISED,
    STRATEGY_CONSERVATIVE,
    STRATEGY_AGGRESSIVE,
    STRATEGY_LEARNING
} healing_strategy_t;

// System diagnosis structure
typedef struct {
    uint64_t diagnosis_id;
    healing_type_t primary_healing_type;
    complexity_level_t complexity;
    double severity_score;  // 0.0 - 100.0
    uint64_t diagnosis_time;
    char issue_description[256];
    char affected_components[512];
    char root_cause_analysis[256];
    double recovery_probability;  // 0.0 - 100.0
    uint64_t estimated_recovery_time_ms;
    char required_actions[1024];
    bool is_critical_issue;
    healing_strategy_t recommended_strategy;
    int priority_level;  // 1-10
} system_diagnosis_t;

// Healing operation structure
typedef struct {
    uint64_t healing_id;
    uint64_t diagnosis_id;
    healing_type_t healing_type;
    system_diagnosis_t* diagnosis;
    char* executed_steps;
    uint64_t start_time;
    uint64_t end_time;
    bool is_completed;
    bool is_successful;
    double effectiveness_score;  // 0.0 - 100.0
    char result_summary[256];
    uint64_t resource_cost;
    char error_messages[512];
    int retry_count;
    int max_retries;
} healing_operation_t;

// Self-healing configuration
typedef struct {
    int enable_autonomous_healing;
    healing_strategy_t default_strategy;
    int max_concurrent_healing_operations;
    int healing_timeout_seconds;
    double success_threshold_percent;
    int retry_attempts;
    int retry_delay_seconds;
    bool enable_healing_learning;
    int learning_window_days;
    bool enable_preventive_healing;
    int preventive_check_interval_seconds;
    double preventive_threshold_percent;
    bool enable_component_isolation;
    int isolation_timeout_seconds;
    bool enable_rollback_mechanism;
    int rollback_timeout_seconds;
    bool enable_performance_monitoring;
    int monitoring_interval_seconds;
} self_healing_config_t;

// Healing capabilities
typedef struct {
    bool can_restart_components;
    bool can_reallocate_resources;
    bool can_modify_configurations;
    bool can_isolate_components;
    bool can_perform_rollback;
    bool can_apply_patches;
    bool can_balance_load;
    bool can_cleanup_resources;
    double capability_scores[8];  // Scores for each capability
    char capability_status[8][64];  // Status messages for each capability
} healing_capabilities_t;

// Self-healing statistics
typedef struct {
    uint64_t total_healing_operations;
    uint64_t successful_healings;
    uint64_t failed_healings;
    uint64_t automatic_healings;
    uint64_t manual_interventions;
    uint64_t prevented_issues;
    uint64_t false_positives;
    double healing_success_rate;
    double average_healing_time_ms;
    double average_recovery_time_ms;
    double system_availability_percent;
    uint64_t last_healing_time;
    uint64_t next_preventive_check_time;
    double overall_system_health_score;
    uint64_t healing_cost_total;
} healing_stats_t;

// Self-healing context
typedef struct {
    self_healing_config_t config;
    healing_stats_t stats;
    healing_capabilities_t capabilities;
    system_diagnosis_t* diagnosis_history;
    int diagnosis_count;
    healing_operation_t* healing_operations;
    int operation_count;
    uint64_t last_diagnosis_time;
    uint64_t last_healing_time;
    uint64_t last_preventive_check_time;
    bool is_diagnosing;
    bool is_healing;
    bool is_learning;
    healing_strategy_t current_strategy;
    void* healing_algorithms[6];  // Pointers to different healing algorithms
    int active_algorithm_index;
    double system_health_trend[1000];
    int trend_index;
    void* integration_contexts[10];  // Contexts for integrated systems
} self_healing_ctx_t;

// Healing workflow structure
typedef struct {
    char workflow_name[64];
    healing_type_t target_healing_type;
    complexity_level_t max_complexity;
    int (*diagnosis_function)(self_healing_ctx_t* ctx, system_diagnosis_t* diagnosis);
    int (*healing_function)(self_healing_ctx_t* ctx, const system_diagnosis_t* diagnosis, healing_operation_t* operation);
    int (*verification_function)(self_healing_ctx_t* ctx, const healing_operation_t* operation);
    double success_probability;
    int estimated_duration_seconds;
    char required_permissions[256];
    bool is_enabled;
    int priority;
} healing_workflow_t;

// System state structure
typedef struct {
    uint64_t timestamp;
    double overall_health_score;
    double cpu_health;
    double memory_health;
    double network_health;
    double storage_health;
    double security_health;
    uint64_t active_components;
    uint64_t failed_components;
    uint64_t degraded_components;
    double performance_score;
    uint64_t error_count;
    uint64_t warning_count;
    char system_status[128];
    bool is_stable;
    bool requires_attention;
    bool is_degraded;
    bool is_critical;
} system_state_t;

// Healing policy structure
typedef struct {
    char policy_name[64];
    healing_strategy_t strategy;
    complexity_level_t max_allowed_complexity;
    double minimum_success_probability;
    int max_allowed_duration_seconds;
    bool require_admin_approval;
    char approval_conditions[256];
    bool enable_rollback;
    int rollback_conditions;
    char applicable_scenarios[512];
    bool is_active;
} healing_policy_t;

// Recovery plan structure
typedef struct {
    uint64_t plan_id;
    healing_type_t target_healing_type;
    uint64_t creation_time;
    char plan_description[256];
    int step_count;
    char** healing_steps;
    uint64_t* estimated_times;
    double* success_probabilities;
    char* required_resources;
    bool is_executable;
    uint64_t total_estimated_time;
    double overall_success_probability;
} recovery_plan_t;

// Callback function types
typedef void (*diagnosis_callback_t)(const system_diagnosis_t* diagnosis);
typedef void (*healing_callback_t)(const healing_operation_t* operation);
typedef void (*healing_stats_callback_t)(const healing_stats_t* stats);
typedef void (*system_state_callback_t)(const system_state_t* state);
typedef int (*component_control_callback_t)(const char* component_name, int action);
typedef int (*resource_management_callback_t)(const char* resource_type, uint64_t amount, int action);

// Function prototypes

// Initialization and cleanup
int init_self_healing(self_healing_ctx_t* ctx);
int init_self_healing_with_config(self_healing_ctx_t* ctx, const self_healing_config_t* config);
void cleanup_self_healing(self_healing_ctx_t* ctx);

// Configuration management
void get_self_healing_config(self_healing_ctx_t* ctx, self_healing_config_t* config);
int set_self_healing_config(self_healing_ctx_t* ctx, const self_healing_config_t* config);

// System diagnosis
system_diagnosis_t diagnose_system_issues(self_healing_ctx_t* ctx);
int perform_comprehensive_diagnosis(self_healing_ctx_t* ctx);
int analyze_system_health(self_healing_ctx_t* ctx, system_state_t* state);
system_diagnosis_t* get_diagnosis_history(self_healing_ctx_t* ctx, int* diagnosis_count);
bool is_system_healthy(self_healing_ctx_t* ctx);
double get_system_health_score(self_healing_ctx_t* ctx);

// Healing operations
healing_operation_t initiate_healing_operation(self_healing_ctx_t* ctx, const system_diagnosis_t* diagnosis);
int execute_healing_operation(self_healing_ctx_t* ctx, healing_operation_t* operation);
int monitor_healing_progress(self_healing_ctx_t* ctx, uint64_t healing_id);
int cancel_healing_operation(self_healing_ctx_t* ctx, uint64_t healing_id);
healing_operation_t* get_active_healing_operations(self_healing_ctx_t* ctx, int* operation_count);
healing_operation_t* get_completed_healing_operations(self_healing_ctx_t* ctx, int* operation_count);

// Healing workflow management
int register_healing_workflow(self_healing_ctx_t* ctx, const healing_workflow_t* workflow);
int unregister_healing_workflow(self_healing_ctx_t* ctx, const char* workflow_name);
int enable_healing_workflow(self_healing_ctx_t* ctx, const char* workflow_name);
int disable_healing_workflow(self_healing_ctx_t* ctx, const char* workflow_name);
healing_workflow_t* get_healing_workflow(self_healing_ctx_t* ctx, const char* workflow_name);

// Healing policy management
int register_healing_policy(self_healing_ctx_t* ctx, const healing_policy_t* policy);
int unregister_healing_policy(self_healing_ctx_t* ctx, const char* policy_name);
int set_active_healing_policy(self_healing_ctx_t* ctx, const char* policy_name);
healing_policy_t* get_active_healing_policy(self_healing_ctx_t* ctx);
int validate_healing_policy(self_healing_ctx_t* ctx, const healing_policy_t* policy);

// Recovery planning
recovery_plan_t* generate_recovery_plan(self_healing_ctx_t* ctx, const system_diagnosis_t* diagnosis);
int execute_recovery_plan(self_healing_ctx_t* ctx, recovery_plan_t* plan);
int validate_recovery_plan(self_healing_ctx_t* ctx, const recovery_plan_t* plan);
int optimize_recovery_plan(self_healing_ctx_t* ctx, recovery_plan_t* plan);

// Preventive healing
int enable_preventive_healing(self_healing_ctx_t* ctx);
int disable_preventive_healing(self_healing_ctx_t* ctx);
int perform_preventive_check(self_healing_ctx_t* ctx);
int schedule_preventive_maintenance(self_healing_ctx_t* ctx, uint64_t scheduled_time);
bool is_preventive_healing_enabled(self_healing_ctx_t* ctx);

// Component management
int isolate_faulty_component(self_healing_ctx_t* ctx, const char* component_name);
int restore_isolated_component(self_healing_ctx_t* ctx, const char* component_name);
int restart_component(self_healing_ctx_t* ctx, const char* component_name);
int reconfigure_component(self_healing_ctx_t* ctx, const char* component_name, const char* new_config);
bool is_component_isolated(self_healing_ctx_t* ctx, const char* component_name);

// Rollback mechanisms
int enable_rollback_mechanism(self_healing_ctx_t* ctx);
int disable_rollback_mechanism(self_healing_ctx_t* ctx);
int perform_rollback(self_healing_ctx_t* ctx, uint64_t operation_id);
int create_rollback_point(self_healing_ctx_t* ctx, const char* description);
int get_rollback_points(self_healing_ctx_t* ctx, uint64_t* points, int max_points);

// Learning and adaptation
int enable_healing_learning(self_healing_ctx_t* ctx);
int disable_healing_learning(self_healing_ctx_t* ctx);
int update_healing_models(self_healing_ctx_t* ctx);
double get_learning_effectiveness_score(self_healing_ctx_t* ctx);
int retrain_healing_algorithms(self_healing_ctx_t* ctx);

// Capability management
int assess_healing_capabilities(self_healing_ctx_t* ctx);
int enable_healing_capability(self_healing_ctx_t* ctx, int capability_index);
int disable_healing_capability(self_healing_ctx_t* ctx, int capability_index);
bool is_healing_capability_enabled(self_healing_ctx_t* ctx, int capability_index);
double get_capability_score(self_healing_ctx_t* ctx, int capability_index);

// Statistics and reporting
healing_stats_t get_healing_statistics(self_healing_ctx_t* ctx);
void reset_healing_statistics(self_healing_ctx_t* ctx);
void print_healing_report(self_healing_ctx_t* ctx);
int export_healing_data(self_healing_ctx_t* ctx, const char* filename);

// Utility functions
const char* healing_type_to_string(healing_type_t type);
const char* complexity_level_to_string(complexity_level_t level);
const char* healing_strategy_to_string(healing_strategy_t strategy);
double calculate_system_stability(const system_state_t* state);
bool is_healing_required(const system_diagnosis_t* diagnosis);
uint64_t estimate_healing_duration(const system_diagnosis_t* diagnosis);
int get_healing_priority(const system_diagnosis_t* diagnosis);

// Callback registration
void register_diagnosis_callback(diagnosis_callback_t callback);
void register_healing_callback(healing_callback_t callback);
void register_healing_stats_callback(healing_stats_callback_t callback);
void register_system_state_callback(system_state_callback_t callback);
void register_component_control_callback(component_control_callback_t callback);
void register_resource_management_callback(resource_management_callback_t callback);

// Integration functions
int integrate_with_predictive_optimizer(self_healing_ctx_t* ctx);
int integrate_with_proactive_allocator(self_healing_ctx_t* ctx);
int integrate_with_failure_predictor(self_healing_ctx_t* ctx);
int integrate_with_performance_monitor(self_healing_ctx_t* ctx);
int apply_self_healing_operations(self_healing_ctx_t* ctx);
int verify_healing_integrity(self_healing_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // SELF_HEALING_H