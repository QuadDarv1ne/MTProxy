/*
 * NUMA-aware Memory Allocator for MTProxy
 * Optimizes memory access patterns by binding buffers to NUMA nodes
 */

#ifndef NUMA_ALLOCATOR_H
#define NUMA_ALLOCATOR_H

#include <stdint.h>
#include <stddef.h>

// NUMA configuration constants
#define MAX_NUMA_NODES 8
#define DEFAULT_NODE_AFFINITY -1  // Use default NUMA node
#define CACHE_LINE_SIZE 64

// Memory allocation policies
typedef enum {
    NUMA_POLICY_DEFAULT = 0,      // System default allocation
    NUMA_POLICY_LOCAL = 1,        // Allocate on local node
    NUMA_POLICY_INTERLEAVE = 2,   // Interleave across nodes
    NUMA_POLICY_BIND = 3,         // Bind to specific node
    NUMA_POLICY_PREFERRED = 4     // Prefer specific node
} numa_policy_t;

// Memory types for different use cases
typedef enum {
    MEMORY_TYPE_NETWORK_BUFFER = 0,    // Network I/O buffers
    MEMORY_TYPE_CRYPTO_CONTEXT = 1,    // Cryptographic contexts
    MEMORY_TYPE_CONNECTION_POOL = 2,   // Connection pool memory
    MEMORY_TYPE_CACHE_STORAGE = 3,     // Cache storage
    MEMORY_TYPE_TEMPORARY = 4,         // Temporary allocations
    MEMORY_TYPE_MONITORING = 5         // Monitoring data
} memory_type_t;

// NUMA node information
typedef struct numa_node_info {
    int node_id;
    int cpu_cores;
    size_t memory_available;
    size_t memory_used;
    double memory_utilization;
    int is_online;
    int distance_to_other_nodes[MAX_NUMA_NODES];
} numa_node_info_t;

// Memory allocation statistics
typedef struct numa_allocation_stats {
    size_t total_allocated;
    size_t allocations_by_type[6];      // For each memory_type_t
    size_t allocations_by_policy[5];    // For each numa_policy_t
    size_t cache_aligned_allocations;
    size_t numa_local_allocations;
    size_t numa_remote_allocations;
    long long allocation_count;
    long long deallocation_count;
    long long rebalance_count;          // Add missing field
    double avg_allocation_time_us;
    double avg_deallocation_time_us;
} numa_allocation_stats_t;

// NUMA allocator configuration
typedef struct numa_allocator_config {
    int enable_numa_awareness;
    int max_nodes;
    numa_policy_t default_policy;
    size_t min_allocation_size;
    size_t max_allocation_size;
    int enable_cache_alignment;
    int enable_memory_profiling;
    int enable_stats_collection;
    double memory_pressure_threshold;
    int rebalance_interval_seconds;
} numa_allocator_config_t;

// NUMA memory allocation context
typedef struct numa_memory_context {
    numa_allocator_config_t config;
    numa_node_info_t nodes[MAX_NUMA_NODES];
    numa_allocation_stats_t stats;
    int node_count;
    int default_node;
    void *memory_pools[MAX_NUMA_NODES];
    size_t pool_sizes[MAX_NUMA_NODES];
    int is_initialized;
    long long init_timestamp;
} numa_memory_context_t;

// Memory allocation request
typedef struct numa_allocation_request {
    size_t size;
    memory_type_t memory_type;
    numa_policy_t policy;
    int preferred_node;
    int require_cache_alignment;
    const char *debug_info;  // For debugging purposes
} numa_allocation_request_t;

// Memory allocation result
typedef struct numa_allocation_result {
    void *ptr;
    size_t actual_size;
    int allocated_node;
    int is_cache_aligned;
    long long allocation_time_us;
    const char *error_message;
} numa_allocation_result_t;

// NUMA allocator interface functions

/*
 * Initialize NUMA allocator system
 * @param config: Configuration parameters for the allocator
 * @return: 0 on success, -1 on error
 */
int numa_allocator_init(const numa_allocator_config_t *config);

/*
 * Allocate memory with NUMA awareness
 * @param request: Allocation request parameters
 * @return: Allocation result structure
 */
numa_allocation_result_t numa_allocate(const numa_allocation_request_t *request);

/*
 * Allocate memory with simplified parameters
 * @param size: Size of memory to allocate
 * @param memory_type: Type of memory for policy selection
 * @return: Pointer to allocated memory or NULL on failure
 */
void* numa_malloc(size_t size, memory_type_t memory_type);

/*
 * Allocate cache-aligned memory
 * @param size: Size of memory to allocate
 * @param memory_type: Type of memory for policy selection
 * @return: Pointer to cache-aligned memory or NULL on failure
 */
void* numa_malloc_aligned(size_t size, memory_type_t memory_type);

/*
 * Free NUMA-aware allocated memory
 * @param ptr: Pointer to memory to free
 * @return: 0 on success, -1 on error
 */
int numa_free(void *ptr);

/*
 * Get NUMA node information for a memory pointer
 * @param ptr: Memory pointer to query
 * @param node_id: Output parameter for node ID
 * @return: 0 on success, -1 on error
 */
int numa_get_memory_node(const void *ptr, int *node_id);

/*
 * Rebalance memory allocation across NUMA nodes
 * @return: 0 on success, -1 on error
 */
int numa_rebalance_memory(void);

/*
 * Get current NUMA allocation statistics
 * @param stats: Output parameter for statistics
 * @return: 0 on success, -1 on error
 */
int numa_get_stats(numa_allocation_stats_t *stats);

/*
 * Reset allocation statistics
 * @return: 0 on success, -1 on error
 */
int numa_reset_stats(void);

/*
 * Get NUMA topology information
 * @param node_info: Array to fill with node information
 * @param max_nodes: Maximum number of nodes to query
 * @param node_count: Output parameter for actual node count
 * @return: 0 on success, -1 on error
 */
int numa_get_topology_info(numa_node_info_t *node_info, int max_nodes, int *node_count);

/*
 * Set CPU affinity for current thread to specific NUMA node
 * @param node_id: Target NUMA node (-1 for current node)
 * @return: 0 on success, -1 on error
 */
int numa_set_thread_affinity(int node_id);

/*
 * Get optimal NUMA node for current thread
 * @return: Node ID or -1 on error
 */
int numa_get_optimal_node(void);

/*
 * Pre-allocate memory pools for specific NUMA nodes
 * @param node_id: Target node (-1 for all nodes)
 * @param size_per_node: Size to allocate per node
 * @return: 0 on success, -1 on error
 */
int numa_preallocate_pools(int node_id, size_t size_per_node);

/*
 * Cleanup and shutdown NUMA allocator
 * @return: 0 on success, -1 on error
 */
int numa_allocator_cleanup(void);

// Utility functions

/*
 * Check if NUMA is available on this system
 * @return: 1 if NUMA available, 0 if not
 */
int numa_is_available(void);

/*
 * Get human-readable error message for last error
 * @return: Error message string
 */
const char* numa_get_last_error(void);

/*
 * Print detailed NUMA allocation report
 * @param output: Output stream or file descriptor
 * @return: 0 on success, -1 on error
 */
int numa_print_allocation_report(void *output);

#endif // NUMA_ALLOCATOR_H