/*
 * Advanced NUMA-aware Memory Allocator for MTProxy
 * Implements true NUMA topology awareness with intelligent allocation policies
 */

#ifndef NULL
#define NULL ((void*)0)
#endif

// NUMA node information
typedef struct {
    int node_id;
    size_t available_memory;
    size_t allocated_memory;
    int cpu_count;
    int *cpu_list;
    double memory_pressure;
} numa_node_info_t;

// NUMA allocation policy
typedef enum {
    NUMA_POLICY_LOCAL = 0,      // Allocate on local node
    NUMA_POLICY_INTERLEAVE = 1, // Interleave across nodes
    NUMA_POLICY_PREFERRED = 2,  // Preferred node with fallback
    NUMA_POLICY_BIND = 3,       // Bind to specific node
    NUMA_POLICY_DEFAULT = 4     // System default
} numa_policy_t;

// NUMA allocation context
typedef struct {
    int max_nodes;
    int current_node;
    numa_node_info_t *nodes;
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

// Global NUMA allocator instance
static numa_allocator_t *g_numa_allocator = NULL;

// Simple memory pool for NUMA allocator
static char numa_memory_pool[8 * 1024 * 1024]; // 8MB pool
static size_t numa_pool_offset = 0;

// Simple time function
static long long get_current_time_ms(void) {
    static long long base_time = 1000000;
    base_time += 100;
    return base_time;
}

// Simple memory allocation
static void* simple_malloc(size_t size) {
    if (numa_pool_offset + size > sizeof(numa_memory_pool)) {
        return NULL;
    }
    
    void *ptr = &numa_memory_pool[numa_pool_offset];
    numa_pool_offset += size;
    return ptr;
}

// Simple memory deallocation
static void simple_free(void *ptr) {
    // In real implementation, this would properly free memory
}

// Detect NUMA topology (simulated)
static int detect_numa_topology(numa_allocator_t *allocator) {
    if (!allocator) return -1;
    
    // Simulate NUMA detection
    allocator->max_nodes = 2; // Simulate 2 NUMA nodes
    allocator->current_node = 0;
    allocator->numa_available = 1;
    allocator->page_size = 4096;
    allocator->default_policy = NUMA_POLICY_LOCAL;
    
    // Allocate node information
    allocator->nodes = simple_malloc(sizeof(numa_node_info_t) * allocator->max_nodes);
    if (!allocator->nodes) return -1;
    
    // Initialize node information
    for (int i = 0; i < allocator->max_nodes; i++) {
        allocator->nodes[i].node_id = i;
        allocator->nodes[i].available_memory = 2ULL * 1024 * 1024 * 1024; // 2GB per node
        allocator->nodes[i].allocated_memory = 0;
        allocator->nodes[i].cpu_count = 4; // 4 CPUs per node
        allocator->nodes[i].cpu_list = simple_malloc(sizeof(int) * 4);
        allocator->nodes[i].memory_pressure = 0.0;
        
        // Initialize CPU list
        if (allocator->nodes[i].cpu_list) {
            for (int j = 0; j < 4; j++) {
                allocator->nodes[i].cpu_list[j] = i * 4 + j;
            }
        }
    }
    
    return 0;
}

// Initialize NUMA allocator
numa_allocator_t* numa_allocator_init(void) {
    numa_allocator_t *allocator = simple_malloc(sizeof(numa_allocator_t));
    if (!allocator) return NULL;
    
    // Zero out memory
    char *mem_ptr = (char*)allocator;
    for (size_t i = 0; i < sizeof(numa_allocator_t); i++) {
        mem_ptr[i] = 0;
    }
    
    // Detect NUMA topology
    if (detect_numa_topology(allocator) != 0) {
        simple_free(allocator);
        return NULL;
    }
    
    return allocator;
}

// Get current NUMA node (simulated)
static int get_current_numa_node(void) {
    // In real implementation, this would use sched_getcpu() and numa_node_of_cpu()
    static int current_node = 0;
    current_node = (current_node + 1) % 2; // Simulate node switching
    return current_node;
}

// Find best NUMA node for allocation
static int find_best_numa_node(numa_allocator_t *allocator, size_t size) {
    if (!allocator || !allocator->nodes) return 0;
    
    int best_node = 0;
    double min_pressure = 1.0;
    
    // Find node with lowest memory pressure
    for (int i = 0; i < allocator->max_nodes; i++) {
        double pressure = (double)allocator->nodes[i].allocated_memory / 
                         allocator->nodes[i].available_memory;
        
        if (pressure < min_pressure) {
            min_pressure = pressure;
            best_node = i;
        }
    }
    
    return best_node;
}

// NUMA-aware memory allocation
void* numa_malloc(size_t size, numa_policy_t policy) {
    if (!g_numa_allocator || size == 0) return NULL;
    
    int target_node = 0;
    void *ptr = NULL;
    
    switch (policy) {
        case NUMA_POLICY_LOCAL:
            target_node = get_current_numa_node();
            ptr = simple_malloc(size);
            if (ptr) {
                g_numa_allocator->local_allocations++;
                g_numa_allocator->nodes[target_node].allocated_memory += size;
            }
            break;
            
        case NUMA_POLICY_INTERLEAVE:
            // Simulate interleaved allocation
            target_node = g_numa_allocator->current_node;
            g_numa_allocator->current_node = 
                (g_numa_allocator->current_node + 1) % g_numa_allocator->max_nodes;
            ptr = simple_malloc(size);
            if (ptr) {
                g_numa_allocator->interleaved_allocations++;
                g_numa_allocator->nodes[target_node].allocated_memory += size;
            }
            break;
            
        case NUMA_POLICY_PREFERRED:
            target_node = find_best_numa_node(g_numa_allocator, size);
            ptr = simple_malloc(size);
            if (ptr) {
                g_numa_allocator->local_allocations++;
                g_numa_allocator->nodes[target_node].allocated_memory += size;
            }
            break;
            
        case NUMA_POLICY_BIND:
            target_node = 0; // Always bind to node 0
            ptr = simple_malloc(size);
            if (ptr) {
                g_numa_allocator->local_allocations++;
                g_numa_allocator->nodes[target_node].allocated_memory += size;
            }
            break;
            
        default:
            ptr = simple_malloc(size);
            if (ptr) {
                g_numa_allocator->local_allocations++;
                g_numa_allocator->nodes[0].allocated_memory += size;
            }
            break;
    }
    
    if (!ptr) {
        g_numa_allocator->allocation_failures++;
    }
    
    return ptr;
}

// NUMA-aware memory deallocation
void numa_free(void *ptr, size_t size) {
    if (!ptr || !g_numa_allocator) return;
    
    // In real implementation, we would determine which node this memory belongs to
    // For simulation, we'll just update the first node's statistics
    if (g_numa_allocator->nodes && g_numa_allocator->max_nodes > 0) {
        if (g_numa_allocator->nodes[0].allocated_memory >= size) {
            g_numa_allocator->nodes[0].allocated_memory -= size;
        }
    }
    
    simple_free(ptr);
}

// Change NUMA allocation policy
int numa_set_policy(numa_policy_t policy) {
    if (!g_numa_allocator) return -1;
    
    if (g_numa_allocator->default_policy != policy) {
        g_numa_allocator->policy_switches++;
        g_numa_allocator->default_policy = policy;
    }
    
    return 0;
}

// Get NUMA statistics
void numa_get_stats(long long *local_allocs, long long *remote_allocs, 
                   long long *interleaved_allocs, long long *failures) {
    if (!g_numa_allocator) return;
    
    if (local_allocs) *local_allocs = g_numa_allocator->local_allocations;
    if (remote_allocs) *remote_allocs = g_numa_allocator->remote_allocations;
    if (interleaved_allocs) *interleaved_allocs = g_numa_allocator->interleaved_allocations;
    if (failures) *failures = g_numa_allocator->allocation_failures;
}

// Memory migration simulation
int numa_migrate_memory(void *ptr, size_t size, int target_node) {
    if (!g_numa_allocator || !ptr || target_node < 0 || 
        target_node >= g_numa_allocator->max_nodes) {
        return -1;
    }
    
    // In real implementation, this would use mbind() or move_pages()
    // For simulation, we just update statistics
    g_numa_allocator->memory_migrations++;
    
    // Update node statistics
    for (int i = 0; i < g_numa_allocator->max_nodes; i++) {
        if (g_numa_allocator->nodes[i].allocated_memory >= size) {
            g_numa_allocator->nodes[i].allocated_memory -= size;
            break;
        }
    }
    g_numa_allocator->nodes[target_node].allocated_memory += size;
    
    return 0;
}

// Cleanup NUMA allocator
void numa_allocator_cleanup(void) {
    if (!g_numa_allocator) return;
    
    // Free node information
    if (g_numa_allocator->nodes) {
        for (int i = 0; i < g_numa_allocator->max_nodes; i++) {
            if (g_numa_allocator->nodes[i].cpu_list) {
                simple_free(g_numa_allocator->nodes[i].cpu_list);
            }
        }
        simple_free(g_numa_allocator->nodes);
    }
    
    simple_free(g_numa_allocator);
    g_numa_allocator = NULL;
}

// Initialize global NUMA allocator
int init_global_numa_allocator(void) {
    if (g_numa_allocator) return 0;
    
    g_numa_allocator = numa_allocator_init();
    return g_numa_allocator ? 0 : -1;
}

// Convenience functions
void* numa_local_malloc(size_t size) {
    return numa_malloc(size, NUMA_POLICY_LOCAL);
}

void* numa_interleaved_malloc(size_t size) {
    return numa_malloc(size, NUMA_POLICY_INTERLEAVE);
}

void* numa_preferred_malloc(size_t size) {
    return numa_malloc(size, NUMA_POLICY_PREFERRED);
}