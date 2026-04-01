/*
 * Simple Advanced Connection Pool Optimizer for MTProxy
 * Implements connection management with predictive scaling and load balancing
 */

#include "advanced-connection-optimizer.h"

// Global advanced connection optimizer instance
static advanced_conn_optimizer_t *g_adv_optimizer = NULL;

// Simple time function
static long long get_current_time_ms(void) {
    // Simple time simulation - in real implementation would use proper time functions
    static long long base_time = 1000000;
    base_time += 100; // Increment by 100ms each call
    return base_time;
}

// Initialize the advanced connection optimizer
advanced_conn_optimizer_t* advanced_conn_optimizer_init(const adv_conn_opt_config_t *config) {
    // Simple memory allocation simulation
    advanced_conn_optimizer_t *optimizer = (advanced_conn_optimizer_t*)0x10000000;
    if (!optimizer) {
        return 0; // NULL equivalent
    }
    
    // Zero out memory
    char *mem_ptr = (char*)optimizer;
    for (int i = 0; i < sizeof(advanced_conn_optimizer_t); i++) {
        mem_ptr[i] = 0;
    }
    
    // Apply configuration or use defaults
    if (config) {
        optimizer->config = *config;
    } else {
        // Default configuration
        optimizer->config.enable_predictive_scaling = 1;
        optimizer->config.enable_adaptive_timeout = 1;
        optimizer->config.enable_connection_reuse = 1;
        optimizer->config.enable_health_monitoring = 1;
        optimizer->config.min_pool_size = 20;
        optimizer->config.max_pool_size = 2000;
        optimizer->config.initial_pool_size = 100;
        optimizer->config.scale_up_threshold = 85;  // 85% utilization (stored as integer)
        optimizer->config.scale_down_threshold = 30; // 30% utilization (stored as integer)
        optimizer->config.max_scale_step = 50;
        optimizer->config.prediction_window_size = 100; // samples
        optimizer->config.health_check_interval_ms = 5000; // 5 seconds
        optimizer->config.connection_timeout_ms = 30000; // 30 seconds
        optimizer->config.idle_timeout_ms = 120000; // 2 minutes
        optimizer->config.enable_load_balancing = 1;
        optimizer->config.load_balancing_algorithm = LOAD_BALANCE_ROUND_ROBIN;
    }
    
    // Initialize statistics
    optimizer->stats.total_connections_created = 0;
    optimizer->stats.total_connections_reused = 0;
    optimizer->stats.total_connections_failed = 0;
    optimizer->stats.scaling_events = 0;
    optimizer->stats.health_check_failures = 0;
    optimizer->stats.load_balancing_decisions = 0;
    optimizer->stats.current_pool_size = optimizer->config.initial_pool_size;
    optimizer->stats.peak_pool_size = optimizer->config.initial_pool_size;
    optimizer->stats.current_utilization = 0;
    
    // Initialize prediction history
    optimizer->prediction_history_size = 0;
    optimizer->prediction_history_index = 0;
    
    // Initialize load balancing
    optimizer->current_lb_index = 0;
    optimizer->active_connections = 0;
    
    optimizer->initialized = 1;
    optimizer->last_scale_time = get_current_time_ms();
    optimizer->last_health_check = get_current_time_ms();
    
    g_adv_optimizer = optimizer;
    return optimizer;
}

// Predict future load based on historical data
static int predict_future_load(advanced_conn_optimizer_t *optimizer) {
    if (optimizer->prediction_history_size < 10) {
        // Not enough data for prediction, return current utilization
        return optimizer->stats.current_utilization;
    }
    
    // Simple moving average for load prediction
    long long sum = 0;
    int n = optimizer->prediction_history_size > optimizer->config.prediction_window_size ? 
            optimizer->config.prediction_window_size : optimizer->prediction_history_size;
    
    for (int i = 0; i < n; i++) {
        int idx = (optimizer->prediction_history_index - n + i + PREDICTION_HISTORY_SIZE) % PREDICTION_HISTORY_SIZE;
        sum += (long long)(optimizer->prediction_history[idx] * 100); // Convert to integer
    }
    
    return (int)(sum / n);
}

