/*
 * System Health Monitor Implementation for MTProxy
 * Monitors overall system health and coordinates with resource optimization
 */

#include "system-health-monitor.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

// Helper function to compare strings
static int string_compare(const char* str1, const char* str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return (str1[i] < str2[i]) ? -1 : 1;
        }
        i++;
    }
    
    if (str1[i] == '\0' && str2[i] == '\0') {
        return 0;
    }
    
    return (str1[i] == '\0') ? -1 : 1;
}

// Helper function to get string length
static int string_length(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// Helper function to copy string with bounds checking
static void string_copy(char* dest, const char* src, int max_len) {
    int i = 0;
    while (i < max_len - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Helper function to calculate absolute value
static int int_abs(int value) {
    return (value < 0) ? -value : value;
}

// Initialize the health monitor
int init_health_monitor(health_monitor_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    // Initialize all fields to default values
    ctx->indicator_count = 0;
    ctx->overall_health = HEALTH_STATUS_EXCELLENT;
    ctx->monitor_interval_ms = HEALTH_MONITOR_INTERVAL_MS;
    ctx->health_score = 100; // Perfect health initially
    ctx->last_evaluation_time = 0;
    ctx->critical_events_count = 0;
    ctx->warning_events_count = 0;
    ctx->resource_manager_ref = NULL;
    
    // Initialize all health indicators
    int i;
    for (i = 0; i < MAX_HEALTH_INDICATORS; i++) {
        health_indicator_entry_t* indicator = &ctx->indicators[i];
        indicator->indicator_type = HEALTH_INDICATOR_CPU; // Default value
        indicator->current_value = 0;
        indicator->threshold_critical = CRITICAL_HEALTH_THRESHOLD;
        indicator->threshold_warning = WARNING_HEALTH_THRESHOLD;
        indicator->current_status = HEALTH_STATUS_EXCELLENT;
        indicator->last_update_time = 0;
        indicator->indicator_name[0] = '\0';
    }
    
    return 0;
}

// Register a health indicator with the monitor
int register_health_indicator(health_monitor_context_t* ctx, health_indicator_t type, const char* name, int warning_threshold, int critical_threshold) {
    if (!ctx || !name || ctx->indicator_count >= MAX_HEALTH_INDICATORS) {
        return -1;
    }
    
    health_indicator_entry_t* indicator = &ctx->indicators[ctx->indicator_count];
    
    indicator->indicator_type = type;
    indicator->current_value = 0;
    indicator->threshold_warning = warning_threshold;
    indicator->threshold_critical = critical_threshold;
    indicator->current_status = HEALTH_STATUS_EXCELLENT;
    indicator->last_update_time = 0;
    
    // Copy indicator name with bounds checking
    string_copy(indicator->indicator_name, name, 64);
    
    ctx->indicator_count++;
    return 0;
}

// Update a health indicator value
int update_health_indicator(health_monitor_context_t* ctx, const char* indicator_name, int current_value) {
    if (!ctx || !indicator_name) {
        return -1;
    }
    
    int ind_idx = -1;
    int i;
    for (i = 0; i < ctx->indicator_count; i++) {
        if (string_compare(ctx->indicators[i].indicator_name, indicator_name) == 0) {
            ind_idx = i;
            break;
        }
    }
    
    if (ind_idx == -1) {
        return -1;
    }
    
    health_indicator_entry_t* indicator = &ctx->indicators[ind_idx];
    indicator->current_value = current_value;
    
    // Update status based on thresholds
    if (current_value >= indicator->threshold_critical) {
        indicator->current_status = HEALTH_STATUS_CRITICAL;
    } else if (current_value >= indicator->threshold_warning) {
        indicator->current_status = HEALTH_STATUS_POOR;
    } else {
        indicator->current_status = HEALTH_STATUS_GOOD;
    }
    
    indicator->last_update_time = 0; // In real implementation, this would be current time
    
    return 0;
}

// Evaluate overall system health
int evaluate_system_health(health_monitor_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    if (ctx->indicator_count == 0) {
        ctx->overall_health = HEALTH_STATUS_EXCELLENT;
        ctx->health_score = 100;
        return 0;
    }
    
    int total_score = 0;
    int critical_count = 0;
    int warning_count = 0;
    int i;
    
    for (i = 0; i < ctx->indicator_count; i++) {
        health_indicator_entry_t* indicator = &ctx->indicators[i];
        
        // Calculate score based on current value and thresholds
        int score;
        if (indicator->current_value >= indicator->threshold_critical) {
            score = 10; // Critical - very low score
            critical_count++;
        } else if (indicator->current_value >= indicator->threshold_warning) {
            score = 40; // Warning - moderate score
            warning_count++;
        } else {
            score = 80; // Good - high score
        }
        
        total_score += score;
    }
    
    // Calculate average score
    int avg_score = total_score / ctx->indicator_count;
    
    // Update counts
    ctx->critical_events_count = critical_count;
    ctx->warning_events_count = warning_count;
    
    // Determine overall health status
    if (critical_count > 0) {
        ctx->overall_health = HEALTH_STATUS_CRITICAL;
        avg_score = (avg_score < 20) ? avg_score : 20; // Cap at 20 for critical
    } else if (warning_count > 0) {
        ctx->overall_health = HEALTH_STATUS_FAIR;
        avg_score = (avg_score < 60) ? avg_score : 60; // Cap at 60 for fair
    } else {
        ctx->overall_health = HEALTH_STATUS_GOOD;
    }
    
    ctx->health_score = avg_score;
    ctx->last_evaluation_time = 0; // In real implementation, this would be current time
    
    return avg_score;
}

// Get overall health status
int get_overall_health_status(health_monitor_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    return ctx->overall_health;
}

// Trigger health optimization based on current status
int trigger_health_optimization(health_monitor_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    // In a real implementation, this would trigger optimization based on health status
    // For now, we'll just return the number of indicators that need attention
    int optimizations_needed = 0;
    int i;
    for (i = 0; i < ctx->indicator_count; i++) {
        if (ctx->indicators[i].current_status >= HEALTH_STATUS_POOR) {
            optimizations_needed++;
        }
    }
    
    // If we have a resource manager reference, we could trigger optimizations there too
    if (ctx->resource_manager_ref != NULL) {
        // In real implementation, we would call resource optimization
        // optimize_resources(ctx->resource_manager_ref);
    }
    
    return optimizations_needed;
}

// Generate health report
int get_health_report(health_monitor_context_t* ctx, char* report_buffer, int buffer_size) {
    if (!ctx || !report_buffer || buffer_size <= 0) {
        return -1;
    }
    
    // Build health report
    int pos = 0;
    
    // Add header
    const char* header = "MTProxy System Health Report\n";
    int header_len = string_length(header);
    if (pos + header_len >= buffer_size) return -1;
    
    int i;
    for (i = 0; i < header_len; i++) {
        report_buffer[pos++] = header[i];
    }
    
    // Add health score
    const char* score_prefix = "Overall Health Score: ";
    int score_prefix_len = string_length(score_prefix);
    if (pos + score_prefix_len >= buffer_size) return -1;
    
    for (i = 0; i < score_prefix_len; i++) {
        report_buffer[pos++] = score_prefix[i];
    }
    
    // Convert health score to string
    int score_val = ctx->health_score;
    if (score_val == 0) {
        report_buffer[pos++] = '0';
    } else {
        int digits = 0;
        int temp_calc = score_val;
        while (temp_calc > 0) {
            temp_calc /= 10;
            digits++;
        }
        
        if (pos + digits >= buffer_size) return -1;
        
        int digit_pos = pos + digits - 1;
        temp_calc = score_val;
        while (temp_calc > 0 && digit_pos >= pos) {
            report_buffer[digit_pos--] = '0' + (temp_calc % 10);
            temp_calc /= 10;
        }
        
        pos += digits;
    }
    
    report_buffer[pos++] = '/'; 
    report_buffer[pos++] = '1';
    report_buffer[pos++] = '0';
    report_buffer[pos++] = '0';
    report_buffer[pos++] = '\n';
    
    // Add health status
    const char* status_prefix = "Status: ";
    int status_prefix_len = string_length(status_prefix);
    if (pos + status_prefix_len >= buffer_size) return -1;
    
    for (i = 0; i < status_prefix_len; i++) {
        report_buffer[pos++] = status_prefix[i];
    }
    
    // Add status name based on overall health
    const char* status_name;
    switch (ctx->overall_health) {
        case HEALTH_STATUS_EXCELLENT:
            status_name = "EXCELLENT";
            break;
        case HEALTH_STATUS_GOOD:
            status_name = "GOOD";
            break;
        case HEALTH_STATUS_FAIR:
            status_name = "FAIR";
            break;
        case HEALTH_STATUS_POOR:
            status_name = "POOR";
            break;
        case HEALTH_STATUS_CRITICAL:
            status_name = "CRITICAL";
            break;
        default:
            status_name = "UNKNOWN";
            break;
    }
    
    int status_name_len = string_length(status_name);
    if (pos + status_name_len >= buffer_size) return -1;
    
    for (i = 0; i < status_name_len; i++) {
        report_buffer[pos++] = status_name[i];
    }
    
    report_buffer[pos++] = '\n';
    
    // Add event counts
    const char* events_line = "\nCritical Events: ";
    int events_len = string_length(events_line);
    if (pos + events_len >= buffer_size) return -1;
    
    for (i = 0; i < events_len; i++) {
        report_buffer[pos++] = events_line[i];
    }
    
    // Convert critical events count to string
    int crit_events = ctx->critical_events_count;
    if (crit_events == 0) {
        report_buffer[pos++] = '0';
    } else {
        int digits = 0;
        int temp_calc = crit_events;
        while (temp_calc > 0) {
            temp_calc /= 10;
            digits++;
        }
        
        if (pos + digits >= buffer_size) return -1;
        
        int digit_pos = pos + digits - 1;
        temp_calc = crit_events;
        while (temp_calc > 0 && digit_pos >= pos) {
            report_buffer[digit_pos--] = '0' + (temp_calc % 10);
            temp_calc /= 10;
        }
        
        pos += digits;
    }
    
    const char* warning_line = "\nWarning Events: ";
    int warning_len = string_length(warning_line);
    if (pos + warning_len >= buffer_size) return -1;
    
    for (i = 0; i < warning_len; i++) {
        report_buffer[pos++] = warning_line[i];
    }
    
    // Convert warning events count to string
    int warn_events = ctx->warning_events_count;
    if (warn_events == 0) {
        report_buffer[pos++] = '0';
    } else {
        int digits = 0;
        int temp_calc = warn_events;
        while (temp_calc > 0) {
            temp_calc /= 10;
            digits++;
        }
        
        if (pos + digits >= buffer_size) return -1;
        
        int digit_pos = pos + digits - 1;
        temp_calc = warn_events;
        while (temp_calc > 0 && digit_pos >= pos) {
            report_buffer[digit_pos--] = '0' + (temp_calc % 10);
            temp_calc /= 10;
        }
        
        pos += digits;
    }
    
    report_buffer[pos++] = '\n';
    
    // Add individual indicators
    const char* indicators_header = "\nDetailed Indicators:\n";
    int ind_header_len = string_length(indicators_header);
    if (pos + ind_header_len >= buffer_size) return -1;
    
    for (i = 0; i < ind_header_len; i++) {
        report_buffer[pos++] = indicators_header[i];
    }
    
    for (i = 0; i < ctx->indicator_count && i < 10; i++) { // Limit to first 10 for brevity
        health_indicator_entry_t* ind = &ctx->indicators[i];
        
        // Add indicator name
        int name_len = string_length(ind->indicator_name);
        if (pos + name_len + 20 >= buffer_size) break;
        
        int j;
        for (j = 0; j < name_len; j++) {
            report_buffer[pos++] = ind->indicator_name[j];
        }
        
        report_buffer[pos++] = ':';
        report_buffer[pos++] = ' ';
        
        // Add current value
        int val = ind->current_value;
        if (val == 0) {
            report_buffer[pos++] = '0';
        } else {
            int digits = 0;
            int temp_calc = val;
            while (temp_calc > 0) {
                temp_calc /= 10;
                digits++;
            }
            
            if (pos + digits >= buffer_size) break;
            
            int digit_pos = pos + digits - 1;
            temp_calc = val;
            while (temp_calc > 0 && digit_pos >= pos) {
                report_buffer[digit_pos--] = '0' + (temp_calc % 10);
                temp_calc /= 10;
            }
            
            pos += digits;
        }
        
        report_buffer[pos++] = '%';
        report_buffer[pos++] = ' ';
        
        // Add status
        const char* ind_status;
        switch (ind->current_status) {
            case HEALTH_STATUS_EXCELLENT:
            case HEALTH_STATUS_GOOD:
                ind_status = "(OK)";
                break;
            case HEALTH_STATUS_FAIR:
            case HEALTH_STATUS_POOR:
                ind_status = "(WARNING)";
                break;
            case HEALTH_STATUS_CRITICAL:
                ind_status = "(CRITICAL)";
                break;
            default:
                ind_status = "(UNKNOWN)";
                break;
        }
        
        int status_len = string_length(ind_status);
        if (pos + status_len >= buffer_size) break;
        
        for (j = 0; j < status_len; j++) {
            report_buffer[pos++] = ind_status[j];
        }
        
        report_buffer[pos++] = '\n';
    }
    
    report_buffer[pos] = '\0';
    return pos;
}

// Set reference to resource manager for coordinated optimization
int set_resource_manager_reference(health_monitor_context_t* ctx, struct resource_manager_context* res_mgr) {
    if (!ctx) {
        return -1;
    }
    
    ctx->resource_manager_ref = res_mgr;
    return 0;
}

// Cleanup the health monitor
void cleanup_health_monitor(health_monitor_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    // Reset all fields to default values
    ctx->indicator_count = 0;
    ctx->overall_health = HEALTH_STATUS_EXCELLENT;
    ctx->monitor_interval_ms = HEALTH_MONITOR_INTERVAL_MS;
    ctx->health_score = 100; // Perfect health initially
    ctx->last_evaluation_time = 0;
    ctx->critical_events_count = 0;
    ctx->warning_events_count = 0;
    ctx->resource_manager_ref = NULL;
    
    // Initialize all health indicators
    int i;
    for (i = 0; i < MAX_HEALTH_INDICATORS; i++) {
        health_indicator_entry_t* indicator = &ctx->indicators[i];
        indicator->indicator_type = HEALTH_INDICATOR_CPU; // Default value
        indicator->current_value = 0;
        indicator->threshold_critical = CRITICAL_HEALTH_THRESHOLD;
        indicator->threshold_warning = WARNING_HEALTH_THRESHOLD;
        indicator->current_status = HEALTH_STATUS_EXCELLENT;
        indicator->last_update_time = 0;
        indicator->indicator_name[0] = '\0';
    }
}

// Find indicator index by name
static int find_indicator_index(health_monitor_context_t* ctx, const char* indicator_name) {
    if (!ctx || !indicator_name) {
        return -1;
    }
    
    int i;
    for (i = 0; i < ctx->indicator_count; i++) {
        if (string_compare(ctx->indicators[i].indicator_name, indicator_name) == 0) {
            return i;
        }
    }
    
    return -1;
}

// Get indicator value by name
int get_indicator_value(health_monitor_context_t* ctx, const char* indicator_name) {
    if (!ctx || !indicator_name) {
        return -1;
    }
    
    int idx = find_indicator_index(ctx, indicator_name);
    if (idx == -1) {
        return -1;
    }
    
    return ctx->indicators[idx].current_value;
}