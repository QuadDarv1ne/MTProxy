/*
 * intelligent-optimization-integrator.h
 * Intelligent System Integration Layer for MTProxy
 *
 * Coordinates and integrates all optimization systems including adaptive protocol management,
 * predictive analytics, performance forecasting, and existing optimization components.
 */

#ifndef INTELLIGENT_OPTIMIZATION_INTEGRATOR_H
#define INTELLIGENT_OPTIMIZATION_INTEGRATOR_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations for integrated systems
typedef struct adaptive_protocol_manager adaptive_protocol_manager_t;
typedef struct predictive_analytics_context predictive_analytics_context_t;
typedef struct forecasting_engine_context forecasting_engine_context_t;

// System types
typedef enum {
    SYSTEM_TYPE_ADAPTIVE_PROTOCOL_MANAGER = 0,
    SYSTEM_TYPE_PREDICTIVE_ANALYTICS = 1,
    SYSTEM_TYPE_PERFORMANCE_FORECASTING = 2,
    SYSTEM_TYPE_PERFORMANCE_OPTIMIZER = 3,
    SYSTEM_TYPE_MEMORY_OPTIMIZER = 4,
    SYSTEM_TYPE_CONNECTION_POOL = 5,
    SYSTEM_TYPE_SECURITY_MONITOR = 6,
    SYSTEM_TYPE_LOAD_BALANCER = 7,
    SYSTEM_TYPE_AUTO_SCALER = 8,
    SYSTEM_TYPE_THREAT_DETECTOR = 9
} system_type_t;

// Integration modes
typedef enum {
    INTEGRATION_MODE_STANDALONE = 0,    // Systems operate independently
    INTEGRATION_MODE_COORDINATED = 1,   // Systems coordinate decisions
    INTEGRATION_MODE_HIERARCHICAL = 2,  // Master-slave relationship
    INTEGRATION_MODE_ENSEMBLE = 3       // Collective decision making
} integration_mode_t;

// System status
typedef enum {
    SYSTEM_STATUS_UNINITIALIZED = 0,
    SYSTEM_STATUS_INITIALIZED = 1,
    SYSTEM_STATUS_ACTIVE = 2,
    SYSTEM_STATUS_PAUSED = 3,
    SYSTEM_STATUS_ERROR = 4,
    SYSTEM_STATUS_DEGRADED = 5
} system_status_t;

// Integration event types
typedef enum {
    EVENT_TYPE_SYSTEM_INITIALIZED = 0,
    EVENT_TYPE_PERFORMANCE_DEGRADATION = 1,
    EVENT_TYPE_RESOURCE_PRESSURE = 2,
    EVENT_TYPE_PROTOCOL_SWITCH = 3,
    EVENT_TYPE_ANOMALY_DETECTED = 4,
    EVENT_TYPE_FORECAST_GENERATED = 5,
    EVENT_TYPE_SCALING_EVENT = 6,
    EVENT_TYPE_THREAT_DETECTED = 7,
    EVENT_TYPE_OPTIMIZATION_APPLIED = 8,
    EVENT_TYPE_SYSTEM_ERROR = 9
} integration_event_type_t;

// Coordination strategy
typedef enum {
    STRATEGY_CONSENSUS = 0,     // All systems must agree
    STRATEGY_MAJORITY = 1,      // Majority vote
    STRATEGY_WEIGHTED = 2,      // Weighted decision based on confidence
    STRATEGY_HIERARCHICAL = 3,  // Follow hierarchy
    STRATEGY_EMERGENCY = 4      // Emergency override
} coordination_strategy_t;

// System integration configuration
typedef struct {
    bool enable_cross_system_communication;
    bool enable_shared_decision_making;
    bool enable_performance_feedback;
    bool enable_resource_sharing;
    bool enable_conflict_resolution;
    bool enable_emergency_coordination;
    bool enable_predictive_coordination;
    bool enable_adaptive_integration;
    int communication_timeout_ms;
    int decision_timeout_ms;
    coordination_strategy_t default_strategy;
    double confidence_threshold;
    int max_concurrent_decisions;
    bool enable_logging;
    int log_level;
    bool enable_metrics_collection;
    int metrics_collection_interval_seconds;
    bool enable_health_monitoring;
    int health_check_interval_seconds;
    bool enable_automatic_recovery;
    int recovery_attempts;
    bool enable_performance_optimization_sharing;
    bool enable_security_information_sharing;
    double integration_efficiency_target;
} integration_config_t;

