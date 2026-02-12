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
#include <sys/time.h>

#include "net/net-connections.h"
#include "net/net-rpc-targets.h"
#include "net/net-tcp-rpc-client.h"
#include "mtproto/mtproto-proxy.h"
#include "vv/vv-tree.h"
#include "common/mp-queue.h"
#include "common/common-stats.h"
#include "kprintf.h"
#include "precise-time.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Connection Pool Implementation
 *
 * This module implements connection pooling and reuse optimizations for MTProxy.
 * Key features:
 * 1. Connection reuse based on target characteristics
 * 2. Connection lifecycle management
 * 3. Efficient connection lookup and retrieval
 */

// Define connection pool structures
#define CONNECTION_POOL_SIZE 1024
#define MAX_POOLED_CONNECTIONS 256
#define CONNECTION_REUSE_TIMEOUT 30.0  // seconds

struct connection_entry {
  connection_job_t conn;
  conn_target_job_t target;
  time_t last_used;
  int ref_count;
  struct connection_entry *next;  // for linked list
};

struct connection_pool {
  struct connection_entry entries[MAX_POOLED_CONNECTIONS];
  struct connection_entry *free_list;
  int total_entries;
  pthread_mutex_t mutex;
  
  // Statistics
  long long hits, misses, recycled, reused;
};

static struct connection_pool conn_pool;

// Initialize the connection pool
void init_connection_pool(void) {
  memset(&conn_pool, 0, sizeof(conn_pool));
  pthread_mutex_init(&conn_pool.mutex, NULL);
  
  // Initialize free list
  int i;
  for (i = 0; i < MAX_POOLED_CONNECTIONS - 1; i++) {
    conn_pool.entries[i].next = &conn_pool.entries[i + 1];
  }
  conn_pool.entries[MAX_POOLED_CONNECTIONS - 1].next = NULL;
  conn_pool.free_list = conn_pool.entries;
}

// Get a reusable connection from the pool for the given target
connection_job_t get_pooled_connection(conn_target_job_t target) {
  pthread_mutex_lock(&conn_pool.mutex);
  
  struct connection_entry *entry = NULL;
  time_t now = time(NULL);
  
  // Look for a matching connection in the pool
  for (int i = 0; i < conn_pool.total_entries; i++) {
    if (conn_pool.entries[i].target == target && 
        conn_pool.entries[i].ref_count == 0 &&
        (now - conn_pool.entries[i].last_used) < CONNECTION_REUSE_TIMEOUT) {
      
      entry = &conn_pool.entries[i];
      entry->ref_count++;
      conn_pool.hits++;
      connection_job_t conn = job_incref(entry->conn);
      
      pthread_mutex_unlock(&conn_pool.mutex);
      vkprintf(2, "Reusing pooled connection for target %p\n", target);
      return conn;
    }
  }
  
  conn_pool.misses++;
  pthread_mutex_unlock(&conn_pool.mutex);
  return NULL;
}

// Return a connection to the pool for reuse
int return_connection_to_pool(connection_job_t conn, conn_target_job_t target) {
  pthread_mutex_lock(&conn_pool.mutex);
  
  // Check if connection is still valid and can be pooled
  struct connection_info *c = CONN_INFO(conn);
  if (c && (c->flags & (C_ERROR | C_FAILED | C_NET_FAILED))) {
    pthread_mutex_unlock(&conn_pool.mutex);
    return -1;  // Connection is not reusable
  }
  
  // Find a free slot in the pool
  if (!conn_pool.free_list) {
    // Pool is full, try to find oldest unused connection
    int oldest_idx = -1;
    time_t oldest_time = time(NULL);
    
    for (int i = 0; i < MAX_POOLED_CONNECTIONS; i++) {
      if (conn_pool.entries[i].ref_count == 0 && 
          conn_pool.entries[i].last_used < oldest_time) {
        oldest_time = conn_pool.entries[i].last_used;
        oldest_idx = i;
      }
    }
    
    if (oldest_idx >= 0) {
      // Recycle the oldest connection
      if (conn_pool.entries[oldest_idx].conn) {
        job_decref(JOB_REF_PASS(conn_pool.entries[oldest_idx].conn));
        conn_pool.recycled++;
      }
      conn_pool.entries[oldest_idx].conn = job_incref(conn);
      conn_pool.entries[oldest_idx].target = target;
      conn_pool.entries[oldest_idx].last_used = time(NULL);
      conn_pool.entries[oldest_idx].ref_count = 0;
      conn_pool.reused++;
      
      pthread_mutex_unlock(&conn_pool.mutex);
      vkprintf(2, "Recycling connection to pool for target %p\n", target);
      return 0;
    } else {
      // No space available, cannot pool this connection
      pthread_mutex_unlock(&conn_pool.mutex);
      return -1;
    }
  }
  
  // Use a free slot
  struct connection_entry *entry = conn_pool.free_list;
  conn_pool.free_list = entry->next;
  
  entry->conn = job_incref(conn);
  entry->target = target;
  entry->last_used = time(NULL);
  entry->ref_count = 0;
  conn_pool.total_entries++;
  conn_pool.reused++;
  
  pthread_mutex_unlock(&conn_pool.mutex);
  vkprintf(2, "Adding connection to pool for target %p\n", target);
  return 0;
}