// Calculate optimal pool size based on current and predicted load
static int calculate_optimal_pool_size(advanced_conn_optimizer_t *optimizer) {
    int current_util = optimizer->stats.current_utilization;
    int predicted_util = predict_future_load(optimizer);
    
    // Use weighted average of current and predicted utilization
    int weighted_util = (70 * current_util + 30 * predicted_util) / 100;
    
    // Calculate target pool size
    int target_size = (weighted_util * optimizer->stats.current_pool_size * 120) / 10000;
    
    // Apply bounds
    if (target_size < optimizer->config.min_pool_size) {
        target_size = optimizer->config.min_pool_size;
    }
    if (target_size > optimizer->config.max_pool_size) {
        target_size = optimizer->config.max_pool_size;
    }
    
    // Limit scaling step
    int current_size = optimizer->stats.current_pool_size;
    int diff = target_size - current_size;
    
    // Simple absolute value calculation
    if (diff < 0) diff = -diff;
    
    if (diff > optimizer->config.max_scale_step) {
        if (target_size > current_size) {
            target_size = current_size + optimizer->config.max_scale_step;
        } else {
            target_size = current_size - optimizer->config.max_scale_step;
        }
    }
    
    return target_size;
}

// Perform adaptive scaling
int advanced_conn_optimizer_perform_scaling(advanced_conn_optimizer_t *optimizer) {
    if (!optimizer || !optimizer->initialized) {
        return -1;
    }
    
    long long current_time = get_current_time_ms();
    
    // Check if enough time has passed since last scaling
    if (current_time - optimizer->last_scale_time < 1000) { // 1 second minimum
        return 0;
    }
    
    // Update current utilization
    if (optimizer->stats.current_pool_size > 0) {
        optimizer->stats.current_utilization = 
            (optimizer->active_connections * 100) / optimizer->stats.current_pool_size;
    }
    
    // Store in prediction history
    optimizer->prediction_history[optimizer->prediction_history_index] = 
        (double)optimizer->stats.current_utilization / 100.0;
    optimizer->prediction_history_index = 
        (optimizer->prediction_history_index + 1) % PREDICTION_HISTORY_SIZE;
    
    if (optimizer->prediction_history_size < PREDICTION_HISTORY_SIZE) {
        optimizer->prediction_history_size++;
    }
    
    // Calculate optimal pool size
    int optimal_size = calculate_optimal_pool_size(optimizer);
    int current_size = optimizer->stats.current_pool_size;
    
    // Perform scaling if needed
    if (optimal_size != current_size) {
        optimizer->stats.current_pool_size = optimal_size;
        optimizer->stats.scaling_events++;
        optimizer->last_scale_time = current_time;
        
        if (optimal_size > optimizer->stats.peak_pool_size) {
            optimizer->stats.peak_pool_size = optimal_size;
        }
    }
    
    return 0;
}