// System information
typedef struct {
    system_type_t system_type;
    system_status_t current_status;
    void* system_context;
    double performance_score;  // 0.0 - 100.0
    double reliability_score;  // 0.0 - 100.0
    double resource_utilization;
    uint64_t last_update_time;
    uint64_t uptime_seconds;
    int error_count;
    int successful_operations;
    int failed_operations;
    char system_name[64];
    char system_version[32];
    bool is_critical_system;
    bool supports_predictive_features;
    bool supports_adaptive_features;
    int supported_integration_features;
} system_info_t;

// Integration event
typedef struct {
    uint64_t event_id;
    integration_event_type_t event_type;
    system_type_t source_system;
    system_type_t target_system;
    uint64_t timestamp;
    void* event_data;
    size_t data_size;
    double event_severity;  // 0.0 - 100.0
    bool requires_immediate_attention;
    char event_description[256];
    char recommended_action[256];
    bool action_taken;
    uint64_t resolution_time;
} integration_event_t;

// Decision context
typedef struct {
    uint64_t decision_id;
    system_type_t requesting_system;
    system_type_t affected_systems[16];
    int affected_system_count;
    coordination_strategy_t strategy_used;
    uint64_t decision_timestamp;
    uint64_t deadline_timestamp;
    double confidence_level;
    char decision_description[256];
    char decision_rationale[512];
    bool decision_executed;
    bool decision_successful;
    double execution_time_ms;
    char execution_result[256];
    bool requires_follow_up;
    uint64_t follow_up_deadline;
} decision_context_t;

// Performance correlation
typedef struct {
    system_type_t system1;
    system_type_t system2;
    double correlation_coefficient;  // -1.0 to 1.0
    uint64_t correlation_timestamp;
    int sample_count;
    double mutual_impact_score;  // 0.0 - 100.0
    bool positive_correlation;
    char correlation_description[128];
    bool is_significant;
} performance_correlation_t;

// Resource sharing agreement
typedef struct {
    system_type_t provider_system;
    system_type_t consumer_system;
    char resource_type[64];
    double shared_amount;
    uint64_t allocation_timestamp;
    uint64_t expiration_timestamp;
    double utilization_efficiency;
    bool is_active;
    char agreement_terms[256];
    int priority_level;
} resource_sharing_agreement_t;

// Conflict resolution
typedef struct {
    uint64_t conflict_id;
    system_type_t conflicting_systems[8];
    int conflicting_system_count;
    char conflict_description[256];
    coordination_strategy_t resolution_strategy;
    uint64_t detection_timestamp;
    uint64_t resolution_timestamp;
    bool resolved_successfully;
    char resolution_method[128];
    double resolution_confidence;
    char resolution_outcome[256];
} conflict_resolution_t;

// Integration metrics
typedef struct {
    uint64_t total_integration_events;
    uint64_t successful_decisions;
    uint64_t failed_decisions;
    uint64_t conflicts_detected;
    uint64_t conflicts_resolved;
    uint64_t system_initializations;
    uint64_t system_errors;
    uint64_t performance_improvements;
    uint64_t resource_optimizations;
    uint64_t security_events;
    double overall_integration_efficiency;
    double average_decision_time_ms;
    double conflict_resolution_rate;
    double system_availability_percentage;
    uint64_t last_metrics_update;
} integration_metrics_t;

