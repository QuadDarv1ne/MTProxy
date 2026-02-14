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

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#include "memory-optimization.h"

// Memory pool configuration
static memory_pool_config_t global_pool_config = {
    .enabled = 1,
    .max_pool_size = 1024 * 1024 * 1024,  // 1GB
    .min_block_size = 64,
    .max_block_size = 65536,  // 64KB
    .alignment = 16,
    .gc_threshold = 0.7,  // 70% utilization triggers GC
    .gc_interval_ms = 5000  // GC every 5 seconds
};

// Global memory manager
static memory_manager_t *global_memory_manager = NULL;
static pthread_mutex_t manager_mutex = PTHREAD_MUTEX_INITIALIZER;
static int memory_manager_initialized = 0;

// Memory statistics
static memory_stats_t memory_stats = {0};

// Initialize memory optimization system
int init_memory_optimization(memory_pool_config_t *config) {
    if (memory_manager_initialized) {
        return 0; // Already initialized
    }

    pthread_mutex_lock(&manager_mutex);
    
    // Apply configuration
    if (config) {
        global_pool_config = *config;
    }
    
    // Initialize memory manager
    global_memory_manager = calloc(1, sizeof(memory_manager_t));
    if (!global_memory_manager) {
        pthread_mutex_unlock(&manager_mutex);
        return -1;
    }
    
    global_memory_manager->pool_count = 0;
    global_memory_manager->total_allocated = 0;
    global_memory_manager->total_used = 0;
    global_memory_manager->start_time = time(NULL);
    
    // Initialize memory pools for different size classes
    init_memory_pools();
    
    memory_manager_initialized = 1;
    pthread_mutex_unlock(&manager_mutex);
    
    return 0;
}

