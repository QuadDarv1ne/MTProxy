/*
 * Intelligent Workload Distributor for MTProxy
 * Automatically distributes workloads across available resources
 * for optimal performance and resource utilization
 */

#ifndef _INTELLIGENT_WORKLOAD_DISTRIBUTOR_H_
#define _INTELLIGENT_WORKLOAD_DISTRIBUTOR_H_

#include <stdint.h>
#include <stddef.h>

// Workload types
typedef enum {
    WORKLOAD_TYPE_UNKNOWN = 0,
    WORKLOAD_TYPE_LIGHT_PROXY = 1,    // Light proxy connections
    WORKLOAD_TYPE_HEAVY_PROXY = 2,    // Heavy proxy connections
    WORKLOAD_TYPE_CRYPTO = 3,         // Cryptographic operations
    WORKLOAD_TYPE_NETWORK = 4,        // Network I/O operations
    WORKLOAD_TYPE_MEMORY = 5,         // Memory operations
    WORKLOAD_TYPE_DATABASE = 6,       // Database operations
    WORKLOAD_TYPE_MISC = 7            // Miscellaneous operations
} workload_type_t;

// Distribution algorithms
typedef enum {
    DIST_ALGORITHM_ROUND_ROBIN = 0,
    DIST_ALGORITHM_WEIGHTED = 1,
    DIST_ALGORITHM_LEAST_LOADED = 2,
    DIST_ALGORITHM_ADAPTIVE = 3,
    DIST_ALGORITHM_PREDICTIVE = 4
} distribution_algorithm_t;

// Resource types
typedef enum {
    RESOURCE_TYPE_CPU = 0,
    RESOURCE_TYPE_MEMORY = 1,
    RESOURCE_TYPE_NETWORK = 2,
    RESOURCE_TYPE_STORAGE = 3,
    RESOURCE_TYPE_CRYPTO = 4
} resource_type_t;

// Resource metrics structure
typedef struct {
    double cpu_usage_percent;
    double memory_usage_percent;
    double network_bandwidth_utilization;
    double storage_io_utilization;
    double crypto_accelerator_usage;
    long long available_bandwidth_bytes;
    long long available_memory_bytes;
    int cpu_cores_available;
    int thread_capacity;
} resource_metrics_t;

// Workload characteristics
typedef struct {
    workload_type_t type;
    int priority;                    // 1-10, higher is more important
    size_t data_size_bytes;
    double expected_duration_ms;
    double resource_intensity_cpu;
    double resource_intensity_memory;
    double resource_intensity_network;
    int parallelizable;              // Can be split across resources
    int latency_sensitive;           // Requires low latency
} workload_characteristics_t;

// Distribution target
typedef struct {
    int target_id;                   // Resource identifier
    resource_type_t resource_type;
    double load_factor;              // Current load 0.0-1.0
    resource_metrics_t metrics;
    int active_workloads;
    double efficiency_score;         // Performance efficiency 0.0-1.0
} distribution_target_t;

// Distribution decision
typedef struct {
    int target_id;
    double suitability_score;        // 0.0-1.0, higher is better
    double predicted_performance;    // Expected performance improvement
    double resource_utilization_impact;
    char reason[256];                // Reason for this decision
} distribution_decision_t;

// Workload distribution context
typedef struct {
    // Configuration
    distribution_algorithm_t algorithm;
    int max_targets;
    int enable_prediction;
    double load_balancing_threshold;
    
    // Targets
    distribution_target_t *targets;
    int target_count;
    
    // Statistics
    long long total_distributions;
    long long successful_distributions;
    long long failed_distributions;
    double average_distribution_time_ms;
    double distribution_efficiency_percent;
    
    // History for learning
    distribution_decision_t *decision_history;
    int history_size;
    int history_index;
    
    // Performance metrics
    double current_system_efficiency;
    double predicted_optimal_efficiency;
    double improvement_potential_percent;
    
    // State
    int initialized;
    int active;
} workload_distributor_t;

// Configuration structure
typedef struct {
    distribution_algorithm_t algorithm;
    int max_targets;
    int history_buffer_size;
    double load_threshold_high;
    double load_threshold_low;
    int enable_adaptive_learning;
    double learning_rate;
    int prediction_window_seconds;
} workload_distributor_config_t;

// Initialize the workload distributor
int workload_distributor_init(workload_distributor_t *distributor, 
                            const workload_distributor_config_t *config);

// Cleanup the workload distributor
void workload_distributor_cleanup(workload_distributor_t *distributor);

// Add a resource target
int workload_distributor_add_target(workload_distributor_t *distributor,
                                  int target_id,
                                  resource_type_t resource_type,
                                  const resource_metrics_t *metrics);

// Remove a resource target
int workload_distributor_remove_target(workload_distributor_t *distributor,
                                     int target_id);

// Update target metrics
int workload_distributor_update_target_metrics(workload_distributor_t *distributor,
                                             int target_id,
                                             const resource_metrics_t *metrics);

// Distribute workload
distribution_decision_t* workload_distributor_distribute(workload_distributor_t *distributor,
                                                       const workload_characteristics_t *workload);

// Get distribution statistics
void workload_distributor_get_stats(workload_distributor_t *distributor,
                                  long long *total_distributions,
                                  long long *successful_distributions,
                                  double *efficiency_percent);

// Get current system efficiency
double workload_distributor_get_system_efficiency(workload_distributor_t *distributor);

// Get optimization recommendations
int workload_distributor_get_recommendations(workload_distributor_t *distributor,
                                           char *recommendations_buffer,
                                           size_t buffer_size);

// Enable/disable the distributor
int workload_distributor_enable(workload_distributor_t *distributor);
int workload_distributor_disable(workload_distributor_t *distributor);

// Reset statistics
void workload_distributor_reset_stats(workload_distributor_t *distributor);

// Get global instance
workload_distributor_t* get_global_workload_distributor(void);

#endif // _INTELLIGENT_WORKLOAD_DISTRIBUTOR_H_