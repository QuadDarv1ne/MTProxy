/*
 * advanced-diagnostic-system.h
 * Advanced Diagnostic System for MTProxy
 *
 * Comprehensive diagnostic and troubleshooting system with real-time monitoring,
 * performance analysis, and automated issue detection.
 */

#ifndef ADVANCED_DIAGNOSTIC_SYSTEM_H
#define ADVANCED_DIAGNOSTIC_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

// Diagnostic categories
typedef enum {
    DIAG_CATEGORY_GENERAL = 0,
    DIAG_CATEGORY_NETWORK = 1,
    DIAG_CATEGORY_PERFORMANCE = 2,
    DIAG_CATEGORY_SECURITY = 3,
    DIAG_CATEGORY_MEMORY = 4,
    DIAG_CATEGORY_PROTOCOL = 5,
    DIAG_CATEGORY_CONNECTION = 6,
    DIAG_CATEGORY_CRYPTO = 7,
    DIAG_CATEGORY_DISK_IO = 8,
    DIAG_CATEGORY_SYSTEM_HEALTH = 9
} diag_category_t;

// Diagnostic severity levels
typedef enum {
    DIAG_SEVERITY_INFO = 0,
    DIAG_SEVERITY_WARNING = 1,
    DIAG_SEVERITY_ERROR = 2,
    DIAG_SEVERITY_CRITICAL = 3,
    DIAG_SEVERITY_EMERGENCY = 4
} diag_severity_t;

// Diagnostic issue types
typedef enum {
    DIAG_ISSUE_TYPE_CONNECTION_TIMEOUT = 0,
    DIAG_ISSUE_TYPE_HIGH_LATENCY = 1,
    DIAG_ISSUE_TYPE_MEMORY_LEAK = 2,
    DIAG_ISSUE_TYPE_RESOURCE_STARVATION = 3,
    DIAG_ISSUE_TYPE_PROTOCOL_ERROR = 4,
    DIAG_ISSUE_TYPE_CRYPTO_FAILURE = 5,
    DIAG_ISSUE_TYPE_SECURITY_BREACH = 6,
    DIAG_ISSUE_TYPE_DISK_SPACE_LOW = 7,
    DIAG_ISSUE_TYPE_BANDWIDTH_LIMIT = 8,
    DIAG_ISSUE_TYPE_DEADLOCK_DETECTED = 9,
    DIAG_ISSUE_TYPE_THREAD_STARVATION = 10,
    DIAG_ISSUE_TYPE_CACHE_MISS_HIGH = 11
} diag_issue_type_t;

// Diagnostic status
typedef enum {
    DIAG_STATUS_PENDING = 0,
    DIAG_STATUS_RUNNING = 1,
    DIAG_STATUS_COMPLETED = 2,
    DIAG_STATUS_ERROR = 3,
    DIAG_STATUS_TIMEOUT = 4,
    DIAG_STATUS_ABORTED = 5
} diag_status_t;

// Diagnostic test types
typedef enum {
    DIAG_TEST_CONNECTIVITY = 0,
    DIAG_TEST_PERFORMANCE = 1,
    DIAG_TEST_SECURITY = 2,
    DIAG_TEST_STRESS = 3,
    DIAG_TEST_MEMORY = 4,
    DIAG_TEST_PROTOCOL = 5,
    DIAG_TEST_CRYPTO = 6,
    DIAG_TEST_NETWORK = 7
} diag_test_type_t;

// Diagnostic result
typedef struct {
    uint64_t diagnostic_id;
    diag_category_t category;
    diag_issue_type_t issue_type;
    diag_severity_t severity;
    diag_status_t status;
    uint64_t timestamp;
    char description[256];
    char suggested_solution[512];
    double confidence_score;  // 0.0 - 100.0
    uint64_t execution_time_ms;
    bool requires_immediate_action;
    char affected_component[128];
    double impact_score;  // 0.0 - 100.0
    bool is_recurring_issue;
    uint64_t recurrence_count;
    char diagnostic_details[1024];
} diagnostic_result_t;

