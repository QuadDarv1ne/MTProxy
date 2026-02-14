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
#include <time.h>

#include "auto-scaling.h"

// Global auto-scaling configuration
static auto_scaling_config_t global_scaling_config = {
    .enabled = 1,
    .min_workers = 4,
    .max_workers = 64,
    .target_cpu_utilization = 70.0,
    .target_memory_utilization = 80.0,
    .scale_up_threshold = 85.0,
    .scale_down_threshold = 30.0,
    .cooldown_period_seconds = 300,  // 5 minutes
    .evaluation_interval_seconds = 30,
    .prediction_window_seconds = 300  // 5 minutes
};

// Global auto-scaling manager
static auto_scaling_manager_t *global_scaling_manager = NULL;
static pthread_mutex_t manager_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t scaling_thread;
static int scaling_thread_running = 0;
static int auto_scaling_initialized = 0;

// Initialize auto-scaling system
int init_auto_scaling(auto_scaling_config_t *config) {
    if (auto_scaling_initialized) {
        return 0; // Already initialized
    }

    pthread_mutex_lock(&manager_mutex);
    
    // Apply configuration
    if (config) {
        global_scaling_config = *config;
    }
    
    // Initialize scaling manager
    global_scaling_manager = calloc(1, sizeof(auto_scaling_manager_t));
    if (!global_scaling_manager) {
        pthread_mutex_unlock(&manager_mutex);
        return -1;
    }
    
    global_scaling_manager->current_workers = global_scaling_config.min_workers;
    global_scaling_manager->target_workers = global_scaling_config.min_workers;
    global_scaling_manager->last_scaling_time = time(NULL);
    global_scaling_manager->start_time = time(NULL);
    
    // Initialize metrics history
    init_metrics_history();
    
    // Start scaling thread
    scaling_thread_running = 1;
    if (pthread_create(&scaling_thread, NULL, scaling_worker_thread, NULL) != 0) {
        free(global_scaling_manager);
        global_scaling_manager = NULL;
        pthread_mutex_unlock(&manager_mutex);
        return -1;
    }
    
    auto_scaling_initialized = 1;
    pthread_mutex_unlock(&manager_mutex);
    
    return 0;
}

// Initialize metrics history
void init_metrics_history() {
    if (!global_scaling_manager) {
        return;
    }
    
    global_scaling_manager->metrics_history_count = 0;
    for (int i = 0; i < METRICS_HISTORY_SIZE; i++) {
        global_scaling_manager->metrics_history[i].timestamp = 0;
        global_scaling_manager->metrics_history[i].cpu_utilization = 0.0;
        global_scaling_manager->metrics_history[i].memory_utilization = 0.0;
        global_scaling_manager->metrics_history[i].active_connections = 0;
        global_scaling_manager->metrics_history[i].requests_per_second = 0;
    }
}

// Scaling worker thread
void* scaling_worker_thread(void *arg) {
    while (scaling_thread_running) {
        if (global_scaling_config.enabled) {
            evaluate_scaling_needs();
            apply_scaling_decisions();
        }
        
        sleep(global_scaling_config.evaluation_interval_seconds);
    }
    
    return NULL;
}

// Evaluate scaling needs based on current metrics
void evaluate_scaling_needs() {
    if (!global_scaling_manager) {
        return;
    }
    
    // Get current system metrics
    system_metrics_t current_metrics = collect_system_metrics();
    
    // Add to history
    add_metrics_to_history(&current_metrics);
    
    pthread_mutex_lock(&manager_mutex);
    
    // Calculate scaling decision
    scaling_decision_t decision = calculate_scaling_decision(&current_metrics);
    
    // Apply cooldown logic
    time_t current_time = time(NULL);
    if (current_time - global_scaling_manager->last_scaling_time >= 
        global_scaling_config.cooldown_period_seconds) {
        
        global_scaling_manager->target_workers = decision.recommended_workers;
        global_scaling_manager->last_decision = decision;
    }
    
    pthread_mutex_unlock(&manager_mutex);
}

