/*
 * Behavioral Anomaly Detection System for MTProxy
 * Detects unusual patterns and behaviors that may indicate security threats
 */

#ifndef _BEHAVIORAL_ANOMALY_DETECTION_H_
#define _BEHAVIORAL_ANOMALY_DETECTION_H_

#include <stdint.h>
#include <stddef.h>

// Behavior types
typedef enum {
    BEHAVIOR_TYPE_CONNECTION_RATE = 0,     // Connection frequency patterns
    BEHAVIOR_TYPE_DATA_TRANSFER = 1,       // Data transfer patterns
    BEHAVIOR_TYPE_REQUEST_PATTERNS = 2,    // Request behavior patterns
    BEHAVIOR_TYPE_TIMING_ANALYSIS = 3,     // Timing-based anomalies
    BEHAVIOR_TYPE_GEOGRAPHIC = 4,          // Geographic location anomalies
    BEHAVIOR_TYPE_PROTOCOL_USAGE = 5,      // Protocol usage patterns
    BEHAVIOR_TYPE_RESOURCE_CONSUMPTION = 6, // Resource usage patterns
    BEHAVIOR_TYPE_USER_AGENT = 7           // User agent analysis
} behavior_type_t;

// Anomaly severity levels
typedef enum {
    ANOMALY_SEVERITY_LOW = 0,              // Minor deviation
    ANOMALY_SEVERITY_MEDIUM = 1,           // Notable anomaly
    ANOMALY_SEVERITY_HIGH = 2,             // Significant anomaly
    ANOMALY_SEVERITY_CRITICAL = 3          // Critical anomaly requiring immediate attention
} anomaly_severity_t;

// Behavior profile
typedef struct {
    char client_id[64];                    // Client identifier (IP, user ID, etc.)
    behavior_type_t type;
    double baseline_value;                 // Normal/expected value
    double current_value;                  // Current observed value
    double deviation_score;                // 0.0-1.0 deviation from baseline
    long long observation_count;           // Number of observations
    long long anomaly_count;               // Number of anomalies detected
    long long last_update;                 // Last update timestamp
    int is_active;                         // Currently tracking this behavior
    double confidence_level;               // Confidence in baseline accuracy
} behavior_profile_t;

// Anomaly detection
typedef struct {
    long long detection_id;
    char client_id[64];
    behavior_type_t behavior_type;
    anomaly_severity_t severity;
    double anomaly_score;                  // 0.0-1.0 anomaly confidence
    double baseline_value;
    double observed_value;
    double deviation_percentage;
    long long timestamp;
    char description[256];
    int action_taken;                      // Was action taken for this anomaly
    char action_description[128];
} anomaly_detection_t;

// Behavioral pattern
typedef struct {
    long long pattern_id;
    char pattern_name[64];
    behavior_type_t primary_behavior;
    double pattern_signature[10];          // Numerical representation of pattern
    int signature_length;
    double frequency;                      // How often this pattern occurs
    anomaly_severity_t typical_severity;
    long long first_seen;
    long long last_seen;
    int is_suspicious;                     // Flagged as potentially malicious
} behavioral_pattern_t;

// Detection rules
typedef struct {
    char rule_name[64];
    behavior_type_t target_behavior;
    double threshold_value;
    double sensitivity;                    // 0.0-1.0 sensitivity level
    anomaly_severity_t severity_level;
    int enabled;
    long long trigger_count;
    long long last_trigger;
    char action_on_detection[128];         // Action to take when rule triggers
} detection_rule_t;

// Behavioral anomaly detection configuration
typedef struct {
    int enable_behavioral_analysis;
    int enable_pattern_learning;
    int max_behavior_profiles;
    int max_detection_history;
    int learning_window_minutes;
    double default_sensitivity;
    int enable_auto_baselines;
    int baseline_update_interval_seconds;
    int enable_correlation_analysis;
    int correlation_window_seconds;
    int enable_geographic_analysis;
    double geographic_anomaly_threshold;
    int enable_timing_analysis;
    int timing_window_seconds;
} behavioral_detection_config_t;