// Diagnostic test configuration
typedef struct {
    diag_test_type_t test_type;
    bool enable_test;
    int priority;  // 1-10, higher is more important
    uint64_t interval_ms;
    bool auto_run_on_error;
    bool detailed_logging;
    int timeout_seconds;
    bool include_in_health_check;
    char test_parameters[256];
    bool run_continuously;
    int max_execution_time_seconds;
    bool generate_detailed_report;
} diagnostic_test_config_t;

// System metrics
typedef struct {
    uint64_t timestamp;
    double cpu_usage_percent;
    double memory_usage_percent;
    double disk_usage_percent;
    double network_in_mbps;
    double network_out_mbps;
    uint64_t active_connections;
    uint64_t total_connections;
    uint64_t failed_connections;
    double avg_response_time_ms;
    double p95_response_time_ms;
    double p99_response_time_ms;
    uint64_t requests_per_second;
    double error_rate_percent;
    uint64_t current_rss_kb;
    uint64_t peak_rss_kb;
    uint64_t virtual_memory_kb;
    uint64_t open_files_count;
    uint64_t threads_count;
    double crypto_operations_per_second;
    double encryption_time_avg_ms;
    double decryption_time_avg_ms;
    uint64_t dropped_packets;
    uint64_t corrupted_packets;
    double cache_hit_ratio;
    uint64_t cache_misses;
    uint64_t cache_hits;
} system_metrics_t;

// Diagnostic session
typedef struct {
    uint64_t session_id;
    diag_test_type_t test_type;
    diag_category_t category;
    uint64_t start_time;
    uint64_t end_time;
    diag_status_t status;
    int total_tests_run;
    int tests_passed;
    int tests_failed;
    int tests_skipped;
    diagnostic_result_t* results;
    int result_count;
    int max_results;
    system_metrics_t baseline_metrics;
    system_metrics_t final_metrics;
    double overall_performance_score;
    bool diagnostic_complete;
    char session_description[256];
    bool auto_cleanup_after_completion;
} diagnostic_session_t;

// Health check result
typedef struct {
    uint64_t check_id;
    diag_category_t category;
    uint64_t timestamp;
    bool is_healthy;
    double health_score;  // 0.0 - 100.0
    char status_message[256];
    diagnostic_result_t* issues_found;
    int issue_count;
    int max_issues;
    uint64_t response_time_ms;
    bool requires_attention;
    char component_name[128];
} health_check_result_t;

// Diagnostic configuration
typedef struct {
    bool enable_real_time_monitoring;
    bool enable_auto_diagnosis;
    bool enable_detailed_logging;
    bool enable_performance_profiling;
    bool enable_security_scanning;
    bool enable_memory_tracking;
    bool enable_network_monitoring;
    uint64_t metrics_collection_interval_ms;
    uint64_t diagnostic_scan_interval_ms;
    uint64_t health_check_interval_ms;
    int max_log_entries;
    int max_diagnostic_results;
    int max_health_checks;
    diag_severity_t min_severity_to_log;
    diag_severity_t min_severity_for_notification;
    bool enable_email_notifications;
    bool enable_syslog_integration;
    bool enable_external_alerting;
    uint64_t notification_cooldown_ms;
    bool enable_machine_learning_analysis;
    bool enable_predictive_diagnostics;
    double anomaly_detection_threshold;
    int max_concurrent_diagnostics;
    bool enable_remote_diagnostics;
    bool enable_detailed_troubleshooting;
    int max_troubleshooting_steps;
    bool enable_automated_fixes;
    double max_automation_confidence_threshold;
} diagnostic_config_t;

