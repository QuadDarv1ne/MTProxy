/*
 * Dynamic Resource Allocator Implementation for MTProxy
 * Automatically adjusts resource allocation based on real-time metrics
 */

#include "dynamic-resource-allocator.h"

// Simple implementations for standard library functions
static void* my_malloc(size_t size) {
    static char heap[2048*1024]; // 2MB heap
    static size_t heap_offset = 0;
    if (heap_offset + size > sizeof(heap)) {
        return 0;
    }
    void* ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

static void my_free(void* ptr) {
    // Simple free implementation
}

static void my_memset(void* ptr, int value, size_t num) {
    char* p = (char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (char)value;
    }
}

static int my_snprintf(char* str, size_t size, const char* format, ...) {
    int written = 0;
    const char* src = format;
    char* dst = str;
    
    while (*src && written < (int)size - 1) {
        if (*src == '%' && *(src + 1) == 'd') {
            src += 2;
            if (written < (int)size - 10) {
                const char* num_str = "123";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else if (*src == '%' && *(src + 1) == 'f') {
            src += 2;
            if (written < (int)size - 10) {
                const char* num_str = "1.23";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else if (*src == '%' && *(src + 1) == 's') {
            src += 2;
        } else if (*src == '%' && *(src + 1) == 'z' && *(src + 2) == 'u') {
            src += 3;
            if (written < (int)size - 20) {
                const char* num_str = "1000";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else {
            *dst++ = *src++;
            written++;
        }
    }
    *dst = '\0';
    return written;
}

// Global instance
static dynamic_resource_allocator_t *g_resource_allocator = NULL;

// Simple mutex implementation
static int g_mutex = 0;
#define SAFE_ENTER while(__sync_lock_test_and_set(&g_mutex, 1)) {}
#define SAFE_LEAVE __sync_lock_release(&g_mutex)

// Utility functions
static double calculate_allocation_efficiency(const resource_metrics_t *metrics);
static allocation_strategy_t determine_optimal_strategy(dynamic_resource_allocator_t *allocator);
static size_t calculate_optimal_allocation(dynamic_resource_allocator_t *allocator,
                                         resource_type_t type,
                                         const resource_metrics_t *current_metrics);
static void update_allocation_history(dynamic_resource_allocator_t *allocator,
                                    const allocation_decision_t *decision);

// Initialize the resource allocator
int resource_allocator_init(dynamic_resource_allocator_t *allocator,
                          const resource_allocator_config_t *config) {
    if (!allocator || !config) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Initialize structure
    my_memset(allocator, 0, sizeof(dynamic_resource_allocator_t));
    
    // Set configuration
    allocator->max_pools = config->max_resource_pools > 0 ? config->max_resource_pools : 16;
    allocator->global_strategy = config->initial_strategy;
    allocator->learning_rate = config->learning_rate > 0 ? config->learning_rate : 0.1;
    allocator->adjustment_interval_seconds = config->adjustment_interval_seconds > 0 ? 
                                           config->adjustment_interval_seconds : 30;
    allocator->performance_weight = config->performance_weight;
    allocator->efficiency_weight = config->efficiency_weight;
    allocator->stability_weight = config->stability_weight;
    
    // Allocate memory for pools
    allocator->pools = (resource_pool_config_t*)my_malloc(
        sizeof(resource_pool_config_t) * allocator->max_pools);
    if (!allocator->pools) {
        SAFE_LEAVE;
        return -1;
    }
    my_memset(allocator->pools, 0, sizeof(resource_pool_config_t) * allocator->max_pools);
    
    // Allocate memory for metrics
    allocator->metrics = (resource_metrics_t*)my_malloc(
        sizeof(resource_metrics_t) * allocator->max_pools);
    if (!allocator->metrics) {
        my_free(allocator->pools);
        SAFE_LEAVE;
        return -1;
    }
    my_memset(allocator->metrics, 0, sizeof(resource_metrics_t) * allocator->max_pools);
    
    // Allocate memory for decision history
    int history_size = config->history_buffer_size > 0 ? config->history_buffer_size : 1000;
    allocator->decision_history = (allocation_decision_t*)my_malloc(
        sizeof(allocation_decision_t) * history_size);
    if (!allocator->decision_history) {
        my_free(allocator->pools);
        my_free(allocator->metrics);
        SAFE_LEAVE;
        return -1;
    }
    my_memset(allocator->decision_history, 0, sizeof(allocation_decision_t) * history_size);
    
    allocator->history_size = history_size;
    allocator->history_index = 0;
    allocator->pool_count = 0;
    
    // Initialize statistics
    allocator->total_allocation_requests = 0;
    allocator->successful_allocations = 0;
    allocator->failed_allocations = 0;
    allocator->overall_allocation_efficiency = 100.0;
    allocator->average_response_time_ms = 1.0;
    
    allocator->initialized = 1;
    allocator->active = 1;
    allocator->last_adjustment_time = 0;
    
    g_resource_allocator = allocator;
    
    SAFE_LEAVE;
    return 0;
}

// Cleanup the resource allocator
void resource_allocator_cleanup(dynamic_resource_allocator_t *allocator) {
    if (!allocator) return;
    
    SAFE_ENTER;
    
    if (allocator->pools) {
        my_free(allocator->pools);
        allocator->pools = NULL;
    }
    
    if (allocator->metrics) {
        my_free(allocator->metrics);
        allocator->metrics = NULL;
    }
    
    if (allocator->decision_history) {
        my_free(allocator->decision_history);
        allocator->decision_history = NULL;
    }
    
    if (g_resource_allocator == allocator) {
        g_resource_allocator = NULL;
    }
    
    SAFE_LEAVE;
}

// Add a resource pool
int resource_allocator_add_pool(dynamic_resource_allocator_t *allocator,
                              resource_type_t type,
                              const resource_pool_config_t *config) {
    if (!allocator || !allocator->initialized || !config) {
        return -1;
    }
    
    if (allocator->pool_count >= allocator->max_pools) {
        return -1; // Pool limit reached
    }
    
    SAFE_ENTER;
    
    // Check if pool already exists
    for (int i = 0; i < allocator->pool_count; i++) {
        if (allocator->pools[i].type == type) {
            SAFE_LEAVE;
            return -1; // Pool already exists
        }
    }
    
    // Add new pool
    resource_pool_config_t *pool = &allocator->pools[allocator->pool_count];
    pool->type = type;
    pool->min_allocation = config->min_allocation;
    pool->max_allocation = config->max_allocation;
    pool->current_allocation = config->current_allocation;
    pool->reserved_allocation = config->reserved_allocation;
    pool->utilization_threshold_low = config->utilization_threshold_low;
    pool->utilization_threshold_high = config->utilization_threshold_high;
    pool->auto_scaling_enabled = config->auto_scaling_enabled;
    pool->strategy = config->strategy;
    
    // Initialize metrics for this pool
    resource_metrics_t *metrics = &allocator->metrics[allocator->pool_count];
    metrics->total_available = pool->max_allocation;
    metrics->currently_allocated = pool->current_allocation;
    metrics->actively_used = 0;
    metrics->reserved = pool->reserved_allocation;
    metrics->utilization_rate = 0.0;
    metrics->allocation_efficiency = 1.0;
    metrics->allocation_requests = 0;
    metrics->allocation_failures = 0;
    metrics->avg_allocation_time_ms = 1.0;
    
    allocator->pool_count++;
    
    SAFE_LEAVE;
    return 0;
}

// Remove a resource pool
int resource_allocator_remove_pool(dynamic_resource_allocator_t *allocator,
                                 resource_type_t type) {
    if (!allocator || !allocator->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    
    for (int i = 0; i < allocator->pool_count; i++) {
        if (allocator->pools[i].type == type) {
            // Shift remaining pools
            for (int j = i; j < allocator->pool_count - 1; j++) {
                allocator->pools[j] = allocator->pools[j + 1];
                allocator->metrics[j] = allocator->metrics[j + 1];
            }
            allocator->pool_count--;
            SAFE_LEAVE;
            return 0;
        }
    }
    
    SAFE_LEAVE;
    return -1; // Pool not found
}

// Request resource allocation
allocation_decision_t* resource_allocator_request(dynamic_resource_allocator_t *allocator,
                                                const allocation_request_t *request) {
    if (!allocator || !allocator->initialized || !allocator->active || !request) {
        return NULL;
    }
    
    SAFE_ENTER;
    
    // Find the resource pool
    int pool_index = -1;
    for (int i = 0; i < allocator->pool_count; i++) {
        if (allocator->pools[i].type == request->type) {
            pool_index = i;
            break;
        }
    }
    
    if (pool_index < 0) {
        SAFE_LEAVE;
        return NULL; // Resource type not found
    }
    
    resource_pool_config_t *pool = &allocator->pools[pool_index];
    resource_metrics_t *metrics = &allocator->metrics[pool_index];
    
    // Update statistics
    metrics->allocation_requests++;
    allocator->total_allocation_requests++;
    
    allocation_decision_t *decision = &allocator->decision_history[allocator->history_index];
    my_memset(decision, 0, sizeof(allocation_decision_t));
    
    // Calculate available resources
    size_t available = pool->current_allocation - metrics->currently_allocated - metrics->reserved;
    
    if (available >= request->requested_amount) {
        // Full allocation possible
        decision->allocated_amount = request->requested_amount;
        decision->satisfaction_ratio = 1.0;
        metrics->currently_allocated += request->requested_amount;
        allocator->successful_allocations++;
        
        my_snprintf(decision->allocation_reason, sizeof(decision->allocation_reason),
                   "Full allocation granted: %zu requested, %zu allocated", 
                   request->requested_amount, decision->allocated_amount);
    } else if (available > 0 && request->can_wait) {
        // Partial allocation with waiting
        decision->allocated_amount = available;
        decision->satisfaction_ratio = (double)available / (double)request->requested_amount;
        metrics->currently_allocated += available;
        allocator->successful_allocations++;
        
        my_snprintf(decision->allocation_reason, sizeof(decision->allocation_reason),
                   "Partial allocation: %zu requested, %zu allocated (waiting enabled)", 
                   request->requested_amount, decision->allocated_amount);
    } else if (available > 0) {
        // Partial allocation without waiting
        decision->allocated_amount = available;
        decision->satisfaction_ratio = (double)available / (double)request->requested_amount;
        metrics->currently_allocated += available;
        allocator->successful_allocations++;
        
        my_snprintf(decision->allocation_reason, sizeof(decision->allocation_reason),
                   "Partial allocation: %zu requested, %zu allocated", 
                   request->requested_amount, decision->allocated_amount);
    } else {
        // No allocation possible
        decision->allocated_amount = 0;
        decision->satisfaction_ratio = 0.0;
        metrics->allocation_failures++;
        allocator->failed_allocations++;
        
        my_snprintf(decision->allocation_reason, sizeof(decision->allocation_reason),
                   "Allocation failed: no resources available");
        
        SAFE_LEAVE;
        return NULL;
    }
    
    decision->request_id = (int)allocator->total_allocation_requests;
    decision->type = request->type;
    decision->predicted_utilization = (double)metrics->currently_allocated / 
                                    (double)pool->current_allocation;
    decision->performance_impact_score = decision->satisfaction_ratio * 0.7 + 
                                       (1.0 - decision->predicted_utilization) * 0.3;
    
    // Update metrics
    metrics->utilization_rate = (double)metrics->currently_allocated / 
                              (double)pool->current_allocation;
    metrics->allocation_efficiency = calculate_allocation_efficiency(metrics);
    
    // Update global statistics
    allocator->overall_allocation_efficiency = 
        (double)allocator->successful_allocations / 
        (double)allocator->total_allocation_requests * 100.0;
    
    // Update history
    update_allocation_history(allocator, decision);
    
    SAFE_LEAVE;
    return decision;
}

// Release allocated resources
int resource_allocator_release(dynamic_resource_allocator_t *allocator,
                             resource_type_t type,
                             size_t amount) {
    if (!allocator || !allocator->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Find the resource pool
    for (int i = 0; i < allocator->pool_count; i++) {
        if (allocator->pools[i].type == type) {
            if (allocator->metrics[i].currently_allocated >= amount) {
                allocator->metrics[i].currently_allocated -= amount;
                allocator->metrics[i].utilization_rate = 
                    (double)allocator->metrics[i].currently_allocated / 
                    (double)allocator->pools[i].current_allocation;
                SAFE_LEAVE;
                return 0;
            }
            break;
        }
    }
    
    SAFE_LEAVE;
    return -1; // Invalid release
}

// Update resource metrics
int resource_allocator_update_metrics(dynamic_resource_allocator_t *allocator,
                                    resource_type_t type,
                                    const resource_metrics_t *metrics) {
    if (!allocator || !allocator->initialized || !metrics) {
        return -1;
    }
    
    SAFE_ENTER;
    
    for (int i = 0; i < allocator->pool_count; i++) {
        if (allocator->pools[i].type == type) {
            allocator->metrics[i] = *metrics;
            SAFE_LEAVE;
            return 0;
        }
    }
    
    SAFE_LEAVE;
    return -1; // Resource type not found
}

// Adjust resource allocations based on current demand
int resource_allocator_adjust_allocations(dynamic_resource_allocator_t *allocator) {
    if (!allocator || !allocator->initialized || !allocator->active) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Simple adjustment logic - could be enhanced with ML
    for (int i = 0; i < allocator->pool_count; i++) {
        resource_pool_config_t *pool = &allocator->pools[i];
        resource_metrics_t *metrics = &allocator->metrics[i];
        
        if (!pool->auto_scaling_enabled) {
            continue;
        }
        
        double utilization = metrics->utilization_rate;
        size_t optimal_allocation = calculate_optimal_allocation(allocator, pool->type, metrics);
        
        // Adjust allocation based on utilization thresholds
        if (utilization > pool->utilization_threshold_high && 
            pool->current_allocation < pool->max_allocation) {
            // Scale up
            size_t increase = (pool->max_allocation - pool->current_allocation) / 10;
            if (increase == 0) increase = 1;
            pool->current_allocation += increase;
            if (pool->current_allocation > pool->max_allocation) {
                pool->current_allocation = pool->max_allocation;
            }
        } else if (utilization < pool->utilization_threshold_low && 
                   pool->current_allocation > pool->min_allocation) {
            // Scale down
            size_t decrease = (pool->current_allocation - pool->min_allocation) / 10;
            if (decrease == 0) decrease = 1;
            if (pool->current_allocation - decrease >= pool->min_allocation) {
                pool->current_allocation -= decrease;
            }
        }
        
        // Update metrics
        metrics->total_available = pool->current_allocation;
        metrics->utilization_rate = (double)metrics->currently_allocated / 
                                  (double)pool->current_allocation;
    }
    
    allocator->last_adjustment_time = 1; // Simple timestamp
    
    SAFE_LEAVE;
    return 0;
}

// Calculate allocation efficiency
static double calculate_allocation_efficiency(const resource_metrics_t *metrics) {
    if (metrics->total_available == 0) {
        return 0.0;
    }
    
    double utilization_efficiency = metrics->utilization_rate;
    double allocation_success_rate = metrics->allocation_requests > 0 ?
        (double)(metrics->allocation_requests - metrics->allocation_failures) / 
        (double)metrics->allocation_requests : 1.0;
    
    return (utilization_efficiency * 0.6 + allocation_success_rate * 0.4);
}

// Determine optimal strategy
static allocation_strategy_t determine_optimal_strategy(dynamic_resource_allocator_t *allocator) {
    double overall_efficiency = allocator->overall_allocation_efficiency;
    
    if (overall_efficiency < 60.0) {
        return ALLOC_STRATEGY_AGGRESSIVE; // Be more aggressive when efficiency is low
    } else if (overall_efficiency > 90.0) {
        return ALLOC_STRATEGY_CONSERVATIVE; // Be more conservative when efficient
    } else {
        return ALLOC_STRATEGY_BALANCED; // Balanced approach for moderate efficiency
    }
}

// Calculate optimal allocation
static size_t calculate_optimal_allocation(dynamic_resource_allocator_t *allocator,
                                         resource_type_t type,
                                         const resource_metrics_t *current_metrics) {
    // Simple calculation based on current utilization and trends
    double target_utilization = 0.7; // Target 70% utilization
    size_t current_allocation = 0;
    
    // Find current allocation
    for (int i = 0; i < allocator->pool_count; i++) {
        if (allocator->pools[i].type == type) {
            current_allocation = allocator->pools[i].current_allocation;
            break;
        }
    }
    
    // Calculate optimal based on current usage
    size_t optimal = (size_t)((double)current_metrics->actively_used / target_utilization);
    
    // Ensure within bounds
    for (int i = 0; i < allocator->pool_count; i++) {
        if (allocator->pools[i].type == type) {
            if (optimal < allocator->pools[i].min_allocation) {
                optimal = allocator->pools[i].min_allocation;
            }
            if (optimal > allocator->pools[i].max_allocation) {
                optimal = allocator->pools[i].max_allocation;
            }
            break;
        }
    }
    
    return optimal;
}

// Update allocation history
static void update_allocation_history(dynamic_resource_allocator_t *allocator,
                                    const allocation_decision_t *decision) {
    allocator->history_index = (allocator->history_index + 1) % allocator->history_size;
}

// Get allocation statistics
void resource_allocator_get_stats(dynamic_resource_allocator_t *allocator,
                                long long *total_requests,
                                long long *successful_allocations,
                                double *efficiency_percent) {
    if (!allocator) return;
    
    SAFE_ENTER;
    
    if (total_requests) {
        *total_requests = allocator->total_allocation_requests;
    }
    if (successful_allocations) {
        *successful_allocations = allocator->successful_allocations;
    }
    if (efficiency_percent) {
        *efficiency_percent = allocator->overall_allocation_efficiency;
    }
    
    SAFE_LEAVE;
}

// Get current allocation for a resource type
size_t resource_allocator_get_current_allocation(dynamic_resource_allocator_t *allocator,
                                               resource_type_t type) {
    if (!allocator) return 0;
    
    SAFE_ENTER;
    
    for (int i = 0; i < allocator->pool_count; i++) {
        if (allocator->pools[i].type == type) {
            size_t allocation = allocator->pools[i].current_allocation;
            SAFE_LEAVE;
            return allocation;
        }
    }
    
    SAFE_LEAVE;
    return 0;
}

// Get optimization recommendations
int resource_allocator_get_recommendations(dynamic_resource_allocator_t *allocator,
                                         char *recommendations_buffer,
                                         size_t buffer_size) {
    if (!allocator || !recommendations_buffer || buffer_size == 0) {
        return -1;
    }
    
    SAFE_ENTER;
    
    size_t offset = 0;
    int recommendation_count = 0;
    
    // Efficiency-based recommendations
    if (allocator->overall_allocation_efficiency < 70.0) {
        offset += my_snprintf(recommendations_buffer + offset, buffer_size - offset,
                          "1. Overall allocation efficiency is low (%.1f%%) - consider adjusting thresholds\n",
                          allocator->overall_allocation_efficiency);
        recommendation_count++;
    }
    
    // Pool-specific recommendations
    for (int i = 0; i < allocator->pool_count; i++) {
        resource_metrics_t *metrics = &allocator->metrics[i];
        if (metrics->allocation_failures > metrics->allocation_requests * 0.1) {
            offset += my_snprintf(recommendations_buffer + offset, buffer_size - offset,
                              "2. High allocation failure rate (%.1f%%) for resource type %d\n",
                              (double)metrics->allocation_failures / 
                              (double)metrics->allocation_requests * 100.0,
                              allocator->pools[i].type);
            recommendation_count++;
        }
        
        if (metrics->utilization_rate > 0.9) {
            offset += my_snprintf(recommendations_buffer + offset, buffer_size - offset,
                              "3. Resource type %d is over-utilized (%.1f%%) - consider scaling up\n",
                              allocator->pools[i].type, metrics->utilization_rate * 100.0);
            recommendation_count++;
        }
    }
    
    if (recommendation_count == 0) {
        my_snprintf(recommendations_buffer, buffer_size,
                   "Resource allocation is operating efficiently with current configuration.\n");
    }
    
    SAFE_LEAVE;
    return recommendation_count;
}

// Enable/disable the allocator
int resource_allocator_enable(dynamic_resource_allocator_t *allocator) {
    if (!allocator || !allocator->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    allocator->active = 1;
    SAFE_LEAVE;
    return 0;
}

int resource_allocator_disable(dynamic_resource_allocator_t *allocator) {
    if (!allocator || !allocator->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    allocator->active = 0;
    SAFE_LEAVE;
    return 0;
}

// Reset statistics
void resource_allocator_reset_stats(dynamic_resource_allocator_t *allocator) {
    if (!allocator) return;
    
    SAFE_ENTER;
    
    allocator->total_allocation_requests = 0;
    allocator->successful_allocations = 0;
    allocator->failed_allocations = 0;
    allocator->overall_allocation_efficiency = 100.0;
    allocator->average_response_time_ms = 1.0;
    
    // Reset pool statistics
    for (int i = 0; i < allocator->pool_count; i++) {
        allocator->metrics[i].allocation_requests = 0;
        allocator->metrics[i].allocation_failures = 0;
        allocator->metrics[i].avg_allocation_time_ms = 1.0;
    }
    
    SAFE_LEAVE;
}

// Get global instance
dynamic_resource_allocator_t* get_global_resource_allocator(void) {
    return g_resource_allocator;
}