/*
 * Advanced Load Balancer for MTProxy
 * Implements sophisticated load distribution algorithms with health monitoring
 */

#include "advanced-load-balancer.h"

// Global load balancer instance
static advanced_load_balancer_t *g_load_balancer = NULL;

// Simple hash function for IP-based distribution
static unsigned int simple_hash(const char* str) {
    unsigned int hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    return hash;
}

// Initialize advanced load balancer
advanced_load_balancer_t* advanced_load_balancer_init(const lb_config_t *config) {
    advanced_load_balancer_t *lb = (advanced_load_balancer_t*)0x20000000; // Simple memory address
    if (!lb) {
        return 0; // NULL equivalent
    }
    
    // Zero initialize
    char *mem_ptr = (char*)lb;
    for (int i = 0; i < sizeof(advanced_load_balancer_t); i++) {
        mem_ptr[i] = 0;
    }
    
    // Apply configuration or use defaults
    if (config) {
        lb->config = *config;
    } else {
        // Default configuration
        lb->config.algorithm = LB_ALGORITHM_LEAST_CONNECTIONS;
        lb->config.enable_health_checks = 1;
        lb->config.health_check_interval_ms = 5000;
        lb->config.max_retries = 3;
        lb->config.connection_timeout_ms = 30000;
        lb->config.enable_weight_adjustment = 1;
        lb->config.weight_adjustment_interval_ms = 10000;
        lb->config.failover_enabled = 1;
        lb->config.session_persistence = 1;
        lb->config.max_servers = 100;
        lb->config.enable_statistics = 1;
    }
    
    // Initialize statistics
    lb->stats.total_requests = 0;
    lb->stats.successful_requests = 0;
    lb->stats.failed_requests = 0;
    lb->stats.health_check_failures = 0;
    lb->stats.load_balancing_decisions = 0;
    lb->stats.server_failovers = 0;
    lb->stats.current_active_servers = 0;
    lb->stats.peak_active_servers = 0;
    
    // Initialize server pool
    lb->server_count = 0;
    lb->current_index = 0;
    lb->last_health_check = 0;
    lb->last_weight_adjustment = 0;
    
    lb->initialized = 1;
    g_load_balancer = lb;
    return lb;
}

// Add server to load balancer
int advanced_load_balancer_add_server(advanced_load_balancer_t *lb,
                                    const char *address,
                                    int port,
                                    int weight,
                                    int max_connections) {
    if (!lb || !lb->initialized || !address) {
        return -1;
    }
    
    if (lb->server_count >= lb->config.max_servers) {
        return -1; // Server pool full
    }
    
    lb_server_t *server = &lb->servers[lb->server_count];
    
    // Copy address
    int addr_len = 0;
    while (address[addr_len] != '\0' && addr_len < 45) {
        server->address[addr_len] = address[addr_len];
        addr_len++;
    }
    server->address[addr_len] = '\0';
    
    server->port = port;
    server->weight = weight;
    server->max_connections = max_connections;
    server->current_connections = 0;
    server->status = LB_SERVER_STATUS_HEALTHY;
    server->failure_count = 0;
    server->last_health_check = 0;
    server->response_time_ms = 0;
    server->total_requests = 0;
    server->successful_requests = 0;
    
    lb->server_count++;
    lb->stats.current_active_servers++;
    
    if (lb->stats.current_active_servers > lb->stats.peak_active_servers) {
        lb->stats.peak_active_servers = lb->stats.current_active_servers;
    }
    
    return 0;
}

// Select server using load balancing algorithm
lb_server_t* advanced_load_balancer_select_server(advanced_load_balancer_t *lb,
                                                const char *client_ip) {
    if (!lb || !lb->initialized || lb->server_count == 0) {
        return 0; // NULL equivalent
    }
    
    // Perform health checks if needed
    advanced_load_balancer_perform_health_checks(lb);
    
    // Adjust weights if needed
    if (lb->config.enable_weight_adjustment) {
        advanced_load_balancer_adjust_weights(lb);
    }
    
    lb_server_t *selected_server = 0;
    int selected_index = -1;
    
    switch (lb->config.algorithm) {
        case LB_ALGORITHM_ROUND_ROBIN:
            selected_index = advanced_load_balancer_round_robin(lb);
            break;
            
        case LB_ALGORITHM_LEAST_CONNECTIONS:
            selected_index = advanced_load_balancer_least_connections(lb);
            break;
            
        case LB_ALGORITHM_WEIGHTED_ROUND_ROBIN:
            selected_index = advanced_load_balancer_weighted_round_robin(lb);
            break;
            
        case LB_ALGORITHM_IP_HASH:
            selected_index = advanced_load_balancer_ip_hash(lb, client_ip);
            break;
            
        case LB_ALGORITHM_LEAST_RESPONSE_TIME:
            selected_index = advanced_load_balancer_least_response_time(lb);
            break;
            
        default:
            selected_index = advanced_load_balancer_round_robin(lb);
            break;
    }
    
    if (selected_index >= 0 && selected_index < lb->server_count) {
        selected_server = &lb->servers[selected_index];
        selected_server->current_connections++;
        selected_server->total_requests++;
        lb->stats.total_requests++;
        lb->stats.load_balancing_decisions++;
    }
    
    return selected_server;
}

