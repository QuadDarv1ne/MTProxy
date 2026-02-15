/*
 * Enhanced Performance Monitor for MTProxy
 * Provides advanced performance metrics and optimization suggestions
 */

#include "enhanced-performance-monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Global performance monitor instance
static enhanced_perf_monitor_t *g_perf_monitor = NULL;

// Simplified lock and unlock functions
#define SAFE_ENTER if (monitor) { \
    while (monitor->mutex_lock) continue; /* wait for unlocked */ \
    monitor->mutex_lock = 1;                /* lock */ \
}

#define SAFE_LEAVE if (monitor) { \
    monitor->mutex_lock = 0;                /* unlock */ \
}

// Initialize the enhanced performance monitor
enhanced_perf_monitor_t* enhanced_perf_monitor_init() {
    enhanced_perf_monitor_t *monitor = (enhanced_perf_monitor_t*)calloc(1, sizeof(enhanced_perf_monitor_t));
    if (!monitor) {
        return NULL;
    }
    
    // Initialize mutex for thread safety
    monitor->mutex_lock = 0;
    
    // Initialize default thresholds
    monitor->thresholds.cpu_high_watermark = 80.0;      // 80% CPU usage
    monitor->thresholds.memory_high_watermark = 85.0;   // 85% memory usage
    monitor->thresholds.connections_high_watermark = 8000; // 8000 connections
    monitor->thresholds.response_time_warning_ms = 100.0; // 100ms response time
    monitor->thresholds.error_rate_warning_percent = 1.0; // 1% error rate
    
    // Initialize history arrays
    for (int i = 0; i < HISTORY_SIZE; i++) {
        monitor->cpu_history[i] = 0.0;
        monitor->memory_history[i] = 0.0;
        monitor->response_time_history[i] = 0.0;
    }
    
    monitor->history_index = 0;
    monitor->samples_collected = 0;
    monitor->initialized = 1;
    
    g_perf_monitor = monitor;
    return monitor;
}

// Update performance metrics
void enhanced_perf_monitor_update(enhanced_perf_monitor_t *monitor, 
                                double cpu_usage, 
                                double memory_usage, 
                                long long active_connections,
                                double avg_response_time_ms,
                                long long total_requests,
                                long long failed_requests) {
    if (!monitor || !monitor->initialized) {
        return;
    }
    
    SAFE_ENTER
    
    // Update current metrics
    monitor->current_metrics.cpu_usage = cpu_usage;
    monitor->current_metrics.memory_usage = memory_usage;
    monitor->current_metrics.active_connections = active_connections;
    monitor->current_metrics.avg_response_time_ms = avg_response_time_ms;
    monitor->current_metrics.total_requests = total_requests;
    monitor->current_metrics.failed_requests = failed_requests;
    
    // Calculate error rate
    if (total_requests > 0) {
        monitor->current_metrics.error_rate_percent = 
            ((double)failed_requests / (double)total_requests) * 100.0;
    } else {
        monitor->current_metrics.error_rate_percent = 0.0;
    }
    
    // Store in history for trend analysis
    int idx = monitor->history_index;
    monitor->cpu_history[idx] = cpu_usage;
    monitor->memory_history[idx] = memory_usage;
    monitor->response_time_history[idx] = avg_response_time_ms;
    
    monitor->history_index = (monitor->history_index + 1) % HISTORY_SIZE;
    monitor->samples_collected++;
    
    // Calculate derived metrics
    monitor->current_metrics.throughput = 
        total_requests / (monitor->samples_collected > 0 ? monitor->samples_collected : 1);
    
    SAFE_LEAVE
}

