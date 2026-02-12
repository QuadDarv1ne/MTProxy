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

    Copyright 2024 Telegram Messenger Inc
*/

#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "net/net-connections.h"
#include "net/net-msg.h"
#include "common/mp-queue.h"
#include "common/common-stats.h"
#include "kprintf.h"
#include "precise-time.h"
#include "net/net-buffer-manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Network Buffer Management System
 *
 * This module implements optimized buffer management for MTProxy.
 * Key features:
 * 1. Memory pooling to reduce allocation/deallocation overhead
 * 2. Efficient buffer reuse patterns
 * 3. Thread-safe buffer allocation
 * 4. Reduced memory fragmentation
 */

// Buffer pool configuration
#define BUFFER_POOL_SIZE 1024
#define MIN_BUFFER_SIZE 1024
#define MAX_BUFFER_SIZE (64 * 1024)  // 64KB
#define DEFAULT_BUFFER_SIZE 8192     // 8KB

// Define power-of-2 bucket sizes for efficient allocation
#define NUM_BUCKET_SIZES 8
static const int bucket_sizes[NUM_BUCKET_SIZES] = {
    1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072
};

// Per-size buffer pool structure
struct buffer_bucket {
    void *buffers[BUFFER_POOL_SIZE];
    int count;
    pthread_mutex_t mutex;
    
    // Statistics
    long long allocated, deallocated, reused;
};

// Global buffer manager
struct buffer_manager {
    struct buffer_bucket buckets[NUM_BUCKET_SIZES];
    pthread_mutex_t global_mutex;
    
    // Overall statistics
    long long total_allocated_bytes;
    long long total_freed_bytes;
    long long peak_usage_bytes;
};

static struct buffer_manager buf_mgr;

// Helper function to find appropriate bucket index for size
static int get_bucket_index(int size) {
    for (int i = 0; i < NUM_BUCKET_SIZES; i++) {
        if (size <= bucket_sizes[i]) {
            return i;
        }
    }
    // If size is larger than our biggest bucket, return -1 to indicate direct allocation
    return -1;
}

// Initialize the buffer management system
int init_buffer_manager(void) {
    memset(&buf_mgr, 0, sizeof(buf_mgr));
    pthread_mutex_init(&buf_mgr.global_mutex, NULL);
    
    for (int i = 0; i < NUM_BUCKET_SIZES; i++) {
        buf_mgr.buckets[i].count = 0;
        pthread_mutex_init(&buf_mgr.buckets[i].mutex, NULL);
    }
    
    vkprintf(2, "Buffer manager initialized with %d size buckets\n", NUM_BUCKET_SIZES);
    return 0;
}

// Allocate a buffer of specified size
void *allocate_buffer(int size) {
    if (size <= 0) {
        return NULL;
    }
    
    int bucket_idx = get_bucket_index(size);
    
    if (bucket_idx >= 0) {
        // Use pooled allocation
        struct buffer_bucket *bucket = &buf_mgr.buckets[bucket_idx];
        pthread_mutex_lock(&bucket->mutex);
        
        if (bucket->count > 0) {
            // Reuse an existing buffer
            void *buffer = bucket->buffers[--bucket->count];
            bucket->reused++;
            pthread_mutex_unlock(&bucket->mutex);
            vkprintf(4, "Reused buffer of size %d from bucket %d\n", size, bucket_idx);
            return buffer;
        }
        
        pthread_mutex_unlock(&bucket->mutex);
        
        // No pooled buffer available, allocate new one
        void *buffer = malloc(bucket_sizes[bucket_idx]);
        if (buffer) {
            pthread_mutex_lock(&buf_mgr.global_mutex);
            buf_mgr.total_allocated_bytes += bucket_sizes[bucket_idx];
            if (buf_mgr.total_allocated_bytes > buf_mgr.peak_usage_bytes) {
                buf_mgr.peak_usage_bytes = buf_mgr.total_allocated_bytes;
            }
            pthread_mutex_unlock(&buf_mgr.global_mutex);
            
            pthread_mutex_lock(&bucket->mutex);
            bucket->allocated++;
            pthread_mutex_unlock(&bucket->mutex);
            
            vkprintf(4, "Allocated new buffer of size %d (bucket %d)\n", bucket_sizes[bucket_idx], bucket_idx);
        }
        return buffer;
    } else {
        // Size too large for any bucket, use direct allocation
        void *buffer = malloc(size);
        if (buffer) {
            pthread_mutex_lock(&buf_mgr.global_mutex);
            buf_mgr.total_allocated_bytes += size;
            if (buf_mgr.total_allocated_bytes > buf_mgr.peak_usage_bytes) {
                buf_mgr.peak_usage_bytes = buf_mgr.total_allocated_bytes;
            }
            pthread_mutex_unlock(&buf_mgr.global_mutex);
            
            vkprintf(4, "Allocated large buffer of size %d (direct allocation)\n", size);
        }
        return buffer;
    }
}