// Main diagnostic system context
typedef struct {
    // Configuration
    diagnostic_config_t config;
    
    // Active sessions
    diagnostic_session_t* active_sessions;
    int active_session_count;
    int max_active_sessions;
    
    // Results storage
    diagnostic_result_t* stored_results;
    int result_count;
    int max_results;
    uint64_t last_result_id;
    
    // Health checks
    health_check_result_t* health_checks;
    int health_check_count;
    int max_health_checks;
    uint64_t last_health_check_time;
    
    // System metrics
    system_metrics_t current_metrics;
    system_metrics_t* metrics_history;
    int metrics_history_count;
    int max_metrics_history;
    uint64_t last_metrics_update;
    
    // Diagnostic tests
    diagnostic_test_config_t test_configs[16];
    int enabled_test_count;
    
    // Statistics
    uint64_t total_diagnostics_run;
    uint64_t diagnostics_passed;
    uint64_t diagnostics_failed;
    uint64_t issues_detected;
    uint64_t issues_resolved;
    uint64_t automated_fixes_applied;
    double average_diagnostic_time_ms;
    double system_health_score;
    uint64_t last_diagnostic_time;
    
    // Notification management
    uint64_t last_notification_time;
    int notification_suppression_count;
    bool notifications_enabled;
    
    // Performance tracking
    uint64_t peak_memory_usage_kb;
    uint64_t peak_cpu_usage_percent;
    double peak_response_time_ms;
    uint64_t total_uptime_seconds;
    uint64_t last_restart_time;
    
    // State management
    bool system_active;
    bool diagnostic_running;
    bool real_time_monitoring_active;
    bool learning_mode_active;
    int active_components;
    uint64_t initialization_time;
    
    // Thread and synchronization (simulated)
    bool multithreaded_mode;
    int worker_thread_count;
    uint64_t active_threads;
    
    // State
    int initialized;
    int active;
    uint64_t start_time;
    char system_id[64];
    char version_string[32];
} diagnostic_system_context_t;

// Callback function types
typedef void (*diagnostic_result_callback_t)(const diagnostic_result_t* result);
typedef void (*health_check_callback_t)(const health_check_result_t* health_result);
typedef void (*metrics_update_callback_t)(const system_metrics_t* metrics);
typedef void (*diagnostic_progress_callback_t)(uint64_t session_id, int progress_percent, 
                                            diag_status_t status);
typedef void (*issue_detected_callback_t)(const diagnostic_result_t* issue);

// Function declarations

// Initialization and cleanup
int init_diagnostic_system(diagnostic_system_context_t* ctx);
int init_diagnostic_system_with_config(diagnostic_system_context_t* ctx, 
                                    const diagnostic_config_t* config);
void cleanup_diagnostic_system(diagnostic_system_context_t* ctx);

// Configuration management
void get_diagnostic_config(diagnostic_system_context_t* ctx, diagnostic_config_t* config);
int set_diagnostic_config(diagnostic_system_context_t* ctx, const diagnostic_config_t* config);
int enable_real_time_monitoring(diagnostic_system_context_t* ctx, bool enable);
int set_min_severity_level(diagnostic_system_context_t* ctx, diag_severity_t severity);

// Diagnostic execution
int run_diagnostic_test(diagnostic_system_context_t* ctx, diag_test_type_t test_type);
int run_comprehensive_diagnostic(diagnostic_system_context_t* ctx);
int run_category_diagnostic(diagnostic_system_context_t* ctx, diag_category_t category);
int run_custom_diagnostic(diagnostic_system_context_t* ctx, const char* test_params);
int cancel_active_diagnostic(diagnostic_system_context_t* ctx, uint64_t session_id);

// Health checking
health_check_result_t perform_health_check(diagnostic_system_context_t* ctx, 
                                        diag_category_t category);
int perform_system_health_check(diagnostic_system_context_t* ctx);
health_check_result_t get_latest_health_status(diagnostic_system_context_t* ctx, 
                                             diag_category_t category);
double get_overall_system_health(diagnostic_system_context_t* ctx);

// Metrics collection and monitoring
int collect_system_metrics(diagnostic_system_context_t* ctx);
system_metrics_t get_current_metrics(diagnostic_system_context_t* ctx);
int get_metrics_history(diagnostic_system_context_t* ctx, system_metrics_t* history, int max_count);
double calculate_performance_score(diagnostic_system_context_t* ctx);

// Result management
int get_diagnostic_results(diagnostic_system_context_t* ctx, diagnostic_result_t* results, 
                         int max_results);
int get_recent_issues(diagnostic_system_context_t* ctx, diagnostic_result_t* issues, 
                     int max_issues, diag_severity_t min_severity);
int clear_old_results(diagnostic_system_context_t* ctx, uint64_t older_than_timestamp);
diagnostic_result_t get_latest_diagnostic_result(diagnostic_system_context_t* ctx);