// Main integration context
typedef struct {
    // Configuration
    integration_config_t config;
    
    // System management
    system_info_t* integrated_systems;
    int system_count;
    int max_systems;
    system_status_t overall_system_status;
    
    // Event management
    integration_event_t* event_history;
    int event_history_count;
    int max_event_history;
    integration_event_t* pending_events;
    int pending_event_count;
    int max_pending_events;
    
    // Decision coordination
    decision_context_t* decision_history;
    int decision_history_count;
    int max_decision_history;
    decision_context_t* pending_decisions;
    int pending_decision_count;
    int max_pending_decisions;
    
    // Performance analysis
    performance_correlation_t* correlations;
    int correlation_count;
    int max_correlations;
    double system_performance_matrix[16][16];  // Performance relationships
    
    // Resource management
    resource_sharing_agreement_t* resource_agreements;
    int agreement_count;
    int max_agreements;
    double total_shared_resources;
    double resource_utilization_efficiency;
    
    // Conflict management
    conflict_resolution_t* conflict_history;
    int conflict_history_count;
    int max_conflict_history;
    int active_conflicts;
    
    // Integration state
    integration_mode_t current_integration_mode;
    coordination_strategy_t active_strategy;
    bool integration_active;
    bool emergency_mode;
    bool learning_mode;
    bool adaptive_integration;
    uint64_t last_coordination_update;
    uint64_t last_performance_review;
    uint64_t last_system_health_check;
    
    // Metrics and statistics
    integration_metrics_t metrics;
    double current_integration_score;
    double predicted_integration_efficiency;
    double system_stability_score;
    int system_degradation_events;
    
    // Emergency management
    system_type_t emergency_coordinator;
    uint64_t emergency_start_time;
    double emergency_severity;
    char emergency_description[256];
    bool emergency_resolved;
    uint64_t emergency_resolution_time;
    
    // Learning and adaptation
    double integration_model_weights[16][16];  // ML model for system relationships
    int learning_iterations;
    double adaptation_rate;
    bool model_retraining_needed;
    uint64_t last_model_training;
    
    // State management
    int initialized;
    int active;
    uint64_t start_time;
    char integration_id[64];
    char version_string[32];
} integration_context_t;

// Callback function types
typedef void (*integration_event_callback_t)(const integration_event_t* event);
typedef void (*decision_made_callback_t)(const decision_context_t* decision);
typedef void (*system_status_callback_t)(system_type_t system, system_status_t status);
typedef void (*performance_correlation_callback_t)(const performance_correlation_t* correlation);
typedef void (*conflict_resolution_callback_t)(const conflict_resolution_t* resolution);
typedef void (*integration_metrics_callback_t)(const integration_metrics_t* metrics);

// Function declarations

// Initialization and cleanup
int init_integration_layer(integration_context_t* ctx);
int init_integration_layer_with_config(integration_context_t* ctx, 
                                     const integration_config_t* config);
void cleanup_integration_layer(integration_context_t* ctx);

// Configuration management
void get_integration_config(integration_context_t* ctx, integration_config_t* config);
int set_integration_config(integration_context_t* ctx, const integration_config_t* config);
int set_integration_mode(integration_context_t* ctx, integration_mode_t mode);
int set_coordination_strategy(integration_context_t* ctx, coordination_strategy_t strategy);

// System management
int register_system(integration_context_t* ctx, system_type_t system_type, 
                   void* system_context, const char* system_name);
int unregister_system(integration_context_t* ctx, system_type_t system_type);
int update_system_status(integration_context_t* ctx, system_type_t system_type, 
                        system_status_t status);
system_info_t* get_system_info(integration_context_t* ctx, system_type_t system_type);
int get_all_systems(integration_context_t* ctx, system_info_t* systems, int max_systems);
system_status_t get_overall_system_status(integration_context_t* ctx);

// Event management
int post_integration_event(integration_context_t* ctx, const integration_event_t* event);
int process_pending_events(integration_context_t* ctx);
int get_event_history(integration_context_t* ctx, integration_event_t* events, int max_events);
int clear_event_history(integration_context_t* ctx);

// Decision coordination
decision_context_t make_coordinated_decision(integration_context_t* ctx, 
                                           system_type_t requesting_system,
                                           const char* decision_description);
int execute_decision(integration_context_t* ctx, const decision_context_t* decision);
int get_decision_history(integration_context_t* ctx, decision_context_t* decisions, int max_decisions);
int resolve_pending_decisions(integration_context_t* ctx);

