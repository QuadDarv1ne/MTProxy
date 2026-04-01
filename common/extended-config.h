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

// Extended configuration structure for security settings
typedef struct {
    int ddos_protection_enabled;
    int max_connections_per_ip;
    double ddos_time_window;
    double ddos_block_duration;
    
    int cert_pinning_enabled;
    char cert_pinning_mode[16];  // "strict", "warn", etc.
    
    int hsm_enabled;
    char hsm_module_path[256];
    int hsm_slot_id;
    
    int access_control_enabled;
} security_config_t;

// Extended configuration structure for performance settings
typedef struct {
    int buffer_pool_enabled;
    int small_buffer_size;      // e.g., 512 bytes
    int medium_buffer_size;     // e.g., 2048 bytes
    int large_buffer_size;      // e.g., 8192 bytes
    int max_small_buffers;
    int max_medium_buffers;
    int max_large_buffers;
    
    int connection_pool_enabled;
    int max_pooled_connections;
    
    int async_io_enabled;
    int io_batch_size;
} performance_config_t;

// Extended configuration structure for monitoring settings
typedef struct {
    int prometheus_exporter_enabled;
    int prometheus_port;
    char prometheus_prefix[64];
    
    int health_checks_enabled;
    int health_check_port;
    int liveness_threshold;
    int readiness_timeout;
    
    int structured_logging_enabled;
    char log_format[16];        // "json", "text"
    int log_level;              // 0-4 for debug, info, warn, error, fatal
} monitoring_config_t;

// Main extended configuration structure
typedef struct {
    security_config_t security;
    performance_config_t performance;
    monitoring_config_t monitoring;
} extended_config_t;

// Function declarations for extended configuration
int init_extended_config(void);
int load_extended_config_from_file(const char *filename);
int validate_extended_config(void);
int apply_extended_config(void);
void cleanup_extended_config(void);

// Configuration update functions
int update_extended_security_config(const security_config_t *new_config);
int update_extended_performance_config(const performance_config_t *new_config);
int update_extended_monitoring_config(const monitoring_config_t *new_config);

// Get configuration values
const security_config_t* get_security_config(void);
const performance_config_t* get_performance_config(void);
const monitoring_config_t* get_monitoring_config(void);