// Analyze performance and provide optimization suggestions
void enhanced_perf_monitor_analyze(enhanced_perf_monitor_t *monitor, 
                                 perf_recommendations_t *recommendations) {
    if (!monitor || !recommendations || !monitor->initialized) {
        return;
    }
    
    SAFE_ENTER
    
    // Reset recommendations manually
    recommendations->cpu_optimization_needed = 0;
    recommendations->memory_optimization_needed = 0;
    recommendations->connection_optimization_needed = 0;
    recommendations->performance_optimization_needed = 0;
    recommendations->stability_optimization_needed = 0;
    recommendations->cpu_suggestion_count = 0;
    recommendations->memory_suggestion_count = 0;
    recommendations->connection_suggestion_count = 0;
    recommendations->performance_suggestion_count = 0;
    recommendations->stability_suggestion_count = 0;
    recommendations->trend_suggestion_count = 0;
    
    // Analyze CPU usage
    if (monitor->current_metrics.cpu_usage > monitor->thresholds.cpu_high_watermark) {
        recommendations->cpu_optimization_needed = 1;
        recommendations->cpu_suggestion_priority = 2; // High priority
        snprintf(recommendations->cpu_suggestions[recommendations->cpu_suggestion_count++],
                 sizeof(recommendations->cpu_suggestions[0]),
                 "CPU usage (%.2f%%) exceeds threshold (%.2f%%)",
                 monitor->current_metrics.cpu_usage, monitor->thresholds.cpu_high_watermark);
    }
    
    // Analyze memory usage
    if (monitor->current_metrics.memory_usage > monitor->thresholds.memory_high_watermark) {
        recommendations->memory_optimization_needed = 1;
        recommendations->memory_suggestion_priority = 2; // High priority
        snprintf(recommendations->memory_suggestions[recommendations->memory_suggestion_count++],
                 sizeof(recommendations->memory_suggestions[0]),
                 "Memory usage (%.2f%%) exceeds threshold (%.2f%%)",
                 monitor->current_metrics.memory_usage, monitor->thresholds.memory_high_watermark);
    }
    
    // Analyze connection count
    if (monitor->current_metrics.active_connections > monitor->thresholds.connections_high_watermark) {
        recommendations->connection_optimization_needed = 1;
        recommendations->connection_suggestion_priority = 1; // Medium priority
        snprintf(recommendations->connection_suggestions[recommendations->connection_suggestion_count++],
                 sizeof(recommendations->connection_suggestions[0]),
                 "Active connections (%lld) exceeds threshold (%lld)",
                 monitor->current_metrics.active_connections, monitor->thresholds.connections_high_watermark);
    }
    
    // Analyze response time
    if (monitor->current_metrics.avg_response_time_ms > monitor->thresholds.response_time_warning_ms) {
        recommendations->performance_optimization_needed = 1;
        recommendations->performance_suggestion_priority = 2; // High priority
        snprintf(recommendations->performance_suggestions[recommendations->performance_suggestion_count++],
                 sizeof(recommendations->performance_suggestions[0]),
                 "Average response time (%.2f ms) exceeds threshold (%.2f ms)",
                 monitor->current_metrics.avg_response_time_ms, monitor->thresholds.response_time_warning_ms);
    }
    
    // Analyze error rate
    if (monitor->current_metrics.error_rate_percent > monitor->thresholds.error_rate_warning_percent) {
        recommendations->stability_optimization_needed = 1;
        recommendations->stability_suggestion_priority = 3; // Critical priority
        snprintf(recommendations->stability_suggestions[recommendations->stability_suggestion_count++],
                 sizeof(recommendations->stability_suggestions[0]),
                 "Error rate (%.2f%%) exceeds threshold (%.2f%%)",
                 monitor->current_metrics.error_rate_percent, monitor->thresholds.error_rate_warning_percent);
    }
    
    // Trend analysis - check if metrics are consistently increasing
    if (monitor->samples_collected >= HISTORY_SIZE) {
        // Check for upward trends in CPU usage
        double cpu_trend = calculate_trend(monitor->cpu_history, HISTORY_SIZE);
        if (cpu_trend > 0.5) { // Significant upward trend
            snprintf(recommendations->trend_suggestions[recommendations->trend_suggestion_count++],
                     sizeof(recommendations->trend_suggestions[0]),
                     "CPU usage showing increasing trend (%.2f%% per sample)", cpu_trend);
        }
        
        // Check for upward trends in memory usage
        double memory_trend = calculate_trend(monitor->memory_history, HISTORY_SIZE);
        if (memory_trend > 0.3) { // Moderate upward trend
            snprintf(recommendations->trend_suggestions[recommendations->trend_suggestion_count++],
                     sizeof(recommendations->trend_suggestions[0]),
                     "Memory usage showing increasing trend (%.2f%% per sample)", memory_trend);
        }
        
        // Check for upward trends in response time
        double response_trend = calculate_trend(monitor->response_time_history, HISTORY_SIZE);
        if (response_trend > 2.0) { // Increasing trend
            snprintf(recommendations->trend_suggestions[recommendations->trend_suggestion_count++],
                     sizeof(recommendations->trend_suggestions[0]),
                     "Response time showing increasing trend (%.2f ms per sample)", response_trend);
        }
    }
    
    SAFE_LEAVE
}

// Helper function to calculate trend
static double calculate_trend(double *values, int size) {
    if (size < 2) return 0.0;
    
    // Simple linear regression to calculate trend
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_xx = 0;
    for (int i = 0; i < size; i++) {
        double x = (double)i;
        double y = values[i];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }
    
    double n = (double)size;
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
    return slope;
}

// Generate performance report
void enhanced_perf_monitor_generate_report(enhanced_perf_monitor_t *monitor, 
                                        char *report_buffer, 
                                        size_t buffer_size) {
    if (!monitor || !report_buffer || !monitor->initialized) {
        return;
    }
    
    SAFE_ENTER
    
    snprintf(report_buffer, buffer_size,
        "=== Enhanced Performance Report ===\n"
        "Timestamp: %ld\n"
        "Active Connections: %lld\n"
        "CPU Usage: %.2f%%\n"
        "Memory Usage: %.2f%%\n"
        "Avg Response Time: %.2f ms\n"
        "Total Requests: %lld\n"
        "Failed Requests: %lld\n"
        "Error Rate: %.2f%%\n"
        "Throughput: %.2f reqs/sample\n"
        "Samples Collected: %lld\n"
        "===============================\n",
        (long)time(NULL),
        monitor->current_metrics.active_connections,
        monitor->current_metrics.cpu_usage,
        monitor->current_metrics.memory_usage,
        monitor->current_metrics.avg_response_time_ms,
        monitor->current_metrics.total_requests,
        monitor->current_metrics.failed_requests,
        monitor->current_metrics.error_rate_percent,
        monitor->current_metrics.throughput,
        monitor->samples_collected
    );
    
    SAFE_LEAVE
}

// Cleanup the performance monitor
void enhanced_perf_monitor_cleanup(enhanced_perf_monitor_t *monitor) {
    if (!monitor) return;
    
    free(monitor);
    if (g_perf_monitor == monitor) {
        g_perf_monitor = NULL;
    }
}

// Get global monitor instance
enhanced_perf_monitor_t* get_global_perf_monitor() {
    return g_perf_monitor;
}