/*
 * adaptive-protocol-manager.h
 * Adaptive Protocol Selection and Management System
 *
 * Dynamically selects and switches between different protocols based on
 * real-time network conditions, performance requirements, and client capabilities.
 */

#ifndef ADAPTIVE_PROTOCOL_MANAGER_H
#define ADAPTIVE_PROTOCOL_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// Protocol types supported
typedef enum {
    PROTOCOL_TYPE_UNKNOWN = 0,
    PROTOCOL_TYPE_MTPROTO_V1 = 1,
    PROTOCOL_TYPE_MTPROTO_V2 = 2,
    PROTOCOL_TYPE_MTPROTO_V3 = 3,
    PROTOCOL_TYPE_HTTP_PROXY = 4,
    PROTOCOL_TYPE_SOCKS5 = 5,
    PROTOCOL_TYPE_SHADOWSOCKS = 6,
    PROTOCOL_TYPE_WEBSOCKET = 7,
    PROTOCOL_TYPE_QUIC = 8,
    PROTOCOL_TYPE_TLS_PROXY = 9,
    PROTOCOL_TYPE_OBLIVIOUS_HTTP = 10
} protocol_type_t;

// Protocol characteristics
typedef enum {
    PROTOCOL_CHARACTERISTIC_LOW_LATENCY = 1,
    PROTOCOL_CHARACTERISTIC_HIGH_THROUGHPUT = 2,
    PROTOCOL_CHARACTERISTIC_ENCRYPTED = 4,
    PROTOCOL_CHARACTERISTIC_COMPRESSED = 8,
    PROTOCOL_CHARACTERISTIC_CONNECTIONLESS = 16,
    PROTOCOL_CHARACTERISTIC_CONNECTION_ORIENTED = 32,
    PROTOCOL_CHARACTERISTIC_RELIABLE = 64,
    PROTOCOL_CHARACTERISTIC_STREAMING = 128
} protocol_characteristic_t;

// Network conditions
typedef struct {
    uint32_t source_ip;
    uint32_t destination_ip;
    uint16_t destination_port;
    double bandwidth_mbps;
    double latency_ms;
    double packet_loss_percent;
    double jitter_ms;
    int mtu_size;
    char network_type[32];  // "wifi", "cellular", "ethernet", "satellite"
    int signal_strength;    // -100 to 0 dBm
    bool is_mobile_network;
    bool is_congested;
    uint64_t timestamp;
} network_conditions_t;

// Protocol performance metrics
typedef struct {
    protocol_type_t protocol_type;
    double average_latency_ms;
    double throughput_mbps;
    double reliability_percent;
    double cpu_utilization_percent;
    double memory_usage_mb;
    uint64_t connection_count;
    uint64_t total_bytes_transferred;
    uint64_t error_count;
    double compression_ratio;
    uint64_t timestamp;
    bool is_active;
} protocol_performance_t;

// Protocol selection criteria
typedef struct {
    double latency_weight;          // 0.0 - 1.0
    double throughput_weight;       // 0.0 - 1.0
    double reliability_weight;      // 0.0 - 1.0
    double cpu_efficiency_weight;   // 0.0 - 1.0
    double security_weight;         // 0.0 - 1.0
    double compatibility_weight;    // 0.0 - 1.0
    double cost_weight;             // 0.0 - 1.0 (bandwidth/power)
    bool prefer_encrypted;
    bool prefer_compressed;
    int max_switch_frequency;       // switches per minute
    double minimum_performance_threshold;
    protocol_characteristic_t required_characteristics;
    protocol_characteristic_t preferred_characteristics;
} protocol_selection_config_t;

// Protocol switching decision
typedef struct {
    protocol_type_t from_protocol;
    protocol_type_t to_protocol;
    uint64_t decision_timestamp;
    double confidence_score;        // 0.0 - 100.0
    double expected_improvement;    // percentage improvement
    char reason[256];
    network_conditions_t conditions;
    protocol_performance_t current_performance;
    protocol_performance_t predicted_performance;
    bool emergency_switch;          // immediate switch due to critical failure
} protocol_switch_decision_t;

// Protocol adaptation statistics
typedef struct {
    uint64_t total_switches;
    uint64_t successful_switches;
    uint64_t failed_switches;
    uint64_t emergency_switches;
    uint64_t performance_improvements;
    uint64_t performance_degradations;
    double average_improvement_percent;
    double average_switch_time_ms;
    uint64_t last_switch_timestamp;
    protocol_type_t current_protocol;
    uint64_t current_protocol_duration_ms;
    uint64_t protocol_usage_count[16];  // usage count per protocol type
} protocol_adaptation_stats_t;

// Main protocol manager context
typedef struct {
    // Configuration
    protocol_selection_config_t config;
    
    // Current state
    protocol_type_t current_protocol;
    bool adaptation_enabled;
    bool learning_mode;
    uint64_t last_switch_time;
    uint64_t switch_cooldown_period_ms;
    
    // Performance data
    protocol_performance_t protocol_performance[16];
    network_conditions_t current_conditions;
    protocol_performance_t baseline_performance;
    
    // Statistics
    protocol_adaptation_stats_t stats;
    
    // Learning and prediction
    double performance_history[16][1000]; // performance metrics over time
    int history_index[16];
    double protocol_efficiency_scores[16];
    double adaptation_model_weights[16][8]; // ML model weights
    
    // Client information
    uint64_t client_id;
    char client_capabilities[256];
    bool client_supports_protocol[16];
    
    // Emergency handling
    bool emergency_mode;
    protocol_type_t emergency_protocol;
    uint64_t emergency_start_time;
    double emergency_threshold;
    
    // State management
    int initialized;
    int active;
    uint64_t start_time;
} adaptive_protocol_manager_t;