// Mark a connection as no longer needed by the current user
int release_pooled_connection(connection_job_t conn) {
  pthread_mutex_lock(&conn_pool.mutex);
  
  for (int i = 0; i < MAX_POOLED_CONNECTIONS; i++) {
    if (conn_pool.entries[i].conn == conn && conn_pool.entries[i].ref_count > 0) {
      conn_pool.entries[i].ref_count--;
      conn_pool.entries[i].last_used = time(NULL);
      pthread_mutex_unlock(&conn_pool.mutex);
      return 0;
    }
  }
  
  pthread_mutex_unlock(&conn_pool.mutex);
  // Connection not found in pool, just decref normally
  job_decref(JOB_REF_PASS(conn));
  return -1;
}

// Cleanup connections that have been unused for too long
void cleanup_old_connections(void) {
  pthread_mutex_lock(&conn_pool.mutex);
  
  time_t now = time(NULL);
  int cleaned = 0;
  
  for (int i = 0; i < MAX_POOLED_CONNECTIONS; i++) {
    if (conn_pool.entries[i].conn && 
        conn_pool.entries[i].ref_count == 0 &&
        (now - conn_pool.entries[i].last_used) >= CONNECTION_REUSE_TIMEOUT) {
      
      job_decref(JOB_REF_PASS(conn_pool.entries[i].conn));
      conn_pool.entries[i].conn = NULL;
      conn_pool.entries[i].target = NULL;
      conn_pool.entries[i].ref_count = 0;
      
      // Add to free list
      conn_pool.entries[i].next = conn_pool.free_list;
      conn_pool.free_list = &conn_pool.entries[i];
      conn_pool.total_entries--;
      cleaned++;
    }
  }
  
  if (cleaned > 0) {
    vkprintf(2, "Cleaned up %d old connections from pool\n", cleaned);
  }
  
  pthread_mutex_unlock(&conn_pool.mutex);
}

// Get statistics about the connection pool
void get_connection_pool_stats(long long *hits, long long *misses, long long *recycled, long long *reused, int *total) {
  pthread_mutex_lock(&conn_pool.mutex);
  *hits = conn_pool.hits;
  *misses = conn_pool.misses;
  *recycled = conn_pool.recycled;
  *reused = conn_pool.reused;
  *total = conn_pool.total_entries;
  pthread_mutex_unlock(&conn_pool.mutex);
}

/*
 * Optimized connection creation/retrieval functions
 */
connection_job_t get_or_create_connection(conn_target_job_t target) {
  // Try to get a pooled connection first
  connection_job_t conn = get_pooled_connection(target);
  if (conn) {
    vkprintf(2, "Reusing pooled connection for target %p\n", target);
    return conn;
  }
  
  // No pooled connection available, create a new one using existing infrastructure
  struct conn_target_info *t = CONN_TARGET_INFO(target);
  if (t && t->conn_tree) {
    // Try to get an existing connection from the target's connection tree
    conn = conn_target_get_connection(target, 0);
    if (conn) {
      vkprintf(2, "Got existing connection for target %p\n", target);
      return conn;
    }
  }
  
  // No suitable connection found, indicate that a new one should be created
  // The actual creation would be handled by the caller using alloc_new_connection
  return NULL;
}

// Integration function to replace the original conn_target_get_connection
connection_job_t get_pooled_target_connection(conn_target_job_t CT, int allow_stopped) {
  // First, try to get a connection from the pool
  connection_job_t pooled_conn = get_pooled_connection(CT);
  if (pooled_conn) {
    vkprintf(2, "Using pooled connection for target %p\n", CT);
    return pooled_conn;
  }
  
  // If no pooled connection, fall back to original logic
  struct conn_target_info *t = CONN_TARGET_INFO (CT);
  struct tree_connection *T = get_tree_ptr_connection (&t->conn_tree);

  connection_job_t S = NULL;
  tree_act_ex_connection (T, allow_stopped ? check_connection_stopped : check_connection, &S);

  if (S) { 
    job_incref (S); 
    // This connection is being used, so we shouldn't pool it again until it's released
  }
  tree_free_connection (T);

  return S;
}

void release_connection(connection_job_t conn, conn_target_job_t target) {
  // Try to return the connection to the pool
  if (return_connection_to_pool(conn, target) != 0) {
    // If pooling fails, just decref normally
    job_decref(JOB_REF_PASS(conn));
  }
}

// Periodic cleanup function to be called from cron
void connection_pool_cron(void) {
  cleanup_old_connections();
}

#ifdef __cplusplus
}
#endif