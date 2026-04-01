/*
 * Advanced Memory Management System with Predictive Allocation for MTProxy
 * Intelligent memory allocation with forecasting and optimization
 */

#ifndef _ADVANCED_MEMORY_MANAGEMENT_H_
#define _ADVANCED_MEMORY_MANAGEMENT_H_

#include <stdint.h>
#include <stddef.h>

// Memory pool types
typedef enum {
    POOL_TYPE_GENERAL = 0,         // General purpose memory
    POOL_TYPE_NETWORK = 1,         // Network buffer memory
    POOL_TYPE_CRYPTO = 2,          // Cryptographic buffer memory
    POOL_TYPE_CONNECTION = 3,      // Connection context memory
    POOL_TYPE_CACHE = 4,           // Cache memory
    POOL_TYPE_TEMPORARY = 5,       // Temporary/scratch memory
    POOL_TYPE_LARGE_OBJECT = 6,    // Large object memory
    POOL_TYPE_SMALL_OBJECT = 7     // Small object memory
} memory_pool_type_t;

// Allocation strategies
typedef enum {
    STRATEGY_FIRST_FIT = 0,        // First fit allocation
    STRATEGY_BEST_FIT = 1,         // Best fit allocation
    STRATEGY_WORST_FIT = 2,        // Worst fit allocation
    STRATEGY_BUDDY_SYSTEM = 3,     // Buddy system allocation
    STRATEGY_SLAB_ALLOCATOR = 4,   // Slab allocator
    STRATEGY_ADAPTIVE = 5          // Adaptive strategy selection
} allocation_strategy_t;

// Memory allocation request
typedef struct {
    size_t requested_size;
    memory_pool_type_t pool_type;
    int alignment;                 // Required alignment (power of 2)
    int priority;                  // 1-10, higher is more urgent
    long long timeout_ms;          // Allocation timeout
    void* user_data;               // User context
    int can_wait;                  // Can request wait for memory
} memory_request_t;

// Memory block descriptor
typedef struct memory_block {
    void* address;                 // Memory block address
    size_t size;                   // Block size
    size_t requested_size;         // Originally requested size
    memory_pool_type_t pool_type;  // Pool type
    int is_allocated;              // Allocation status
    int alignment;                 // Alignment requirement
    long long allocation_time;     // When allocated
    long long last_access_time;    // Last access timestamp
    int access_count;              // Access frequency counter
    struct memory_block* next;     // Next block in free list
    struct memory_block* prev;     // Previous block in free list
} memory_block_t;

// Memory pool
typedef struct {
    memory_pool_type_t type;
    size_t total_size;
    size_t used_size;
    size_t free_size;
    size_t peak_usage;
    int block_count;
    int allocated_block_count;
    memory_block_t* free_blocks;
    memory_block_t* allocated_block_list;
    allocation_strategy_t strategy;
    int enable_compaction;
    int enable_defragmentation;
    double fragmentation_ratio;
    long long last_compaction_time;
    int compaction_count;
} memory_pool_t;

// Memory prediction model
typedef struct {
    double usage_patterns[8];      // Historical usage patterns
    double prediction_weights[8];  // Weights for different time periods
    size_t predicted_demand;       // Predicted memory demand
    size_t confidence_level;       // Prediction confidence 0-100
    long long last_prediction_time;
    long long prediction_horizon;  // Prediction time horizon in seconds
    int model_accuracy;            // Model accuracy percentage
} memory_prediction_model_t;

// Garbage collection configuration
typedef struct {
    int enable_garbage_collection;
    int gc_threshold_percent;      // Trigger GC when usage exceeds this
    int gc_interval_seconds;       // Regular GC interval
    int enable_incremental_gc;     // Incremental collection
    size_t min_gc_size;            // Minimum size to trigger GC
    double gc_cpu_limit;           // Maximum CPU usage for GC
    int enable_concurrent_gc;      // Concurrent collection
} gc_config_t;

// Memory pressure indicators
typedef struct {
    double current_pressure;       // 0.0-1.0 memory pressure level
    double predicted_pressure;     // Predicted future pressure
    int pressure_trend;            // -1=decreasing, 0=stable, 1=increasing
    long long pressure_timestamp;  // When pressure was measured
    size_t available_memory;       // Currently available memory
    size_t total_memory;           // Total managed memory
    int critical_level;            // Critical memory level reached
} memory_pressure_t;

// Advanced memory manager configuration
typedef struct {
    size_t initial_pool_sizes[8];  // Initial sizes for each pool type
    allocation_strategy_t default_strategy;
    int enable_prediction;
    int prediction_window_seconds;
    int enable_garbage_collection;
    gc_config_t gc_config;
    int enable_memory_profiling;
    int profile_sampling_rate;
    int enable_statistics;
    int stats_collection_interval;
    int enable_auto_tuning;
    double auto_tuning_threshold;
    int memory_safety_checks;
    int enable_logging;
    char log_file[256];
} advanced_memory_config_t;