// Callback function types
typedef void (*protocol_switch_callback_t)(const protocol_switch_decision_t* decision);
typedef void (*protocol_performance_callback_t)(protocol_type_t protocol, const protocol_performance_t* performance);
typedef void (*emergency_switch_callback_t)(protocol_type_t from, protocol_type_t to, const char* reason);

// Function declarations

// Initialization and cleanup
int init_adaptive_protocol_manager(adaptive_protocol_manager_t* manager);
int init_adaptive_protocol_manager_with_config(adaptive_protocol_manager_t* manager, 
                                             const protocol_selection_config_t* config);
void cleanup_adaptive_protocol_manager(adaptive_protocol_manager_t* manager);

// Configuration management
void get_protocol_selection_config(adaptive_protocol_manager_t* manager, protocol_selection_config_t* config);
int set_protocol_selection_config(adaptive_protocol_manager_t* manager, const protocol_selection_config_t* config);
int set_protocol_characteristics(adaptive_protocol_manager_t* manager, protocol_type_t protocol, 
                               protocol_characteristic_t characteristics);
protocol_characteristic_t get_protocol_characteristics(adaptive_protocol_manager_t* manager, protocol_type_t protocol);

// Protocol management
int register_protocol_support(adaptive_protocol_manager_t* manager, protocol_type_t protocol, bool supported);
bool is_protocol_supported(adaptive_protocol_manager_t* manager, protocol_type_t protocol);
int set_client_capabilities(adaptive_protocol_manager_t* manager, uint64_t client_id, const char* capabilities);
int update_client_protocol_support(adaptive_protocol_manager_t* manager, uint64_t client_id, 
                                 protocol_type_t protocol, bool supported);

// Performance monitoring
int update_network_conditions(adaptive_protocol_manager_t* manager, const network_conditions_t* conditions);
int update_protocol_performance(adaptive_protocol_manager_t* manager, protocol_type_t protocol,
                              const protocol_performance_t* performance);
protocol_performance_t get_current_protocol_performance(adaptive_protocol_manager_t* manager);
int collect_performance_metrics(adaptive_protocol_manager_t* manager, protocol_type_t protocol,
                              protocol_performance_t* performance);

// Protocol selection and adaptation
protocol_type_t select_optimal_protocol(adaptive_protocol_manager_t* manager);
int request_protocol_switch(adaptive_protocol_manager_t* manager, protocol_type_t target_protocol);
int force_protocol_switch(adaptive_protocol_manager_t* manager, protocol_type_t target_protocol, 
                        const char* reason);
protocol_switch_decision_t evaluate_protocol_switch(adaptive_protocol_manager_t* manager);
int execute_protocol_switch(adaptive_protocol_manager_t* manager, const protocol_switch_decision_t* decision);

// Emergency handling
int enter_emergency_mode(adaptive_protocol_manager_t* manager, protocol_type_t emergency_protocol);
int exit_emergency_mode(adaptive_protocol_manager_t* manager);
bool is_emergency_mode_active(adaptive_protocol_manager_t* manager);
int handle_protocol_failure(adaptive_protocol_manager_t* manager, protocol_type_t failed_protocol,
                          const char* failure_reason);

// Learning and optimization
int update_adaptation_model(adaptive_protocol_manager_t* manager);
int train_performance_prediction_model(adaptive_protocol_manager_t* manager);
double predict_protocol_performance(adaptive_protocol_manager_t* manager, protocol_type_t protocol,
                                  const network_conditions_t* conditions);
int enable_learning_mode(adaptive_protocol_manager_t* manager, bool enable);
int reset_performance_history(adaptive_protocol_manager_t* manager);

// Statistics and reporting
void get_protocol_adaptation_stats(adaptive_protocol_manager_t* manager, protocol_adaptation_stats_t* stats);
protocol_performance_t get_protocol_performance_history(adaptive_protocol_manager_t* manager, 
                                                      protocol_type_t protocol, int history_index);
double get_protocol_efficiency_score(adaptive_protocol_manager_t* manager, protocol_type_t protocol);
int get_supported_protocols(adaptive_protocol_manager_t* manager, protocol_type_t* protocols, int max_count);

// Callback registration
void register_protocol_switch_callback(protocol_switch_callback_t callback);
void register_protocol_performance_callback(protocol_performance_callback_t callback);
void register_emergency_switch_callback(emergency_switch_callback_t callback);

// Integration functions
int integrate_with_network_analyzer(adaptive_protocol_manager* manager);
int integrate_with_performance_monitor(adaptive_protocol_manager* manager);
int apply_protocol_optimizations(adaptive_protocol_manager* manager);
int verify_protocol_integrity(adaptive_protocol_manager* manager);

// Utility functions
const char* protocol_type_to_string(protocol_type_t protocol);
protocol_type_t string_to_protocol_type(const char* protocol_str);
const char* protocol_switch_reason_to_string(const protocol_switch_decision_t* decision);
double calculate_protocol_score(adaptive_protocol_manager* manager, protocol_type_t protocol,
                              const network_conditions_t* conditions);

#endif // ADAPTIVE_PROTOCOL_MANAGER_H