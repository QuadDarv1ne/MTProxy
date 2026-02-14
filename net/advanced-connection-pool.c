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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "net-connections.h"
#include "net-rpc-targets.h"
#include "net-tcp-rpc-client.h"
#include "mtproto/mtproto-proxy.h"
#include "vv/vv-tree.h"
#include "common/mp-queue.h"
#include "common/common-stats.h"
#include "kprintf.h"
#include "precise-time.h"

#ifdef __cplusplus
extern "C" {
#endif

// Advanced connection pool configuration
typedef struct {
    int enabled;
    int max_connections_per_target;
    int max_total_connections;
    int min_idle_connections;
    int max_idle_connections;
    double connection_timeout_seconds;
    double idle_timeout_seconds;
    int enable_health_checks;
    double health_check_interval_seconds;
    int max_connection_reuse_count;
    int enable_adaptive_sizing;
    double adaptive_growth_factor;
    int enable_connection_multiplexing;
    int max_multiplexed_streams;
} advanced_pool_config_t;

// Connection health status
typedef enum {
    CONN_HEALTH_UNKNOWN = 0,
    CONN_HEALTH_HEALTHY = 1,
    CONN_HEALTH_DEGRADED = 2,
    CONN_HEALTH_UNHEALTHY = 3,
    CONN_HEALTH_FAILED = 4
} connection_health_t;

// Advanced connection entry with enhanced metadata
typedef struct advanced_connection_entry {
    connection_job_t conn;
    conn_target_job_t target;
    time_t created_time;
    time_t last_used_time;
    time_t last_health_check;
    int ref_count;
    int reuse_count;
    connection_health_t health_status;
    double latency_ms;
    long long bytes_transferred;
    int error_count;
    int consecutive_failures;
    
    // Multiplexing support
    int is_multiplexed;
    int active_streams;
    struct advanced_connection_entry *next_stream;
    
    // Pool management
    struct advanced_connection_entry *next;
    struct advanced_connection_entry *prev;
    int pool_id;
} advanced_connection_entry_t;

// Pool statistics
typedef struct {
    long long total_acquired;
    long long total_released;
    long long total_created;
    long long total_closed;
    long long cache_hits;
    long long cache_misses;
    long long health_checks_performed;
    long long failed_health_checks;
    long long multiplexed_connections;
    long long total_streams;
    double average_connection_latency;
    double pool_utilization;
    int current_active_connections;
    int current_idle_connections;
    int current_total_connections;
} advanced_pool_stats_t;

// Advanced connection pool
typedef struct {
    advanced_connection_entry_t *active_list;
    advanced_connection_entry_t *idle_list;
    advanced_connection_entry_t *free_list;
    advanced_connection_entry_t *all_entries;
    
    int total_entries;
    int active_count;
    int idle_count;
    int free_count;
    
    advanced_pool_config_t config;
    advanced_pool_stats_t stats;
    
    pthread_mutex_t pool_mutex;
    pthread_cond_t pool_cond;
    
    int initialized;
    time_t last_cleanup_time;
    time_t last_health_check_time;
} advanced_connection_pool_t;

// Global pool instance
static advanced_connection_pool_t g_advanced_pool = {0};

// Default configuration
static advanced_pool_config_t default_config = {
    .enabled = 1,
    .max_connections_per_target = 32,
    .max_total_connections = 1024,
    .min_idle_connections = 10,
    .max_idle_connections = 100,
    .connection_timeout_seconds = 300.0,
    .idle_timeout_seconds = 60.0,
    .enable_health_checks = 1,
    .health_check_interval_seconds = 30.0,
    .max_connection_reuse_count = 1000,
    .enable_adaptive_sizing = 1,
    .adaptive_growth_factor = 1.5,
    .enable_connection_multiplexing = 1,
    .max_multiplexed_streams = 8
};

// Simple time function for compatibility
static time_t get_current_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

// Initialize advanced connection pool
int init_advanced_connection_pool(advanced_pool_config_t *config) {
    if (g_advanced_pool.initialized) {
        return 0; // Already initialized
    }
    
    pthread_mutex_init(&g_advanced_pool.pool_mutex, NULL);
    pthread_cond_init(&g_advanced_pool.pool_cond, NULL);
    
    // Apply configuration
    if (config) {
        g_advanced_pool.config = *config;
    } else {
        g_advanced_pool.config = default_config;
    }
    
    // Initialize lists
    g_advanced_pool.active_list = NULL;
    g_advanced_pool.idle_list = NULL;
    g_advanced_pool.free_list = NULL;
    g_advanced_pool.all_entries = NULL;
    
    g_advanced_pool.total_entries = 0;
    g_advanced_pool.active_count = 0;
    g_advanced_pool.idle_count = 0;
    g_advanced_pool.free_count = 0;
    
    // Initialize statistics
    memset(&g_advanced_pool.stats, 0, sizeof(advanced_pool_stats_t));
    
    g_advanced_pool.initialized = 1;
    g_advanced_pool.last_cleanup_time = get_current_time();
    g_advanced_pool.last_health_check_time = get_current_time();
    
    vkprintf(1, "Advanced connection pool initialized with max %d connections\n", 
             g_advanced_pool.config.max_total_connections);
    
    return 0;
}

// Cleanup advanced connection pool
void cleanup_advanced_connection_pool(void) {
    if (!g_advanced_pool.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_advanced_pool.pool_mutex);
    
    // Close all connections
    advanced_connection_entry_t *entry = g_advanced_pool.all_entries;
    while (entry) {
        if (entry->conn) {
            job_decref(JOB_REF_PASS(entry->conn));
            g_advanced_pool.stats.total_closed++;
        }
        entry = entry->next;
    }
    
    // Free all entries
    entry = g_advanced_pool.all_entries;
    while (entry) {
        advanced_connection_entry_t *next = entry->next;
        free(entry);
        entry = next;
    }
    
    g_advanced_pool.all_entries = NULL;
    g_advanced_pool.active_list = NULL;
    g_advanced_pool.idle_list = NULL;
    g_advanced_pool.free_list = NULL;
    
    pthread_mutex_unlock(&g_advanced_pool.pool_mutex);
    pthread_mutex_destroy(&g_advanced_pool.pool_mutex);
    pthread_cond_destroy(&g_advanced_pool.pool_cond);
    
    g_advanced_pool.initialized = 0;
    
    vkprintf(1, "Advanced connection pool cleaned up\n");
}

// Create new connection entry
static advanced_connection_entry_t* create_connection_entry() {
    if (g_advanced_pool.total_entries >= g_advanced_pool.config.max_total_connections) {
        return NULL;
    }
    
    advanced_connection_entry_t *entry = calloc(1, sizeof(advanced_connection_entry_t));
    if (!entry) {
        return NULL;
    }
    
    entry->created_time = get_current_time();
    entry->last_used_time = get_current_time();
    entry->health_status = CONN_HEALTH_UNKNOWN;
    entry->pool_id = g_advanced_pool.total_entries;
    
    // Add to all entries list
    entry->next = g_advanced_pool.all_entries;
    if (g_advanced_pool.all_entries) {
        g_advanced_pool.all_entries->prev = entry;
    }
    g_advanced_pool.all_entries = entry;
    
    // Add to free list
    entry->next = g_advanced_pool.free_list;
    if (g_advanced_pool.free_list) {
        g_advanced_pool.free_list->prev = entry;
    }
    g_advanced_pool.free_list = entry;
    
    g_advanced_pool.total_entries++;
    g_advanced_pool.free_count++;
    
    return entry;
}

// Acquire connection from pool
connection_job_t acquire_connection_from_pool(conn_target_job_t target) {
    if (!g_advanced_pool.initialized || !target) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_advanced_pool.pool_mutex);
    
    // Try to find a suitable connection in idle list
    advanced_connection_entry_t *entry = g_advanced_pool.idle_list;
    advanced_connection_entry_t *best_entry = NULL;
    double best_score = -1.0;
    
    time_t current_time = get_current_time();
    
    while (entry) {
        if (entry->target == target && 
            entry->ref_count == 0 &&
            entry->health_status != CONN_HEALTH_FAILED &&
            (current_time - entry->last_used_time) < g_advanced_pool.config.idle_timeout_seconds) {
            
            // Calculate connection score (lower reuse count and error count is better)
            double score = 1.0 / (1.0 + entry->reuse_count + entry->error_count * 10);
            
            if (score > best_score) {
                best_score = score;
                best_entry = entry;
            }
        }
        entry = entry->next;
    }
    
    if (best_entry) {
        // Move from idle to active list
        if (best_entry->prev) {
            best_entry->prev->next = best_entry->next;
        } else {
            g_advanced_pool.idle_list = best_entry->next;
        }
        if (best_entry->next) {
            best_entry->next->prev = best_entry->prev;
        }
        
        best_entry->next = g_advanced_pool.active_list;
        if (g_advanced_pool.active_list) {
            g_advanced_pool.active_list->prev = best_entry;
        }
        g_advanced_pool.active_list = best_entry;
        
        best_entry->ref_count = 1;
        best_entry->last_used_time = current_time;
        best_entry->reuse_count++;
        
        g_advanced_pool.idle_count--;
        g_advanced_pool.active_count++;
        g_advanced_pool.stats.cache_hits++;
        g_advanced_pool.stats.total_acquired++;
        
        pthread_mutex_unlock(&g_advanced_pool.pool_mutex);
        
        vkprintf(3, "Acquired pooled connection %p for target %p (reuse count: %d)\n", 
                 best_entry->conn, target, best_entry->reuse_count);
        
        return job_incref(best_entry->conn);
    }
    
    g_advanced_pool.stats.cache_misses++;
    pthread_mutex_unlock(&g_advanced_pool.pool_mutex);
    
    return NULL;
}

