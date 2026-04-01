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

#pragma once

#include "net/net-connections.h"
#include "net/net-rpc-targets.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the connection pool
void init_connection_pool(void);

// Get a reusable connection from the pool for the given target
connection_job_t get_pooled_connection(conn_target_job_t target);

// Return a connection to the pool for reuse
int return_connection_to_pool(connection_job_t conn, conn_target_job_t target);

// Mark a connection as no longer needed by the current user
int release_pooled_connection(connection_job_t conn);

// Cleanup connections that have been unused for too long
void cleanup_old_connections(void);

// Get statistics about the connection pool
void get_connection_pool_stats(long long *hits, long long *misses, long long *recycled, long long *reused, int *total);

// Optimized connection creation/retrieval functions
connection_job_t get_or_create_connection(conn_target_job_t target);

// Release a connection back to the pool or free it
void release_connection(connection_job_t conn, conn_target_job_t target);

// Periodic cleanup function to be called from cron
void connection_pool_cron(void);

#ifdef __cplusplus
}
#endif