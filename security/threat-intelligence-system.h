/*
 * Advanced Threat Intelligence System for MTProxy
 * Real-time threat detection and mitigation with intelligence feeds
 */

#ifndef _THREAT_INTELLIGENCE_SYSTEM_H_
#define _THREAT_INTELLIGENCE_SYSTEM_H_

#include <stdint.h>
#include <stddef.h>

// Threat types
typedef enum {
    THREAT_TYPE_UNKNOWN = 0,
    THREAT_TYPE_DOS = 1,           // Denial of Service
    THREAT_TYPE_DDOS = 2,          // Distributed Denial of Service
    THREAT_TYPE_MALWARE = 3,       // Malware distribution
    THREAT_TYPE_BOTNET = 4,        // Botnet activity
    THREAT_TYPE_SCANNING = 5,      // Port scanning/probing
    THREAT_TYPE_EXPLOIT = 6,       // Exploit attempts
    THREAT_TYPE_BRUTE_FORCE = 7,   // Brute force attacks
    THREAT_TYPE_DATA_EXFILTRATION = 8, // Data exfiltration attempts
    THREAT_TYPE_RECONNAISSANCE = 9 // Reconnaissance activities
} threat_type_t;

// Threat severity levels
typedef enum {
    THREAT_SEVERITY_LOW = 0,       // Informational
    THREAT_SEVERITY_MEDIUM = 1,    // Requires attention
    THREAT_SEVERITY_HIGH = 2,      // Immediate action needed
    THREAT_SEVERITY_CRITICAL = 3   // Emergency response required
} threat_severity_t;

// Threat intelligence sources
typedef enum {
    SOURCE_INTERNAL = 0,           // Internal detection
    SOURCE_COMMUNITY = 1,          // Community threat feeds
    SOURCE_COMMERCIAL = 2,         // Commercial threat intelligence
    SOURCE_GOVERNMENT = 3,         // Government/CERT feeds
    SOURCE_RESEARCH = 4            // Security research feeds
} threat_source_t;

// Threat indicator
typedef struct {
    char indicator[256];           // IP, domain, hash, etc.
    threat_type_t type;
    threat_severity_t severity;
    threat_source_t source;
    long long first_seen;
    long long last_seen;
    long long confidence_score;    // 0-100 confidence level
    char description[512];
    int active;                    // Currently active threat
    long long hit_count;           // Number of times detected
} threat_indicator_t;

// Threat intelligence feed
typedef struct {
    char name[64];
    char url[256];
    threat_source_t source_type;
    int enabled;
    int update_interval_seconds;
    long long last_update;
    long long indicators_count;
    threat_indicator_t *indicators;
    int max_indicators;
    int current_indicators;
} threat_feed_t;

// Real-time threat detection
typedef struct {
    char source_ip[46];            // IPv4 or IPv6
    int source_port;
    char destination_ip[46];
    int destination_port;
    char user_agent[256];
    char request_path[512];
    long long timestamp;
    threat_type_t detected_threat;
    threat_severity_t severity;
    long long confidence_score;
    char detection_reason[256];
    int blocked;
    long long connection_id;
} threat_detection_t;

// Threat intelligence system configuration
typedef struct {
    int enable_real_time_detection;
    int enable_threat_feeds;
    int max_threat_indicators;
    int threat_cache_size;
    int detection_threshold;
    int auto_block_severity;
    int enable_logging;
    char log_file[256];
    int log_rotation_mb;
    int retention_days;
    int update_interval_seconds;
    int enable_community_sharing;
} threat_intel_config_t;

// Threat intelligence system context
typedef struct {
    // Configuration
    threat_intel_config_t config;
    
    // Threat feeds
    threat_feed_t *feeds;
    int feed_count;
    int max_feeds;
    
    // Threat indicators
    threat_indicator_t *indicators;
    int indicator_count;
    int max_indicators;
    
    // Real-time detections
    threat_detection_t *detections;
    int detection_count;
    int max_detections;
    int detection_index;
    
    // Statistics
    long long total_detections;
    long long blocked_threats;
    long long false_positives;
    long long feed_updates;
    long long indicators_processed;
    
    // Performance metrics
    double average_detection_time_ms;
    double false_positive_rate;
    double detection_accuracy;
    
    // State
    int initialized;
    int active;
    long long last_feed_update;
    int feeds_enabled;
} threat_intelligence_system_t;

// Threat intelligence statistics
typedef struct {
    long long total_indicators;
    long long active_indicators;
    long long detections_last_hour;
    long long blocks_last_hour;
    long long false_positives_last_hour;
    double detection_rate_percent;
    double block_rate_percent;
    long long feeds_operational;
    long long feeds_failed;
} threat_intel_stats_t;

// Initialize threat intelligence system
int threat_intel_init(threat_intelligence_system_t *system,
                     const threat_intel_config_t *config);

// Cleanup threat intelligence system
void threat_intel_cleanup(threat_intelligence_system_t *system);

// Add threat feed
int threat_intel_add_feed(threat_intelligence_system_t *system,
                         const char *name,
                         const char *url,
                         threat_source_t source_type,
                         int update_interval_seconds);

// Remove threat feed
int threat_intel_remove_feed(threat_intelligence_system_t *system,
                            const char *name);

// Update threat feeds
int threat_intel_update_feeds(threat_intelligence_system_t *system);

// Add threat indicator
int threat_intel_add_indicator(threat_intelligence_system_t *system,
                              const threat_indicator_t *indicator);

// Remove threat indicator
int threat_intel_remove_indicator(threat_intelligence_system_t *system,
                                 const char *indicator_value);

// Check if IP/domain is in threat database
int threat_intel_check_indicator(threat_intelligence_system_t *system,
                                const char *indicator_value,
                                threat_indicator_t *found_indicator);

// Analyze connection for threats
threat_detection_t* threat_intel_analyze_connection(threat_intelligence_system_t *system,
                                                  const char *source_ip,
                                                  int source_port,
                                                  const char *destination_ip,
                                                  int destination_port,
                                                  const char *user_agent,
                                                  const char *request_path);

// Block detected threat
int threat_intel_block_threat(threat_intelligence_system_t *system,
                             threat_detection_t *detection);

// Get threat intelligence statistics
void threat_intel_get_stats(threat_intelligence_system_t *system,
                           threat_intel_stats_t *stats);

// Get recent detections
int threat_intel_get_recent_detections(threat_intelligence_system_t *system,
                                      threat_detection_t *detections,
                                      int max_detections);

// Enable/disable threat intelligence
int threat_intel_enable(threat_intelligence_system_t *system);
int threat_intel_disable(threat_intelligence_system_t *system);

// Reset statistics
void threat_intel_reset_stats(threat_intelligence_system_t *system);

// Export threat intelligence data
int threat_intel_export_data(threat_intelligence_system_t *system,
                            const char *filename);

// Import threat intelligence data
int threat_intel_import_data(threat_intelligence_system_t *system,
                            const char *filename);

// Get global instance
threat_intelligence_system_t* get_global_threat_intel_system(void);

#endif // _THREAT_INTELLIGENCE_SYSTEM_H_