// Release connection to pool
int release_connection_to_pool(connection_job_t conn, conn_target_job_t target) {
    if (!g_advanced_pool.initialized || !conn || !target) {
        return -1;
    }
    
    pthread_mutex_lock(&g_advanced_pool.pool_mutex);
    
    // Check if we have space in the pool
    if (g_advanced_pool.idle_count >= g_advanced_pool.config.max_idle_connections) {
        // Pool is full, close oldest idle connections
        cleanup_expired_connections();
    }
    
    // Create new entry or reuse free entry
    advanced_connection_entry_t *entry = g_advanced_pool.free_list;
    if (entry) {
        // Remove from free list
        if (entry->prev) {
            entry->prev->next = entry->next;
        } else {
            g_advanced_pool.free_list = entry->next;
        }
        if (entry->next) {
            entry->next->prev = entry->prev;
        }
        g_advanced_pool.free_count--;
    } else {
        // Create new entry
        entry = create_connection_entry();
        if (!entry) {
            pthread_mutex_unlock(&g_advanced_pool.pool_mutex);
            return -1;
        }
    }
    
    // Initialize entry
    entry->conn = job_incref(conn);
    entry->target = target;
    entry->ref_count = 0;
    entry->last_used_time = get_current_time();
    entry->health_status = CONN_HEALTH_HEALTHY;
    entry->error_count = 0;
    entry->consecutive_failures = 0;
    
    // Add to idle list
    entry->next = g_advanced_pool.idle_list;
    if (g_advanced_pool.idle_list) {
        g_advanced_pool.idle_list->prev = entry;
    }
    g_advanced_pool.idle_list = entry;
    
    g_advanced_pool.idle_count++;
    g_advanced_pool.stats.total_released++;
    g_advanced_pool.stats.total_created++;
    
    pthread_mutex_unlock(&g_advanced_pool.pool_mutex);
    
    vkprintf(3, "Released connection %p to pool for target %p\n", conn, target);
    
    return 0;
}

