/*
 * advanced-diagnostic-system.c
 * Advanced Diagnostic System Implementation for MTProxy
 */

#include "advanced-diagnostic-system.h"

// Simple implementations for standard functions
static void* simple_malloc(size_t size) {
    static char heap[8192*1024]; // 8MB heap
    static size_t heap_offset = 0;
    
    if (heap_offset + size > sizeof(heap)) {
        return 0;
    }
    
    void *ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

static void simple_free(void *ptr) {
    // Simple free simulation
}

static void simple_memset(void *ptr, int value, size_t num) {
    char *p = (char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (char)value;
    }
}

static void simple_memcpy(void *dest, const void *src, size_t num) {
    char *d = (char*)dest;
    const char *s = (const char*)src;
    for (size_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
}

static int simple_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

static size_t simple_strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

// Global context and callbacks
static diagnostic_system_context_t* g_diag_system = 0;
static diagnostic_result_callback_t g_result_callback = 0;
static health_check_callback_t g_health_callback = 0;
static metrics_update_callback_t g_metrics_callback = 0;
static diagnostic_progress_callback_t g_progress_callback = 0;
static issue_detected_callback_t g_issue_callback = 0;

// Get current timestamp
static uint64_t get_current_timestamp_ms(void) {
    static uint64_t base_time = 1000000;
    base_time += 100;
    return base_time;
}

// Initialize diagnostic system
int init_diagnostic_system(diagnostic_system_context_t* ctx) {
    if (!ctx) return -1;
    
    // Zero initialize
    simple_memset(ctx, 0, sizeof(diagnostic_system_context_t));
    
    // Default configuration
    diagnostic_config_t default_config = {
        .enable_real_time_monitoring = 1,
        .enable_auto_diagnosis = 1,
        .enable_detailed_logging = 1,
        .enable_performance_profiling = 1,
        .enable_security_scanning = 1,
        .enable_memory_tracking = 1,
        .enable_network_monitoring = 1,
        .metrics_collection_interval_ms = 1000,
        .diagnostic_scan_interval_ms = 5000,
        .health_check_interval_ms = 3000,
        .max_log_entries = 10000,
        .max_diagnostic_results = 5000,
        .max_health_checks = 1000,
        .min_severity_to_log = DIAG_SEVERITY_INFO,
        .min_severity_for_notification = DIAG_SEVERITY_WARNING,
        .enable_email_notifications = 0,
        .enable_syslog_integration = 0,
        .enable_external_alerting = 0,
        .notification_cooldown_ms = 30000,
        .enable_machine_learning_analysis = 1,
        .enable_predictive_diagnostics = 1,
        .anomaly_detection_threshold = 2.0,
        .max_concurrent_diagnostics = 5,
        .enable_remote_diagnostics = 0,
        .enable_detailed_troubleshooting = 1,
        .max_troubleshooting_steps = 100,
        .enable_automated_fixes = 1,
        .max_automation_confidence_threshold = 85.0
    };
    
    return init_diagnostic_system_with_config(ctx, &default_config);
}

// Initialize with custom configuration
int init_diagnostic_system_with_config(diagnostic_system_context_t* ctx, 
                                    const diagnostic_config_t* config) {
    if (!ctx) return -1;
    
    // Zero initialize
    simple_memset(ctx, 0, sizeof(diagnostic_system_context_t));
    
    // Apply configuration
    if (config) {
        ctx->config = *config;
    }
    
    // Initialize active sessions
    ctx->max_active_sessions = 10;
    ctx->active_sessions = (diagnostic_session_t*)simple_malloc(
        sizeof(diagnostic_session_t) * ctx->max_active_sessions);
    ctx->active_session_count = 0;
    
    // Initialize stored results
    ctx->max_results = ctx->config.max_diagnostic_results;
    ctx->stored_results = (diagnostic_result_t*)simple_malloc(
        sizeof(diagnostic_result_t) * ctx->max_results);
    ctx->result_count = 0;
    ctx->last_result_id = 1;
    
    // Initialize health checks
    ctx->max_health_checks = ctx->config.max_health_checks;
    ctx->health_checks = (health_check_result_t*)simple_malloc(
        sizeof(health_check_result_t) * ctx->max_health_checks);
    ctx->health_check_count = 0;
    
    // Initialize metrics history
    ctx->max_metrics_history = 1000;
    ctx->metrics_history = (system_metrics_t*)simple_malloc(
        sizeof(system_metrics_t) * ctx->max_metrics_history);
    ctx->metrics_history_count = 0;
    ctx->last_metrics_update = get_current_timestamp_ms();
    
    // Initialize test configurations
    for (int i = 0; i < 16; i++) {
        ctx->test_configs[i].test_type = (diag_test_type_t)i;
        ctx->test_configs[i].enable_test = (i < 8);  // Enable first 8 test types by default
        ctx->test_configs[i].priority = 5;
        ctx->test_configs[i].interval_ms = 30000;  // 30 seconds default
        ctx->test_configs[i].timeout_seconds = 60;
        ctx->test_configs[i].include_in_health_check = 1;
    }
    ctx->enabled_test_count = 8;
    
    // Initialize current metrics
    simple_memset(&ctx->current_metrics, 0, sizeof(system_metrics_t));
    ctx->current_metrics.timestamp = get_current_timestamp_ms();
    ctx->current_metrics.cpu_usage_percent = 25.0;  // Default CPU usage
    ctx->current_metrics.memory_usage_percent = 45.0;  // Default memory usage
    ctx->current_metrics.active_connections = 100;  // Default connections
    ctx->current_metrics.requests_per_second = 1000;  // Default RPS
    ctx->current_metrics.cache_hit_ratio = 95.0;  // Default cache hit ratio
    
    // Initialize statistics
    ctx->total_diagnostics_run = 0;
    ctx->diagnostics_passed = 0;
    ctx->diagnostics_failed = 0;
    ctx->issues_detected = 0;
    ctx->issues_resolved = 0;
    ctx->automated_fixes_applied = 0;
    ctx->average_diagnostic_time_ms = 150.0;
    ctx->system_health_score = 95.0;
    ctx->last_diagnostic_time = get_current_timestamp_ms();
    
    // Initialize notification management
    ctx->last_notification_time = 0;
    ctx->notification_suppression_count = 0;
    ctx->notifications_enabled = 1;
    
    // Initialize performance tracking
    ctx->peak_memory_usage_kb = 102400;  // 100MB default
    ctx->peak_cpu_usage_percent = 100.0;
    ctx->peak_response_time_ms = 100.0;
    ctx->total_uptime_seconds = 0;
    ctx->last_restart_time = get_current_timestamp_ms();
    
    // Initialize state
    ctx->system_active = 1;
    ctx->diagnostic_running = 0;
    ctx->real_time_monitoring_active = ctx->config.enable_real_time_monitoring;
    ctx->learning_mode_active = ctx->config.enable_machine_learning_analysis;
    ctx->active_components = 0;
    ctx->initialization_time = get_current_timestamp_ms();
    
    // Initialize threading
    ctx->multithreaded_mode = 0;
    ctx->worker_thread_count = 1;
    ctx->active_threads = 1;
    
    // Set system state
    ctx->initialized = 1;
    ctx->active = 1;
    ctx->start_time = get_current_timestamp_ms();
    
    // Set system ID and version
    const char* system_id = "MTProxy-Diagnostic-System-v1.0";
    int id_len = 0;
    while (id_len < 63 && system_id[id_len]) {
        ctx->system_id[id_len] = system_id[id_len];
        id_len++;
    }
    ctx->system_id[id_len] = '\0';
    
    const char* version = "1.0.0";
    int ver_len = 0;
    while (ver_len < 31 && version[ver_len]) {
        ctx->version_string[ver_len] = version[ver_len];
        ver_len++;
    }
    ctx->version_string[ver_len] = '\0';
    
    g_diag_system = ctx;
    return 0;
}

// Cleanup diagnostic system
void cleanup_diagnostic_system(diagnostic_system_context_t* ctx) {
    if (!ctx) return;
    
    // Free allocated memory
    if (ctx->active_sessions) {
        simple_free(ctx->active_sessions);
    }
    
    if (ctx->stored_results) {
        simple_free(ctx->stored_results);
    }
    
    if (ctx->health_checks) {
        simple_free(ctx->health_checks);
    }
    
    if (ctx->metrics_history) {
        simple_free(ctx->metrics_history);
    }
    
    // Reset context
    simple_memset(ctx, 0, sizeof(diagnostic_system_context_t));
    
    if (g_diag_system == ctx) {
        g_diag_system = 0;
    }
}

// Run a diagnostic test
int run_diagnostic_test(diagnostic_system_context_t* ctx, diag_test_type_t test_type) {
    if (!ctx || !ctx->initialized) return -1;
    
    if (ctx->active_session_count >= ctx->max_active_sessions) return -1;
    
    // Create diagnostic session
    diagnostic_session_t* session = &ctx->active_sessions[ctx->active_session_count];
    session->session_id = ctx->total_diagnostics_run + 1;
    session->test_type = test_type;
    
    // Set category based on test type
    switch (test_type) {
        case DIAG_TEST_CONNECTIVITY:
        case DIAG_TEST_NETWORK:
            session->category = DIAG_CATEGORY_NETWORK;
            break;
        case DIAG_TEST_PERFORMANCE:
            session->category = DIAG_CATEGORY_PERFORMANCE;
            break;
        case DIAG_TEST_SECURITY:
            session->category = DIAG_CATEGORY_SECURITY;
            break;
        case DIAG_TEST_MEMORY:
            session->category = DIAG_CATEGORY_MEMORY;
            break;
        case DIAG_TEST_PROTOCOL:
            session->category = DIAG_CATEGORY_PROTOCOL;
            break;
        case DIAG_TEST_CRYPTO:
            session->category = DIAG_CATEGORY_CRYPTO;
            break;
        case DIAG_TEST_STRESS:
            session->category = DIAG_CATEGORY_PERFORMANCE;
            break;
        default:
            session->category = DIAG_CATEGORY_GENERAL;
            break;
    }
    
    session->start_time = get_current_timestamp_ms();
    session->status = DIAG_STATUS_RUNNING;
    session->total_tests_run = 0;
    session->tests_passed = 0;
    session->tests_failed = 0;
    session->tests_skipped = 0;
    
    // Allocate results storage
    session->max_results = 50;  // Max 50 results per session
    session->results = (diagnostic_result_t*)simple_malloc(
        sizeof(diagnostic_result_t) * session->max_results);
    session->result_count = 0;
    
    // Capture baseline metrics
    session->baseline_metrics = ctx->current_metrics;
    
    // Set description
    const char* test_descriptions[] = {
        "Connectivity Test", "Performance Test", "Security Scan", "Stress Test",
        "Memory Diagnostics", "Protocol Validation", "Crypto Operations Test", "Network Analysis"
    };
    
    int desc_len = 0;
    const char* desc = test_descriptions[test_type];
    while (desc_len < 255 && desc[desc_len]) {
        session->session_description[desc_len] = desc[desc_len];
        desc_len++;
    }
    session->session_description[desc_len] = '\0';
    
    ctx->active_session_count++;
    ctx->total_diagnostics_run++;
    ctx->diagnostic_running = 1;
    
    // Simulate running the test - in real implementation this would perform actual diagnostics
    // For now, we'll generate some sample results based on the test type
    
    // Generate test results
    int num_tests = 5;  // Run 5 sub-tests
    for (int i = 0; i < num_tests; i++) {
        if (session->result_count >= session->max_results) break;
        
        diagnostic_result_t* result = &session->results[session->result_count];
        result->diagnostic_id = ctx->last_result_id++;
        result->category = session->category;
        result->severity = DIAG_SEVERITY_INFO;
        result->status = DIAG_STATUS_COMPLETED;
        result->timestamp = get_current_timestamp_ms() + (i * 100);
        
        // Set issue type based on test type
        switch (test_type) {
            case DIAG_TEST_CONNECTIVITY:
                result->issue_type = DIAG_ISSUE_TYPE_CONNECTION_TIMEOUT;
                break;
            case DIAG_TEST_PERFORMANCE:
                result->issue_type = DIAG_ISSUE_TYPE_HIGH_LATENCY;
                break;
            case DIAG_TEST_MEMORY:
                result->issue_type = DIAG_ISSUE_TYPE_MEMORY_LEAK;
                break;
            case DIAG_TEST_CRYPTO:
                result->issue_type = DIAG_ISSUE_TYPE_CRYPTO_FAILURE;
                break;
            default:
                result->issue_type = DIAG_ISSUE_TYPE_PROTOCOL_ERROR;
                break;
        }
        
        // Set description
        int desc_len = 0;
        const char* desc = "Diagnostic test completed successfully";
        if (test_type == DIAG_TEST_PERFORMANCE && i == 2) {
            desc = "Performance bottleneck detected";
            result->severity = DIAG_SEVERITY_WARNING;
            session->tests_failed++;
        } else {
            session->tests_passed++;
        }
        
        while (desc_len < 255 && desc[desc_len]) {
            result->description[desc_len] = desc[desc_len];
            desc_len++;
        }
        result->description[desc_len] = '\0';
        
        // Set solution
        int sol_len = 0;
        const char* solution = "No action required - test passed";
        if (result->severity >= DIAG_SEVERITY_WARNING) {
            solution = "Review performance metrics and consider optimization";
        }
        
        while (sol_len < 511 && solution[sol_len]) {
            result->suggested_solution[sol_len] = solution[sol_len];
            sol_len++;
        }
        result->suggested_solution[sol_len] = '\0';
        
        result->confidence_score = 90.0 + (i * 2);  // Vary confidence
        result->execution_time_ms = 50 + (i * 10);  // Vary execution time
        result->requires_immediate_action = (result->severity >= DIAG_SEVERITY_CRITICAL);
        
        // Set affected component
        const char* components[] = {"Network Layer", "Crypto Module", "Memory Manager", 
                                  "Protocol Handler", "Connection Pool"};
        int comp_idx = (test_type + i) % 5;
        int comp_len = 0;
        const char* comp = components[comp_idx];
        while (comp_len < 127 && comp[comp_len]) {
            result->affected_component[comp_len] = comp[comp_len];
            comp_len++;
        }
        result->affected_component[comp_len] = '\0';
        
        result->impact_score = 10.0 + (i * 5);  // Vary impact
        result->is_recurring_issue = 0;
        result->recurrence_count = 0;
        
        // Set diagnostic details
        int det_len = 0;
        const char* details = "Diagnostic completed with standard parameters";
        while (det_len < 1023 && details[det_len]) {
            result->diagnostic_details[det_len] = details[det_len];
            det_len++;
        }
        result->diagnostic_details[det_len] = '\0';
        
        session->result_count++;
        session->total_tests_run++;
        
        // Call result callback
        if (g_result_callback) {
            g_result_callback(result);
        }
        
        // Call issue callback if warning or higher
        if (result->severity >= DIAG_SEVERITY_WARNING && g_issue_callback) {
            g_issue_callback(result);
        }
    }
    
    // Complete session
    session->end_time = get_current_timestamp_ms();
    session->status = DIAG_STATUS_COMPLETED;
    session->diagnostic_complete = 1;
    
    // Capture final metrics
    session->final_metrics = ctx->current_metrics;
    
    // Calculate performance score
    double passed_ratio = (double)session->tests_passed / (double)session->total_tests_run;
    session->overall_performance_score = passed_ratio * 100.0;
    
    ctx->diagnostics_passed += session->tests_passed;
    ctx->diagnostics_failed += session->tests_failed;
    
    // Move completed session to stored results
    for (int i = 0; i < session->result_count; i++) {
        if (ctx->result_count < ctx->max_results) {
            ctx->stored_results[ctx->result_count] = session->results[i];
            ctx->result_count++;
        }
    }
    
    // Update statistics
    ctx->last_diagnostic_time = get_current_timestamp_ms();
    ctx->average_diagnostic_time_ms = 
        ((ctx->average_diagnostic_time_ms * (ctx->total_diagnostics_run - 1)) + 
         (session->end_time - session->start_time)) / ctx->total_diagnostics_run;
    
    ctx->diagnostic_running = 0;
    
    // Decrement active session count (in real implementation, would handle multiple sessions)
    if (ctx->active_session_count > 0) {
        ctx->active_session_count--;
    }
    
    return 0;
}

// Perform health check
health_check_result_t perform_health_check(diagnostic_system_context_t* ctx, 
                                        diag_category_t category) {
    health_check_result_t result = {0};
    
    if (!ctx || !ctx->initialized) return result;
    
    // Set basic result information
    static uint64_t check_counter = 1;
    result.check_id = check_counter++;
    result.category = category;
    result.timestamp = get_current_timestamp_ms();
    result.response_time_ms = 50 + (category * 10);  // Simulate response time
    
    // Determine health based on category and current metrics
    switch (category) {
        case DIAG_CATEGORY_NETWORK:
            result.is_healthy = (ctx->current_metrics.active_connections < 1000);
            result.health_score = result.is_healthy ? 95.0 : 60.0;
            break;
        case DIAG_CATEGORY_PERFORMANCE:
            result.is_healthy = (ctx->current_metrics.avg_response_time_ms < 100.0);
            result.health_score = result.is_healthy ? 90.0 : 40.0;
            break;
        case DIAG_CATEGORY_MEMORY:
            result.is_healthy = (ctx->current_metrics.memory_usage_percent < 80.0);
            result.health_score = result.is_healthy ? 92.0 : 35.0;
            break;
        case DIAG_CATEGORY_SECURITY:
            result.is_healthy = 1;  // Assume healthy for now
            result.health_score = 98.0;
            break;
        case DIAG_CATEGORY_PROTOCOL:
            result.is_healthy = 1;  // Assume healthy for now
            result.health_score = 96.0;
            break;
        case DIAG_CATEGORY_CRYPTO:
            result.is_healthy = 1;  // Assume healthy for now
            result.health_score = 97.0;
            break;
        case DIAG_CATEGORY_CONNECTION:
            result.is_healthy = (ctx->current_metrics.failed_connections < 
                               ctx->current_metrics.total_connections * 0.05);  // < 5% failure rate
            result.health_score = result.is_healthy ? 94.0 : 50.0;
            break;
        case DIAG_CATEGORY_DISK_IO:
            result.is_healthy = (ctx->current_metrics.disk_usage_percent < 90.0);
            result.health_score = result.is_healthy ? 93.0 : 45.0;
            break;
        case DIAG_CATEGORY_SYSTEM_HEALTH:
            result.is_healthy = 1;  // Overall system health
            result.health_score = 95.0;
            break;
        default:
            result.is_healthy = 1;
            result.health_score = 85.0;
            break;
    }
    
    // Set status message
    int msg_len = 0;
    const char* status_msg = result.is_healthy ? "System operating normally" : "Potential issues detected";
    while (msg_len < 255 && status_msg[msg_len]) {
        result.status_message[msg_len] = status_msg[msg_len];
        msg_len++;
    }
    result.status_message[msg_len] = '\0';
    
    // Set component name
    int comp_len = 0;
    const char* comp_name = diag_category_to_string(category);
    while (comp_len < 127 && comp_name[comp_len]) {
        result.component_name[comp_len] = comp_name[comp_len];
        comp_len++;
    }
    result.component_name[comp_len] = '\0';
    
    // Set if requires attention
    result.requires_attention = !result.is_healthy;
    
    // Update system health score
    ctx->system_health_score = result.health_score;
    
    // Store in health checks history
    if (ctx->health_check_count < ctx->max_health_checks) {
        ctx->health_checks[ctx->health_check_count] = result;
        ctx->health_check_count++;
    }
    
    // Call health callback
    if (g_health_callback) {
        g_health_callback(&result);
    }
    
    return result;
}

// Collect system metrics
int collect_system_metrics(diagnostic_system_context_t* ctx) {
    if (!ctx || !ctx->initialized) return -1;
    
    // Update current metrics with simulated realistic values
    ctx->current_metrics.timestamp = get_current_timestamp_ms();
    ctx->current_metrics.cpu_usage_percent += 2.0;  // Simulate slight variation
    if (ctx->current_metrics.cpu_usage_percent > 100.0) {
        ctx->current_metrics.cpu_usage_percent = 25.0;  // Reset periodically
    }
    
    ctx->current_metrics.memory_usage_percent += 1.5;  // Simulate slight variation
    if (ctx->current_metrics.memory_usage_percent > 100.0) {
        ctx->current_metrics.memory_usage_percent = 45.0;  // Reset periodically
    }
    
    ctx->current_metrics.network_in_mbps += 0.5;  // Simulate network activity
    ctx->current_metrics.network_out_mbps += 0.3;
    
    ctx->current_metrics.active_connections += 5;  // Simulate connection growth
    if (ctx->current_metrics.active_connections > 2000) {
        ctx->current_metrics.active_connections = 100;  // Reset periodically
    }
    
    ctx->current_metrics.avg_response_time_ms = 25.0 + (ctx->current_metrics.active_connections / 100.0);
    ctx->current_metrics.p95_response_time_ms = ctx->current_metrics.avg_response_time_ms * 1.5;
    ctx->current_metrics.p99_response_time_ms = ctx->current_metrics.avg_response_time_ms * 2.0;
    
    ctx->current_metrics.requests_per_second = 1000 + (ctx->current_metrics.active_connections / 10);
    ctx->current_metrics.error_rate_percent = 0.1 + (ctx->current_metrics.cpu_usage_percent / 1000.0);
    
    ctx->current_metrics.current_rss_kb = 51200 + (int)(ctx->current_metrics.memory_usage_percent * 100);
    ctx->current_metrics.peak_rss_kb = ctx->current_metrics.current_rss_kb + 10000;
    ctx->current_metrics.virtual_memory_kb = ctx->current_metrics.current_rss_kb * 3;
    
    ctx->current_metrics.open_files_count = 50 + (int)(ctx->current_metrics.active_connections / 20);
    ctx->current_metrics.threads_count = 10 + (int)(ctx->current_metrics.active_connections / 100);
    
    ctx->current_metrics.crypto_operations_per_second = 5000 + (int)(ctx->current_metrics.active_connections * 2);
    ctx->current_metrics.encryption_time_avg_ms = 0.1 + (ctx->current_metrics.cpu_usage_percent / 1000.0);
    ctx->current_metrics.decryption_time_avg_ms = 0.08 + (ctx->current_metrics.cpu_usage_percent / 1200.0);
    
    ctx->current_metrics.cache_hit_ratio = 95.0 - (ctx->current_metrics.error_rate_percent * 2);
    if (ctx->current_metrics.cache_hit_ratio < 80.0) ctx->current_metrics.cache_hit_ratio = 80.0;
    
    // Store in history (circular buffer)
    int hist_idx = ctx->metrics_history_count % ctx->max_metrics_history;
    ctx->metrics_history[hist_idx] = ctx->current_metrics;
    ctx->metrics_history_count++;
    
    ctx->last_metrics_update = get_current_timestamp_ms();
    
    // Call metrics callback
    if (g_metrics_callback) {
        g_metrics_callback(&ctx->current_metrics);
    }
    
    return 0;
}

// Get diagnostic statistics
void get_diagnostic_statistics(diagnostic_system_context_t* ctx,
                             uint64_t* total_diagnostics, uint64_t* passed_diagnostics,
                             uint64_t* failed_diagnostics, uint64_t* issues_detected) {
    if (!ctx) return;
    
    if (total_diagnostics) *total_diagnostics = ctx->total_diagnostics_run;
    if (passed_diagnostics) *passed_diagnostics = ctx->diagnostics_passed;
    if (failed_diagnostics) *failed_diagnostics = ctx->diagnostics_failed;
    if (issues_detected) *issues_detected = ctx->issues_detected;
}

// Utility functions
const char* diag_category_to_string(diag_category_t category) {
    switch (category) {
        case DIAG_CATEGORY_GENERAL: return "General";
        case DIAG_CATEGORY_NETWORK: return "Network";
        case DIAG_CATEGORY_PERFORMANCE: return "Performance";
        case DIAG_CATEGORY_SECURITY: return "Security";
        case DIAG_CATEGORY_MEMORY: return "Memory";
        case DIAG_CATEGORY_PROTOCOL: return "Protocol";
        case DIAG_CATEGORY_CONNECTION: return "Connection";
        case DIAG_CATEGORY_CRYPTO: return "Crypto";
        case DIAG_CATEGORY_DISK_IO: return "Disk IO";
        case DIAG_CATEGORY_SYSTEM_HEALTH: return "System Health";
        default: return "Unknown";
    }
}

const char* diag_severity_to_string(diag_severity_t severity) {
    switch (severity) {
        case DIAG_SEVERITY_INFO: return "Info";
        case DIAG_SEVERITY_WARNING: return "Warning";
        case DIAG_SEVERITY_ERROR: return "Error";
        case DIAG_SEVERITY_CRITICAL: return "Critical";
        case DIAG_SEVERITY_EMERGENCY: return "Emergency";
        default: return "Unknown";
    }
}

const char* diag_issue_type_to_string(diag_issue_type_t issue_type) {
    switch (issue_type) {
        case DIAG_ISSUE_TYPE_CONNECTION_TIMEOUT: return "Connection Timeout";
        case DIAG_ISSUE_TYPE_HIGH_LATENCY: return "High Latency";
        case DIAG_ISSUE_TYPE_MEMORY_LEAK: return "Memory Leak";
        case DIAG_ISSUE_TYPE_RESOURCE_STARVATION: return "Resource Starvation";
        case DIAG_ISSUE_TYPE_PROTOCOL_ERROR: return "Protocol Error";
        case DIAG_ISSUE_TYPE_CRYPTO_FAILURE: return "Crypto Failure";
        case DIAG_ISSUE_TYPE_SECURITY_BREACH: return "Security Breach";
        case DIAG_ISSUE_TYPE_DISK_SPACE_LOW: return "Low Disk Space";
        case DIAG_ISSUE_TYPE_BANDWIDTH_LIMIT: return "Bandwidth Limit";
        case DIAG_ISSUE_TYPE_DEADLOCK_DETECTED: return "Deadlock Detected";
        case DIAG_ISSUE_TYPE_THREAD_STARVATION: return "Thread Starvation";
        case DIAG_ISSUE_TYPE_CACHE_MISS_HIGH: return "High Cache Miss";
        default: return "Unknown";
    }
}

const char* diag_status_to_string(diag_status_t status) {
    switch (status) {
        case DIAG_STATUS_PENDING: return "Pending";
        case DIAG_STATUS_RUNNING: return "Running";
        case DIAG_STATUS_COMPLETED: return "Completed";
        case DIAG_STATUS_ERROR: return "Error";
        case DIAG_STATUS_TIMEOUT: return "Timeout";
        case DIAG_STATUS_ABORTED: return "Aborted";
        default: return "Unknown";
    }
}

const char* diag_test_type_to_string(diag_test_type_t test_type) {
    switch (test_type) {
        case DIAG_TEST_CONNECTIVITY: return "Connectivity";
        case DIAG_TEST_PERFORMANCE: return "Performance";
        case DIAG_TEST_SECURITY: return "Security";
        case DIAG_TEST_STRESS: return "Stress";
        case DIAG_TEST_MEMORY: return "Memory";
        case DIAG_TEST_PROTOCOL: return "Protocol";
        case DIAG_TEST_CRYPTO: return "Crypto";
        case DIAG_TEST_NETWORK: return "Network";
        default: return "Unknown";
    }
}

// Callback registration
void register_diagnostic_result_callback(diagnostic_result_callback_t callback) {
    g_result_callback = callback;
}

void register_health_check_callback(health_check_callback_t callback) {
    g_health_callback = callback;
}

void register_metrics_update_callback(metrics_update_callback_t callback) {
    g_metrics_callback = callback;
}

void register_diagnostic_progress_callback(diagnostic_progress_callback_t callback) {
    g_progress_callback = callback;
}

void register_issue_detected_callback(issue_detected_callback_t callback) {
    g_issue_callback = callback;
}