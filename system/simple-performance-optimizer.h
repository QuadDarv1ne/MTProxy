/*
 * Simple performance optimizer interface for MTProxy
 * This version is designed to integrate seamlessly with existing MTProxy code
 */

#ifndef _SIMPLE_PERFORMANCE_OPTIMIZER_H_
#define _SIMPLE_PERFORMANCE_OPTIMIZER_H_

#include <stdint.h>

// Simple performance metrics structure
typedef struct {
    long long connection_count;
    long long bytes_processed;
    double cpu_usage_percent;
} perf_simple_metrics_t;

// Function prototypes
int simple_init_optimizer(void);
int simple_enable_optimizations(void);
int simple_disable_optimizations(void);
int simple_are_optimizations_enabled(void);
perf_simple_metrics_t simple_get_metrics(void);
void simple_update_metrics(long long bytes_processed);
int simple_adjust_for_load(int current_connections);

#endif