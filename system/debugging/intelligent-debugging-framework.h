/*
 * intelligent-debugging-framework.h
 * Intelligent Debugging Framework for MTProxy
 *
 * Advanced debugging system with automated issue detection, root cause analysis,
 * and intelligent troubleshooting.
 */

#ifndef INTELLIGENT_DEBUGGING_FRAMEWORK_H
#define INTELLIGENT_DEBUGGING_FRAMEWORK_H

#include <stdint.h>
#include <stdbool.h>

// Debug level
typedef enum {
    DEBUG_LEVEL_TRACE = 0,
    DEBUG_LEVEL_DEBUG = 1,
    DEBUG_LEVEL_INFO = 2,
    DEBUG_LEVEL_WARN = 3,
    DEBUG_LEVEL_ERROR = 4,
    DEBUG_LEVEL_CRITICAL = 5,
    DEBUG_LEVEL_OFF = 6
} debug_level_t;

// Debug target
typedef enum {
    DEBUG_TARGET_ALL = 0,
    DEBUG_TARGET_NETWORK = 1,
    DEBUG_TARGET_CRYPTO = 2,
    DEBUG_TARGET_PROTOCOL = 3,
    DEBUG_TARGET_MEMORY = 4,
    DEBUG_TARGET_PERFORMANCE = 5,
    DEBUG_TARGET_SECURITY = 6,
    DEBUG_TARGET_CONNECTION = 7,
    DEBUG_TARGET_FILESYSTEM = 8,
    DEBUG_TARGET_PROCESS = 9
} debug_target_t;

// Debug event type
typedef enum {
    DEBUG_EVENT_FUNCTION_ENTRY = 0,
    DEBUG_EVENT_FUNCTION_EXIT = 1,
    DEBUG_EVENT_VARIABLE_CHANGE = 2,
    DEBUG_EVENT_CONDITION_CHECK = 3,
    DEBUG_EVENT_LOOP_ITERATION = 4,
    DEBUG_EVENT_MEMORY_ALLOCATION = 5,
    DEBUG_EVENT_MEMORY_DEALLOCATION = 6,
    DEBUG_EVENT_NETWORK_PACKET = 7,
    DEBUG_EVENT_ERROR_OCCURRED = 8,
    DEBUG_EVENT_EXCEPTION_THROWN = 9,
    DEBUG_EVENT_THREAD_CREATED = 10,
    DEBUG_EVENT_THREAD_DESTROYED = 11,
    DEBUG_EVENT_LOCK_ACQUIRED = 12,
    DEBUG_EVENT_LOCK_RELEASED = 13
} debug_event_type_t;

// Debug breakpoint type
typedef enum {
    BREAKPOINT_TYPE_LINE = 0,
    BREAKPOINT_TYPE_FUNCTION = 1,
    BREAKPOINT_TYPE_CONDITIONAL = 2,
    BREAKPOINT_TYPE_WATCHPOINT = 3,
    BREAKPOINT_TYPE_EXCEPTION = 4,
    BREAKPOINT_TYPE_MEMORY_ACCESS = 5
} breakpoint_type_t;

// Debug action
typedef enum {
    DEBUG_ACTION_CONTINUE = 0,
    DEBUG_ACTION_STEP_INTO = 1,
    DEBUG_ACTION_STEP_OVER = 2,
    DEBUG_ACTION_STEP_OUT = 3,
    DEBUG_ACTION_BREAK = 4,
    DEBUG_ACTION_TERMINATE = 5,
    DEBUG_ACTION_DETACH = 6
} debug_action_t;

// Debug variable type
typedef enum {
    VAR_TYPE_INT = 0,
    VAR_TYPE_FLOAT = 1,
    VAR_TYPE_DOUBLE = 2,
    VAR_TYPE_POINTER = 3,
    VAR_TYPE_STRING = 4,
    VAR_TYPE_ARRAY = 5,
    VAR_TYPE_STRUCT = 6,
    VAR_TYPE_BOOL = 7,
    VAR_TYPE_CHAR = 8,
    VAR_TYPE_VOID = 9
} var_type_t;

// Root cause analysis result
typedef enum {
    RCA_RESULT_NO_ISSUE = 0,
    RCA_RESULT_SUSPECTED_ISSUE = 1,
    RCA_RESULT_LIKELY_ISSUE = 2,
    RCA_RESULT_CONFIRMED_ISSUE = 3,
    RCA_RESULT_CRITICAL_ISSUE = 4
} rca_result_t;

