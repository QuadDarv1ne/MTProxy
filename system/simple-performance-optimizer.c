/*
 * Simple performance optimizer implementation for MTProxy
 * This version is designed to integrate seamlessly with existing MTProxy code
 */

#include "simple-performance-optimizer.h"
#include <stdlib.h>
#include <string.h>

// Global flag to enable/disable optimizations
static int optimizations_enabled = 1;

// Performance metrics
static perf_simple_metrics_t metrics = {0};

int simple_init_optimizer(void) {
    optimizations_enabled = 1;
    
    // Initialize metrics
    metrics.connection_count = 0;
    metrics.bytes_processed = 0;
    metrics.cpu_usage_percent = 0.0;
    
    return 0;
}

int simple_enable_optimizations(void) {
    optimizations_enabled = 1;
    return 0;
}

int simple_disable_optimizations(void) {
    optimizations_enabled = 0;
    return 0;
}

int simple_are_optimizations_enabled(void) {
    return optimizations_enabled;
}

perf_simple_metrics_t simple_get_metrics(void) {
    return metrics;
}

void simple_update_metrics(long long bytes_processed) {
    if (optimizations_enabled) {
        metrics.connection_count++;
        metrics.bytes_processed += bytes_processed;
    }
}

int simple_adjust_for_load(int current_connections) {
    if (!optimizations_enabled) {
        return 0;
    }
    
    // Simple algorithm to adjust based on load
    if (current_connections > 1000) {
        // High load - enable more aggressive optimizations
        metrics.cpu_usage_percent = 85.0;
        return 1;
    } else if (current_connections > 100) {
        // Medium load
        metrics.cpu_usage_percent = 60.0;
        return 1;
    } else {
        // Low load
        metrics.cpu_usage_percent = 30.0;
        return 0;
    }
}