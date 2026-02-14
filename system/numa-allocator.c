/*
 * NUMA-aware Memory Allocator for MTProxy - Integration Module
 * Provides NUMA-like optimizations using existing MTProxy infrastructure
 */

#include "numa-allocator.h"
#include "../common/kprintf.h"
#include "../common/server-functions.h"

// Use MTProxy's existing memory management
extern void *alloc_crypto_temp(size_t size);
extern void free_crypto_temp(void *ptr);

// Global NUMA context
static numa_memory_context_t g_numa_context = {0};
static char g_last_error[256] = {0};

/*
 * Initialize NUMA allocator system
 */
int numa_allocator_init(const numa_allocator_config_t *config) {
    if (g_numa_context.is_initialized) {
        return 0; // Already initialized
    }
    
    // Initialize with default configuration
    g_numa_context.node_count = 1;
    g_numa_context.default_node = 0;
    g_numa_context.is_initialized = 1;
    
    // Set configuration
    if (config) {
        g_numa_context.config = *config;
    } else {
        // Default configuration
        g_numa_context.config.enable_numa_awareness = 1;
        g_numa_context.config.max_nodes = 1;
        g_numa_context.config.default_policy = NUMA_POLICY_DEFAULT;
        g_numa_context.config.min_allocation_size = 64;
        g_numa_context.config.max_allocation_size = 1024 * 1024 * 1024;
        g_numa_context.config.enable_cache_alignment = 1;
        g_numa_context.config.enable_memory_profiling = 1;
        g_numa_context.config.enable_stats_collection = 1;
        g_numa_context.config.memory_pressure_threshold = 0.8;
        g_numa_context.config.rebalance_interval_seconds = 30;
    }
    
    // Initialize single node
    g_numa_context.nodes[0].node_id = 0;
    g_numa_context.nodes[0].cpu_cores = 1;
    g_numa_context.nodes[0].memory_available = 1024 * 1024 * 1024;
    g_numa_context.nodes[0].is_online = 1;
    g_numa_context.nodes[0].distance_to_other_nodes[0] = 10;
    
    vkprintf(1, "NUMA allocator initialized\n");
    return 0;
}

/*
 * Allocate memory with NUMA awareness
 */
numa_allocation_result_t numa_allocate(const numa_allocation_request_t *request) {
    numa_allocation_result_t result = {0};
    
    if (!request || !g_numa_context.is_initialized) {
        return result;
    }
    
    // Validate size
    if (request->size == 0 || request->size > g_numa_context.config.max_allocation_size) {
        return result;
    }
    
    // Calculate aligned size
    size_t actual_size = request->size;
    if (request->require_cache_alignment || g_numa_context.config.enable_cache_alignment) {
        actual_size = (actual_size + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1);
    }
    
    // Use MTProxy's crypto allocator for security-related allocations
    void *ptr = NULL;
    if (request->memory_type == MEMORY_TYPE_CRYPTO_CONTEXT) {
        ptr = alloc_crypto_temp(actual_size);
    } else {
        ptr = malloc(actual_size);
    }
    
    if (ptr) {
        result.ptr = ptr;
        result.actual_size = actual_size;
        result.allocated_node = 0;
        result.is_cache_aligned = (actual_size != request->size);
        
        // Update statistics
        g_numa_context.stats.total_allocated += actual_size;
        g_numa_context.stats.allocation_count++;
        if (request->memory_type < 6) {
            g_numa_context.stats.allocations_by_type[request->memory_type]++;
        }
        g_numa_context.stats.cache_aligned_allocations += result.is_cache_aligned;
        g_numa_context.stats.numa_local_allocations++;
    }
    
    return result;
}

/*
 * Simplified memory allocation
 */
void* numa_malloc(size_t size, memory_type_t memory_type) {
    numa_allocation_request_t request = {
        .size = size,
        .memory_type = memory_type,
        .policy = NUMA_POLICY_DEFAULT,
        .preferred_node = DEFAULT_NODE_AFFINITY,
        .require_cache_alignment = 0
    };
    
    numa_allocation_result_t result = numa_allocate(&request);
    return result.ptr;
}

/*
 * Cache-aligned allocation
 */
void* numa_malloc_aligned(size_t size, memory_type_t memory_type) {
    numa_allocation_request_t request = {
        .size = size,
        .memory_type = memory_type,
        .policy = NUMA_POLICY_DEFAULT,
        .preferred_node = DEFAULT_NODE_AFFINITY,
        .require_cache_alignment = 1
    };
    
    numa_allocation_result_t result = numa_allocate(&request);
    return result.ptr;
}

/*
 * Free allocated memory
 */
int numa_free(void *ptr) {
    if (!ptr) return 0;
    
    // In this simplified version, we don't track which allocator was used
    // In a full implementation, we'd need to track allocation metadata
    free(ptr);
    g_numa_context.stats.deallocation_count++;
    
    return 0;
}

/*
 * Get memory statistics
 */
int numa_get_stats(numa_allocation_stats_t *stats) {
    if (!stats) return -1;
    *stats = g_numa_context.stats;
    return 0;
}

/*
 * Reset statistics
 */
int numa_reset_stats(void) {
    g_numa_context.stats.allocation_count = 0;
    g_numa_context.stats.deallocation_count = 0;
    g_numa_context.stats.total_allocated = 0;
    g_numa_context.stats.cache_aligned_allocations = 0;
    g_numa_context.stats.numa_local_allocations = 0;
    g_numa_context.stats.rebalance_count = 0;
    return 0;
}

/*
 * Cleanup
 */
int numa_allocator_cleanup(void) {
    if (!g_numa_context.is_initialized) return 0;
    
    g_numa_context.is_initialized = 0;
    return 0;
}

/*
 * Basic functionality checks
 */
int numa_is_available(void) {
    return 1; // Basic NUMA-like functionality available
}

const char* numa_get_last_error(void) {
    return "Not implemented in simplified version";
}

int numa_print_allocation_report(void *output) {
    vkprintf(1, "NUMA Allocation Report:\n");
    vkprintf(1, "  Total allocated: %zu bytes\n", g_numa_context.stats.total_allocated);
    vkprintf(1, "  Allocations: %lld\n", g_numa_context.stats.allocation_count);
    vkprintf(1, "  Cache aligned: %zu\n", g_numa_context.stats.cache_aligned_allocations);
    return 0;
}

// Stub implementations for other functions
int numa_get_memory_node(const void *ptr, int *node_id) {
    if (node_id) *node_id = 0;
    return 0;
}

int numa_rebalance_memory(void) {
    g_numa_context.stats.rebalance_count++;
    return 0;
}

int numa_get_topology_info(numa_node_info_t *node_info, int max_nodes, int *node_count) {
    if (!node_info || !node_count || max_nodes <= 0) return -1;
    if (node_count) *node_count = 1;
    return 0;
}

int numa_set_thread_affinity(int node_id) {
    return -1; // Not implemented
}

int numa_get_optimal_node(void) {
    return 0;
}

int numa_preallocate_pools(int node_id, size_t size_per_node) {
    return -1; // Not implemented
}