// Behavioral anomaly detection system
typedef struct {
    // Configuration
    behavioral_detection_config_t config;
    
    // Behavior profiles
    behavior_profile_t *profiles;
    int profile_count;
    int max_profiles;
    
    // Anomaly detections
    anomaly_detection_t *detections;
    int detection_count;
    int max_detections;
    int detection_index;
    
    // Behavioral patterns
    behavioral_pattern_t *patterns;
    int pattern_count;
    int max_patterns;
    
    // Detection rules
    detection_rule_t *rules;
    int rule_count;
    int max_rules;
    
    // Statistics
    long long total_observations;
    long long total_anomalies;
    long long false_positives;
    long long true_positives;
    long long baseline_updates;
    long long pattern_learnings;
    
    // Performance metrics
    double average_detection_accuracy;
    double false_positive_rate;
    double detection_latency_ms;
    double system_efficiency;
    
    // Learning state
    long long learning_start_time;
    int learning_phase_active;
    int baseline_established;
    
    // State
    int initialized;
    int active;
    long long last_analysis_time;
    long long last_baseline_update;
} behavioral_anomaly_detector_t;

// Behavioral statistics
typedef struct {
    long long total_profiles;
    long long active_profiles;
    long long anomalies_last_hour;
    long long high_severity_anomalies;
    long long patterns_identified;
    long long rules_triggered;
    double detection_accuracy_percent;
    double anomaly_rate_per_hour;
    long long learning_progress;
    double confidence_in_baselines;
} behavioral_stats_t;

// Initialize behavioral anomaly detection system
int behavioral_detector_init(behavioral_anomaly_detector_t *detector,
                           const behavioral_detection_config_t *config);

// Cleanup behavioral anomaly detection system
void behavioral_detector_cleanup(behavioral_anomaly_detector_t *detector);

// Add behavior observation
int behavioral_detector_add_observation(behavioral_anomaly_detector_t *detector,
                                      const char *client_id,
                                      behavior_type_t behavior_type,
                                      double value);

// Analyze behavior for anomalies
anomaly_detection_t* behavioral_detector_analyze_behavior(behavioral_anomaly_detector_t *detector,
                                                       const char *client_id,
                                                       behavior_type_t behavior_type,
                                                       double current_value);

// Update behavior baseline
int behavioral_detector_update_baseline(behavioral_anomaly_detector_t *detector,
                                      const char *client_id,
                                      behavior_type_t behavior_type,
                                      double new_value);

// Add detection rule
int behavioral_detector_add_rule(behavioral_anomaly_detector_t *detector,
                               const char *rule_name,
                               behavior_type_t target_behavior,
                               double threshold_value,
                               double sensitivity,
                               anomaly_severity_t severity_level,
                               const char *action_on_detection);

// Remove detection rule
int behavioral_detector_remove_rule(behavioral_anomaly_detector_t *detector,
                                  const char *rule_name);

// Enable/disable detection rule
int behavioral_detector_enable_rule(behavioral_anomaly_detector_t *detector,
                                  const char *rule_name,
                                  int enable);

// Learn behavioral patterns
int behavioral_detector_learn_patterns(behavioral_anomaly_detector_t *detector);

// Get behavioral statistics
void behavioral_detector_get_stats(behavioral_anomaly_detector_t *detector,
                                 behavioral_stats_t *stats);

// Get recent anomalies
int behavioral_detector_get_recent_anomalies(behavioral_anomaly_detector_t *detector,
                                           anomaly_detection_t *anomalies,
                                           int max_anomalies);

// Get behavior profile
behavior_profile_t* behavioral_detector_get_profile(behavioral_anomaly_detector_t *detector,
                                                  const char *client_id,
                                                  behavior_type_t behavior_type);

// Reset behavioral learning
int behavioral_detector_reset_learning(behavioral_anomaly_detector_t *detector);

// Export behavioral data
int behavioral_detector_export_data(behavioral_anomaly_detector_t *detector,
                                  const char *filename);

// Import behavioral data
int behavioral_detector_import_data(behavioral_anomaly_detector_t *detector,
                                  const char *filename);

// Enable/disable behavioral detection
int behavioral_detector_enable(behavioral_anomaly_detector_t *detector);
int behavioral_detector_disable(behavioral_anomaly_detector_t *detector);

// Reset statistics
void behavioral_detector_reset_stats(behavioral_anomaly_detector_t *detector);

// Get global instance
behavioral_anomaly_detector_t* get_global_behavioral_detector(void);

#endif // _BEHAVIORAL_ANOMALY_DETECTION_H_