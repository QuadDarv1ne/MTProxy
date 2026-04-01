/*
    This file is part of MTProxy project.

    MTProxy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    MTProxy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MTProxy.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdint.h>

// Performance statistics structure
typedef struct {
    uint64_t total_connections_created;
    uint64_t total_connections_reused;
    uint64_t total_buffer_allocations;
    uint64_t total_buffer_frees;
    uint64_t total_buffer_recycles;
    uint32_t connection_pool_size;
    uint32_t connection_pool_capacity;
    double avg_buffer_utilization;
    double peak_memory_usage;
    double current_memory_usage;
} perf_stats_t;

// Function declarations for performance manager
int init_performance_manager(void);
void cleanup_performance_manager(void);
void* alloc_buffer_efficient(size_t size);
void free_buffer_efficient(void *buffer, size_t size);
int return_connection_to_pool(void *conn);
void get_performance_stats(perf_stats_t *stats);