// Collect current system metrics
system_metrics_t collect_system_metrics() {
    system_metrics_t metrics = {0};
    
    // In a real implementation, this would collect actual system metrics
    // For now, we'll simulate with realistic values
    
    metrics.timestamp = time(NULL);
    
    // Simulate CPU utilization (0-100%)
    metrics.cpu_utilization = 45.0 + (rand() % 40);  // 45-85%
    
    // Simulate memory utilization (0-100%)
    metrics.memory_utilization = 30.0 + (rand() % 50);  // 30-80%
    
    // Simulate active connections
    metrics.active_connections = 1000 + (rand() % 2000);  // 1000-3000
    
    // Simulate requests per second
    metrics.requests_per_second = 500 + (rand() % 1500);  // 500-2000
    
    // Simulate network I/O
    metrics.network_in_bytes = (uint64_t)(1000000 + (rand() % 5000000));  // 1-6 MB/s
    metrics.network_out_bytes = (uint64_t)(800000 + (rand() % 4000000));   // 0.8-4.8 MB/s
    
    return metrics;
}

// Add metrics to history
void add_metrics_to_history(system_metrics_t *metrics) {
    if (!global_scaling_manager || !metrics) {
        return;
    }
    
    pthread_mutex_lock(&manager_mutex);
    
    // Add to circular buffer
    int index = global_scaling_manager->metrics_history_count % METRICS_HISTORY_SIZE;
    global_scaling_manager->metrics_history[index] = *metrics;
    
    if (global_scaling_manager->metrics_history_count < METRICS_HISTORY_SIZE) {
        global_scaling_manager->metrics_history_count++;
    }
    
    pthread_mutex_unlock(&manager_mutex);
}

// Calculate scaling decision based on metrics
scaling_decision_t calculate_scaling_decision(system_metrics_t *current_metrics) {
    scaling_decision_t decision = {0};
    decision.timestamp = time(NULL);
    
    if (!current_metrics) {
        decision.action = SCALE_NO_ACTION;
        decision.recommended_workers = global_scaling_manager->current_workers;
        return decision;
    }
    
    // Check CPU utilization
    if (current_metrics->cpu_utilization > global_scaling_config.scale_up_threshold) {
        decision.action = SCALE_UP;
        decision.reason = "High CPU utilization";
    } else if (current_metrics->cpu_utilization < global_scaling_config.scale_down_threshold) {
        decision.action = SCALE_DOWN;
        decision.reason = "Low CPU utilization";
    }
    
    // Check memory utilization
    if (current_metrics->memory_utilization > global_scaling_config.target_memory_utilization) {
        if (decision.action != SCALE_DOWN) {  // Don't scale down if CPU says scale up
            decision.action = SCALE_UP;
            decision.reason = "High memory utilization";
        }
    }
    
    // Check connection load
    double connections_per_worker = (double)current_metrics->active_connections / 
                                   global_scaling_manager->current_workers;
    
    if (connections_per_worker > 200) {  // More than 200 connections per worker
        if (decision.action != SCALE_DOWN) {
            decision.action = SCALE_UP;
            decision.reason = "High connection load";
        }
    }
    
    // Calculate recommended worker count
    decision.recommended_workers = calculate_optimal_workers(current_metrics);
    
    // Apply bounds
    if (decision.recommended_workers < global_scaling_config.min_workers) {
        decision.recommended_workers = global_scaling_config.min_workers;
    }
    if (decision.recommended_workers > global_scaling_config.max_workers) {
        decision.recommended_workers = global_scaling_config.max_workers;
    }
    
    return decision;
}

// Calculate optimal number of workers
int calculate_optimal_workers(system_metrics_t *metrics) {
    if (!metrics) {
        return global_scaling_config.min_workers;
    }
    
    int recommended = global_scaling_manager->current_workers;
    
    // CPU-based calculation
    double cpu_factor = metrics->cpu_utilization / global_scaling_config.target_cpu_utilization;
    int cpu_workers = (int)(global_scaling_manager->current_workers * cpu_factor);
    
    // Memory-based calculation
    double memory_factor = metrics->memory_utilization / global_scaling_config.target_memory_utilization;
    int memory_workers = (int)(global_scaling_manager->current_workers * memory_factor);
    
    // Connection-based calculation
    int connection_workers = metrics->active_connections / 150;  // 150 connections per worker
    
    // Take maximum of all factors
    int max_workers = cpu_workers > memory_workers ? cpu_workers : memory_workers;
    max_workers = max_workers > connection_workers ? max_workers : connection_workers;
    
    // Apply smoothing to prevent rapid fluctuations
    if (max_workers > recommended + 2) {
        recommended += 2;  // Gradual scale up
    } else if (max_workers < recommended - 1) {
        recommended -= 1;  // Gradual scale down
    } else {
        recommended = max_workers;
    }
    
    return recommended;
}

