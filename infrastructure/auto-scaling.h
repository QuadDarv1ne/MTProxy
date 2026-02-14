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

#ifndef __AUTO_SCALING_H__
#define __AUTO_SCALING_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
#define METRICS_HISTORY_SIZE 100
#define MAX_REASON_LENGTH 128

/* Scaling actions */
typedef enum {
    SCALE_NO_ACTION = 0,
    SCALE_UP,
    SCALE_DOWN
} scaling_action_t;

/* Auto-scaling configuration */
typedef struct {
    int enabled;
    int min_workers;
    int max_workers;
    double target_cpu_utilization;
    double target_memory_utilization;
    double scale_up_threshold;
    double scale_down_threshold;
    int cooldown_period_seconds;
    int evaluation_interval_seconds;
    int prediction_window_seconds;
} auto_scaling_config_t;

/* System metrics structure */
typedef struct {
    long timestamp;
    double cpu_utilization;        /* Percentage 0-100 */
    double memory_utilization;     /* Percentage 0-100 */
    int active_connections;
    int requests_per_second;
    uint64_t network_in_bytes;
    uint64_t network_out_bytes;
} system_metrics_t;

/* Metrics history */
typedef struct {
    system_metrics_t metrics[METRICS_HISTORY_SIZE];
    int count;
    int current_index;
} metrics_history_t;

/* Scaling decision */
typedef struct {
    long timestamp;
    scaling_action_t action;
    int recommended_workers;
    char reason[MAX_REASON_LENGTH];
} scaling_decision_t;

/* Auto-scaling manager */
typedef struct {
    int current_workers;
    int target_workers;
    long last_scaling_time;
    long start_time;
    int scaling_events;
    
    system_metrics_t metrics_history[METRICS_HISTORY_SIZE];
    int metrics_history_count;
    
    scaling_decision_t last_decision;
} auto_scaling_manager_t;

/* Auto-scaling status */
typedef struct {
    int enabled;
    int current_workers;
    int target_workers;
    int min_workers;
    int max_workers;
    long last_scaling_time;
    int scaling_events;
    system_metrics_t current_metrics;
    scaling_decision_t last_decision;
} auto_scaling_status_t;

/* Function prototypes */

/**
 * Initialize auto-scaling system
 * @param config: Auto-scaling configuration (optional, uses defaults if NULL)
 * @return: 0 on success, -1 on error
 */
int init_auto_scaling(auto_scaling_config_t *config);

/**
 * Initialize metrics history
 */
void init_metrics_history();

/**
 * Scaling worker thread function
 * @param arg: Thread argument (unused)
 * @return: Thread return value
 */
void* scaling_worker_thread(void *arg);

/**
 * Evaluate scaling needs based on current metrics
 */
void evaluate_scaling_needs();

/**
 * Collect current system metrics
 * @return: Current system metrics
 */
system_metrics_t collect_system_metrics();

/**
 * Add metrics to history
 * @param metrics: Metrics to add
 */
void add_metrics_to_history(system_metrics_t *metrics);

/**
 * Calculate scaling decision based on metrics
 * @param current_metrics: Current system metrics
 * @return: Scaling decision
 */
scaling_decision_t calculate_scaling_decision(system_metrics_t *current_metrics);

/**
 * Calculate optimal number of workers
 * @param metrics: System metrics
 * @return: Recommended number of workers
 */
int calculate_optimal_workers(system_metrics_t *metrics);

/**
 * Apply scaling decisions
 */
void apply_scaling_decisions();

/**
 * Scale up by adding workers
 * @param workers_to_add: Number of workers to add
 * @return: 1 on success, 0 on error
 */
int scale_up(int workers_to_add);

/**
 * Scale down by removing workers
 * @param workers_to_remove: Number of workers to remove
 * @return: 1 on success, 0 on error
 */
int scale_down(int workers_to_remove);

/**
 * Get current auto-scaling status
 * @return: Current auto-scaling status
 */
auto_scaling_status_t get_auto_scaling_status();

/**
 * Update auto-scaling configuration
 * @param new_config: New configuration
 * @return: 0 on success, -1 on error
 */
int update_auto_scaling_config(auto_scaling_config_t *new_config);

/**
 * Cleanup auto-scaling system
 */
void cleanup_auto_scaling();

#ifdef __cplusplus
}
#endif

#endif /* __AUTO_SCALING_H__ */