// Debug session status
typedef enum {
    DEBUG_SESSION_STOPPED = 0,
    DEBUG_SESSION_RUNNING = 1,
    DEBUG_SESSION_PAUSED = 2,
    DEBUG_SESSION_ERROR = 3,
    DEBUG_SESSION_ATTACHED = 4,
    DEBUG_SESSION_DETACHED = 5
} debug_session_status_t;

// Debug trace
typedef struct {
    uint64_t trace_id;
    uint64_t timestamp;
    debug_event_type_t event_type;
    debug_target_t target;
    uint64_t thread_id;
    uint64_t process_id;
    char function_name[128];
    char file_name[256];
    int line_number;
    char message[512];
    uint64_t execution_time_ns;
    uint64_t memory_address;
    uint64_t memory_size;
    bool is_error;
    bool is_warning;
    char call_stack[1024];
    uint64_t stack_depth;
    char parameters[256];
    char return_value[128];
} debug_trace_t;

// Debug variable
typedef struct {
    uint64_t var_id;
    char var_name[64];
    var_type_t var_type;
    void* var_address;
    size_t var_size;
    char var_value[256];
    uint64_t timestamp;
    bool is_changed;
    bool is_watched;
    uint64_t last_change_time;
    char scope[64];
    bool is_static;
    bool is_const;
    bool is_pointer;
    void* pointed_to_address;
} debug_variable_t;

// Debug breakpoint
typedef struct {
    uint64_t bp_id;
    breakpoint_type_t bp_type;
    char target_location[256];
    int line_number;
    char function_name[128];
    char condition[256];
    uint64_t hit_count;
    uint64_t ignore_count;
    bool is_enabled;
    bool is_temporary;
    bool is_conditional;
    uint64_t thread_id;
    uint64_t process_id;
    uint64_t timestamp;
    char description[256];
    bool is_verified;
    uint64_t actual_address;
} debug_breakpoint_t;

// Debug frame
typedef struct {
    uint64_t frame_id;
    char function_name[128];
    char file_name[256];
    int line_number;
    uint64_t frame_address;
    uint64_t return_address;
    uint64_t stack_pointer;
    uint64_t frame_pointer;
    debug_variable_t* local_variables;
    int local_var_count;
    int max_local_vars;
    debug_variable_t* parameters;
    int param_count;
    int max_params;
    uint64_t timestamp;
    uint64_t execution_time_ns;
} debug_frame_t;

// Debug thread info
typedef struct {
    uint64_t thread_id;
    char thread_name[64];
    uint64_t process_id;
    debug_session_status_t status;
    debug_frame_t* call_stack;
    int stack_depth;
    int max_stack_depth;
    uint64_t creation_time;
    uint64_t last_activity_time;
    char current_function[128];
    int current_line;
    char current_file[256];
    uint64_t cpu_time_used;
    uint64_t memory_used;
    bool is_suspended;
    bool is_system_thread;
    bool is_debugger_thread;
} debug_thread_info_t;

// Debug issue
typedef struct {
    uint64_t issue_id;
    rca_result_t rca_result;
    debug_target_t affected_target;
    debug_level_t severity;
    uint64_t timestamp;
    char issue_title[256];
    char issue_description[1024];
    char suspected_cause[512];
    char recommended_fix[512];
    char affected_functions[512];
    debug_trace_t* related_traces;
    int trace_count;
    int max_traces;
    char root_cause[512];
    double confidence_score;  // 0.0 - 100.0
    bool is_auto_resolved;
    bool requires_manual_intervention;
    uint64_t estimated_fix_time_minutes;
    char potential_side_effects[256];
    bool is_known_issue;
    char known_issue_id[64];
    bool is_security_related;
    bool is_performance_related;
    uint64_t reproduction_steps_count;
    char reproduction_steps[1024];
} debug_issue_t;

