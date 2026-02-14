/*
 * Performance optimizer interface for MTProxy
 * Implements advanced optimization techniques for better performance
 */

#ifndef _PERFORMANCE_OPTIMIZER_H_
#define _PERFORMANCE_OPTIMIZER_H_

#include <stdint.h>
#include <stdbool.h>

// Performance optimization levels
typedef enum {
    PERF_LEVEL_BASIC = 0,
    PERF_LEVEL_ADVANCED,
    PERF_LEVEL_AGGRESSIVE,
    PERF_LEVEL_MAXIMUM
} perf_optimization_level_t;

// Performance metrics structure
typedef struct {
    uint64_t total_connections;
    uint64_t active_connections;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    double cpu_usage;
    double memory_usage;
    uint32_t avg_response_time;
    uint32_t peak_connections_per_sec;
    long long timestamp;
} perf_metrics_t;

// Main performance optimizer structure
typedef struct {
    perf_optimization_level_t level;
    bool adaptive_optimization_enabled;
    bool cpu_affinity_enabled;
    bool numa_optimization_enabled;
    bool memory_pool_optimized;
    int worker_threads_count;
    perf_metrics_t metrics;
} performance_optimizer_t;

// Function prototypes
int init_performance_optimizer(perf_optimization_level_t level);
int update_performance_optimizer(void);
int set_optimization_level(perf_optimization_level_t level);
perf_metrics_t get_current_metrics(void);
int apply_cpu_affinity(void);
int optimize_memory_allocation(void);
int adjust_worker_threads(int new_thread_count);
void cleanup_performance_optimizer(void);

#endif