// Initialize memory pools for different size classes
int init_memory_pools() {
    if (!global_memory_manager) {
        return -1;
    }
    
    // Create pools for different size classes (powers of 2)
    size_t sizes[] = {64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
    int size_count = sizeof(sizes) / sizeof(sizes[0]);
    
    for (int i = 0; i < size_count && global_memory_manager->pool_count < MAX_MEMORY_POOLS; i++) {
        if (sizes[i] >= global_pool_config.min_block_size && 
            sizes[i] <= global_pool_config.max_block_size) {
            
            memory_pool_t *pool = &global_memory_manager->pools[global_memory_manager->pool_count];
            init_memory_pool(pool, sizes[i]);
            global_memory_manager->pool_count++;
        }
    }
    
    return 0;
}

// Initialize single memory pool
int init_memory_pool(memory_pool_t *pool, size_t block_size) {
    if (!pool) {
        return -1;
    }
    
    memset(pool, 0, sizeof(memory_pool_t));
    pool->block_size = block_size;
    pool->block_count = 0;
    pool->free_blocks = 0;
    pool->pool_size = 0;
    
    pthread_mutex_init(&pool->pool_mutex, NULL);
    
    return 0;
}

// Allocate memory with optimization
void* optimized_malloc(size_t size) {
    if (!memory_manager_initialized || !global_pool_config.enabled) {
        memory_stats.system_allocations++;
        return malloc(size);
    }
    
    // Find appropriate pool
    memory_pool_t *pool = find_appropriate_pool(size);
    if (!pool) {
        // Fall back to system malloc for large allocations
        memory_stats.system_allocations++;
        void *ptr = malloc(size);
        if (ptr) {
            memory_stats.system_allocated += size;
        }
        return ptr;
    }
    
    pthread_mutex_lock(&pool->pool_mutex);
    
    void *ptr = allocate_from_pool(pool);
    if (ptr) {
        memory_stats.pool_allocations++;
        memory_stats.pool_allocated += pool->block_size;
        pool->free_blocks--;
    } else {
        // Pool exhausted, try to expand
        if (expand_pool(pool)) {
            ptr = allocate_from_pool(pool);
            if (ptr) {
                memory_stats.pool_allocations++;
                memory_stats.pool_allocated += pool->block_size;
                pool->free_blocks--;
            }
        }
    }
    
    pthread_mutex_unlock(&pool->pool_mutex);
    
    return ptr;
}

// Free memory with optimization
void optimized_free(void *ptr) {
    if (!ptr) {
        return;
    }
    
    if (!memory_manager_initialized || !global_pool_config.enabled) {
        memory_stats.system_frees++;
        free(ptr);
        return;
    }
    
    // Try to return to pool
    memory_pool_t *pool = find_pool_for_pointer(ptr);
    if (pool) {
        pthread_mutex_lock(&pool->pool_mutex);
        if (return_to_pool(pool, ptr)) {
            memory_stats.pool_frees++;
            memory_stats.pool_allocated -= pool->block_size;
            pool->free_blocks++;
        } else {
            // Not a pool pointer, fall back to system free
            memory_stats.system_frees++;
            free(ptr);
        }
        pthread_mutex_unlock(&pool->pool_mutex);
    } else {
        // Not a pool pointer, fall back to system free
        memory_stats.system_frees++;
        free(ptr);
    }
}

// Reallocate memory with optimization
void* optimized_realloc(void *ptr, size_t size) {
    if (!ptr) {
        return optimized_malloc(size);
    }
    
    if (size == 0) {
        optimized_free(ptr);
        return NULL;
    }
    
    if (!memory_manager_initialized || !global_pool_config.enabled) {
        memory_stats.system_reallocations++;
        return realloc(ptr, size);
    }
    
    // Check if we can reuse the existing allocation
    memory_pool_t *old_pool = find_pool_for_pointer(ptr);
    memory_pool_t *new_pool = find_appropriate_pool(size);
    
    if (old_pool == new_pool) {
        // Same pool, no reallocation needed
        return ptr;
    }
    
    if (!new_pool) {
        // New size requires system allocation
        memory_stats.system_reallocations++;
        void *new_ptr = malloc(size);
        if (new_ptr) {
            size_t copy_size = (old_pool) ? old_pool->block_size : size;
            memcpy(new_ptr, ptr, copy_size < size ? copy_size : size);
            optimized_free(ptr);
        }
        return new_ptr;
    }
    
    // Allocate from new pool and copy data
    void *new_ptr = optimized_malloc(size);
    if (new_ptr) {
        size_t copy_size = (old_pool) ? old_pool->block_size : size;
        memcpy(new_ptr, ptr, copy_size < size ? copy_size : size);
        optimized_free(ptr);
        memory_stats.pool_reallocations++;
    }
    
    return new_ptr;
}

// Find appropriate memory pool for size
memory_pool_t* find_appropriate_pool(size_t size) {
    if (!global_memory_manager) {
        return NULL;
    }
    
    // Round up to next power of 2
    size_t aligned_size = global_pool_config.min_block_size;
    while (aligned_size < size && aligned_size < global_pool_config.max_block_size) {
        aligned_size <<= 1;
    }
    
    if (aligned_size > global_pool_config.max_block_size) {
        return NULL; // Too large for pooling
    }
    
    // Find pool with matching block size
    for (int i = 0; i < global_memory_manager->pool_count; i++) {
        if (global_memory_manager->pools[i].block_size == aligned_size) {
            return &global_memory_manager->pools[i];
        }
    }
    
    return NULL;
}

// Find pool that contains pointer
memory_pool_t* find_pool_for_pointer(void *ptr) {
    if (!global_memory_manager || !ptr) {
        return NULL;
    }
    
    // In a real implementation, we would need to track which pool
    // each allocated block belongs to. This is a simplified version.
    for (int i = 0; i < global_memory_manager->pool_count; i++) {
        memory_pool_t *pool = &global_memory_manager->pools[i];
        // Check if ptr falls within pool's memory range
        // This is simplified - real implementation would need proper tracking
        if (pool->pool_start && pool->pool_end) {
            if ((char*)ptr >= (char*)pool->pool_start && 
                (char*)ptr < (char*)pool->pool_end) {
                return pool;
            }
        }
    }
    
    return NULL;
}

// Allocate from pool
void* allocate_from_pool(memory_pool_t *pool) {
    if (!pool || pool->free_blocks == 0) {
        return NULL;
    }
    
    // Simple free list implementation
    if (pool->free_list) {
        void *ptr = pool->free_list;
        pool->free_list = *(void**)ptr;  // Next pointer stored at start of block
        return ptr;
    }
    
    return NULL;
}

// Return block to pool
int return_to_pool(memory_pool_t *pool, void *ptr) {
    if (!pool || !ptr) {
        return 0;
    }
    
    // Add to free list
    *(void**)ptr = pool->free_list;
    pool->free_list = ptr;
    
    return 1;
}

// Expand memory pool
int expand_pool(memory_pool_t *pool) {
    if (!pool) {
        return 0;
    }
    
    // Check if we can expand within limits
    size_t new_pool_size = pool->pool_size + POOL_EXPANSION_SIZE;
    if (new_pool_size > global_pool_config.max_pool_size) {
        return 0;
    }
    
    // Allocate new memory
    void *new_memory = mmap(NULL, POOL_EXPANSION_SIZE, 
                           PROT_READ | PROT_WRITE, 
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (new_memory == MAP_FAILED) {
        return 0;
    }
    
    // Add new blocks to free list
    size_t blocks_added = POOL_EXPANSION_SIZE / pool->block_size;
    char *block_ptr = (char*)new_memory;
    
    for (size_t i = 0; i < blocks_added; i++) {
        *(void**)block_ptr = pool->free_list;
        pool->free_list = block_ptr;
        block_ptr += pool->block_size;
    }
    
    // Update pool metadata
    if (!pool->pool_start) {
        pool->pool_start = new_memory;
    }
    pool->pool_end = (char*)new_memory + POOL_EXPANSION_SIZE;
    pool->pool_size = new_pool_size;
    pool->block_count += blocks_added;
    pool->free_blocks += blocks_added;
    
    memory_stats.pool_expansions++;
    
    return 1;
}

// Run garbage collection
int run_memory_gc() {
    if (!memory_manager_initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&manager_mutex);
    
    time_t current_time = time(NULL);
    if (current_time - memory_stats.last_gc_time < global_pool_config.gc_interval_ms / 1000) {
        pthread_mutex_unlock(&manager_mutex);
        return 0; // Too soon for GC
    }
    
    // Check utilization for each pool
    for (int i = 0; i < global_memory_manager->pool_count; i++) {
        memory_pool_t *pool = &global_memory_manager->pools[i];
        pthread_mutex_lock(&pool->pool_mutex);
        
        if (pool->block_count > 0) {
            double utilization = (double)(pool->block_count - pool->free_blocks) / pool->block_count;
            if (utilization < global_pool_config.gc_threshold) {
                // Low utilization - consider shrinking
                shrink_pool(pool);
            }
        }
        
        pthread_mutex_unlock(&pool->pool_mutex);
    }
    
    memory_stats.last_gc_time = current_time;
    memory_stats.gc_runs++;
    
    pthread_mutex_unlock(&manager_mutex);
    
    return 1;
}

// Shrink memory pool
int shrink_pool(memory_pool_t *pool) {
    if (!pool || pool->pool_size <= POOL_MIN_SIZE) {
        return 0;
    }
    
    // This is a simplified implementation
    // In practice, you'd want to be more careful about which blocks to free
    // and ensure you don't free blocks that are still in use
    
    memory_stats.pool_shrinks++;
    return 1;
}

// Get memory statistics
memory_stats_t get_memory_stats() {
    if (!memory_manager_initialized) {
        memory_stats_t empty_stats = {0};
        return empty_stats;
    }
    
    pthread_mutex_lock(&manager_mutex);
    
    memory_stats_t stats = memory_stats;
    stats.total_allocated = global_memory_manager->total_allocated;
    stats.total_used = global_memory_manager->total_used;
    stats.pool_count = global_memory_manager->pool_count;
    
    // Calculate current utilization
    if (global_memory_manager->total_allocated > 0) {
        stats.utilization = (double)global_memory_manager->total_used / 
                           global_memory_manager->total_allocated;
    }
    
    pthread_mutex_unlock(&manager_mutex);
    
    return stats;
}

// Cleanup memory optimization system
void cleanup_memory_optimization() {
    if (!memory_manager_initialized) {
        return;
    }
    
    pthread_mutex_lock(&manager_mutex);
    
    // Free all pool memory
    for (int i = 0; i < global_memory_manager->pool_count; i++) {
        memory_pool_t *pool = &global_memory_manager->pools[i];
        if (pool->pool_start) {
            munmap(pool->pool_start, pool->pool_size);
        }
        pthread_mutex_destroy(&pool->pool_mutex);
    }
    
    free(global_memory_manager);
    global_memory_manager = NULL;
    memory_manager_initialized = 0;
    
    pthread_mutex_unlock(&manager_mutex);
}