// Release a buffer back to the pool
void release_buffer(void *buffer, int size) {
    if (!buffer || size <= 0) {
        return;
    }
    
    int bucket_idx = get_bucket_index(size);
    
    if (bucket_idx >= 0 && bucket_idx < NUM_BUCKET_SIZES) {
        // Try to return to the appropriate bucket
        struct buffer_bucket *bucket = &buf_mgr.buckets[bucket_idx];
        pthread_mutex_lock(&bucket->mutex);
        
        if (bucket->count < BUFFER_POOL_SIZE) {
            // There's room in the pool
            bucket->buffers[bucket->count++] = buffer;
            bucket->deallocated++;
            pthread_mutex_unlock(&bucket->mutex);
            
            pthread_mutex_lock(&buf_mgr.global_mutex);
            buf_mgr.total_freed_bytes += bucket_sizes[bucket_idx];
            buf_mgr.total_allocated_bytes -= bucket_sizes[bucket_idx];
            pthread_mutex_unlock(&buf_mgr.global_mutex);
            
            vkprintf(4, "Returned buffer of size %d to bucket %d\n", size, bucket_idx);
            return;
        }
        
        pthread_mutex_unlock(&bucket->mutex);
    }
    
    // Bucket is full or size doesn't match any bucket, free directly
    free(buffer);
    
    pthread_mutex_lock(&buf_mgr.global_mutex);
    buf_mgr.total_freed_bytes += (bucket_idx >= 0) ? bucket_sizes[bucket_idx] : size;
    buf_mgr.total_allocated_bytes -= (bucket_idx >= 0) ? bucket_sizes[bucket_idx] : size;
    pthread_mutex_unlock(&buf_mgr.global_mutex);
    
    vkprintf(4, "Freed buffer of size %d (direct free)\n", size);
}

// Pre-allocate buffers to warm up the pool
void warmup_buffer_pool(void) {
    for (int i = 0; i < NUM_BUCKET_SIZES; i++) {
        struct buffer_bucket *bucket = &buf_mgr.buckets[i];
        pthread_mutex_lock(&bucket->mutex);
        
        // Pre-allocate half the pool size for each bucket
        int to_allocate = BUFFER_POOL_SIZE / 2;
        for (int j = 0; j < to_allocate && bucket->count < BUFFER_POOL_SIZE; j++) {
            void *buffer = malloc(bucket_sizes[i]);
            if (buffer) {
                bucket->buffers[bucket->count++] = buffer;
                bucket->allocated++;
            }
        }
        
        pthread_mutex_unlock(&bucket->mutex);
        vkprintf(3, "Warmed up bucket %d with %d buffers of size %d\n", 
                 i, bucket->count, bucket_sizes[i]);
    }
}