// Session management
diagnostic_session_t* get_active_session(diagnostic_system_context_t* ctx, uint64_t session_id);
int get_active_sessions(diagnostic_system_context_t* ctx, diagnostic_session_t* sessions, 
                      int max_sessions);
int get_session_results(diagnostic_system_context_t* ctx, uint64_t session_id, 
                       diagnostic_result_t* results, int max_results);

// Issue management
int get_open_issues(diagnostic_system_context_t* ctx, diagnostic_result_t* issues, 
                   int max_issues);
int resolve_issue(diagnostic_system_context_t* ctx, uint64_t diagnostic_id);
int acknowledge_issue(diagnostic_system_context_t* ctx, uint64_t diagnostic_id);
int get_issue_statistics(diagnostic_system_context_t* ctx, diag_category_t category, 
                        uint64_t* total_issues, uint64_t* unresolved_issues,
                        double* average_resolution_time);

// Performance analysis
int perform_performance_analysis(diagnostic_system_context_t* ctx);
int identify_bottlenecks(diagnostic_system_context_t* ctx);
int analyze_resource_usage(diagnostic_system_context_t* ctx);
int detect_anomalies(diagnostic_system_context_t* ctx);

// Security scanning
int perform_security_scan(diagnostic_system_context_t* ctx);
int check_protocol_compliance(diagnostic_system_context_t* ctx);
int verify_crypto_integrity(diagnostic_system_context_t* ctx);

// Memory diagnostics
int perform_memory_diagnostic(diagnostic_system_context_t* ctx);
int detect_memory_leaks(diagnostic_system_context_t* ctx);
int analyze_heap_usage(diagnostic_system_context_t* ctx);

// Network diagnostics
int perform_network_diagnostic(diagnostic_system_context_t* ctx);
int test_connectivity(diagnostic_system_context_t* ctx);
int measure_bandwidth(diagnostic_system_context_t* ctx);
int detect_network_anomalies(diagnostic_system_context_t* ctx);

// Statistics and reporting
void get_diagnostic_statistics(diagnostic_system_context_t* ctx,
                             uint64_t* total_diagnostics, uint64_t* passed_diagnostics,
                             uint64_t* failed_diagnostics, uint64_t* issues_detected);
int generate_diagnostic_report(diagnostic_system_context_t* ctx, char* report, int max_length);
int export_diagnostic_data(diagnostic_system_context_t* ctx, const char* filename);
double get_system_efficiency_score(diagnostic_system_context_t* ctx);

// Callback registration
void register_diagnostic_result_callback(diagnostic_result_callback_t callback);
void register_health_check_callback(health_check_callback_t callback);
void register_metrics_update_callback(metrics_update_callback_t callback);
void register_diagnostic_progress_callback(diagnostic_progress_callback_t callback);
void register_issue_detected_callback(issue_detected_callback_t callback);

// Integration functions
int integrate_with_performance_monitor(diagnostic_system_context_t* ctx);
int integrate_with_security_system(diagnostic_system_context_t* ctx);
int integrate_with_network_analyzer(diagnostic_system_context_t* ctx);
int apply_diagnostic_recommendations(diagnostic_system_context_t* ctx);
int verify_diagnostic_integrity(diagnostic_system_context_t* ctx);

// Utility functions
const char* diag_category_to_string(diag_category_t category);
const char* diag_severity_to_string(diag_severity_t severity);
const char* diag_issue_type_to_string(diag_issue_type_t issue_type);
const char* diag_status_to_string(diag_status_t status);
const char* diag_test_type_to_string(diag_test_type_t test_type);
diag_category_t string_to_diag_category(const char* str);
diag_severity_t string_to_diag_severity(const char* str);
diag_issue_type_t string_to_diag_issue_type(const char* str);
int create_troubleshooting_guide(diagnostic_system_context_t* ctx, 
                               diag_issue_type_t issue_type, char* guide, int max_length);
int run_automated_fix(diagnostic_system_context_t* ctx, uint64_t diagnostic_id);

#endif // ADVANCED_DIAGNOSTIC_SYSTEM_H