/*
 * Enhanced Memory Optimizer Header for MTProxy
 * Implements advanced memory management with fragmentation reduction
 * and intelligent allocation strategies
 */

#ifndef ENHANCED_MEMORY_OPTIMIZER_H
#define ENHANCED_MEMORY_OPTIMIZER_H

#include <stddef.h>

// Allocation strategies
typedef enum {
    MEM_ALLOC_STRATEGY_SPEED = 0,      // Prioritize allocation speed
    MEM_ALLOC_STRATEGY_MEMORY = 1,     // Prioritize memory efficiency
    MEM_ALLOC_STRATEGY_BALANCED = 2    // Balanced approach
} mem_allocation_strategy_t;

// Configuration structure
typedef struct {
    int enable_fragmentation_reduction;
    int enable_adaptive_allocation;
    int enable_memory_pooling;
    int enable_garbage_collection;
    size_t min_pool_size;
    size_t max_pool_size;
    int fragmentation_threshold;       // Percentage
    int gc_threshold;                  // Percentage
    long long gc_interval_ms;
    mem_allocation_strategy_t allocation_strategy;
    int pool_growth_factor;            // Percentage (e.g., 150 = 1.5x)
    size_t max_fragmentation_size;
    int enable_statistics;
} enhanced_mem_config_t;

// Statistics structure
typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
    int fragmentation_level;           // Percentage
    long long gc_cycles;
    long long allocation_count;
    long long free_count;
    long long pool_expansions;
    long long fragmentation_reductions;
} enhanced_mem_stats_t;

// Enhanced memory optimizer structure
typedef struct {
    // Configuration
    enhanced_mem_config_t config;
    
    // Statistics
    enhanced_mem_stats_t stats;
    
    // Memory pool management
    int pool_count;
    size_t total_pool_size;
    size_t fragmented_memory;
    
    // State management
    long long last_gc_time;
    long long last_fragmentation_check;
    int initialized;
} enhanced_memory_optimizer_t;

// Function declarations
enhanced_memory_optimizer_t* enhanced_memory_optimizer_init(const enhanced_mem_config_t *config);
void* enhanced_malloc(enhanced_memory_optimizer_t *optimizer, size_t size);
void enhanced_free(enhanced_memory_optimizer_t *optimizer, void *ptr);
int enhanced_memory_optimizer_check_pool_expansion(enhanced_memory_optimizer_t *optimizer);
int enhanced_memory_optimizer_check_garbage_collection(enhanced_memory_optimizer_t *optimizer);
int enhanced_memory_optimizer_perform_gc(enhanced_memory_optimizer_t *optimizer);
enhanced_mem_stats_t enhanced_memory_optimizer_get_stats(enhanced_memory_optimizer_t *optimizer);
void enhanced_memory_optimizer_reset_stats(enhanced_memory_optimizer_t *optimizer);
void enhanced_memory_optimizer_cleanup(enhanced_memory_optimizer_t *optimizer);
enhanced_memory_optimizer_t* get_global_enhanced_memory_optimizer(void);
void enhanced_memory_optimizer_get_usage_report(enhanced_memory_optimizer_t *optimizer,
                                              char *report_buffer, size_t buffer_size);

#endif // ENHANCED_MEMORY_OPTIMIZER_H