// Debug session
typedef struct {
    uint64_t session_id;
    char session_name[128];
    debug_session_status_t status;
    uint64_t start_time;
    uint64_t end_time;
    uint64_t pause_time;
    uint64_t resume_time;
    uint64_t current_time;
    debug_level_t current_level;
    debug_target_t current_target;
    debug_thread_info_t* threads;
    int thread_count;
    int max_threads;
    debug_frame_t* current_frames;
    int frame_count;
    int max_frames;
    debug_breakpoint_t* breakpoints;
    int bp_count;
    int max_breakpoints;
    debug_trace_t* traces;
    int trace_count;
    int max_traces;
    debug_issue_t* detected_issues;
    int issue_count;
    int max_issues;
    uint64_t total_traces_collected;
    uint64_t total_issues_detected;
    uint64_t total_breakpoints_hit;
    uint64_t total_exceptions_caught;
    uint64_t total_variables_watched;
    uint64_t total_memory_accesses;
    uint64_t total_network_packets;
    bool is_interactive;
    bool is_attached_to_process;
    uint64_t target_process_id;
    char target_process_name[128];
    bool is_remote_debugging;
    char remote_host[64];
    int remote_port;
    bool is_profiling_enabled;
    bool is_memory_tracing_enabled;
    bool is_network_tracing_enabled;
    bool is_crypto_tracing_enabled;
    bool is_security_tracing_enabled;
    uint64_t session_options;
} debug_session_t;

// Debug configuration
typedef struct {
    bool enable_intelligent_debugging;
    bool enable_root_cause_analysis;
    bool enable_automatic_issue_detection;
    bool enable_smart_breakpoints;
    bool enable_variable_watchdog;
    bool enable_memory_debugging;
    bool enable_network_debugging;
    bool enable_crypto_debugging;
    bool enable_security_debugging;
    debug_level_t default_debug_level;
    debug_target_t default_debug_target;
    uint64_t max_trace_buffer_size;
    uint64_t max_issue_buffer_size;
    uint64_t max_variable_buffer_size;
    uint64_t trace_buffer_flush_interval_ms;
    uint64_t issue_analysis_interval_ms;
    bool enable_call_stack_tracing;
    bool enable_parameter_tracing;
    bool enable_return_value_tracing;
    bool enable_exception_tracing;
    bool enable_thread_tracing;
    bool enable_lock_tracing;
    bool enable_memory_allocation_tracing;
    bool enable_performance_counters;
    bool enable_security_counters;
    bool enable_network_counters;
    bool enable_crypto_counters;
    bool enable_automated_fix_suggestions;
    bool enable_intelligent_filtering;
    bool enable_pattern_matching;
    bool enable_machine_learning_analysis;
    double rca_confidence_threshold;
    int max_call_stack_depth;
    int max_variables_to_track;
    int max_memory_allocations_to_track;
    bool enable_remote_debugging;
    bool enable_multi_process_debugging;
    bool enable_kernel_mode_debugging;
    bool enable_hardware_breakpoints;
    bool enable_software_breakpoints;
    bool enable_watchpoints;
    bool enable_conditional_breakpoints;
    bool enable_logging;
    bool enable_console_output;
    bool enable_file_logging;
    bool enable_syslog_output;
    char log_file_path[256];
    bool enable_compression;
    bool enable_encryption;
    bool enable_access_control;
    bool enable_authentication;
} debug_config_t;

// Intelligent debugging context
typedef struct {
    // Configuration
    debug_config_t config;
    
    // Current session
    debug_session_t current_session;
    bool session_active;
    
    // Trace management
    debug_trace_t* trace_buffer;
    int trace_buffer_count;
    int trace_buffer_max;
    uint64_t trace_buffer_start_time;
    uint64_t trace_buffer_end_time;
    
    // Variable tracking
    debug_variable_t* watched_variables;
    int watched_var_count;
    int max_watched_vars;
    
    // Breakpoint management
    debug_breakpoint_t* active_breakpoints;
    int active_bp_count;
    int max_breakpoints;
    
    // Thread management
    debug_thread_info_t* active_threads;
    int active_thread_count;
    int max_threads;
    
    // Issue detection
    debug_issue_t* detected_issues;
    int issue_count;
    int max_issues;
    uint64_t last_issue_analysis_time;
    bool issue_analysis_needed;
    
    // Performance counters
    uint64_t total_traces;
    uint64_t total_issues;
    uint64_t total_breakpoints;
    uint64_t total_exceptions;
    uint64_t total_memory_ops;
    uint64_t total_network_ops;
    uint64_t total_crypto_ops;
    double avg_trace_time_ns;
    double avg_analysis_time_ns;
    
    // Root cause analysis
    bool rca_active;
    uint64_t last_rca_time;
    int rca_iterations;
    double overall_confidence_score;
    
    // Pattern matching
    bool pattern_matching_active;
    uint64_t last_pattern_match_time;
    
    // Remote debugging
    bool remote_debugging_enabled;
    uint64_t remote_session_id;
    
    // Security and access control
    bool access_control_enabled;
    bool authentication_enabled;
    uint64_t current_user_id;
    
    // State management
    bool debugger_active;
    bool tracing_active;
    bool analysis_active;
    bool profiling_active;
    int active_components;
    
    // Statistics
    uint64_t uptime_seconds;
    uint64_t start_time;
    double efficiency_score;
    uint64_t total_debugging_sessions;
    uint64_t total_issues_resolved;
    uint64_t total_automated_fixes;
    uint64_t total_manual_interventions;
    
    // State
    int initialized;
    int active;
    uint64_t initialization_time;
    char framework_id[64];
    char version_string[32];
} debug_framework_context_t;