// Performance analysis
int analyze_system_performance(integration_context_t* ctx);
int calculate_performance_correlations(integration_context_t* ctx);
performance_correlation_t* get_performance_correlation(integration_context_t* ctx, 
                                                     system_type_t system1, system_type_t system2);
int get_all_correlations(integration_context_t* ctx, performance_correlation_t* correlations, 
                        int max_correlations);
double get_system_performance_score(integration_context_t* ctx, system_type_t system_type);

// Resource sharing
int create_resource_sharing_agreement(integration_context_t* ctx, 
                                    system_type_t provider, system_type_t consumer,
                                    const char* resource_type, double amount);
int terminate_resource_sharing_agreement(integration_context_t* ctx, 
                                       system_type_t provider, system_type_t consumer);
int get_active_resource_agreements(integration_context_t* ctx, 
                                 resource_sharing_agreement_t* agreements, int max_agreements);
double get_total_resource_utilization(integration_context_t* ctx);

// Conflict resolution
int detect_conflicts(integration_context_t* ctx);
int resolve_conflict(integration_context_t* ctx, const conflict_resolution_t* conflict);
int get_conflict_history(integration_context_t* ctx, conflict_resolution_t* conflicts, 
                        int max_conflicts);
int get_active_conflicts(integration_context_t* ctx, conflict_resolution_t* conflicts, 
                        int max_conflicts);

// Emergency management
int enter_emergency_mode(integration_context_t* ctx, system_type_t coordinator, 
                        double severity, const char* description);
int exit_emergency_mode(integration_context_t* ctx);
bool is_emergency_mode_active(integration_context_t* ctx);
system_type_t get_emergency_coordinator(integration_context_t* ctx);

// Learning and adaptation
int update_integration_model(integration_context_t* ctx);
int train_integration_models(integration_context_t* ctx);
int enable_adaptive_integration(integration_context_t* ctx, bool enable);
int reset_learning_model(integration_context_t* ctx);

// Integration with existing systems
int integrate_adaptive_protocol_manager(integration_context_t* ctx, 
                                      adaptive_protocol_manager_t* manager);
int integrate_predictive_analytics(integration_context_t* ctx, 
                                 predictive_analytics_context_t* analytics);
int integrate_performance_forecasting(integration_context_t* ctx, 
                                    forecasting_engine_context_t* forecasting);
int integrate_performance_optimizer(integration_context_t* ctx, void* optimizer);
int integrate_memory_optimizer(integration_context_t* ctx, void* memory_optimizer);
int integrate_connection_pool(integration_context_t* ctx, void* connection_pool);
int integrate_security_monitor(integration_context_t* ctx, void* security_monitor);

// Metrics and monitoring
void get_integration_metrics(integration_context_t* ctx, integration_metrics_t* metrics);
int get_integration_score(integration_context_t* ctx, double* score);
int get_system_stability_score(integration_context_t* ctx, double* score);
int generate_integration_report(integration_context_t* ctx, char* report, int max_length);

// Callback registration
void register_integration_event_callback(integration_event_callback_t callback);
void register_decision_made_callback(decision_made_callback_t callback);
void register_system_status_callback(system_status_callback_t callback);
void register_performance_correlation_callback(performance_correlation_callback_t callback);
void register_conflict_resolution_callback(conflict_resolution_callback_t callback);
void register_integration_metrics_callback(integration_metrics_callback_t callback);

// Utility functions
const char* system_type_to_string(system_type_t type);
const char* integration_mode_to_string(integration_mode_t mode);
const char* coordination_strategy_to_string(coordination_strategy_t strategy);
const char* system_status_to_string(system_status_t status);
const char* event_type_to_string(integration_event_type_t type);
system_type_t string_to_system_type(const char* str);
integration_mode_t string_to_integration_mode(const char* str);
coordination_strategy_t string_to_coordination_strategy(const char* str);
int calculate_system_health_score(integration_context_t* ctx);
bool validate_system_compatibility(integration_context_t* ctx, system_type_t system1, 
                                  system_type_t system2);
int export_integration_data(integration_context_t* ctx, const char* filename);

#endif // INTELLIGENT_OPTIMIZATION_INTEGRATOR_H