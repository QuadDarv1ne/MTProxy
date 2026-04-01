/*
 * NUMA-aware Memory Allocator Header for MTProxy
 * Provides NUMA topology awareness and intelligent memory allocation
 */

#ifndef NUMA_AWARE_ALLOCATOR_H
#define NUMA_AWARE_ALLOCATOR_H

#include <stddef.h>

// NUMA allocation policy
typedef enum {
    NUMA_POLICY_LOCAL = 0,      // Allocate on local node
    NUMA_POLICY_INTERLEAVE = 1, // Interleave across nodes
    NUMA_POLICY_PREFERRED = 2,  // Preferred node with fallback
    NUMA_POLICY_BIND = 3,       // Bind to specific node
    NUMA_POLICY_DEFAULT = 4     // System default
} numa_policy_t;

// NUMA allocator context
typedef struct {
    int max_nodes;
    int current_node;
    void *nodes; // In real implementation, this would be numa_node_info_t*
    numa_policy_t default_policy;
    size_t page_size;
    int numa_available;
    // Statistics
    long long local_allocations;
    long long remote_allocations;
    long long interleaved_allocations;
    long long allocation_failures;
    long long memory_migrations;
    long long policy_switches;
} numa_allocator_t;

// Function declarations
numa_allocator_t* numa_allocator_init(void);
void* numa_malloc(size_t size, numa_policy_t policy);
void numa_free(void *ptr, size_t size);
int numa_set_policy(numa_policy_t policy);
void numa_get_stats(long long *local_allocs, long long *remote_allocs, 
                   long long *interleaved_allocs, long long *failures);
int numa_migrate_memory(void *ptr, size_t size, int target_node);
void numa_allocator_cleanup(void);
int init_global_numa_allocator(void);

// Convenience functions
void* numa_local_malloc(size_t size);
void* numa_interleaved_malloc(size_t size);
void* numa_preferred_malloc(size_t size);

// Convenience macros
#define NUMA_MALLOC(size) numa_malloc(size, NUMA_POLICY_LOCAL)
#define NUMA_FREE(ptr, size) numa_free(ptr, size)
#define NUMA_SET_POLICY(policy) numa_set_policy(policy)

#endif // NUMA_AWARE_ALLOCATOR_H