// Return connection to pool (decrement reference count)
int return_connection_to_pool(connection_job_t conn) {
    if (!g_advanced_pool.initialized || !conn) {
        return -1;
    }
    
    pthread_mutex_lock(&g_advanced_pool.pool_mutex);
    
    // Find the connection in active list
    advanced_connection_entry_t *entry = g_advanced_pool.active_list;
    while (entry) {
        if (entry->conn == conn && entry->ref_count > 0) {
            entry->ref_count--;
            entry->last_used_time = get_current_time();
            
            if (entry->ref_count == 0) {
                // Move from active to idle list
                if (entry->prev) {
                    entry->prev->next = entry->next;
                } else {
                    g_advanced_pool.active_list = entry->next;
                }
                if (entry->next) {
                    entry->next->prev = entry->prev;
                }
                
                entry->next = g_advanced_pool.idle_list;
                if (g_advanced_pool.idle_list) {
                    g_advanced_pool.idle_list->prev = entry;
                }
                g_advanced_pool.idle_list = entry;
                
                g_advanced_pool.active_count--;
                g_advanced_pool.idle_count++;
                
                vkprintf(3, "Returned connection %p to idle pool\n", conn);
            }
            
            pthread_mutex_unlock(&g_advanced_pool.pool_mutex);
            return 0;
        }
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&g_advanced_pool.pool_mutex);
    
    // Connection not found in pool, just decref normally
    job_decref(JOB_REF_PASS(conn));
    return -1;
}

