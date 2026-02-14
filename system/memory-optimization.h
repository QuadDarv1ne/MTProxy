/*
    This file is part of Mtproto-proxy Library.

    Mtproto-proxy Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Mtproto-proxy Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Mtproto-proxy Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2024-2026 MTProto Proxy Enhanced Project
              2024-2026 Dupley Maxim Igorevich (Maestro7IT)
*/

#ifndef __MEMORY_OPTIMIZATION_H__
#define __MEMORY_OPTIMIZATION_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
#define MAX_MEMORY_POOLS 16
#define POOL_EXPANSION_SIZE (1024 * 1024)  // 1MB
#define POOL_MIN_SIZE (256 * 1024)         // 256KB

/* Memory pool configuration */
typedef struct {
    int enabled;                /* Enable memory pooling */
    size_t max_pool_size;       /* Maximum total pool size */
    size_t min_block_size;      /* Minimum block size for pooling */
    size_t max_block_size;      /* Maximum block size for pooling */
    size_t alignment;           /* Memory alignment */
    double gc_threshold;        /* GC threshold (0.0 - 1.0) */
    int gc_interval_ms;         /* GC interval in milliseconds */
} memory_pool_config_t;

/* Memory pool structure */
typedef struct {
    size_t block_size;          /* Size of each block in pool */
    size_t block_count;         /* Total number of blocks */
    size_t free_blocks;         /* Number of free blocks */
    size_t pool_size;           /* Current pool size */
    
    void *pool_start;           /* Start of pool memory */
    void *pool_end;             /* End of pool memory */
    void *free_list;            /* Free block list */
    
    pthread_mutex_t pool_mutex; /* Pool mutex */
} memory_pool_t;

/* Memory manager structure */
typedef struct {
    memory_pool_t pools[MAX_MEMORY_POOLS];
    int pool_count;
    
    size_t total_allocated;
    size_t total_used;
    
    long start_time;
} memory_manager_t;

/* Memory statistics */
typedef struct {
    /* Pool statistics */
    uint64_t pool_allocations;
    uint64_t pool_frees;
    uint64_t pool_reallocations;
    uint64_t pool_allocated;
    
    /* System statistics */
    uint64_t system_allocations;
    uint64_t system_frees;
    uint64_t system_reallocations;
    uint64_t system_allocated;
    
    /* GC statistics */
    uint64_t gc_runs;
    uint64_t pool_expansions;
    uint64_t pool_shrinks;
    
    /* Advanced statistics */
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t memory_pressure_events;
    uint64_t allocation_failures;
    uint64_t fragmentation_events;
    double average_allocation_time;
    size_t peak_memory_usage;
    size_t current_memory_usage;
    
    /* Current state */
    size_t total_allocated;
    size_t total_used;
    double utilization;
    int pool_count;
    
    long last_gc_time;
} memory_stats_t;

/* Function prototypes */

/**
 * Initialize memory optimization system
 * @param config: Memory pool configuration (optional, uses defaults if NULL)
 * @return: 0 on success, -1 on error
 */
int init_memory_optimization(memory_pool_config_t *config);

/**
 * Initialize memory pools for different size classes
 * @return: 0 on success, -1 on error
 */
int init_memory_pools();

/**
 * Initialize single memory pool
 * @param pool: Pool to initialize
 * @param block_size: Size of blocks in pool
 * @return: 0 on success, -1 on error
 */
int init_memory_pool(memory_pool_t *pool, size_t block_size);

/**
 * Allocate memory with optimization
 * @param size: Size of memory to allocate
 * @return: Pointer to allocated memory or NULL on error
 */
void* optimized_malloc(size_t size);

/**
 * Free memory with optimization
 * @param ptr: Pointer to memory to free
 */
void optimized_free(void *ptr);

/**
 * Reallocate memory with optimization
 * @param ptr: Pointer to existing memory
 * @param size: New size
 * @return: Pointer to reallocated memory or NULL on error
 */
void* optimized_realloc(void *ptr, size_t size);

/**
 * Find appropriate memory pool for size
 * @param size: Requested size
 * @return: Pointer to appropriate pool or NULL
 */
memory_pool_t* find_appropriate_pool(size_t size);

/**
 * Find pool that contains pointer
 * @param ptr: Pointer to check
 * @return: Pool containing pointer or NULL
 */
memory_pool_t* find_pool_for_pointer(void *ptr);

/**
 * Allocate from pool
 * @param pool: Pool to allocate from
 * @return: Pointer to allocated block or NULL
 */
void* allocate_from_pool(memory_pool_t *pool);

/**
 * Return block to pool
 * @param pool: Pool to return to
 * @param ptr: Pointer to block
 * @return: 1 on success, 0 on error
 */
int return_to_pool(memory_pool_t *pool, void *ptr);

/**
 * Expand memory pool
 * @param pool: Pool to expand
 * @return: 1 on success, 0 on error
 */
int expand_pool(memory_pool_t *pool);

/**
 * Run garbage collection
 * @return: 1 if GC ran, 0 if skipped
 */
int run_memory_gc();

/**
 * Shrink memory pool
 * @param pool: Pool to shrink
 * @return: 1 on success, 0 on error
 */
int shrink_pool(memory_pool_t *pool);

/**
 * Get memory statistics
 * @return: Current memory statistics
 */
memory_stats_t get_memory_stats();

/**
 * Cleanup memory optimization system
 */
void cleanup_memory_optimization();

/**
 * Advanced memory management functions
 */

/**
 * Set memory pressure threshold
 * @param threshold: Memory usage percentage that triggers pressure handling (0.0 - 1.0)
 */
void set_memory_pressure_threshold(double threshold);

/**
 * Get current memory pressure level
 * @return: Current memory pressure level (0.0 - 1.0)
 */
double get_memory_pressure_level();

/**
 * Enable/disable memory pressure handling
 * @param enabled: 1 to enable, 0 to disable
 */
void set_memory_pressure_handling(int enabled);

/**
 * Perform memory defragmentation
 * @return: 1 on success, 0 on error
 */
int perform_memory_defragmentation();

/**
 * Get memory fragmentation level
 * @return: Fragmentation level (0.0 - 1.0)
 */
double get_memory_fragmentation_level();

/**
 * Set allocation tracking
 * @param enabled: 1 to enable, 0 to disable
 */
void set_allocation_tracking(int enabled);

/**
 * Print detailed memory statistics
 */
void print_detailed_memory_stats();

/**
 * Reset memory statistics
 */
void reset_memory_stats();

/* Macro overrides for standard malloc/free */
#define malloc(size) optimized_malloc(size)
#define free(ptr) optimized_free(ptr)
#define realloc(ptr, size) optimized_realloc(ptr, size)

#ifdef __cplusplus
}
#endif

#endif /* __MEMORY_OPTIMIZATION_H__ */