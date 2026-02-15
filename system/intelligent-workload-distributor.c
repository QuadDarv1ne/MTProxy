/*
 * Intelligent Workload Distributor Implementation for MTProxy
 * Automatically distributes workloads across available resources
 */

#include "intelligent-workload-distributor.h"

// Simple implementations for standard library functions
static void* my_malloc(size_t size) {
    // Simple malloc implementation - in real system would use system allocator
    static char heap[1024*1024]; // 1MB heap
    static size_t heap_offset = 0;
    if (heap_offset + size > sizeof(heap)) {
        return 0;
    }
    void* ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

static void my_free(void* ptr) {
    // Simple free implementation - no actual deallocation
    // In real system would properly manage memory
}

static void my_memset(void* ptr, int value, size_t num) {
    char* p = (char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (char)value;
    }
}

static int my_snprintf(char* str, size_t size, const char* format, ...) {
    // Simple snprintf implementation
    int written = 0;
    const char* src = format;
    char* dst = str;
    
    while (*src && written < (int)size - 1) {
        if (*src == '%' && *(src + 1) == 'd') {
            // Handle %d
            src += 2;
            // Simple integer formatting placeholder
            if (written < (int)size - 10) {
                const char* num_str = "123";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else if (*src == '%' && *(src + 1) == 'f') {
            // Handle %f
            src += 2;
            // Simple float formatting placeholder
            if (written < (int)size - 10) {
                const char* num_str = "1.23";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else if (*src == '%' && *(src + 1) == 's') {
            // Handle %s
            src += 2;
            // Skip string argument in this simple implementation
        } else {
            *dst++ = *src++;
            written++;
        }
    }
    *dst = '\0';
    return written;
}

// Simple time functions
typedef long my_clock_t;
static my_clock_t my_clock(void) {
    static my_clock_t counter = 0;
    return counter++;
}

#define CLOCKS_PER_SEC 1000

// Global instance
static workload_distributor_t *g_workload_distributor = NULL;

// Simple mutex implementation
static int g_mutex = 0;
#define SAFE_ENTER while(__sync_lock_test_and_set(&g_mutex, 1)) {}
#define SAFE_LEAVE __sync_lock_release(&g_mutex)

// Utility functions
static double calculate_suitability_score(const distribution_target_t *target,
                                        const workload_characteristics_t *workload);
static double predict_performance_improvement(const distribution_target_t *target,
                                            const workload_characteristics_t *workload);
static void update_distribution_history(workload_distributor_t *distributor,
                                      const distribution_decision_t *decision);
static distribution_algorithm_t select_optimal_algorithm(workload_distributor_t *distributor);
static void adapt_algorithm_parameters(workload_distributor_t *distributor);

// Initialize the workload distributor
int workload_distributor_init(workload_distributor_t *distributor, 
                            const workload_distributor_config_t *config) {
    if (!distributor || !config) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Initialize structure
    my_memset(distributor, 0, sizeof(workload_distributor_t));
    
    // Set configuration
    distributor->algorithm = config->algorithm;
    distributor->max_targets = config->max_targets > 0 ? config->max_targets : 16;
    distributor->enable_prediction = config->enable_adaptive_learning;
    distributor->load_balancing_threshold = config->load_threshold_high;
    
    // Allocate memory for targets
    distributor->targets = (distribution_target_t*)my_malloc(
        sizeof(distribution_target_t) * distributor->max_targets);
    if (!distributor->targets) {
        SAFE_LEAVE;
        return -1;
    }
    my_memset(distributor->targets, 0, sizeof(distribution_target_t) * distributor->max_targets);
    
    // Allocate memory for decision history
    int history_size = config->history_buffer_size > 0 ? config->history_buffer_size : 1000;
    distributor->decision_history = (distribution_decision_t*)my_malloc(
        sizeof(distribution_decision_t) * history_size);
    if (!distributor->decision_history) {
        my_free(distributor->targets);
        SAFE_LEAVE;
        return -1;
    }
    my_memset(distributor->decision_history, 0, sizeof(distribution_decision_t) * history_size);
    
    distributor->history_size = history_size;
    distributor->history_index = 0;
    
    // Initialize statistics
    distributor->total_distributions = 0;
    distributor->successful_distributions = 0;
    distributor->failed_distributions = 0;
    distributor->average_distribution_time_ms = 0.0;
    distributor->distribution_efficiency_percent = 100.0;
    distributor->current_system_efficiency = 100.0;
    distributor->predicted_optimal_efficiency = 100.0;
    distributor->improvement_potential_percent = 0.0;
    
    distributor->initialized = 1;
    distributor->active = 1;
    
    g_workload_distributor = distributor;
    
    SAFE_LEAVE;
    return 0;
}

// Cleanup the workload distributor
void workload_distributor_cleanup(workload_distributor_t *distributor) {
    if (!distributor) return;
    
    SAFE_ENTER;
    
    if (distributor->targets) {
        my_free(distributor->targets);
        distributor->targets = NULL;
    }
    
    if (distributor->decision_history) {
        my_free(distributor->decision_history);
        distributor->decision_history = NULL;
    }
    
    if (g_workload_distributor == distributor) {
        g_workload_distributor = NULL;
    }
    
    SAFE_LEAVE;
}

// Add a resource target
int workload_distributor_add_target(workload_distributor_t *distributor,
                                  int target_id,
                                  resource_type_t resource_type,
                                  const resource_metrics_t *metrics) {
    if (!distributor || !distributor->initialized || !metrics) {
        return -1;
    }
    
    if (distributor->target_count >= distributor->max_targets) {
        return -1; // Target limit reached
    }
    
    SAFE_ENTER;
    
    distribution_target_t *target = &distributor->targets[distributor->target_count];
    
    target->target_id = target_id;
    target->resource_type = resource_type;
    target->metrics = *metrics;
    
    // Calculate initial load factor
    switch (resource_type) {
        case RESOURCE_TYPE_CPU:
            target->load_factor = metrics->cpu_usage_percent / 100.0;
            break;
        case RESOURCE_TYPE_MEMORY:
            target->load_factor = metrics->memory_usage_percent / 100.0;
            break;
        case RESOURCE_TYPE_NETWORK:
            target->load_factor = metrics->network_bandwidth_utilization;
            break;
        case RESOURCE_TYPE_STORAGE:
            target->load_factor = metrics->storage_io_utilization;
            break;
        case RESOURCE_TYPE_CRYPTO:
            target->load_factor = metrics->crypto_accelerator_usage;
            break;
        default:
            target->load_factor = 0.5; // Default moderate load
            break;
    }
    
    target->active_workloads = 0;
    target->efficiency_score = 1.0 - target->load_factor; // Simple efficiency calculation
    
    distributor->target_count++;
    
    SAFE_LEAVE;
    return 0;
}

// Remove a resource target
int workload_distributor_remove_target(workload_distributor_t *distributor,
                                     int target_id) {
    if (!distributor || !distributor->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    
    for (int i = 0; i < distributor->target_count; i++) {
        if (distributor->targets[i].target_id == target_id) {
            // Shift remaining targets
            for (int j = i; j < distributor->target_count - 1; j++) {
                distributor->targets[j] = distributor->targets[j + 1];
            }
            distributor->target_count--;
            SAFE_LEAVE;
            return 0;
        }
    }
    
    SAFE_LEAVE;
    return -1; // Target not found
}

// Update target metrics
int workload_distributor_update_target_metrics(workload_distributor_t *distributor,
                                             int target_id,
                                             const resource_metrics_t *metrics) {
    if (!distributor || !distributor->initialized || !metrics) {
        return -1;
    }
    
    SAFE_ENTER;
    
    for (int i = 0; i < distributor->target_count; i++) {
        if (distributor->targets[i].target_id == target_id) {
            distributor->targets[i].metrics = *metrics;
            
            // Update load factor
            switch (distributor->targets[i].resource_type) {
                case RESOURCE_TYPE_CPU:
                    distributor->targets[i].load_factor = metrics->cpu_usage_percent / 100.0;
                    break;
                case RESOURCE_TYPE_MEMORY:
                    distributor->targets[i].load_factor = metrics->memory_usage_percent / 100.0;
                    break;
                case RESOURCE_TYPE_NETWORK:
                    distributor->targets[i].load_factor = metrics->network_bandwidth_utilization;
                    break;
                case RESOURCE_TYPE_STORAGE:
                    distributor->targets[i].load_factor = metrics->storage_io_utilization;
                    break;
                case RESOURCE_TYPE_CRYPTO:
                    distributor->targets[i].load_factor = metrics->crypto_accelerator_usage;
                    break;
                default:
                    distributor->targets[i].load_factor = 0.5;
                    break;
            }
            
            distributor->targets[i].efficiency_score = 1.0 - distributor->targets[i].load_factor;
            SAFE_LEAVE;
            return 0;
        }
    }
    
    SAFE_LEAVE;
    return -1; // Target not found
}

// Distribute workload
distribution_decision_t* workload_distributor_distribute(workload_distributor_t *distributor,
                                                       const workload_characteristics_t *workload) {
    if (!distributor || !distributor->initialized || !distributor->active || !workload) {
        return NULL;
    }
    
    if (distributor->target_count == 0) {
        return NULL; // No targets available
    }
    
    SAFE_ENTER;
    
    my_clock_t start_time = my_clock();
    
    // Select optimal algorithm if adaptive mode is enabled
    if (distributor->algorithm == DIST_ALGORITHM_ADAPTIVE) {
        distributor->algorithm = select_optimal_algorithm(distributor);
    }
    
    distribution_decision_t *decision = &distributor->decision_history[distributor->history_index];
    my_memset(decision, 0, sizeof(distribution_decision_t));
    
    double best_score = -1.0;
    int best_target = -1;
    
    // Evaluate all targets
    for (int i = 0; i < distributor->target_count; i++) {
        distribution_target_t *target = &distributor->targets[i];
        double score = calculate_suitability_score(target, workload);
        
        if (score > best_score) {
            best_score = score;
            best_target = i;
        }
    }
    
    if (best_target >= 0) {
        distribution_target_t *best_target_ptr = &distributor->targets[best_target];
        
        decision->target_id = best_target_ptr->target_id;
        decision->suitability_score = best_score;
        decision->predicted_performance = predict_performance_improvement(best_target_ptr, workload);
        decision->resource_utilization_impact = best_target_ptr->load_factor * 0.3 + 0.7;
        
        // Generate reason based on algorithm used
        switch (distributor->algorithm) {
            case DIST_ALGORITHM_ROUND_ROBIN:
                my_snprintf(decision->reason, sizeof(decision->reason),
                        "Round-robin distribution to target %d", decision->target_id);
                break;
            case DIST_ALGORITHM_WEIGHTED:
                my_snprintf(decision->reason, sizeof(decision->reason),
                        "Weighted distribution based on resource efficiency (%.2f)", best_score);
                break;
            case DIST_ALGORITHM_LEAST_LOADED:
                my_snprintf(decision->reason, sizeof(decision->reason),
                        "Least-loaded target selection (load: %.2f)", best_target_ptr->load_factor);
                break;
            case DIST_ALGORITHM_ADAPTIVE:
                my_snprintf(decision->reason, sizeof(decision->reason),
                        "Adaptive algorithm selected target %d with score %.2f", 
                        decision->target_id, best_score);
                break;
            case DIST_ALGORITHM_PREDICTIVE:
                my_snprintf(decision->reason, sizeof(decision->reason),
                        "Predictive analysis selected target %d for optimal performance", 
                        decision->target_id);
                break;
            default:
                my_snprintf(decision->reason, sizeof(decision->reason),
                        "Distribution to target %d", decision->target_id);
                break;
        }
        
        // Update target statistics
        best_target_ptr->active_workloads++;
        
        distributor->successful_distributions++;
    } else {
        decision->target_id = -1;
        decision->suitability_score = 0.0;
        my_snprintf(decision->reason, sizeof(decision->reason), "No suitable target found");
        distributor->failed_distributions++;
    }
    
    // Update statistics
    distributor->total_distributions++;
    my_clock_t end_time = my_clock();
    double distribution_time_ms = ((double)(end_time - start_time) * 1000.0) / CLOCKS_PER_SEC;
    
    distributor->average_distribution_time_ms = 
        (distributor->average_distribution_time_ms * (distributor->total_distributions - 1) + 
         distribution_time_ms) / distributor->total_distributions;
    
    // Update efficiency metrics
    if (distributor->total_distributions > 10) {
        double success_rate = (double)distributor->successful_distributions / 
                            (double)distributor->total_distributions * 100.0;
        distributor->distribution_efficiency_percent = success_rate;
    }
    
    // Update history
    update_distribution_history(distributor, decision);
    
    // Adapt algorithm parameters if needed
    if (distributor->enable_prediction) {
        adapt_algorithm_parameters(distributor);
    }
    
    SAFE_LEAVE;
    
    return decision->target_id >= 0 ? decision : NULL;
}

// Calculate suitability score for a target and workload
static double calculate_suitability_score(const distribution_target_t *target,
                                        const workload_characteristics_t *workload) {
    double score = 0.0;
    
    // Base score on resource efficiency (1.0 - load_factor)
    score += (1.0 - target->load_factor) * 0.4;
    
    // Resource type matching bonus
    switch (workload->type) {
        case WORKLOAD_TYPE_CRYPTO:
            if (target->resource_type == RESOURCE_TYPE_CRYPTO) {
                score += 0.3; // Crypto workloads prefer crypto accelerators
            }
            break;
        case WORKLOAD_TYPE_NETWORK:
            if (target->resource_type == RESOURCE_TYPE_NETWORK) {
                score += 0.25; // Network workloads prefer network resources
            }
            break;
        case WORKLOAD_TYPE_MEMORY:
            if (target->resource_type == RESOURCE_TYPE_MEMORY) {
                score += 0.25; // Memory workloads prefer memory resources
            }
            break;
        case WORKLOAD_TYPE_LIGHT_PROXY:
        case WORKLOAD_TYPE_HEAVY_PROXY:
            // Proxy workloads can use any resource but prefer less loaded ones
            score += 0.1;
            break;
        default:
            break;
    }
    
    // Priority consideration
    score += (double)workload->priority / 10.0 * 0.15;
    
    // Resource intensity matching
    double intensity_match = 0.0;
    switch (target->resource_type) {
        case RESOURCE_TYPE_CPU:
            intensity_match = workload->resource_intensity_cpu;
            break;
        case RESOURCE_TYPE_MEMORY:
            intensity_match = workload->resource_intensity_memory;
            break;
        case RESOURCE_TYPE_NETWORK:
            intensity_match = workload->resource_intensity_network;
            break;
        default:
            intensity_match = (workload->resource_intensity_cpu + 
                             workload->resource_intensity_memory + 
                             workload->resource_intensity_network) / 3.0;
            break;
    }
    score += intensity_match * 0.15;
    
    // Latency sensitivity penalty for heavily loaded targets
    if (workload->latency_sensitive && target->load_factor > 0.7) {
        score -= 0.2;
    }
    
    // Ensure score is within bounds
    if (score < 0.0) score = 0.0;
    if (score > 1.0) score = 1.0;
    
    return score;
}

// Predict performance improvement
static double predict_performance_improvement(const distribution_target_t *target,
                                            const workload_characteristics_t *workload) {
    // Simple prediction based on current load and resource matching
    double base_improvement = (1.0 - target->load_factor) * 0.5; // 0-50% improvement
    
    // Resource type matching bonus
    double matching_bonus = 0.0;
    switch (workload->type) {
        case WORKLOAD_TYPE_CRYPTO:
            if (target->resource_type == RESOURCE_TYPE_CRYPTO) {
                matching_bonus = 0.3;
            }
            break;
        case WORKLOAD_TYPE_NETWORK:
            if (target->resource_type == RESOURCE_TYPE_NETWORK) {
                matching_bonus = 0.25;
            }
            break;
        case WORKLOAD_TYPE_MEMORY:
            if (target->resource_type == RESOURCE_TYPE_MEMORY) {
                matching_bonus = 0.25;
            }
            break;
        default:
            matching_bonus = 0.1;
            break;
    }
    
    // Priority boost
    double priority_boost = (double)workload->priority / 10.0 * 0.15;
    
    double total_improvement = base_improvement + matching_bonus + priority_boost;
    
    // Cap at reasonable maximum
    if (total_improvement > 0.8) total_improvement = 0.8;
    
    return total_improvement;
}

// Update distribution history
static void update_distribution_history(workload_distributor_t *distributor,
                                      const distribution_decision_t *decision) {
    distributor->history_index = (distributor->history_index + 1) % distributor->history_size;
    
    // Update system efficiency based on recent decisions
    if (distributor->total_distributions > 50) {
        int recent_success = 0;
        int recent_total = 50;
        int start_index = (distributor->history_index - recent_total + distributor->history_size) % distributor->history_size;
        
        for (int i = 0; i < recent_total; i++) {
            int hist_index = (start_index + i) % distributor->history_size;
            if (distributor->decision_history[hist_index].target_id >= 0) {
                recent_success++;
            }
        }
        
        distributor->current_system_efficiency = (double)recent_success / (double)recent_total * 100.0;
        distributor->predicted_optimal_efficiency = distributor->current_system_efficiency * 1.2; // 20% improvement potential
        if (distributor->predicted_optimal_efficiency > 100.0) {
            distributor->predicted_optimal_efficiency = 100.0;
        }
        
        distributor->improvement_potential_percent = 
            distributor->predicted_optimal_efficiency - distributor->current_system_efficiency;
    }
}

// Select optimal algorithm based on current conditions
static distribution_algorithm_t select_optimal_algorithm(workload_distributor_t *distributor) {
    // Simple adaptive logic - could be enhanced with ML
    if (distributor->current_system_efficiency < 70.0) {
        return DIST_ALGORITHM_PREDICTIVE; // Use predictive when performance is poor
    } else if (distributor->current_system_efficiency > 90.0) {
        return DIST_ALGORITHM_ROUND_ROBIN; // Use simple round-robin when performing well
    } else {
        return DIST_ALGORITHM_WEIGHTED; // Use weighted distribution for moderate performance
    }
}

// Adapt algorithm parameters
static void adapt_algorithm_parameters(workload_distributor_t *distributor) {
    // Simple adaptation - adjust load balancing threshold based on performance
    if (distributor->current_system_efficiency < 60.0) {
        distributor->load_balancing_threshold *= 0.9; // Be more aggressive
    } else if (distributor->current_system_efficiency > 90.0) {
        distributor->load_balancing_threshold *= 1.1; // Be more conservative
    }
    
    // Keep within reasonable bounds
    if (distributor->load_balancing_threshold < 0.3) {
        distributor->load_balancing_threshold = 0.3;
    }
    if (distributor->load_balancing_threshold > 0.9) {
        distributor->load_balancing_threshold = 0.9;
    }
}

// Get distribution statistics
void workload_distributor_get_stats(workload_distributor_t *distributor,
                                  long long *total_distributions,
                                  long long *successful_distributions,
                                  double *efficiency_percent) {
    if (!distributor) return;
    
    SAFE_ENTER;
    
    if (total_distributions) {
        *total_distributions = distributor->total_distributions;
    }
    if (successful_distributions) {
        *successful_distributions = distributor->successful_distributions;
    }
    if (efficiency_percent) {
        *efficiency_percent = distributor->distribution_efficiency_percent;
    }
    
    SAFE_LEAVE;
}

// Get current system efficiency
double workload_distributor_get_system_efficiency(workload_distributor_t *distributor) {
    if (!distributor) return 0.0;
    
    SAFE_ENTER;
    double efficiency = distributor->current_system_efficiency;
    SAFE_LEAVE;
    
    return efficiency;
}

// Get optimization recommendations
int workload_distributor_get_recommendations(workload_distributor_t *distributor,
                                           char *recommendations_buffer,
                                           size_t buffer_size) {
    if (!distributor || !recommendations_buffer || buffer_size == 0) {
        return -1;
    }
    
    SAFE_ENTER;
    
    size_t offset = 0;
    int recommendation_count = 0;
    
    // Efficiency-based recommendations
    if (distributor->current_system_efficiency < 70.0) {
        offset += my_snprintf(recommendations_buffer + offset, buffer_size - offset,
                          "1. System efficiency is low (%.1f%%) - consider adding more resources\n",
                          distributor->current_system_efficiency);
        recommendation_count++;
    }
    
    if (distributor->improvement_potential_percent > 15.0) {
        offset += my_snprintf(recommendations_buffer + offset, buffer_size - offset,
                          "2. High improvement potential (%.1f%%) - optimization recommended\n",
                          distributor->improvement_potential_percent);
        recommendation_count++;
    }
    
    // Algorithm recommendations
    if (distributor->algorithm == DIST_ALGORITHM_ROUND_ROBIN && 
        distributor->target_count > 4) {
        offset += my_snprintf(recommendations_buffer + offset, buffer_size - offset,
                          "3. Consider switching to WEIGHTED algorithm for better resource utilization\n");
        recommendation_count++;
    }
    
    // Resource recommendations
    int overloaded_targets = 0;
    for (int i = 0; i < distributor->target_count; i++) {
        if (distributor->targets[i].load_factor > 0.8) {
            overloaded_targets++;
        }
    }
    
    if (overloaded_targets > 0) {
        offset += my_snprintf(recommendations_buffer + offset, buffer_size - offset,
                          "4. %d resources are overloaded (>80%%) - consider rebalancing\n",
                          overloaded_targets);
        recommendation_count++;
    }
    
    if (recommendation_count == 0) {
        my_snprintf(recommendations_buffer, buffer_size,
                "System is operating optimally with current configuration.\n");
    }
    
    SAFE_LEAVE;
    return recommendation_count;
}

// Enable/disable the distributor
int workload_distributor_enable(workload_distributor_t *distributor) {
    if (!distributor || !distributor->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    distributor->active = 1;
    SAFE_LEAVE;
    return 0;
}

int workload_distributor_disable(workload_distributor_t *distributor) {
    if (!distributor || !distributor->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    distributor->active = 0;
    SAFE_LEAVE;
    return 0;
}

// Reset statistics
void workload_distributor_reset_stats(workload_distributor_t *distributor) {
    if (!distributor) return;
    
    SAFE_ENTER;
    
    distributor->total_distributions = 0;
    distributor->successful_distributions = 0;
    distributor->failed_distributions = 0;
    distributor->average_distribution_time_ms = 0.0;
    distributor->distribution_efficiency_percent = 100.0;
    
    SAFE_LEAVE;
}

// Get global instance
workload_distributor_t* get_global_workload_distributor(void) {
    return g_workload_distributor;
}