/*
 * Dynamic Resource Allocator for MTProxy
 * Automatically adjusts resource allocation based on real-time metrics
 * and workload demands
 */

#ifndef _DYNAMIC_RESOURCE_ALLOCATOR_H_
#define _DYNAMIC_RESOURCE_ALLOCATOR_H_

#include <stdint.h>
#include <stddef.h>

// Resource types
typedef enum {
    RESOURCE_TYPE_THREAD = 0,
    RESOURCE_TYPE_MEMORY = 1,
    RESOURCE_TYPE_NETWORK_BUFFER = 2,
    RESOURCE_TYPE_CRYPTO_CONTEXT = 3,
    RESOURCE_TYPE_CONNECTION_SLOT = 4,
    RESOURCE_TYPE_FILE_DESCRIPTOR = 5
} resource_type_t;

// Allocation strategies
typedef enum {
    ALLOC_STRATEGY_CONSERVATIVE = 0,  // Minimal allocation
    ALLOC_STRATEGY_BALANCED = 1,      // Balanced approach
    ALLOC_STRATEGY_AGGRESSIVE = 2,    // Maximum allocation
    ALLOC_STRATEGY_ADAPTIVE = 3       // Learn from usage patterns
} allocation_strategy_t;

// Resource pool configuration
typedef struct {
    resource_type_t type;
    size_t min_allocation;
    size_t max_allocation;
    size_t current_allocation;
    size_t reserved_allocation;
    double utilization_threshold_low;   // 0.0-1.0
    double utilization_threshold_high;  // 0.0-1.0
    int auto_scaling_enabled;
    allocation_strategy_t strategy;
} resource_pool_config_t;

// Resource metrics
typedef struct {
    size_t total_available;
    size_t currently_allocated;
    size_t actively_used;
    size_t reserved;
    double utilization_rate;           // 0.0-1.0
    double allocation_efficiency;      // 0.0-1.0
    long long allocation_requests;
    long long allocation_failures;
    double avg_allocation_time_ms;
} resource_metrics_t;

// Resource allocation request
typedef struct {
    resource_type_t type;
    size_t requested_amount;
    int priority;                      // 1-10, higher is more urgent
    int can_wait;                      // Can request wait for resources
    double timeout_seconds;
    void* user_data;                   // User context
} allocation_request_t;

// Allocation decision
typedef struct {
    int request_id;
    resource_type_t type;
    size_t allocated_amount;
    double satisfaction_ratio;         // 0.0-1.0 (requested vs allocated)
    double predicted_utilization;
    double performance_impact_score;   // Expected performance impact
    char allocation_reason[256];
} allocation_decision_t;

// Resource allocator context
typedef struct {
    // Configuration
    resource_pool_config_t *pools;
    int pool_count;
    int max_pools;
    allocation_strategy_t global_strategy;
    
    // Metrics and statistics
    resource_metrics_t *metrics;
    long long total_allocation_requests;
    long long successful_allocations;
    long long failed_allocations;
    double overall_allocation_efficiency;
    double average_response_time_ms;
    
    // History for learning
    allocation_decision_t *decision_history;
    int history_size;
    int history_index;
    
    // Adaptive parameters
    double learning_rate;
    double performance_weight;
    double efficiency_weight;
    double stability_weight;
    
    // Current state
    int initialized;
    int active;
    long long last_adjustment_time;
    int adjustment_interval_seconds;
} dynamic_resource_allocator_t;

// Configuration structure
typedef struct {
    int max_resource_pools;
    int history_buffer_size;
    allocation_strategy_t initial_strategy;
    double learning_rate;
    int adjustment_interval_seconds;
    double performance_weight;
    double efficiency_weight;
    double stability_weight;
} resource_allocator_config_t;

// Initialize the resource allocator
int resource_allocator_init(dynamic_resource_allocator_t *allocator,
                          const resource_allocator_config_t *config);

// Cleanup the resource allocator
void resource_allocator_cleanup(dynamic_resource_allocator_t *allocator);

// Add a resource pool
int resource_allocator_add_pool(dynamic_resource_allocator_t *allocator,
                              resource_type_t type,
                              const resource_pool_config_t *config);

// Remove a resource pool
int resource_allocator_remove_pool(dynamic_resource_allocator_t *allocator,
                                 resource_type_t type);

// Request resource allocation
allocation_decision_t* resource_allocator_request(dynamic_resource_allocator_t *allocator,
                                                const allocation_request_t *request);

// Release allocated resources
int resource_allocator_release(dynamic_resource_allocator_t *allocator,
                             resource_type_t type,
                             size_t amount);

// Update resource metrics
int resource_allocator_update_metrics(dynamic_resource_allocator_t *allocator,
                                    resource_type_t type,
                                    const resource_metrics_t *metrics);

// Adjust resource allocations based on current demand
int resource_allocator_adjust_allocations(dynamic_resource_allocator_t *allocator);

// Get allocation statistics
void resource_allocator_get_stats(dynamic_resource_allocator_t *allocator,
                                long long *total_requests,
                                long long *successful_allocations,
                                double *efficiency_percent);

// Get current allocation for a resource type
size_t resource_allocator_get_current_allocation(dynamic_resource_allocator_t *allocator,
                                               resource_type_t type);

// Get optimization recommendations
int resource_allocator_get_recommendations(dynamic_resource_allocator_t *allocator,
                                         char *recommendations_buffer,
                                         size_t buffer_size);

// Enable/disable the allocator
int resource_allocator_enable(dynamic_resource_allocator_t *allocator);
int resource_allocator_disable(dynamic_resource_allocator_t *allocator);

// Reset statistics
void resource_allocator_reset_stats(dynamic_resource_allocator_t *allocator);

// Get global instance
dynamic_resource_allocator_t* get_global_resource_allocator(void);

#endif // _DYNAMIC_RESOURCE_ALLOCATOR_H_