// Get buffer manager statistics
void get_buffer_manager_stats(struct buffer_manager_stats *stats) {
    pthread_mutex_lock(&buf_mgr.global_mutex);
    stats->total_allocated_bytes = buf_mgr.total_allocated_bytes;
    stats->total_freed_bytes = buf_mgr.total_freed_bytes;
    stats->peak_usage_bytes = buf_mgr.peak_usage_bytes;
    pthread_mutex_unlock(&buf_mgr.global_mutex);
    
    // Collect per-bucket statistics
    for (int i = 0; i < NUM_BUCKET_SIZES; i++) {
        struct buffer_bucket *bucket = &buf_mgr.buckets[i];
        pthread_mutex_lock(&bucket->mutex);
        stats->bucket_counts[i] = bucket->count;
        stats->bucket_allocated[i] = bucket->allocated;
        stats->bucket_deallocated[i] = bucket->deallocated;
        stats->bucket_reused[i] = bucket->reused;
        stats->bucket_sizes[i] = bucket_sizes[i];
        pthread_mutex_unlock(&bucket->mutex);
    }
    
    stats->num_buckets = NUM_BUCKET_SIZES;
}

// Cleanup the buffer manager (should be called at shutdown)
void cleanup_buffer_manager(void) {
    for (int i = 0; i < NUM_BUCKET_SIZES; i++) {
        struct buffer_bucket *bucket = &buf_mgr.buckets[i];
        pthread_mutex_lock(&bucket->mutex);
        
        // Free all remaining buffers in this bucket
        for (int j = 0; j < bucket->count; j++) {
            free(bucket->buffers[j]);
        }
        bucket->count = 0;
        
        pthread_mutex_unlock(&bucket->mutex);
    }
    
    vkprintf(2, "Buffer manager cleaned up\n");
}

// Optimized raw message operations using buffer manager
int rwm_init_optimized(struct raw_message *M, int size) {
    if (!M) {
        return -1;
    }
    
    // Use the buffer manager to allocate
    M->data = allocate_buffer(size);
    if (!M->data) {
        M->capacity = 0;
        M->total_bytes = 0;
        return -1;
    }
    
    M->capacity = size;
    M->total_bytes = 0;
    return 0;
}

int rwm_extend_optimized(struct raw_message *M, int min_size) {
    if (!M) {
        return -1;
    }
    
    if (min_size <= M->capacity) {
        return 0; // Already sufficient capacity
    }
    
    // Calculate new size (at least double the current capacity or meet min_size requirement)
    int new_size = M->capacity * 2;
    if (new_size < min_size) {
        new_size = min_size;
    }
    
    // Allocate new buffer using our buffer manager
    void *new_data = allocate_buffer(new_size);
    if (!new_data) {
        return -1;
    }
    
    // Copy existing data
    if (M->total_bytes > 0 && M->data) {
        memcpy(new_data, M->data, M->total_bytes);
    }
    
    // Release the old buffer
    if (M->data) {
        release_buffer(M->data, M->capacity);
    }
    
    // Update message structure
    M->data = new_data;
    M->capacity = new_size;
    
    return 0;
}

void rwm_free_optimized(struct raw_message *M) {
    if (M && M->data) {
        release_buffer(M->data, M->capacity);
        M->data = NULL;
        M->capacity = 0;
        M->total_bytes = 0;
    }
}

// Optimized append operation that minimizes allocations
int rwm_append_optimized(struct raw_message *M, const void *data, int len) {
    if (!M || !data || len <= 0) {
        return -1;
    }
    
    // Check if we need to extend the buffer
    if (M->total_bytes + len > M->capacity) {
        if (rwm_extend_optimized(M, M->total_bytes + len) < 0) {
            return -1;
        }
    }
    
    // Append the data
    memcpy((char*)M->data + M->total_bytes, data, len);
    M->total_bytes += len;
    
    return 0;
}

// Batch allocation for multiple connections
int batch_allocate_connection_buffers(connection_job_t *connections, int num_connections, int buffer_size) {
    if (!connections || num_connections <= 0) {
        return -1;
    }
    
    int allocated = 0;
    for (int i = 0; i < num_connections; i++) {
        if (connections[i]) {
            struct connection_info *c = CONN_INFO(connections[i]);
            if (c) {
                // Allocate optimized buffers for input and output
                if (rwm_init_optimized(&c->in, buffer_size) == 0 &&
                    rwm_init_optimized(&c->out, buffer_size) == 0) {
                    allocated++;
                }
            }
        }
    }
    
    vkprintf(3, "Batch allocated buffers for %d of %d connections\n", allocated, num_connections);
    return allocated;
}

#ifdef __cplusplus
}
#endif