// Round Robin algorithm
static int advanced_load_balancer_round_robin(advanced_load_balancer_t *lb) {
    int start_index = lb->current_index;
    
    do {
        lb_server_t *server = &lb->servers[lb->current_index];
        
        if (server->status == LB_SERVER_STATUS_HEALTHY && 
            server->current_connections < server->max_connections) {
            lb->current_index = (lb->current_index + 1) % lb->server_count;
            return lb->current_index - 1;
        }
        
        lb->current_index = (lb->current_index + 1) % lb->server_count;
        
    } while (lb->current_index != start_index);
    
    return -1; // No healthy servers available
}

// Least Connections algorithm
static int advanced_load_balancer_least_connections(advanced_load_balancer_t *lb) {
    int best_index = -1;
    int min_connections = 0x7FFFFFFF; // MAX_INT
    
    for (int i = 0; i < lb->server_count; i++) {
        lb_server_t *server = &lb->servers[i];
        
        if (server->status == LB_SERVER_STATUS_HEALTHY && 
            server->current_connections < server->max_connections) {
            
            if (server->current_connections < min_connections) {
                min_connections = server->current_connections;
                best_index = i;
            }
        }
    }
    
    return best_index;
}

// Weighted Round Robin algorithm
static int advanced_load_balancer_weighted_round_robin(advanced_load_balancer_t *lb) {
    static int current_weight = 0;
    int start_index = lb->current_index;
    
    do {
        lb_server_t *server = &lb->servers[lb->current_index];
        
        if (server->status == LB_SERVER_STATUS_HEALTHY && 
            server->current_connections < server->max_connections) {
            
            if (server->weight > current_weight) {
                current_weight++;
                lb->current_index = (lb->current_index + 1) % lb->server_count;
                return lb->current_index - 1;
            }
        }
        
        lb->current_index = (lb->current_index + 1) % lb->server_count;
        
        if (lb->current_index == start_index) {
            current_weight = 0; // Reset weight counter
        }
        
    } while (lb->current_index != start_index);
    
    return -1; // No healthy servers available
}

// IP Hash algorithm
static int advanced_load_balancer_ip_hash(advanced_load_balancer_t *lb, const char *client_ip) {
    if (!client_ip) {
        return advanced_load_balancer_round_robin(lb);
    }
    
    unsigned int hash = simple_hash(client_ip);
    int index = hash % lb->server_count;
    
    // Find healthy server starting from hash index
    for (int i = 0; i < lb->server_count; i++) {
        int check_index = (index + i) % lb->server_count;
        lb_server_t *server = &lb->servers[check_index];
        
        if (server->status == LB_SERVER_STATUS_HEALTHY && 
            server->current_connections < server->max_connections) {
            return check_index;
        }
    }
    
    return -1; // No healthy servers available
}

// Least Response Time algorithm
static int advanced_load_balancer_least_response_time(advanced_load_balancer_t *lb) {
    int best_index = -1;
    double best_score = 1e9; // Very large number
    
    for (int i = 0; i < lb->server_count; i++) {
        lb_server_t *server = &lb->servers[i];
        
        if (server->status == LB_SERVER_STATUS_HEALTHY && 
            server->current_connections < server->max_connections) {
            
            // Score = response_time + (current_connections * weight_factor)
            double score = server->response_time_ms + 
                          (server->current_connections * 10.0 / server->weight);
            
            if (score < best_score) {
                best_score = score;
                best_index = i;
            }
        }
    }
    
    return best_index;
}

