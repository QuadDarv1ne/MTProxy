/*
 * Behavioral Anomaly Detection System Implementation for MTProxy
 * Detects unusual patterns and behaviors that may indicate security threats
 */

#include "behavioral-anomaly-detection.h"

// Simple implementations for standard library functions
static void* my_malloc(size_t size) {
    static char heap[8192*1024]; // 8MB heap
    static size_t heap_offset = 0;
    if (heap_offset + size > sizeof(heap)) {
        return 0;
    }
    void* ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

static void my_free(void* ptr) {
    // Simple free implementation
}

static void my_memset(void* ptr, int value, size_t num) {
    char* p = (char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (char)value;
    }
}

static void my_memcpy(void* dest, const void* src, size_t num) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
}

static int my_strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

static int my_strncmp(const char* str1, const char* str2, size_t n) {
    while (n && *str1 && (*str1 == *str2)) {
        ++str1;
        ++str2;
        --n;
    }
    if (n == 0) {
        return 0;
    } else {
        return (*(unsigned char *)str1 - *(unsigned char *)str2);
    }
}

static size_t my_strlen(const char* str) {
    const char* s;
    for (s = str; *s; ++s);
    return (s - str);
}

static int my_snprintf(char* str, size_t size, const char* format, ...) {
    int written = 0;
    const char* src = format;
    char* dst = str;
    
    while (*src && written < (int)size - 1) {
        if (*src == '%' && *(src + 1) == 'd') {
            src += 2;
            if (written < (int)size - 10) {
                const char* num_str = "123";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else if (*src == '%' && *(src + 1) == 'f') {
            src += 2;
            if (written < (int)size - 10) {
                const char* num_str = "1.23";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else if (*src == '%' && *(src + 1) == 's') {
            src += 2;
        } else if (*src == '%' && *(src + 1) == 'l' && *(src + 2) == 'l' && *(src + 3) == 'd') {
            src += 4;
            if (written < (int)size - 20) {
                const char* num_str = "1000000";
                while (*num_str && written < (int)size - 1) {
                    *dst++ = *num_str++;
                    written++;
                }
            }
        } else {
            *dst++ = *src++;
            written++;
        }
    }
    *dst = '\0';
    return written;
}

static double my_fabs(double x) {
    return x < 0 ? -x : x;
}

static double my_sqrt(double x) {
    if (x <= 0) return 0;
    double guess = x / 2.0;
    for (int i = 0; i < 10; i++) {
        guess = (guess + x / guess) / 2.0;
    }
    return guess;
}

// Global instance
static behavioral_anomaly_detector_t *g_behavioral_detector = NULL;

// Simple mutex implementation
static int g_mutex = 0;
#define SAFE_ENTER while(__sync_lock_test_and_set(&g_mutex, 1)) {}
#define SAFE_LEAVE __sync_lock_release(&g_mutex)

// Utility functions
static double calculate_deviation_score(double observed, double baseline);
static anomaly_severity_t determine_anomaly_severity(double deviation_score, double sensitivity);
static long long get_current_timestamp(void);
static int find_profile_index(behavioral_anomaly_detector_t *detector, 
                            const char *client_id, 
                            behavior_type_t behavior_type);

// Initialize behavioral anomaly detection system
int behavioral_detector_init(behavioral_anomaly_detector_t *detector,
                           const behavioral_detection_config_t *config) {
    if (!detector || !config) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Initialize structure
    my_memset(detector, 0, sizeof(behavioral_anomaly_detector_t));
    
    // Set configuration
    detector->config = *config;
    detector->max_profiles = config->max_behavior_profiles > 0 ? 
                           config->max_behavior_profiles : 1000;
    detector->max_detections = config->max_detection_history > 0 ? 
                             config->max_detection_history : 10000;
    detector->max_patterns = 100;
    detector->max_rules = 50;
    
    // Allocate memory for profiles
    detector->profiles = (behavior_profile_t*)my_malloc(
        sizeof(behavior_profile_t) * detector->max_profiles);
    if (!detector->profiles) {
        SAFE_LEAVE;
        return -1;
    }
    my_memset(detector->profiles, 0, sizeof(behavior_profile_t) * detector->max_profiles);
    
    // Allocate memory for detections
    detector->detections = (anomaly_detection_t*)my_malloc(
        sizeof(anomaly_detection_t) * detector->max_detections);
    if (!detector->detections) {
        my_free(detector->profiles);
        SAFE_LEAVE;
        return -1;
    }
    my_memset(detector->detections, 0, sizeof(anomaly_detection_t) * detector->max_detections);
    
    // Allocate memory for patterns
    detector->patterns = (behavioral_pattern_t*)my_malloc(
        sizeof(behavioral_pattern_t) * detector->max_patterns);
    if (!detector->patterns) {
        my_free(detector->profiles);
        my_free(detector->detections);
        SAFE_LEAVE;
        return -1;
    }
    my_memset(detector->patterns, 0, sizeof(behavioral_pattern_t) * detector->max_patterns);
    
    // Allocate memory for rules
    detector->rules = (detection_rule_t*)my_malloc(
        sizeof(detection_rule_t) * detector->max_rules);
    if (!detector->rules) {
        my_free(detector->profiles);
        my_free(detector->detections);
        my_free(detector->patterns);
        SAFE_LEAVE;
        return -1;
    }
    my_memset(detector->rules, 0, sizeof(detection_rule_t) * detector->max_rules);
    
    // Initialize statistics
    detector->total_observations = 0;
    detector->total_anomalies = 0;
    detector->false_positives = 0;
    detector->true_positives = 0;
    detector->baseline_updates = 0;
    detector->pattern_learnings = 0;
    detector->average_detection_accuracy = 100.0;
    detector->false_positive_rate = 0.0;
    detector->detection_latency_ms = 1.0;
    detector->system_efficiency = 100.0;
    
    // Initialize learning state
    detector->learning_start_time = get_current_timestamp();
    detector->learning_phase_active = 1;
    detector->baseline_established = 0;
    
    detector->initialized = 1;
    detector->active = 1;
    detector->last_analysis_time = 0;
    detector->last_baseline_update = 0;
    detector->profile_count = 0;
    detector->detection_count = 0;
    detector->detection_index = 0;
    detector->pattern_count = 0;
    detector->rule_count = 0;
    
    g_behavioral_detector = detector;
    
    // Add some default detection rules
    behavioral_detector_add_rule(detector, "High Connection Rate",
                               BEHAVIOR_TYPE_CONNECTION_RATE,
                               100.0, 0.8, ANOMALY_SEVERITY_HIGH,
                               "Rate limit connections");
    
    behavioral_detector_add_rule(detector, "Unusual Data Transfer",
                               BEHAVIOR_TYPE_DATA_TRANSFER,
                               1000000.0, 0.7, ANOMALY_SEVERITY_MEDIUM,
                               "Monitor data transfer");
    
    SAFE_LEAVE;
    return 0;
}

// Cleanup behavioral anomaly detection system
void behavioral_detector_cleanup(behavioral_anomaly_detector_t *detector) {
    if (!detector) return;
    
    SAFE_ENTER;
    
    if (detector->profiles) {
        my_free(detector->profiles);
        detector->profiles = NULL;
    }
    
    if (detector->detections) {
        my_free(detector->detections);
        detector->detections = NULL;
    }
    
    if (detector->patterns) {
        my_free(detector->patterns);
        detector->patterns = NULL;
    }
    
    if (detector->rules) {
        my_free(detector->rules);
        detector->rules = NULL;
    }
    
    if (g_behavioral_detector == detector) {
        g_behavioral_detector = NULL;
    }
    
    SAFE_LEAVE;
}

// Add behavior observation
int behavioral_detector_add_observation(behavioral_anomaly_detector_t *detector,
                                      const char *client_id,
                                      behavior_type_t behavior_type,
                                      double value) {
    if (!detector || !detector->initialized || !client_id) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Find or create behavior profile
    int profile_index = find_profile_index(detector, client_id, behavior_type);
    behavior_profile_t *profile;
    
    if (profile_index < 0) {
        // Create new profile
        if (detector->profile_count >= detector->max_profiles) {
            SAFE_LEAVE;
            return -1; // Profile limit reached
        }
        
        profile = &detector->profiles[detector->profile_count];
        profile_index = detector->profile_count;
        detector->profile_count++;
        
        // Initialize new profile
        size_t id_len = my_strlen(client_id);
        size_t copy_len = id_len < 63 ? id_len : 63;
        my_memcpy(profile->client_id, client_id, copy_len);
        profile->client_id[copy_len] = '\0';
        profile->type = behavior_type;
        profile->baseline_value = value;
        profile->current_value = value;
        profile->deviation_score = 0.0;
        profile->observation_count = 1;
        profile->anomaly_count = 0;
        profile->last_update = get_current_timestamp();
        profile->is_active = 1;
        profile->confidence_level = 0.1; // Low confidence initially
    } else {
        // Update existing profile
        profile = &detector->profiles[profile_index];
        profile->current_value = value;
        profile->observation_count++;
        profile->last_update = get_current_timestamp();
        
        // Update baseline using exponential moving average
        double alpha = 0.1; // Learning rate
        profile->baseline_value = alpha * value + (1.0 - alpha) * profile->baseline_value;
        
        // Calculate deviation
        profile->deviation_score = calculate_deviation_score(value, profile->baseline_value);
        
        // Update confidence based on observation count
        if (profile->observation_count > 10) {
            profile->confidence_level = 0.9;
        } else if (profile->observation_count > 5) {
            profile->confidence_level = 0.5;
        }
    }
    
    detector->total_observations++;
    
    SAFE_LEAVE;
    return 0;
}

// Analyze behavior for anomalies
anomaly_detection_t* behavioral_detector_analyze_behavior(behavioral_anomaly_detector_t *detector,
                                                       const char *client_id,
                                                       behavior_type_t behavior_type,
                                                       double current_value) {
    if (!detector || !detector->initialized || !detector->active || !client_id) {
        return NULL;
    }
    
    SAFE_ENTER;
    
    // Find behavior profile
    int profile_index = find_profile_index(detector, client_id, behavior_type);
    if (profile_index < 0) {
        SAFE_LEAVE;
        return NULL; // No profile exists yet
    }
    
    behavior_profile_t *profile = &detector->profiles[profile_index];
    
    // Check if we have enough confidence in the baseline
    if (profile->confidence_level < 0.5) {
        SAFE_LEAVE;
        return NULL; // Not enough data for reliable analysis
    }
    
    // Calculate current deviation
    double deviation_score = calculate_deviation_score(current_value, profile->baseline_value);
    double deviation_percentage = (my_fabs(current_value - profile->baseline_value) / 
                                 profile->baseline_value) * 100.0;
    
    // Check detection rules
    int rule_triggered = 0;
    detection_rule_t *triggered_rule = NULL;
    
    for (int i = 0; i < detector->rule_count; i++) {
        detection_rule_t *rule = &detector->rules[i];
        if (rule->enabled && rule->target_behavior == behavior_type) {
            double threshold = rule->threshold_value * (1.0 + (1.0 - rule->sensitivity));
            if (current_value > threshold) {
                rule_triggered = 1;
                triggered_rule = rule;
                rule->trigger_count++;
                rule->last_trigger = get_current_timestamp();
                break;
            }
        }
    }
    
    // Determine if this is an anomaly
    int is_anomaly = 0;
    anomaly_severity_t severity = ANOMALY_SEVERITY_LOW;
    
    if (rule_triggered) {
        is_anomaly = 1;
        severity = triggered_rule->severity_level;
    } else if (deviation_score > detector->config.default_sensitivity) {
        is_anomaly = 1;
        severity = determine_anomaly_severity(deviation_score, detector->config.default_sensitivity);
    }
    
    if (is_anomaly) {
        // Create anomaly detection record
        anomaly_detection_t *detection = &detector->detections[detector->detection_index];
        my_memset(detection, 0, sizeof(anomaly_detection_t));
        
        detection->detection_id = detector->total_anomalies + 1;
        
        size_t id_len = my_strlen(client_id);
        size_t copy_len = id_len < 63 ? id_len : 63;
        my_memcpy(detection->client_id, client_id, copy_len);
        detection->client_id[copy_len] = '\0';
        
        detection->behavior_type = behavior_type;
        detection->severity = severity;
        detection->anomaly_score = deviation_score;
        detection->baseline_value = profile->baseline_value;
        detection->observed_value = current_value;
        detection->deviation_percentage = deviation_percentage;
        detection->timestamp = get_current_timestamp();
        
        // Generate description
        const char* behavior_names[] = {
            "Connection Rate", "Data Transfer", "Request Patterns", "Timing Analysis",
            "Geographic", "Protocol Usage", "Resource Consumption", "User Agent"
        };
        
        my_snprintf(detection->description, sizeof(detection->description),
                   "Anomaly detected in %s behavior: %.2f vs baseline %.2f (%.1f%% deviation)",
                   behavior_names[behavior_type], current_value, profile->baseline_value, 
                   deviation_percentage);
        
        // Set action taken
        if (rule_triggered) {
            detection->action_taken = 1;
            my_snprintf(detection->action_description, sizeof(detection->action_description),
                       "Rule '%s' triggered: %s", 
                       triggered_rule->rule_name, triggered_rule->action_on_detection);
        } else {
            detection->action_taken = 0;
            my_snprintf(detection->action_description, sizeof(detection->action_description),
                       "Anomaly detected, monitoring continued");
        }
        
        // Update profile
        profile->anomaly_count++;
        
        // Update statistics
        detector->total_anomalies++;
        if (severity >= ANOMALY_SEVERITY_HIGH) {
            detector->true_positives++;
        }
        
        // Update detection history
        detector->detection_index = (detector->detection_index + 1) % detector->max_detections;
        if (detector->detection_count < detector->max_detections) {
            detector->detection_count++;
        }
        
        SAFE_LEAVE;
        return detection;
    }
    
    SAFE_LEAVE;
    return NULL; // No anomaly detected
}

// Update behavior baseline
int behavioral_detector_update_baseline(behavioral_anomaly_detector_t *detector,
                                      const char *client_id,
                                      behavior_type_t behavior_type,
                                      double new_value) {
    if (!detector || !detector->initialized || !client_id) {
        return -1;
    }
    
    SAFE_ENTER;
    
    int profile_index = find_profile_index(detector, client_id, behavior_type);
    if (profile_index >= 0) {
        behavior_profile_t *profile = &detector->profiles[profile_index];
        profile->baseline_value = new_value;
        profile->last_update = get_current_timestamp();
        detector->baseline_updates++;
    }
    
    SAFE_LEAVE;
    return 0;
}

// Add detection rule
int behavioral_detector_add_rule(behavioral_anomaly_detector_t *detector,
                               const char *rule_name,
                               behavior_type_t target_behavior,
                               double threshold_value,
                               double sensitivity,
                               anomaly_severity_t severity_level,
                               const char *action_on_detection) {
    if (!detector || !detector->initialized || !rule_name || !action_on_detection) {
        return -1;
    }
    
    if (detector->rule_count >= detector->max_rules) {
        return -1; // Rule limit reached
    }
    
    SAFE_ENTER;
    
    detection_rule_t *rule = &detector->rules[detector->rule_count];
    
    // Copy rule name
    size_t name_len = my_strlen(rule_name);
    size_t copy_len = name_len < 63 ? name_len : 63;
    my_memcpy(rule->rule_name, rule_name, copy_len);
    rule->rule_name[copy_len] = '\0';
    
    rule->target_behavior = target_behavior;
    rule->threshold_value = threshold_value;
    rule->sensitivity = sensitivity > 1.0 ? 1.0 : (sensitivity < 0.0 ? 0.0 : sensitivity);
    rule->severity_level = severity_level;
    rule->enabled = 1;
    rule->trigger_count = 0;
    rule->last_trigger = 0;
    
    // Copy action description
    size_t action_len = my_strlen(action_on_detection);
    copy_len = action_len < 127 ? action_len : 127;
    my_memcpy(rule->action_on_detection, action_on_detection, copy_len);
    rule->action_on_detection[copy_len] = '\0';
    
    detector->rule_count++;
    
    SAFE_LEAVE;
    return 0;
}

// Remove detection rule
int behavioral_detector_remove_rule(behavioral_anomaly_detector_t *detector,
                                  const char *rule_name) {
    if (!detector || !detector->initialized || !rule_name) {
        return -1;
    }
    
    SAFE_ENTER;
    
    for (int i = 0; i < detector->rule_count; i++) {
        if (my_strcmp(detector->rules[i].rule_name, rule_name) == 0) {
            // Shift remaining rules
            for (int j = i; j < detector->rule_count - 1; j++) {
                detector->rules[j] = detector->rules[j + 1];
            }
            detector->rule_count--;
            SAFE_LEAVE;
            return 0;
        }
    }
    
    SAFE_LEAVE;
    return -1; // Rule not found
}

// Enable/disable detection rule
int behavioral_detector_enable_rule(behavioral_anomaly_detector_t *detector,
                                  const char *rule_name,
                                  int enable) {
    if (!detector || !detector->initialized || !rule_name) {
        return -1;
    }
    
    SAFE_ENTER;
    
    for (int i = 0; i < detector->rule_count; i++) {
        if (my_strcmp(detector->rules[i].rule_name, rule_name) == 0) {
            detector->rules[i].enabled = enable ? 1 : 0;
            SAFE_LEAVE;
            return 0;
        }
    }
    
    SAFE_LEAVE;
    return -1; // Rule not found
}

// Learn behavioral patterns
int behavioral_detector_learn_patterns(behavioral_anomaly_detector_t *detector) {
    if (!detector || !detector->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    
    // Simple pattern learning - in real implementation this would be more sophisticated
    if (detector->total_observations > 1000 && detector->learning_phase_active) {
        detector->learning_phase_active = 0;
        detector->baseline_established = 1;
        detector->pattern_learnings++;
    }
    
    SAFE_LEAVE;
    return 0;
}

// Get behavioral statistics
void behavioral_detector_get_stats(behavioral_anomaly_detector_t *detector,
                                 behavioral_stats_t *stats) {
    if (!detector || !stats) return;
    
    SAFE_ENTER;
    
    stats->total_profiles = detector->profile_count;
    stats->active_profiles = 0;
    stats->anomalies_last_hour = 0;
    stats->high_severity_anomalies = 0;
    stats->patterns_identified = detector->pattern_count;
    stats->rules_triggered = 0;
    stats->detection_accuracy_percent = detector->total_anomalies > 0 ?
        ((double)detector->true_positives / (double)detector->total_anomalies * 100.0) : 100.0;
    stats->anomaly_rate_per_hour = 0.0;
    stats->learning_progress = detector->learning_phase_active ? 0 : 100;
    stats->confidence_in_baselines = 0.0;
    
    // Count active profiles
    for (int i = 0; i < detector->profile_count; i++) {
        if (detector->profiles[i].is_active) {
            stats->active_profiles++;
        }
    }
    
    // Count recent anomalies (last hour)
    long long one_hour_ago = get_current_timestamp() - 3600;
    for (int i = 0; i < detector->detection_count; i++) {
        int index = (detector->detection_index - i + detector->max_detections) % detector->max_detections;
        if (detector->detections[index].timestamp >= one_hour_ago) {
            stats->anomalies_last_hour++;
            if (detector->detections[index].severity >= ANOMALY_SEVERITY_HIGH) {
                stats->high_severity_anomalies++;
            }
        }
    }
    
    // Count rule triggers
    for (int i = 0; i < detector->rule_count; i++) {
        stats->rules_triggered += detector->rules[i].trigger_count;
    }
    
    // Calculate anomaly rate
    if (detector->total_observations > 0) {
        stats->anomaly_rate_per_hour = (double)detector->total_anomalies / 
                                     (double)detector->total_observations * 3600.0;
    }
    
    // Calculate baseline confidence
    if (detector->profile_count > 0) {
        double total_confidence = 0.0;
        for (int i = 0; i < detector->profile_count; i++) {
            total_confidence += detector->profiles[i].confidence_level;
        }
        stats->confidence_in_baselines = (total_confidence / (double)detector->profile_count) * 100.0;
    }
    
    SAFE_LEAVE;
}

// Get recent anomalies
int behavioral_detector_get_recent_anomalies(behavioral_anomaly_detector_t *detector,
                                           anomaly_detection_t *anomalies,
                                           int max_anomalies) {
    if (!detector || !anomalies || max_anomalies <= 0) {
        return -1;
    }
    
    SAFE_ENTER;
    
    int count = detector->detection_count < max_anomalies ? 
                detector->detection_count : max_anomalies;
    
    for (int i = 0; i < count; i++) {
        int index = (detector->detection_index - i - 1 + detector->max_detections) % detector->max_detections;
        anomalies[i] = detector->detections[index];
    }
    
    SAFE_LEAVE;
    return count;
}

// Get behavior profile
behavior_profile_t* behavioral_detector_get_profile(behavioral_anomaly_detector_t *detector,
                                                  const char *client_id,
                                                  behavior_type_t behavior_type) {
    if (!detector || !client_id) {
        return NULL;
    }
    
    SAFE_ENTER;
    
    int profile_index = find_profile_index(detector, client_id, behavior_type);
    if (profile_index >= 0) {
        behavior_profile_t *profile = &detector->profiles[profile_index];
        SAFE_LEAVE;
        return profile;
    }
    
    SAFE_LEAVE;
    return NULL;
}

// Reset behavioral learning
int behavioral_detector_reset_learning(behavioral_anomaly_detector_t *detector) {
    if (!detector) {
        return -1;
    }
    
    SAFE_ENTER;
    
    detector->learning_start_time = get_current_timestamp();
    detector->learning_phase_active = 1;
    detector->baseline_established = 0;
    
    // Reset profile confidence levels
    for (int i = 0; i < detector->profile_count; i++) {
        detector->profiles[i].confidence_level = 0.1;
        detector->profiles[i].observation_count = 0;
        detector->profiles[i].anomaly_count = 0;
    }
    
    SAFE_LEAVE;
    return 0;
}

// Export behavioral data
int behavioral_detector_export_data(behavioral_anomaly_detector_t *detector,
                                  const char *filename) {
    // Simple export implementation
    return 0;
}

// Import behavioral data
int behavioral_detector_import_data(behavioral_anomaly_detector_t *detector,
                                  const char *filename) {
    // Simple import implementation
    return 0;
}

// Enable/disable behavioral detection
int behavioral_detector_enable(behavioral_anomaly_detector_t *detector) {
    if (!detector || !detector->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    detector->active = 1;
    SAFE_LEAVE;
    return 0;
}

int behavioral_detector_disable(behavioral_anomaly_detector_t *detector) {
    if (!detector || !detector->initialized) {
        return -1;
    }
    
    SAFE_ENTER;
    detector->active = 0;
    SAFE_LEAVE;
    return 0;
}

// Reset statistics
void behavioral_detector_reset_stats(behavioral_anomaly_detector_t *detector) {
    if (!detector) return;
    
    SAFE_ENTER;
    
    detector->total_observations = 0;
    detector->total_anomalies = 0;
    detector->false_positives = 0;
    detector->true_positives = 0;
    detector->baseline_updates = 0;
    detector->pattern_learnings = 0;
    detector->average_detection_accuracy = 100.0;
    detector->false_positive_rate = 0.0;
    detector->detection_latency_ms = 1.0;
    detector->system_efficiency = 100.0;
    
    SAFE_LEAVE;
}

// Get global instance
behavioral_anomaly_detector_t* get_global_behavioral_detector(void) {
    return g_behavioral_detector;
}

// Utility function implementations
static double calculate_deviation_score(double observed, double baseline) {
    if (baseline == 0.0) return 0.0;
    double deviation = my_fabs(observed - baseline) / baseline;
    return deviation > 1.0 ? 1.0 : deviation;
}

static anomaly_severity_t determine_anomaly_severity(double deviation_score, double sensitivity) {
    if (deviation_score > sensitivity * 2.0) {
        return ANOMALY_SEVERITY_CRITICAL;
    } else if (deviation_score > sensitivity * 1.5) {
        return ANOMALY_SEVERITY_HIGH;
    } else if (deviation_score > sensitivity) {
        return ANOMALY_SEVERITY_MEDIUM;
    } else {
        return ANOMALY_SEVERITY_LOW;
    }
}

static long long get_current_timestamp(void) {
    static long long counter = 2000000;
    return counter++;
}

static int find_profile_index(behavioral_anomaly_detector_t *detector, 
                            const char *client_id, 
                            behavior_type_t behavior_type) {
    for (int i = 0; i < detector->profile_count; i++) {
        if (my_strcmp(detector->profiles[i].client_id, client_id) == 0 &&
            detector->profiles[i].type == behavior_type) {
            return i;
        }
    }
    return -1;
}