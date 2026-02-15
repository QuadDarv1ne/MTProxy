/*
 * threat-detector.h
 * Advanced Threat Detection System for MTProxy
 *
 * This system provides real-time threat detection capabilities including
 * DDoS protection, anomaly detection, and malicious traffic identification.
 */

#ifndef THREAT_DETECTOR_H
#define THREAT_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum thresholds for threat detection
#define MAX_CONNECTIONS_PER_IP 1000
#define MAX_REQUESTS_PER_SECOND 10000
#define MAX_PACKET_SIZE 65536
#define THREAT_HISTORY_SIZE 10000

// Threat types
typedef enum {
    THREAT_TYPE_UNKNOWN = 0,
    THREAT_TYPE_DDOS,
    THREAT_TYPE_PORT_SCAN,
    THREAT_TYPE_MALICIOUS_TRAFFIC,
    THREAT_TYPE_ANOMALY,
    THREAT_TYPE_RATE_LIMIT_EXCEEDED,
    THREAT_TYPE_SUSPICIOUS_BEHAVIOR
} threat_type_t;

// Threat severity levels
typedef enum {
    THREAT_SEVERITY_LOW = 0,
    THREAT_SEVERITY_MEDIUM,
    THREAT_SEVERITY_HIGH,
    THREAT_SEVERITY_CRITICAL
} threat_severity_t;

// Connection information structure
typedef struct {
    uint32_t ip_address;  // IPv4 address
    uint16_t port;
    uint64_t connection_time;
    uint64_t last_activity;
    uint64_t bytes_transferred;
    uint64_t packets_count;
    int request_count;
    bool is_blocked;
    int threat_score;  // 0-100 scale
} connection_info_t;

// Threat detection statistics
typedef struct {
    uint64_t total_connections_analyzed;
    uint64_t threats_detected;
    uint64_t ddos_attacks_blocked;
    uint64_t malicious_connections_blocked;
    uint64_t false_positives;
    uint64_t legitimate_connections_allowed;
    double detection_accuracy;
    double false_positive_rate;
} threat_stats_t;

// Threat detection configuration
typedef struct {
    int enable_ddos_detection;
    int enable_anomaly_detection;
    int enable_rate_limiting;
    int enable_behavioral_analysis;
    int enable_machine_learning;
    int connection_timeout_seconds;
    int max_connections_per_ip;
    int max_requests_per_second;
    int threat_threshold_score;
    int auto_block_threshold;
    int learning_mode_duration_seconds;
    bool enable_logging;
    bool enable_alerts;
} threat_config_t;

// Threat detection context
typedef struct {
    threat_config_t config;
    threat_stats_t stats;
    connection_info_t connections[65536];  // Hash table for connections
    int connection_count;
    uint64_t last_cleanup_time;
    bool is_learning_mode;
    uint64_t learning_start_time;
    int threat_history[THREAT_HISTORY_SIZE];
    int threat_history_index;
} threat_detector_ctx_t;

// Threat alert structure
typedef struct {
    uint64_t alert_id;
    threat_type_t threat_type;
    threat_severity_t severity;
    uint32_t source_ip;
    uint16_t source_port;
    uint64_t timestamp;
    char description[256];
    bool acknowledged;
    int threat_score;
} threat_alert_t;

// Callback function types
typedef void (*threat_alert_callback_t)(const threat_alert_t* alert);
typedef void (*connection_blocked_callback_t)(const connection_info_t* conn);
typedef void (*threat_stats_callback_t)(const threat_stats_t* stats);

// Function prototypes

// Initialization and cleanup
int init_threat_detector(threat_detector_ctx_t* ctx);
int init_threat_detector_with_config(threat_detector_ctx_t* ctx, const threat_config_t* config);
void cleanup_threat_detector(threat_detector_ctx_t* ctx);

// Connection analysis
int analyze_connection(threat_detector_ctx_t* ctx, uint32_t ip, uint16_t port, 
                      uint64_t bytes_transferred, int packet_count);
int analyze_packet(threat_detector_ctx_t* ctx, uint32_t source_ip, uint32_t dest_ip,
                  uint16_t source_port, uint16_t dest_port, uint32_t packet_size);
bool is_connection_suspicious(threat_detector_ctx_t* ctx, uint32_t ip, uint16_t port);

// Threat detection functions
int detect_ddos_attack(threat_detector_ctx_t* ctx, uint32_t target_ip);
int detect_port_scanning(threat_detector_ctx_t* ctx, uint32_t source_ip);
int detect_anomalous_traffic(threat_detector_ctx_t* ctx, uint32_t ip, uint64_t traffic_volume);
int detect_malicious_patterns(threat_detector_ctx_t* ctx, const uint8_t* data, size_t data_len);

// Blocking and mitigation
int block_ip(threat_detector_ctx_t* ctx, uint32_t ip, const char* reason);
int unblock_ip(threat_detector_ctx_t* ctx, uint32_t ip);
bool is_ip_blocked(threat_detector_ctx_t* ctx, uint32_t ip);
int get_blocked_ips(threat_detector_ctx_t* ctx, uint32_t* ip_list, int max_count);

// Alert management
int generate_threat_alert(threat_detector_ctx_t* ctx, threat_type_t type, 
                         threat_severity_t severity, uint32_t source_ip, 
                         const char* description, int threat_score);
threat_alert_t* get_unacknowledged_alerts(threat_detector_ctx_t* ctx, int* count);
int acknowledge_alert(threat_detector_ctx_t* ctx, uint64_t alert_id);

// Statistics and reporting
threat_stats_t get_threat_statistics(threat_detector_ctx_t* ctx);
void reset_threat_statistics(threat_detector_ctx_t* ctx);
void print_threat_report(threat_detector_ctx_t* ctx);

// Configuration management
void get_threat_config(threat_detector_ctx_t* ctx, threat_config_t* config);
int set_threat_config(threat_detector_ctx_t* ctx, const threat_config_t* config);
int load_threat_config(threat_config_t* config, const char* config_file);
int save_threat_config(const threat_config_t* config, const char* config_file);

// Learning and adaptation
int enable_learning_mode(threat_detector_ctx_t* ctx, int duration_seconds);
int disable_learning_mode(threat_detector_ctx_t* ctx);
bool is_learning_mode_active(threat_detector_ctx_t* ctx);
int update_threat_model(threat_detector_ctx_t* ctx);
double get_current_threat_level(threat_detector_ctx_t* ctx);

// Utility functions
const char* threat_type_to_string(threat_type_t type);
const char* threat_severity_to_string(threat_severity_t severity);
uint32_t ip_string_to_uint32(const char* ip_string);
void uint32_to_ip_string(uint32_t ip, char* ip_string, size_t buffer_size);
uint64_t get_current_timestamp_ms(void);

// Callback registration
void register_threat_alert_callback(threat_alert_callback_t callback);
void register_connection_blocked_callback(connection_blocked_callback_t callback);
void register_threat_stats_callback(threat_stats_callback_t callback);

// Integration functions
int integrate_with_network_layer(threat_detector_ctx_t* ctx);
int apply_threat_protection(threat_detector_ctx_t* ctx);
int verify_threat_detection(threat_detector_ctx_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // THREAT_DETECTOR_H