// Callback function types
typedef void (*debug_trace_callback_t)(const debug_trace_t* trace);
typedef void (*debug_issue_callback_t)(const debug_issue_t* issue);
typedef void (*debug_breakpoint_callback_t)(const debug_breakpoint_t* bp);
typedef void (*debug_variable_callback_t)(const debug_variable_t* var);
typedef void (*debug_session_callback_t)(const debug_session_t* session);
typedef void (*debug_analysis_callback_t)(const debug_issue_t* issue, rca_result_t result);

// Function declarations

// Initialization and cleanup
int init_debug_framework(debug_framework_context_t* ctx);
int init_debug_framework_with_config(debug_framework_context_t* ctx, 
                                   const debug_config_t* config);
void cleanup_debug_framework(debug_framework_context_t* ctx);

// Configuration management
void get_debug_config(debug_framework_context_t* ctx, debug_config_t* config);
int set_debug_config(debug_framework_context_t* ctx, const debug_config_t* config);
int set_debug_level(debug_framework_context_t* ctx, debug_level_t level);
int set_debug_target(debug_framework_context_t* ctx, debug_target_t target);

// Session management
int start_debug_session(debug_framework_context_t* ctx, const char* session_name);
int stop_debug_session(debug_framework_context_t* ctx);
int pause_debug_session(debug_framework_context_t* ctx);
int resume_debug_session(debug_framework_context_t* ctx);
int attach_to_process(debug_framework_context_t* ctx, uint64_t process_id);
int detach_from_process(debug_framework_context_t* ctx);

// Tracing functions
int add_debug_trace(debug_framework_context_t* ctx, const debug_trace_t* trace);
int trace_function_entry(debug_framework_context_t* ctx, const char* func_name, 
                        const char* file_name, int line_number, const char* params);
int trace_function_exit(debug_framework_context_t* ctx, const char* func_name,
                       const char* file_name, int line_number, const char* return_val);
int trace_variable_change(debug_framework_context_t* ctx, const char* var_name,
                         const char* old_val, const char* new_val);
int trace_error(debug_framework_context_t* ctx, debug_target_t target, 
               const char* error_msg);
int get_trace_history(debug_framework_context_t* ctx, debug_trace_t* traces, 
                     int max_traces);

// Variable tracking
int watch_variable(debug_framework_context_t* ctx, const char* var_name, 
                  void* address, size_t size, var_type_t type);
int unwatch_variable(debug_framework_context_t* ctx, const char* var_name);
int get_variable_value(debug_framework_context_t* ctx, const char* var_name, 
                      char* value_buffer, int max_size);
int update_variable_value(debug_framework_context_t* ctx, const char* var_name,
                         const char* new_value);
int get_watched_variables(debug_framework_context_t* ctx, debug_variable_t* vars, 
                         int max_vars);

// Breakpoint management
int set_breakpoint(debug_framework_context_t* ctx, breakpoint_type_t type,
                  const char* location, int line_number, const char* condition);
int remove_breakpoint(debug_framework_context_t* ctx, uint64_t bp_id);
int enable_breakpoint(debug_framework_context_t* ctx, uint64_t bp_id);
int disable_breakpoint(debug_framework_context_t* ctx, uint64_t bp_id);
int get_breakpoints(debug_framework_context_t* ctx, debug_breakpoint_t* bps, 
                   int max_bps);

// Thread management
int get_thread_info(debug_framework_context_t* ctx, uint64_t thread_id, 
                   debug_thread_info_t* info);
int get_all_threads(debug_framework_context_t* ctx, debug_thread_info_t* threads, 
                   int max_threads);
