/*
 * adaptive-protocol-manager.c
 * Adaptive Protocol Selection and Management System Implementation
 */

#include "adaptive-protocol-manager.h"

// Simple implementations for standard library functions
static void* simple_malloc(size_t size) {
    static char heap[4096*1024]; // 4MB heap
    static size_t heap_offset = 0;
    
    if (heap_offset + size > sizeof(heap)) {
        return 0; // NULL equivalent
    }
    
    void *ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

static void simple_free(void *ptr) {
    // In real implementation, this would properly free memory
}

static void simple_memset(void *ptr, int value, size_t num) {
    char *p = (char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (char)value;
    }
}

static int simple_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

static size_t simple_strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

// Global context and callbacks
static adaptive_protocol_manager_t* g_protocol_manager = 0;
static protocol_switch_callback_t g_switch_callback = 0;
static protocol_performance_callback_t g_performance_callback = 0;
static emergency_switch_callback_t g_emergency_callback = 0;

// Get current timestamp (simple simulation)
static uint64_t get_current_timestamp_ms(void) {
    static uint64_t base_time = 1000000;
    base_time += 100; // Increment by 100ms each call
    return base_time;
}

// Initialize the adaptive protocol manager
int init_adaptive_protocol_manager(adaptive_protocol_manager_t* manager) {
    if (!manager) return -1;
    
    // Zero initialize
    simple_memset(manager, 0, sizeof(adaptive_protocol_manager_t));
    
    // Default configuration
    protocol_selection_config_t default_config = {
        .latency_weight = 0.3,
        .throughput_weight = 0.25,
        .reliability_weight = 0.2,
        .cpu_efficiency_weight = 0.1,
        .security_weight = 0.1,
        .compatibility_weight = 0.03,
        .cost_weight = 0.02,
        .prefer_encrypted = 1,
        .prefer_compressed = 0,
        .max_switch_frequency = 10,
        .minimum_performance_threshold = 50.0,
        .required_characteristics = PROTOCOL_CHARACTERISTIC_RELIABLE | PROTOCOL_CHARACTERISTIC_ENCRYPTED,
        .preferred_characteristics = PROTOCOL_CHARACTERISTIC_LOW_LATENCY | PROTOCOL_CHARACTERISTIC_COMPRESSED
    };
    
    return init_adaptive_protocol_manager_with_config(manager, &default_config);
}

// Initialize with custom configuration
int init_adaptive_protocol_manager_with_config(adaptive_protocol_manager_t* manager, 
                                             const protocol_selection_config_t* config) {
    if (!manager) return -1;
    
    // Zero initialize
    simple_memset(manager, 0, sizeof(adaptive_protocol_manager_t));
    
    // Apply configuration
    if (config) {
        manager->config = *config;
    }
    
    // Initialize default values
    manager->current_protocol = PROTOCOL_TYPE_MTPROTO_V3;
    manager->adaptation_enabled = 1;
    manager->learning_mode = 1;
    manager->last_switch_time = 0;
    manager->switch_cooldown_period_ms = 5000; // 5 seconds
    
    // Initialize protocol characteristics
    for (int i = 0; i < 16; i++) {
        manager->client_supports_protocol[i] = 1; // Assume all protocols supported by default
    }
    
    // Set baseline performance
    manager->baseline_performance.protocol_type = PROTOCOL_TYPE_MTPROTO_V3;
    manager->baseline_performance.average_latency_ms = 50.0;
    manager->baseline_performance.throughput_mbps = 100.0;
    manager->baseline_performance.reliability_percent = 99.5;
    manager->baseline_performance.cpu_utilization_percent = 30.0;
    manager->baseline_performance.is_active = 1;
    
    // Initialize current conditions
    manager->current_conditions.bandwidth_mbps = 100.0;
    manager->current_conditions.latency_ms = 25.0;
    manager->current_conditions.packet_loss_percent = 0.1;
    manager->current_conditions.jitter_ms = 5.0;
    manager->current_conditions.mtu_size = 1500;
    manager->current_conditions.is_mobile_network = 0;
    manager->current_conditions.is_congested = 0;
    
    // Initialize protocol performance data
    for (int i = 0; i < 16; i++) {
        manager->protocol_performance[i].protocol_type = (protocol_type_t)i;
        manager->protocol_performance[i].is_active = 0;
        manager->protocol_efficiency_scores[i] = 50.0; // Default 50% efficiency
    }
    
    // Initialize statistics
    manager->stats.current_protocol = PROTOCOL_TYPE_MTPROTO_V3;
    manager->stats.current_protocol_duration_ms = 0;
    
    // Mark as initialized
    manager->initialized = 1;
    manager->active = 1;
    manager->start_time = get_current_timestamp_ms();
    
    g_protocol_manager = manager;
    return 0;
}

// Cleanup the protocol manager
void cleanup_adaptive_protocol_manager(adaptive_protocol_manager_t* manager) {
    if (!manager) return;
    
    // Reset all fields
    simple_memset(manager, 0, sizeof(adaptive_protocol_manager_t));
    manager->initialized = 0;
    
    if (g_protocol_manager == manager) {
        g_protocol_manager = 0;
    }
}

// Update network conditions
int update_network_conditions(adaptive_protocol_manager_t* manager, const network_conditions_t* conditions) {
    if (!manager || !manager->initialized || !conditions) return -1;
    
    manager->current_conditions = *conditions;
    manager->current_conditions.timestamp = get_current_timestamp_ms();
    
    // Update protocol performance based on new conditions
    for (int i = 0; i < 16; i++) {
        if (manager->protocol_performance[i].is_active) {
            // Adjust performance metrics based on network conditions
            double condition_factor = 1.0;
            
            // Latency impact
            if (conditions->latency_ms > 100) {
                condition_factor *= 0.8; // 20% performance degradation
            } else if (conditions->latency_ms > 50) {
                condition_factor *= 0.9; // 10% performance degradation
            }
            
            // Packet loss impact
            if (conditions->packet_loss_percent > 5.0) {
                condition_factor *= 0.7; // 30% performance degradation
            } else if (conditions->packet_loss_percent > 1.0) {
                condition_factor *= 0.9; // 10% performance degradation
            }
            
            // Apply condition factor
            manager->protocol_performance[i].average_latency_ms *= (1.0 + (1.0 - condition_factor));
            manager->protocol_performance[i].throughput_mbps *= condition_factor;
            manager->protocol_performance[i].reliability_percent *= condition_factor;
        }
    }
    
    return 0;
}

// Update protocol performance metrics
int update_protocol_performance(adaptive_protocol_manager_t* manager, protocol_type_t protocol,
                              const protocol_performance_t* performance) {
    if (!manager || !manager->initialized || !performance) return -1;
    
    if (protocol >= 16) return -1;
    
    manager->protocol_performance[protocol] = *performance;
    manager->protocol_performance[protocol].timestamp = get_current_timestamp_ms();
    manager->protocol_performance[protocol].is_active = 1;
    
    // Update efficiency score
    double score = 0.0;
    score += (100.0 - performance->average_latency_ms) * manager->config.latency_weight;
    score += performance->throughput_mbps * manager->config.throughput_weight;
    score += performance->reliability_percent * manager->config.reliability_weight;
    score += (100.0 - performance->cpu_utilization_percent) * manager->config.cpu_efficiency_weight;
    
    manager->protocol_efficiency_scores[protocol] = score;
    
    // Call performance callback
    if (g_performance_callback) {
        g_performance_callback(protocol, performance);
    }
    
    return 0;
}

// Select optimal protocol based on current conditions
protocol_type_t select_optimal_protocol(adaptive_protocol_manager_t* manager) {
    if (!manager || !manager->initialized) return PROTOCOL_TYPE_UNKNOWN;
    
    protocol_type_t best_protocol = PROTOCOL_TYPE_MTPROTO_V3;
    double best_score = 0.0;
    
    // Evaluate each supported protocol
    for (int i = 1; i < 11; i++) { // Skip UNKNOWN and evaluate valid protocols
        if (!manager->client_supports_protocol[i]) continue;
        
        double score = calculate_protocol_score(manager, (protocol_type_t)i, &manager->current_conditions);
        
        // Apply minimum threshold
        if (score >= manager->config.minimum_performance_threshold && score > best_score) {
            best_score = score;
            best_protocol = (protocol_type_t)i;
        }
    }
    
    return best_protocol;
}

// Calculate protocol score based on conditions and weights
double calculate_protocol_score(adaptive_protocol_manager_t* manager, protocol_type_t protocol,
                              const network_conditions_t* conditions) {
    if (!manager || !conditions) return 0.0;
    
    // Base score from historical performance
    double score = manager->protocol_efficiency_scores[protocol];
    
    // Adjust for current network conditions
    if (conditions->is_mobile_network) {
        // Mobile networks prefer protocols optimized for variable conditions
        if (protocol == PROTOCOL_TYPE_WEBSOCKET || protocol == PROTOCOL_TYPE_QUIC) {
            score *= 1.2; // 20% bonus for mobile-optimized protocols
        }
    }
    
    if (conditions->is_congested) {
        // Congested networks prefer efficient protocols
        if (protocol == PROTOCOL_TYPE_MTPROTO_V3 || protocol == PROTOCOL_TYPE_QUIC) {
            score *= 1.1; // 10% bonus for congestion-optimized protocols
        }
    }
    
    // Apply configuration weights
    score *= (manager->config.latency_weight + manager->config.throughput_weight + 
              manager->config.reliability_weight + manager->config.cpu_efficiency_weight);
    
    return score;
}

// Evaluate protocol switch decision
protocol_switch_decision_t evaluate_protocol_switch(adaptive_protocol_manager_t* manager) {
    protocol_switch_decision_t decision = {0};
    
    if (!manager || !manager->initialized) return decision;
    
    protocol_type_t optimal_protocol = select_optimal_protocol(manager);
    protocol_type_t current_protocol = manager->current_protocol;
    
    decision.from_protocol = current_protocol;
    decision.to_protocol = optimal_protocol;
    decision.decision_timestamp = get_current_timestamp_ms();
    decision.confidence_score = 85.0; // Default confidence
    
    // Check if switch is needed and allowed
    if (optimal_protocol != current_protocol) {
        uint64_t time_since_last_switch = decision.decision_timestamp - manager->last_switch_time;
        
        if (time_since_last_switch >= manager->switch_cooldown_period_ms) {
            // Calculate expected improvement
            double current_score = manager->protocol_efficiency_scores[current_protocol];
            double optimal_score = manager->protocol_efficiency_scores[optimal_protocol];
            
            if (optimal_score > current_score) {
                decision.expected_improvement = ((optimal_score - current_score) / current_score) * 100.0;
                
                // Set reason
                int reason_len = 0;
                const char* reason = "Performance optimization based on current network conditions";
                while (reason_len < 250 && reason[reason_len]) {
                    decision.reason[reason_len] = reason[reason_len];
                    reason_len++;
                }
                decision.reason[reason_len] = '\0';
                
                decision.conditions = manager->current_conditions;
                decision.current_performance = manager->protocol_performance[current_protocol];
            }
        }
    }
    
    return decision;
}

// Execute protocol switch
int execute_protocol_switch(adaptive_protocol_manager_t* manager, const protocol_switch_decision_t* decision) {
    if (!manager || !manager->initialized || !decision) return -1;
    
    if (decision->to_protocol == decision->from_protocol) {
        return 0; // No switch needed
    }
    
    // Check cooldown period
    uint64_t current_time = get_current_timestamp_ms();
    if (current_time - manager->last_switch_time < manager->switch_cooldown_period_ms) {
        return -1; // Too soon to switch
    }
    
    // Perform the switch
    protocol_type_t old_protocol = manager->current_protocol;
    manager->current_protocol = decision->to_protocol;
    manager->last_switch_time = current_time;
    
    // Update statistics
    manager->stats.total_switches++;
    manager->stats.last_switch_timestamp = current_time;
    manager->stats.current_protocol = decision->to_protocol;
    manager->stats.current_protocol_duration_ms = 0;
    manager->stats.protocol_usage_count[decision->to_protocol]++;
    
    // Update protocol duration
    if (old_protocol < 16) {
        manager->stats.protocol_usage_count[old_protocol] += 
            (current_time - manager->start_time);
    }
    
    // Call switch callback
    if (g_switch_callback) {
        g_switch_callback(decision);
    }
    
    return 0;
}

// Utility functions
const char* protocol_type_to_string(protocol_type_t protocol) {
    switch (protocol) {
        case PROTOCOL_TYPE_MTPROTO_V1: return "MTProto v1";
        case PROTOCOL_TYPE_MTPROTO_V2: return "MTProto v2";
        case PROTOCOL_TYPE_MTPROTO_V3: return "MTProto v3";
        case PROTOCOL_TYPE_HTTP_PROXY: return "HTTP Proxy";
        case PROTOCOL_TYPE_SOCKS5: return "SOCKS5";
        case PROTOCOL_TYPE_SHADOWSOCKS: return "Shadowsocks";
        case PROTOCOL_TYPE_WEBSOCKET: return "WebSocket";
        case PROTOCOL_TYPE_QUIC: return "QUIC";
        case PROTOCOL_TYPE_TLS_PROXY: return "TLS Proxy";
        case PROTOCOL_TYPE_OBLIVIOUS_HTTP: return "Oblivious HTTP";
        default: return "Unknown";
    }
}

protocol_type_t string_to_protocol_type(const char* protocol_str) {
    if (!protocol_str) return PROTOCOL_TYPE_UNKNOWN;
    
    if (simple_strcmp(protocol_str, "MTProto v1") == 0) return PROTOCOL_TYPE_MTPROTO_V1;
    if (simple_strcmp(protocol_str, "MTProto v2") == 0) return PROTOCOL_TYPE_MTPROTO_V2;
    if (simple_strcmp(protocol_str, "MTProto v3") == 0) return PROTOCOL_TYPE_MTPROTO_V3;
    if (simple_strcmp(protocol_str, "HTTP Proxy") == 0) return PROTOCOL_TYPE_HTTP_PROXY;
    if (simple_strcmp(protocol_str, "SOCKS5") == 0) return PROTOCOL_TYPE_SOCKS5;
    if (simple_strcmp(protocol_str, "Shadowsocks") == 0) return PROTOCOL_TYPE_SHADOWSOCKS;
    if (simple_strcmp(protocol_str, "WebSocket") == 0) return PROTOCOL_TYPE_WEBSOCKET;
    if (simple_strcmp(protocol_str, "QUIC") == 0) return PROTOCOL_TYPE_QUIC;
    if (simple_strcmp(protocol_str, "TLS Proxy") == 0) return PROTOCOL_TYPE_TLS_PROXY;
    if (simple_strcmp(protocol_str, "Oblivious HTTP") == 0) return PROTOCOL_TYPE_OBLIVIOUS_HTTP;
    
    return PROTOCOL_TYPE_UNKNOWN;
}

// Callback registration
void register_protocol_switch_callback(protocol_switch_callback_t callback) {
    g_switch_callback = callback;
}

void register_protocol_performance_callback(protocol_performance_callback_t callback) {
    g_performance_callback = callback;
}

void register_emergency_switch_callback(emergency_switch_callback_t callback) {
    g_emergency_callback = callback;
}

// Get current protocol performance
protocol_performance_t get_current_protocol_performance(adaptive_protocol_manager_t* manager) {
    protocol_performance_t empty_performance = {0};
    
    if (!manager || !manager->initialized) return empty_performance;
    
    return manager->protocol_performance[manager->current_protocol];
}

// Get protocol adaptation statistics
void get_protocol_adaptation_stats(adaptive_protocol_manager_t* manager, protocol_adaptation_stats_t* stats) {
    if (!manager || !manager->initialized || !stats) return;
    
    *stats = manager->stats;
}