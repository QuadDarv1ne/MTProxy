/*
 * Memory Pool Optimizer Header for MTProxy
 * Provides memory pooling functionality for improved performance
 */

#ifndef MEMORY_POOL_OPTIMIZER_H
#define MEMORY_POOL_OPTIMIZER_H

#include <stddef.h>

// Memory pool configuration structure
typedef struct {
    int enable_pooling;
    size_t pool_size;
    size_t block_size;
} mem_pool_config_t;

// Memory block structure
typedef struct mem_block {
    void *data;
    size_t size;
    int is_free;
    struct mem_block *next;
} mem_block_t;

// Memory pool structure
typedef struct {
    mem_block_t *free_list;
    mem_block_t *used_list;
    size_t total_allocated;
    size_t total_used;
    int block_count;
    int free_count;
    mem_pool_config_t config;
} memory_pool_t;

// Function declarations
memory_pool_t* init_memory_pool(mem_pool_config_t *config);
void* pool_malloc(size_t size);
void pool_free(void *ptr);
void get_memory_pool_stats(long long *allocations, long long *deallocations, 
                          long long *pool_hits, long long *system_fallbacks,
                          long long *gc_runs, size_t *total_allocated, size_t *total_used);
void cleanup_memory_pool();
int init_global_memory_pool();

// Convenience macros
#define POOL_MALLOC(size) pool_malloc(size)
#define POOL_FREE(ptr) pool_free(ptr)

#endif // MEMORY_POOL_OPTIMIZER_H