// Apply scaling decisions
void apply_scaling_decisions() {
    if (!global_scaling_manager) {
        return;
    }
    
    pthread_mutex_lock(&manager_mutex);
    
    int current = global_scaling_manager->current_workers;
    int target = global_scaling_manager->target_workers;
    
    if (target > current) {
        // Scale up
        int workers_to_add = target - current;
        if (scale_up(workers_to_add)) {
            global_scaling_manager->current_workers = target;
            global_scaling_manager->last_scaling_time = time(NULL);
            global_scaling_manager->scaling_events++;
        }
    } else if (target < current) {
        // Scale down
        int workers_to_remove = current - target;
        if (scale_down(workers_to_remove)) {
            global_scaling_manager->current_workers = target;
            global_scaling_manager->last_scaling_time = time(NULL);
            global_scaling_manager->scaling_events++;
        }
    }
    
    pthread_mutex_unlock(&manager_mutex);
}

// Scale up by adding workers
int scale_up(int workers_to_add) {
    // In a real implementation, this would:
    // 1. Spawn new worker processes/threads
    // 2. Update load balancer configuration
    // 3. Wait for workers to become ready
    // 4. Gradually shift load to new workers
    
    printf("Scaling up: Adding %d workers\n", workers_to_add);
    
    // Simulate scaling operation
    sleep(2);  // Simulate time for workers to start
    
    return 1;  // Success
}

// Scale down by removing workers
int scale_down(int workers_to_remove) {
    // In a real implementation, this would:
    // 1. Stop accepting new connections on workers to be removed
    // 2. Wait for existing connections to complete
    // 3. Gracefully terminate workers
    // 4. Update load balancer configuration
    
    printf("Scaling down: Removing %d workers\n", workers_to_remove);
    
    // Simulate scaling operation
    sleep(3);  // Simulate graceful shutdown time
    
    return 1;  // Success
}

// Get current auto-scaling status
auto_scaling_status_t get_auto_scaling_status() {
    auto_scaling_status_t status = {0};
    
    if (!auto_scaling_initialized) {
        return status;
    }
    
    pthread_mutex_lock(&manager_mutex);
    
    status.enabled = global_scaling_config.enabled;
    status.current_workers = global_scaling_manager->current_workers;
    status.target_workers = global_scaling_manager->target_workers;
    status.min_workers = global_scaling_config.min_workers;
    status.max_workers = global_scaling_config.max_workers;
    
    status.last_scaling_time = global_scaling_manager->last_scaling_time;
    status.scaling_events = global_scaling_manager->scaling_events;
    
    // Get latest metrics
    if (global_scaling_manager->metrics_history_count > 0) {
        int latest_index = (global_scaling_manager->metrics_history_count - 1) % METRICS_HISTORY_SIZE;
        status.current_metrics = global_scaling_manager->metrics_history[latest_index];
    }
    
    status.last_decision = global_scaling_manager->last_decision;
    
    pthread_mutex_unlock(&manager_mutex);
    
    return status;
}

// Update auto-scaling configuration
int update_auto_scaling_config(auto_scaling_config_t *new_config) {
    if (!new_config || !auto_scaling_initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&manager_mutex);
    global_scaling_config = *new_config;
    pthread_mutex_unlock(&manager_mutex);
    
    return 0;
}

// Cleanup auto-scaling system
void cleanup_auto_scaling() {
    if (!auto_scaling_initialized) {
        return;
    }
    
    // Stop scaling thread
    scaling_thread_running = 0;
    if (scaling_thread) {
        pthread_join(scaling_thread, NULL);
    }
    
    pthread_mutex_lock(&manager_mutex);
    
    free(global_scaling_manager);
    global_scaling_manager = NULL;
    auto_scaling_initialized = 0;
    
    pthread_mutex_unlock(&manager_mutex);
}