// Memory allocation statistics
typedef struct {
    long long total_allocations;
    long long successful_allocations;
    long long failed_allocations;
    long long total_deallocations;
    long long reuse_count;
    double allocation_success_rate;
    double average_allocation_time_ms;
    double peak_memory_usage_mb;
    double current_memory_usage_mb;
    double fragmentation_percent;
    long long compaction_operations;
    long long garbage_collections;
    double gc_efficiency_percent;
} memory_stats_t;

// Advanced memory manager
typedef struct {
    // Configuration
    advanced_memory_config_t config;
    
    // Memory pools
    memory_pool_t pools[8];
    int pool_count;
    
    // Prediction system
    memory_prediction_model_t prediction_model;
    memory_pressure_t current_pressure;
    
    // Statistics
    memory_stats_t stats;
    memory_stats_t* historical_stats;
    int stats_history_size;
    int stats_history_index;
    
    // Garbage collection
    long long last_gc_time;
    size_t gc_threshold;
    int gc_active;
    
    // Auto-tuning
    long long last_tuning_time;
    int tuning_active;
    double current_efficiency;
    double target_efficiency;
    
    // Memory profiling
    long long profile_start_time;
    int profiling_active;
    long long profiled_allocations;
    
    // Safety and monitoring
    int memory_corruption_detected;
    long long corruption_count;
    int safety_checks_enabled;
    long long safety_check_count;
    
    // State
    int initialized;
    int active;
    long long start_time;
    size_t total_managed_memory;
    size_t currently_used_memory;
} advanced_memory_manager_t;

// Memory allocation result
typedef struct {
    void* address;                 // Allocated memory address
    size_t actual_size;            // Actual allocated size
    size_t requested_size;         // Originally requested size
    memory_pool_type_t pool_type;  // Pool where allocated
    int allocation_success;        // Success flag
    double allocation_time_ms;     // Time taken for allocation
    char error_message[256];       // Error message if failed
    long long allocation_id;       // Unique allocation identifier
} memory_allocation_result_t;

// Memory usage report
typedef struct {
    long long timestamp;
    size_t total_memory;
    size_t used_memory;
    size_t free_memory;
    size_t fragmented_memory;
    double utilization_percent;
    double fragmentation_percent;
    int pool_count;
    struct {
        memory_pool_type_t type;
        size_t total_size;
        size_t used_size;
        size_t free_size;
        double utilization;
        int block_count;
        int allocated_blocks;
    } pool_stats[8];
    memory_pressure_t pressure;
    memory_stats_t recent_stats;
} memory_usage_report_t;

// Initialize advanced memory manager
int memory_manager_init(advanced_memory_manager_t *manager,
                       const advanced_memory_config_t *config);

// Cleanup memory manager
void memory_manager_cleanup(advanced_memory_manager_t *manager);

// Allocate memory
memory_allocation_result_t* memory_manager_allocate(advanced_memory_manager_t *manager,
                                                  const memory_request_t *request);

// Deallocate memory
int memory_manager_deallocate(advanced_memory_manager_t *manager,
                             void* address);

// Reallocate memory
memory_allocation_result_t* memory_manager_reallocate(advanced_memory_manager_t *manager,
                                                    void* address,
                                                    size_t new_size);

// Get memory pool information
int memory_manager_get_pool_info(advanced_memory_manager_t *manager,
                               memory_pool_type_t pool_type,
                               memory_pool_t *pool_info);

// Run garbage collection
int memory_manager_run_garbage_collection(advanced_memory_manager_t *manager);

// Predict memory demand
size_t memory_manager_predict_demand(advanced_memory_manager_t *manager,
                                   memory_pool_type_t pool_type,
                                   long long time_horizon_seconds);

// Get memory pressure status
memory_pressure_t* memory_manager_get_pressure(advanced_memory_manager_t *manager);

// Get memory usage report
memory_usage_report_t* memory_manager_get_usage_report(advanced_memory_manager_t *manager);

// Enable/disable memory manager
int memory_manager_enable(advanced_memory_manager_t *manager);
int memory_manager_disable(advanced_memory_manager_t *manager);

// Reset statistics
void memory_manager_reset_stats(advanced_memory_manager_t *manager);

// Export memory data
int memory_manager_export_data(advanced_memory_manager_t *manager,
                              const char *filename);

// Import memory data
int memory_manager_import_data(advanced_memory_manager_t *manager,
                              const char *filename);

// Get memory statistics
void memory_manager_get_stats(advanced_memory_manager_t *manager,
                             memory_stats_t *stats);

// Get global instance
advanced_memory_manager_t* get_global_memory_manager(void);

#endif // _ADVANCED_MEMORY_MANAGEMENT_H_