// Cleanup expired connections
void cleanup_expired_connections(void) {
    if (!g_advanced_pool.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_advanced_pool.pool_mutex);
    
    time_t current_time = get_current_time();
    int cleaned = 0;
    
    // Cleanup idle connections
    advanced_connection_entry_t *entry = g_advanced_pool.idle_list;
    while (entry && g_advanced_pool.idle_count > g_advanced_pool.config.min_idle_connections) {
        if ((current_time - entry->last_used_time) >= g_advanced_pool.config.idle_timeout_seconds ||
            entry->health_status == CONN_HEALTH_FAILED) {
            
            // Remove from idle list
            if (entry->prev) {
                entry->prev->next = entry->next;
            } else {
                g_advanced_pool.idle_list = entry->next;
            }
            if (entry->next) {
                entry->next->prev = entry->prev;
            }
            
            // Close connection
            if (entry->conn) {
                job_decref(JOB_REF_PASS(entry->conn));
                g_advanced_pool.stats.total_closed++;
            }
            
            // Move to free list
            entry->next = g_advanced_pool.free_list;
            if (g_advanced_pool.free_list) {
                g_advanced_pool.free_list->prev = entry;
            }
            g_advanced_pool.free_list = entry;
            entry->conn = NULL;
            entry->target = NULL;
            
            g_advanced_pool.idle_count--;
            g_advanced_pool.free_count++;
            cleaned++;
        }
        
        entry = entry->next;
    }
    
    if (cleaned > 0) {
        vkprintf(2, "Cleaned up %d expired connections\n", cleaned);
    }
    
    g_advanced_pool.last_cleanup_time = current_time;
    pthread_mutex_unlock(&g_advanced_pool.pool_mutex);
}

// Run health checks on connections
void run_health_checks(void) {
    if (!g_advanced_pool.initialized || !g_advanced_pool.config.enable_health_checks) {
        return;
    }
    
    time_t current_time = get_current_time();
    if (current_time - g_advanced_pool.last_health_check_time < 
        g_advanced_pool.config.health_check_interval_seconds) {
        return;
    }
    
    pthread_mutex_lock(&g_advanced_pool.pool_mutex);
    
    advanced_connection_entry_t *entry = g_advanced_pool.all_entries;
    int checks_performed = 0;
    int failed_checks = 0;
    
    while (entry) {
        if (entry->conn && entry->health_status != CONN_HEALTH_FAILED) {
            // Check if connection is still valid
            struct connection_info *c = CONN_INFO(entry->conn);
            if (!c || (c->flags & (C_ERROR | C_FAILED | C_NET_FAILED))) {
                entry->health_status = CONN_HEALTH_FAILED;
                entry->consecutive_failures++;
                failed_checks++;
            } else {
                entry->health_status = CONN_HEALTH_HEALTHY;
                entry->consecutive_failures = 0;
            }
            checks_performed++;
        }
        entry = entry->next;
    }
    
    g_advanced_pool.stats.health_checks_performed += checks_performed;
    g_advanced_pool.stats.failed_health_checks += failed_checks;
    g_advanced_pool.last_health_check_time = current_time;
    
    if (checks_performed > 0) {
        vkprintf(3, "Performed %d health checks, %d failed\n", checks_performed, failed_checks);
    }
    
    pthread_mutex_unlock(&g_advanced_pool.pool_mutex);
}

// Get advanced pool statistics
advanced_pool_stats_t get_advanced_pool_stats(void) {
    advanced_pool_stats_t stats = {0};
    
    if (!g_advanced_pool.initialized) {
        return stats;
    }
    
    pthread_mutex_lock(&g_advanced_pool.pool_mutex);
    stats = g_advanced_pool.stats;
    stats.current_active_connections = g_advanced_pool.active_count;
    stats.current_idle_connections = g_advanced_pool.idle_count;
    stats.current_total_connections = g_advanced_pool.total_entries;
    
    if (g_advanced_pool.total_entries > 0) {
        stats.pool_utilization = (double)g_advanced_pool.active_count / g_advanced_pool.total_entries;
    }
    
    pthread_mutex_unlock(&g_advanced_pool.pool_mutex);
    
    return stats;
}

// Integration functions with existing code
connection_job_t get_advanced_target_connection(conn_target_job_t CT, int allow_stopped) {
    // First try advanced pool
    connection_job_t pooled_conn = acquire_connection_from_pool(CT);
    if (pooled_conn) {
        vkprintf(2, "Using advanced pooled connection for target %p\n", CT);
        return pooled_conn;
    }
    
    // Fall back to original logic
    struct conn_target_info *t = CONN_TARGET_INFO(CT);
    struct tree_connection *T = get_tree_ptr_connection(&t->conn_tree);
    
    connection_job_t S = NULL;
    tree_act_ex_connection(T, allow_stopped ? check_connection_stopped : check_connection, &S);
    
    if (S) {
        job_incref(S);
    }
    tree_free_connection(T);
    
    return S;
}

void release_advanced_connection(connection_job_t conn, conn_target_job_t target) {
    // Try to return to advanced pool first
    if (release_connection_to_pool(conn, target) != 0) {
        // If pooling fails, just decref normally
        job_decref(JOB_REF_PASS(conn));
    }
}

// Periodic maintenance function
void advanced_connection_pool_cron(void) {
    if (!g_advanced_pool.initialized) {
        return;
    }
    
    cleanup_expired_connections();
    run_health_checks();
}

#ifdef __cplusplus
}
#endif