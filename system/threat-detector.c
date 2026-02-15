/*
 * threat-detector.c
 * Advanced Threat Detection System Implementation
 */

#include "threat-detector.h"

// Global context and callbacks
static threat_detector_ctx_t* g_threat_ctx = 0;
static threat_alert_callback_t g_alert_callback = 0;
static connection_blocked_callback_t g_blocked_callback = 0;
static threat_stats_callback_t g_stats_callback = 0;

// Simple time function
static uint64_t get_timestamp_ms_internal(void) {
    // Simple implementation - return incremented counter
    static uint64_t counter = 1000000;
    return counter++;
}

// Simple hash function for IP addresses
static int hash_ip(uint32_t ip) {
    return (int)(ip % 65536);
}

// String utility functions
static int simple_strcmp(const char* s1, const char* s2) {
    if (!s1 || !s2) return -1;
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int simple_strlen(const char* s) {
    if (!s) return 0;
    int len = 0;
    while (*s++) len++;
    return len;
}

// Threat type to string conversion
const char* threat_type_to_string(threat_type_t type) {
    switch (type) {
        case THREAT_TYPE_UNKNOWN: return "UNKNOWN";
        case THREAT_TYPE_DDOS: return "DDOS";
        case THREAT_TYPE_PORT_SCAN: return "PORT_SCAN";
        case THREAT_TYPE_MALICIOUS_TRAFFIC: return "MALICIOUS_TRAFFIC";
        case THREAT_TYPE_ANOMALY: return "ANOMALY";
        case THREAT_TYPE_RATE_LIMIT_EXCEEDED: return "RATE_LIMIT_EXCEEDED";
        case THREAT_TYPE_SUSPICIOUS_BEHAVIOR: return "SUSPICIOUS_BEHAVIOR";
        default: return "INVALID";
    }
}

const char* threat_severity_to_string(threat_severity_t severity) {
    switch (severity) {
        case THREAT_SEVERITY_LOW: return "LOW";
        case THREAT_SEVERITY_MEDIUM: return "MEDIUM";
        case THREAT_SEVERITY_HIGH: return "HIGH";
        case THREAT_SEVERITY_CRITICAL: return "CRITICAL";
        default: return "INVALID";
    }
}

// Initialization functions
int init_threat_detector(threat_detector_ctx_t* ctx) {
    if (!ctx) return -1;
    
    // Initialize with default configuration
    threat_config_t default_config = {
        .enable_ddos_detection = 1,
        .enable_anomaly_detection = 1,
        .enable_rate_limiting = 1,
        .enable_behavioral_analysis = 1,
        .enable_machine_learning = 0,
        .connection_timeout_seconds = 300,
        .max_connections_per_ip = MAX_CONNECTIONS_PER_IP,
        .max_requests_per_second = MAX_REQUESTS_PER_SECOND,
        .threat_threshold_score = 70,
        .auto_block_threshold = 90,
        .learning_mode_duration_seconds = 3600,
        .enable_logging = 1,
        .enable_alerts = 1
    };
    
    return init_threat_detector_with_config(ctx, &default_config);
}

int init_threat_detector_with_config(threat_detector_ctx_t* ctx, const threat_config_t* config) {
    if (!ctx || !config) return -1;
    
    // Initialize context
    ctx->config = *config;
    ctx->connection_count = 0;
    ctx->last_cleanup_time = get_timestamp_ms_internal();
    ctx->is_learning_mode = 0;
    ctx->learning_start_time = 0;
    ctx->threat_history_index = 0;
    
    // Initialize statistics
    ctx->stats.total_connections_analyzed = 0;
    ctx->stats.threats_detected = 0;
    ctx->stats.ddos_attacks_blocked = 0;
    ctx->stats.malicious_connections_blocked = 0;
    ctx->stats.false_positives = 0;
    ctx->stats.legitimate_connections_allowed = 0;
    ctx->stats.detection_accuracy = 0.0;
    ctx->stats.false_positive_rate = 0.0;
    
    // Clear connections array
    for (int i = 0; i < 65536; i++) {
        ctx->connections[i].ip_address = 0;
        ctx->connections[i].is_blocked = 0;
        ctx->connections[i].threat_score = 0;
    }
    
    g_threat_ctx = ctx;
    return 0;
}

void cleanup_threat_detector(threat_detector_ctx_t* ctx) {
    if (!ctx) return;
    
    // Clear all connections
    ctx->connection_count = 0;
    
    if (g_threat_ctx == ctx) {
        g_threat_ctx = 0;
    }
}

// Connection analysis functions
int analyze_connection(threat_detector_ctx_t* ctx, uint32_t ip, uint16_t port, 
                      uint64_t bytes_transferred, int packet_count) {
    if (!ctx) return -1;
    
    int hash_index = hash_ip(ip);
    connection_info_t* conn = &ctx->connections[hash_index];
    
    // Update connection statistics
    conn->ip_address = ip;
    conn->port = port;
    conn->last_activity = get_timestamp_ms_internal();
    conn->bytes_transferred += bytes_transferred;
    conn->packets_count += packet_count;
    conn->request_count++;
    
    if (conn->connection_time == 0) {
        conn->connection_time = conn->last_activity;
        ctx->connection_count++;
    }
    
    ctx->stats.total_connections_analyzed++;
    
    // Calculate threat score based on various factors
    int threat_score = 0;
    
    // Check connection rate
    if (conn->request_count > ctx->config.max_requests_per_second) {
        threat_score += 30;
    }
    
    // Check connection count per IP
    int ip_connection_count = 0;
    for (int i = 0; i < 65536; i++) {
        if (ctx->connections[i].ip_address == ip && ctx->connections[i].ip_address != 0) {
            ip_connection_count++;
        }
    }
    
    if (ip_connection_count > ctx->config.max_connections_per_ip) {
        threat_score += 25;
    }
    
    // Check for suspicious behavior
    uint64_t connection_duration = conn->last_activity - conn->connection_time;
    if (connection_duration > 0) {
        double bytes_per_second = (double)conn->bytes_transferred / (connection_duration / 1000.0);
        if (bytes_per_second > 1000000) { // 1 MB/s threshold
            threat_score += 20;
        }
    }
    
    conn->threat_score = threat_score;
    
    // Generate alert if threat score is high
    if (threat_score >= ctx->config.threat_threshold_score && !conn->is_blocked) {
        char description[256];
        // Simple description
        int desc_len = 0;
        const char* threat_desc = "High threat score detected";
        while (desc_len < 250 && threat_desc[desc_len]) {
            description[desc_len] = threat_desc[desc_len];
            desc_len++;
        }
        description[desc_len] = '\0';
        
        generate_threat_alert(ctx, THREAT_TYPE_SUSPICIOUS_BEHAVIOR, 
                            threat_score > 85 ? THREAT_SEVERITY_HIGH : THREAT_SEVERITY_MEDIUM,
                            ip, description, threat_score);
        
        ctx->stats.threats_detected++;
        
        // Auto-block if score is very high
        if (threat_score >= ctx->config.auto_block_threshold) {
            block_ip(ctx, ip, "Auto-blocked due to high threat score");
        }
    }
    
    return threat_score;
}

int analyze_packet(threat_detector_ctx_t* ctx, uint32_t source_ip, uint32_t dest_ip,
                  uint16_t source_port, uint16_t dest_port, uint32_t packet_size) {
    if (!ctx) return -1;
    
    // Basic packet analysis
    int threat_score = 0;
    
    // Check for oversized packets
    if (packet_size > MAX_PACKET_SIZE) {
        threat_score += 15;
    }
    
    // Check for port scanning patterns (simplified)
    static int port_scan_counter[65536] = {0};
    int dest_port_hash = dest_port % 65536;
    port_scan_counter[dest_port_hash]++;
    
    if (port_scan_counter[dest_port_hash] > 100) {
        threat_score += 25;
        // Generate port scan alert
        char description[256];
        int desc_len = 0;
        const char* scan_desc = "Potential port scanning detected";
        while (desc_len < 250 && scan_desc[desc_len]) {
            description[desc_len] = scan_desc[desc_len];
            desc_len++;
        }
        description[desc_len] = '\0';
        
        generate_threat_alert(ctx, THREAT_TYPE_PORT_SCAN, THREAT_SEVERITY_MEDIUM,
                            source_ip, description, threat_score);
    }
    
    // Analyze the connection associated with this packet
    return analyze_connection(ctx, source_ip, source_port, packet_size, 1);
}

bool is_connection_suspicious(threat_detector_ctx_t* ctx, uint32_t ip, uint16_t port) {
    if (!ctx) return 0;
    
    int hash_index = hash_ip(ip);
    connection_info_t* conn = &ctx->connections[hash_index];
    
    return (conn->ip_address == ip && conn->threat_score >= ctx->config.threat_threshold_score);
}

// Threat detection functions
int detect_ddos_attack(threat_detector_ctx_t* ctx, uint32_t target_ip) {
    if (!ctx) return -1;
    
    // Simple DDoS detection based on connection count
    int connection_count = 0;
    for (int i = 0; i < 65536; i++) {
        if (ctx->connections[i].ip_address != 0) {
            connection_count++;
        }
    }
    
    if (connection_count > 5000) { // High connection threshold
        ctx->stats.ddos_attacks_blocked++;
        return 1; // DDoS detected
    }
    
    return 0; // No DDoS detected
}

int detect_port_scanning(threat_detector_ctx_t* ctx, uint32_t source_ip) {
    // This is handled in the packet analysis function
    return 0;
}

int detect_anomalous_traffic(threat_detector_ctx_t* ctx, uint32_t ip, uint64_t traffic_volume) {
    if (!ctx) return -1;
    
    // Simple anomaly detection based on traffic volume
    if (traffic_volume > 1000000000ULL) { // 1GB threshold
        char description[256];
        int desc_len = 0;
        const char* anomaly_desc = "High traffic volume detected";
        while (desc_len < 250 && anomaly_desc[desc_len]) {
            description[desc_len] = anomaly_desc[desc_len];
            desc_len++;
        }
        description[desc_len] = '\0';
        
        generate_threat_alert(ctx, THREAT_TYPE_ANOMALY, THREAT_SEVERITY_HIGH,
                            ip, description, 85);
        return 1;
    }
    
    return 0;
}

int detect_malicious_patterns(threat_detector_ctx_t* ctx, const uint8_t* data, size_t data_len) {
    if (!ctx || !data || data_len == 0) return -1;
    
    // Simple pattern detection - look for suspicious byte sequences
    int threat_score = 0;
    
    // Check for common attack patterns (simplified)
    for (size_t i = 0; i < data_len - 4; i++) {
        // Look for SQL injection patterns
        if (data[i] == '\'' && data[i+1] == 'O' && data[i+2] == 'R' && data[i+3] == ' ') {
            threat_score += 30;
        }
        // Look for command injection patterns
        else if (data[i] == ';' && data[i+1] == 'r' && data[i+2] == 'm' && data[i+3] == ' ') {
            threat_score += 25;
        }
    }
    
    return threat_score;
}

// Blocking functions
int block_ip(threat_detector_ctx_t* ctx, uint32_t ip, const char* reason) {
    if (!ctx) return -1;
    
    int hash_index = hash_ip(ip);
    connection_info_t* conn = &ctx->connections[hash_index];
    
    if (conn->ip_address == ip) {
        conn->is_blocked = 1;
        ctx->stats.malicious_connections_blocked++;
        
        // Call callback if registered
        if (g_blocked_callback) {
            g_blocked_callback(conn);
        }
        
        return 0;
    }
    
    return -1; // IP not found
}

int unblock_ip(threat_detector_ctx_t* ctx, uint32_t ip) {
    if (!ctx) return -1;
    
    int hash_index = hash_ip(ip);
    connection_info_t* conn = &ctx->connections[hash_index];
    
    if (conn->ip_address == ip) {
        conn->is_blocked = 0;
        return 0;
    }
    
    return -1;
}

bool is_ip_blocked(threat_detector_ctx_t* ctx, uint32_t ip) {
    if (!ctx) return 0;
    
    int hash_index = hash_ip(ip);
    connection_info_t* conn = &ctx->connections[hash_index];
    
    return (conn->ip_address == ip && conn->is_blocked);
}

// Alert management
int generate_threat_alert(threat_detector_ctx_t* ctx, threat_type_t type, 
                         threat_severity_t severity, uint32_t source_ip, 
                         const char* description, int threat_score) {
    if (!ctx || !description) return -1;
    
    static uint64_t alert_id_counter = 1;
    
    // Store in threat history
    ctx->threat_history[ctx->threat_history_index] = threat_score;
    ctx->threat_history_index = (ctx->threat_history_index + 1) % THREAT_HISTORY_SIZE;
    
    // Call alert callback if registered
    if (g_alert_callback) {
        threat_alert_t alert;
        alert.alert_id = alert_id_counter++;
        alert.threat_type = type;
        alert.severity = severity;
        alert.source_ip = source_ip;
        alert.source_port = 0;
        alert.timestamp = get_timestamp_ms_internal();
        alert.threat_score = threat_score;
        alert.acknowledged = 0;
        
        // Copy description
        int desc_len = 0;
        while (desc_len < 255 && description[desc_len]) {
            alert.description[desc_len] = description[desc_len];
            desc_len++;
        }
        alert.description[desc_len] = '\0';
        
        g_alert_callback(&alert);
    }
    
    return 0;
}

// Statistics functions
threat_stats_t get_threat_statistics(threat_detector_ctx_t* ctx) {
    if (!ctx) {
        threat_stats_t empty_stats = {0};
        return empty_stats;
    }
    return ctx->stats;
}

void reset_threat_statistics(threat_detector_ctx_t* ctx) {
    if (!ctx) return;
    
    ctx->stats.total_connections_analyzed = 0;
    ctx->stats.threats_detected = 0;
    ctx->stats.ddos_attacks_blocked = 0;
    ctx->stats.malicious_connections_blocked = 0;
    ctx->stats.false_positives = 0;
    ctx->stats.legitimate_connections_allowed = 0;
    ctx->stats.detection_accuracy = 0.0;
    ctx->stats.false_positive_rate = 0.0;
}

void print_threat_report(threat_detector_ctx_t* ctx) {
    if (!ctx) return;
    
    // Simple report output
    threat_stats_t stats = get_threat_statistics(ctx);
    
    // In a real implementation, this would use printf or logging functions
    // For now, we'll just update the statistics
    if (stats.total_connections_analyzed > 0) {
        ctx->stats.detection_accuracy = 
            (double)(stats.total_connections_analyzed - stats.false_positives) / 
            stats.total_connections_analyzed * 100.0;
    }
}

// Configuration functions
void get_threat_config(threat_detector_ctx_t* ctx, threat_config_t* config) {
    if (!ctx || !config) return;
    *config = ctx->config;
}

int set_threat_config(threat_detector_ctx_t* ctx, const threat_config_t* config) {
    if (!ctx || !config) return -1;
    ctx->config = *config;
    return 0;
}

// Learning mode functions
int enable_learning_mode(threat_detector_ctx_t* ctx, int duration_seconds) {
    if (!ctx) return -1;
    ctx->is_learning_mode = 1;
    ctx->learning_start_time = get_timestamp_ms_internal();
    return 0;
}

int disable_learning_mode(threat_detector_ctx_t* ctx) {
    if (!ctx) return -1;
    ctx->is_learning_mode = 0;
    ctx->learning_start_time = 0;
    return 0;
}

bool is_learning_mode_active(threat_detector_ctx_t* ctx) {
    if (!ctx) return 0;
    return ctx->is_learning_mode;
}

// Utility functions
uint32_t ip_string_to_uint32(const char* ip_string) {
    if (!ip_string) return 0;
    
    uint32_t ip = 0;
    uint32_t octet = 0;
    int shift = 24;
    
    while (*ip_string && shift >= 0) {
        if (*ip_string >= '0' && *ip_string <= '9') {
            octet = octet * 10 + (*ip_string - '0');
        } else if (*ip_string == '.') {
            ip |= (octet << shift);
            octet = 0;
            shift -= 8;
        }
        ip_string++;
    }
    
    ip |= (octet << shift);
    return ip;
}

void uint32_to_ip_string(uint32_t ip, char* ip_string, size_t buffer_size) {
    if (!ip_string || buffer_size < 16) return;
    
    uint8_t octets[4];
    octets[0] = (ip >> 24) & 0xFF;
    octets[1] = (ip >> 16) & 0xFF;
    octets[2] = (ip >> 8) & 0xFF;
    octets[3] = ip & 0xFF;
    
    // Simple string formatting
    int pos = 0;
    for (int i = 0; i < 4 && pos < (int)buffer_size - 4; i++) {
        if (octets[i] >= 100) {
            ip_string[pos++] = '0' + (octets[i] / 100);
            ip_string[pos++] = '0' + ((octets[i] / 10) % 10);
            ip_string[pos++] = '0' + (octets[i] % 10);
        } else if (octets[i] >= 10) {
            ip_string[pos++] = '0' + (octets[i] / 10);
            ip_string[pos++] = '0' + (octets[i] % 10);
        } else {
            ip_string[pos++] = '0' + octets[i];
        }
        if (i < 3) {
            ip_string[pos++] = '.';
        }
    }
    ip_string[pos] = '\0';
}

uint64_t get_current_timestamp_ms(void) {
    return get_timestamp_ms_internal();
}

// Callback registration functions
void register_threat_alert_callback(threat_alert_callback_t callback) {
    g_alert_callback = callback;
}

void register_connection_blocked_callback(connection_blocked_callback_t callback) {
    g_blocked_callback = callback;
}

void register_threat_stats_callback(threat_stats_callback_t callback) {
    g_stats_callback = callback;
}

// Integration functions
int integrate_with_network_layer(threat_detector_ctx_t* ctx) {
    // Placeholder for network layer integration
    return 0;
}

int apply_threat_protection(threat_detector_ctx_t* ctx) {
    // Placeholder for applying threat protection
    return 0;
}

int verify_threat_detection(threat_detector_ctx_t* ctx) {
    // Placeholder for verification
    return 0;
}