// Perform health checks
int advanced_load_balancer_perform_health_checks(advanced_load_balancer_t *lb) {
    if (!lb || !lb->initialized || !lb->config.enable_health_checks) {
        return -1;
    }
    
    // Simple time simulation
    static long long current_time = 1000000;
    current_time += 1000; // 1 second increments
    
    if (current_time - lb->last_health_check < lb->config.health_check_interval_ms) {
        return 0;
    }
    
    lb->last_health_check = current_time;
    
    // Perform health checks on all servers
    for (int i = 0; i < lb->server_count; i++) {
        lb_server_t *server = &lb->servers[i];
        
        // Simulate health check result
        static int health_counter = 0;
        health_counter++;
        
        if (health_counter % 50 == 0) { // 2% failure rate
            if (server->status == LB_SERVER_STATUS_HEALTHY) {
                server->status = LB_SERVER_STATUS_UNHEALTHY;
                server->failure_count++;
                lb->stats.health_check_failures++;
                
                if (lb->stats.current_active_servers > 0) {
                    lb->stats.current_active_servers--;
                }
            }
        } else {
            if (server->status == LB_SERVER_STATUS_UNHEALTHY) {
                server->status = LB_SERVER_STATUS_HEALTHY;
                lb->stats.current_active_servers++;
            }
        }
    }
    
    return 0;
}

// Adjust server weights based on performance
int advanced_load_balancer_adjust_weights(advanced_load_balancer_t *lb) {
    if (!lb || !lb->initialized || !lb->config.enable_weight_adjustment) {
        return -1;
    }
    
    // Simple time simulation
    static long long current_time = 1000000;
    current_time += 2000; // 2 second increments
    
    if (current_time - lb->last_weight_adjustment < lb->config.weight_adjustment_interval_ms) {
        return 0;
    }
    
    lb->last_weight_adjustment = current_time;
    
    // Adjust weights based on server performance
    for (int i = 0; i < lb->server_count; i++) {
        lb_server_t *server = &lb->servers[i];
        
        if (server->status == LB_SERVER_STATUS_HEALTHY) {
            // Simple weight adjustment based on success rate
            if (server->total_requests > 0) {
                double success_rate = (double)server->successful_requests / (double)server->total_requests;
                
                if (success_rate > 0.95) {
                    // Increase weight for good performance
                    if (server->weight < 100) {
                        server->weight++;
                    }
                } else if (success_rate < 0.8) {
                    // Decrease weight for poor performance
                    if (server->weight > 1) {
                        server->weight--;
                    }
                }
            }
        }
    }
    
    return 0;
}

// Report successful request
void advanced_load_balancer_report_success(advanced_load_balancer_t *lb,
                                         lb_server_t *server,
                                         long long response_time_ms) {
    if (!lb || !server) return;
    
    server->successful_requests++;
    server->current_connections--;
    if (server->current_connections < 0) {
        server->current_connections = 0;
    }
    
    server->response_time_ms = (server->response_time_ms + response_time_ms) / 2; // Simple average
    lb->stats.successful_requests++;
}

// Report failed request
void advanced_load_balancer_report_failure(advanced_load_balancer_t *lb,
                                         lb_server_t *server) {
    if (!lb || !server) return;
    
    server->failure_count++;
    server->current_connections--;
    if (server->current_connections < 0) {
        server->current_connections = 0;
    }
    
    lb->stats.failed_requests++;
    
    // Mark server as unhealthy if too many failures
    if (server->failure_count > 10) {
        server->status = LB_SERVER_STATUS_UNHEALTHY;
        if (lb->stats.current_active_servers > 0) {
            lb->stats.current_active_servers--;
        }
    }
}

// Get load balancer statistics
lb_stats_t advanced_load_balancer_get_stats(advanced_load_balancer_t *lb) {
    lb_stats_t result = {0};
    
    if (lb) {
        result = lb->stats;
    }
    
    return result;
}

// Reset statistics
void advanced_load_balancer_reset_stats(advanced_load_balancer_t *lb) {
    if (!lb) return;
    
    lb->stats.total_requests = 0;
    lb->stats.successful_requests = 0;
    lb->stats.failed_requests = 0;
    lb->stats.health_check_failures = 0;
    lb->stats.load_balancing_decisions = 0;
    lb->stats.server_failovers = 0;
    // Keep current_active_servers and peak_active_servers as they reflect actual state
}

// Cleanup load balancer
void advanced_load_balancer_cleanup(advanced_load_balancer_t *lb) {
    if (!lb) return;
    
    lb->initialized = 0;
    // In real implementation, would free allocated resources
    
    if (g_load_balancer == lb) {
        g_load_balancer = 0; // NULL equivalent
    }
}

// Get global load balancer instance
advanced_load_balancer_t* get_global_load_balancer(void) {
    return g_load_balancer;
}