int suspend_thread(debug_framework_context_t* ctx, uint64_t thread_id);
int resume_thread(debug_framework_context_t* ctx, uint64_t thread_id);
int get_call_stack(debug_framework_context_t* ctx, uint64_t thread_id, 
                  debug_frame_t* frames, int max_frames);

// Issue detection and analysis
int detect_issues(debug_framework_context_t* ctx);
int perform_root_cause_analysis(debug_framework_context_t* ctx, uint64_t issue_id);
debug_issue_t* get_issue_details(debug_framework_context_t* ctx, uint64_t issue_id);
int get_detected_issues(debug_framework_context_t* ctx, debug_issue_t* issues, 
                       int max_issues);
int resolve_issue(debug_framework_context_t* ctx, uint64_t issue_id);
int acknowledge_issue(debug_framework_context_t* ctx, uint64_t issue_id);

// Pattern matching and analysis
int add_pattern(debug_framework_context_t* ctx, const char* pattern_name, 
               const char* pattern_regex, debug_target_t target);
int match_patterns(debug_framework_context_t* ctx);
int get_pattern_matches(debug_framework_context_t* ctx, char* matches_buffer, 
                       int max_size);

// Remote debugging
int start_remote_debug_server(debug_framework_context_t* ctx, int port);
int connect_to_remote_debugger(debug_framework_context_t* ctx, const char* host, int port);
int send_debug_data(debug_framework_context_t* ctx, const void* data, size_t size);
int receive_debug_data(debug_framework_context_t* ctx, void* buffer, size_t size);

// Profiling and performance
int start_profiling(debug_framework_context_t* ctx);
int stop_profiling(debug_framework_context_t* ctx);
int get_profile_data(debug_framework_context_t* ctx, char* profile_buffer, 
                    int max_size);
int start_memory_profiling(debug_framework_context_t* ctx);
int stop_memory_profiling(debug_framework_context_t* ctx);

// Security and access control
int enable_access_control(debug_framework_context_t* ctx, bool enable);
int authenticate_user(debug_framework_context_t* ctx, const char* username, 
                     const char* password);
int authorize_action(debug_framework_context_t* ctx, uint64_t user_id, 
                    const char* action);

// Statistics and reporting
void get_debug_statistics(debug_framework_context_t* ctx,
                         uint64_t* total_traces, uint64_t* total_issues,
                         uint64_t* total_breakpoints, double* efficiency_score);
int generate_debug_report(debug_framework_context_t* ctx, char* report, 
                         int max_length);
int export_debug_data(debug_framework_context_t* ctx, const char* filename, 
                     const char* format);

// Callback registration
void register_debug_trace_callback(debug_trace_callback_t callback);
void register_debug_issue_callback(debug_issue_callback_t callback);
void register_debug_breakpoint_callback(debug_breakpoint_callback_t callback);
void register_debug_variable_callback(debug_variable_callback_t callback);
void register_debug_session_callback(debug_session_callback_t callback);
void register_debug_analysis_callback(debug_analysis_callback_t callback);

// Integration functions
int integrate_with_diagnostic_system(debug_framework_context_t* ctx);
int integrate_with_monitoring_dashboard(debug_framework_context_t* ctx);
int integrate_with_performance_analyzer(debug_framework_context_t* ctx);
int apply_debug_recommendations(debug_framework_context_t* ctx);
int verify_debug_integrity(debug_framework_context_t* ctx);

// Utility functions
const char* debug_level_to_string(debug_level_t level);
const char* debug_target_to_string(debug_target_t target);
const char* debug_event_type_to_string(debug_event_type_t event_type);
const char* breakpoint_type_to_string(breakpoint_type_t type);
const char* debug_action_to_string(debug_action_t action);
const char* var_type_to_string(var_type_t type);
const char* rca_result_to_string(rca_result_t result);
const char* debug_session_status_to_string(debug_session_status_t status);
debug_level_t string_to_debug_level(const char* str);
debug_target_t string_to_debug_target(const char* str);
debug_event_type_t string_to_debug_event_type(const char* str);
breakpoint_type_t string_to_breakpoint_type(const char* str);
var_type_t string_to_var_type(const char* str);
int format_debug_message(debug_level_t level, debug_target_t target, 
                        const char* format, char* buffer, int max_size);
int create_debug_snapshot(debug_framework_context_t* ctx, char* snapshot_buffer, 
                         int max_size);

#endif // INTELLIGENT_DEBUGGING_FRAMEWORK_H