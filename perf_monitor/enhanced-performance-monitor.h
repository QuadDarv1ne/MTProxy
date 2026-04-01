/*
 * Enhanced Performance Monitor Header for MTProxy
 * Provides advanced performance metrics and optimization suggestions
 */

#ifndef ENHANCED_PERFORMANCE_MONITOR_H
#define ENHANCED_PERFORMANCE_MONITOR_H

#include <stdint.h>
#include <stddef.h>

// Threading support - simplified for cross-platform compatibility

#define HISTORY_SIZE 100

// Performance metrics structure
typedef struct {
    double cpu_usage;                    // Current CPU usage percentage
    double memory_usage;                 // Current memory usage percentage
    long long active_connections;        // Current number of active connections
    double avg_response_time_ms;         // Average response time in milliseconds
    long long total_requests;            // Total number of requests processed
    long long failed_requests;           // Total number of failed requests
    double error_rate_percent;           // Error rate as percentage
    double throughput;                   // Requests per time unit
} perf_metrics_t;

// Thresholds for alerting
typedef struct {
    double cpu_high_watermark;           // CPU usage threshold for alerts
    double memory_high_watermark;        // Memory usage threshold for alerts
    long long connections_high_watermark; // Connection count threshold for alerts
    double response_time_warning_ms;     // Response time threshold for alerts
    double error_rate_warning_percent;   // Error rate threshold for alerts
} perf_thresholds_t;

// Recommendations structure
typedef struct {
    int cpu_optimization_needed;         // Flag indicating CPU optimization needed
    int memory_optimization_needed;      // Flag indicating memory optimization needed
    int connection_optimization_needed;  // Flag indicating connection optimization needed
    int performance_optimization_needed; // Flag indicating performance optimization needed
    int stability_optimization_needed;   // Flag indicating stability optimization needed
    
    int cpu_suggestion_priority;         // Priority of CPU suggestions (0=low, 3=critical)
    int memory_suggestion_priority;      // Priority of memory suggestions
    int connection_suggestion_priority;  // Priority of connection suggestions
    int performance_suggestion_priority; // Priority of performance suggestions
    int stability_suggestion_priority;   // Priority of stability suggestions
    
    char cpu_suggestions[10][256];       // CPU optimization suggestions
    int cpu_suggestion_count;            // Number of CPU suggestions
    
    char memory_suggestions[10][256];    // Memory optimization suggestions
    int memory_suggestion_count;         // Number of memory suggestions
    
    char connection_suggestions[10][256]; // Connection optimization suggestions
    int connection_suggestion_count;      // Number of connection suggestions
    
    char performance_suggestions[10][256]; // Performance optimization suggestions
    int performance_suggestion_count;      // Number of performance suggestions
    
    char stability_suggestions[10][256];   // Stability optimization suggestions
    int stability_suggestion_count;        // Number of stability suggestions
    
    char trend_suggestions[10][256];       // Trend-based optimization suggestions
    int trend_suggestion_count;            // Number of trend suggestions
} perf_recommendations_t;

// Main performance monitor structure
typedef struct {
    perf_metrics_t current_metrics;       // Current performance metrics
    perf_thresholds_t thresholds;         // Alert thresholds
    
    // History for trend analysis
    double cpu_history[HISTORY_SIZE];
    double memory_history[HISTORY_SIZE];
    double response_time_history[HISTORY_SIZE];
    int history_index;
    
    long long samples_collected;          // Total samples collected
    
    // Mutex for thread safety - simplified implementation
    int mutex_lock;                       // Simple lock flag for thread safety
    
    int initialized;                      // Initialization flag
} enhanced_perf_monitor_t;

// Function declarations
enhanced_perf_monitor_t* enhanced_perf_monitor_init(void);
void enhanced_perf_monitor_update(enhanced_perf_monitor_t *monitor, 
                                double cpu_usage, 
                                double memory_usage, 
                                long long active_connections,
                                double avg_response_time_ms,
                                long long total_requests,
                                long long failed_requests);
void enhanced_perf_monitor_analyze(enhanced_perf_monitor_t *monitor, 
                                 perf_recommendations_t *recommendations);
void enhanced_perf_monitor_generate_report(enhanced_perf_monitor_t *monitor, 
                                        char *report_buffer, 
                                        size_t buffer_size);
void enhanced_perf_monitor_cleanup(enhanced_perf_monitor_t *monitor);
enhanced_perf_monitor_t* get_global_perf_monitor(void);

// Helper function declaration
static double calculate_trend(double *values, int size);

#endif // ENHANCED_PERFORMANCE_MONITOR_H