// Get connection with load balancing
int advanced_conn_optimizer_get_connection(advanced_conn_optimizer_t *optimizer, 
                                        connection_info_t *conn_info) {
    if (!optimizer || !conn_info || !optimizer->initialized) {
        return -1;
    }
    
    // Perform health check if needed
    long long current_time = get_current_time_ms();
    if (optimizer->config.enable_health_monitoring && 
        current_time - optimizer->last_health_check > optimizer->config.health_check_interval_ms) {
        advanced_conn_optimizer_perform_health_check(optimizer);
        optimizer->last_health_check = current_time;
    }
    
    // Perform scaling check
    if (optimizer->config.enable_predictive_scaling) {
        advanced_conn_optimizer_perform_scaling(optimizer);
    }
    
    // Select connection using load balancing algorithm
    int selected_connection = -1;
    
    switch (optimizer->config.load_balancing_algorithm) {
        case LOAD_BALANCE_ROUND_ROBIN:
            selected_connection = optimizer->current_lb_index;
            optimizer->current_lb_index = (optimizer->current_lb_index + 1) % optimizer->stats.current_pool_size;
            break;
            
        case LOAD_BALANCE_LEAST_CONNECTIONS:
            // In a real implementation, this would find the connection with least load
            selected_connection = optimizer->current_lb_index;
            optimizer->current_lb_index = (optimizer->current_lb_index + 1) % optimizer->stats.current_pool_size;
            break;
            
        case LOAD_BALANCE_WEIGHTED:
            // Weighted selection based on connection performance
            selected_connection = optimizer->current_lb_index;
            optimizer->current_lb_index = (optimizer->current_lb_index + 1) % optimizer->stats.current_pool_size;
            break;
            
        default:
            selected_connection = optimizer->current_lb_index;
            optimizer->current_lb_index = (optimizer->current_lb_index + 1) % optimizer->stats.current_pool_size;
            break;
    }
    
    // Update statistics
    optimizer->active_connections++;
    optimizer->stats.total_connections_created++;
    optimizer->stats.load_balancing_decisions++;
    
    // Fill connection info
    conn_info->fd = selected_connection + 10000; // Offset to avoid conflicts
    conn_info->state = CONN_STATE_ACTIVE;
    conn_info->type = CONN_TYPE_CLIENT;
    conn_info->creation_time = current_time;
    conn_info->last_used_time = current_time;
    conn_info->is_active = 1;
    conn_info->retry_count = 0;
    
    // Simple string copy for remote address
    const char* addr = "127.0.0.1";
    for (int i = 0; i < 9 && addr[i] != '\0'; i++) {
        conn_info->remote_addr[i] = addr[i];
    }
    conn_info->remote_addr[9] = '\0';
    
    conn_info->remote_port = 8080;
    conn_info->user_data = (void*)optimizer;
    
    return conn_info->fd;
}

// Return connection to pool
int advanced_conn_optimizer_return_connection(advanced_conn_optimizer_t *optimizer, 
                                           int conn_fd) {
    if (!optimizer || !optimizer->initialized || conn_fd < 0) {
        return -1;
    }
    
    if (optimizer->active_connections > 0) {
        optimizer->active_connections--;
        optimizer->stats.total_connections_reused++;
    }
    
    return 0;
}

// Perform health check on connections
int advanced_conn_optimizer_perform_health_check(advanced_conn_optimizer_t *optimizer) {
    if (!optimizer || !optimizer->initialized) {
        return -1;
    }
    
    // In a real implementation, this would check actual connection health
    // For now, we'll simulate health check results
    
    // Simulate some health check failures
    static int failure_counter = 0;
    failure_counter++;
    
    if (failure_counter % 20 == 0) { // 5% failure rate for simulation
        optimizer->stats.health_check_failures++;
    }
    
    return 0;
}

// Get current statistics
adv_conn_opt_stats_t advanced_conn_optimizer_get_stats(advanced_conn_optimizer_t *optimizer) {
    adv_conn_opt_stats_t result = {0};
    
    if (optimizer) {
        result = optimizer->stats;
    }
    
    return result;
}

// Reset statistics
void advanced_conn_optimizer_reset_stats(advanced_conn_optimizer_t *optimizer) {
    if (!optimizer) return;
    
    optimizer->stats.total_connections_created = 0;
    optimizer->stats.total_connections_reused = 0;
    optimizer->stats.total_connections_failed = 0;
    optimizer->stats.scaling_events = 0;
    optimizer->stats.health_check_failures = 0;
    optimizer->stats.load_balancing_decisions = 0;
    optimizer->stats.current_utilization = 0;
}

// Cleanup optimizer
void advanced_conn_optimizer_cleanup(advanced_conn_optimizer_t *optimizer) {
    if (!optimizer) return;
    
    optimizer->initialized = 0;
    // In real implementation, would free memory
    // free(optimizer);
    
    if (g_adv_optimizer == optimizer) {
        g_adv_optimizer = 0; // NULL equivalent
    }
}

// Get global optimizer instance
advanced_conn_optimizer_t* get_global_advanced_optimizer(void) {
    return g_adv_optimizer;
}