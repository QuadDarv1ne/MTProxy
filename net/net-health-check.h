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

// Health check types
typedef enum {
    HEALTH_LIVENESS = 0,
    HEALTH_READINESS = 1,
    HEALTH_STARTUP = 2
} health_check_type_t;

// Function declarations for health check system
int init_health_check_system(void);
int register_health_check(const char *name, int (*checker_func)(void), 
                         double warning_threshold, double error_threshold);
int init_standard_health_checks(void);
int get_health_status(health_check_type_t type);
int format_health_status_json(char *buffer, size_t buffer_size, health_check_type_t type);
int handle_liveness_check(void *conn);
int handle_readiness_check(void *conn);
int handle_startup_check(void *conn);
void cleanup_health_check_system(void);

// Standard health check functions
int check_memory_health(void);
int check_connections_health(void);
int check_disk_space_health(void);
int check_upstream_connectivity(void);