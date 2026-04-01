/*
 * System Health Monitor for MTProxy
 * Monitors overall system health and coordinates with resource optimization
 */

#ifndef SYSTEM_HEALTH_MONITOR_H
#define SYSTEM_HEALTH_MONITOR_H

// Forward declarations to avoid circular dependencies
struct resource_manager_context;

#define MAX_HEALTH_INDICATORS 16
#define HEALTH_MONITOR_INTERVAL_MS 500
#define CRITICAL_HEALTH_THRESHOLD 30
#define WARNING_HEALTH_THRESHOLD 60

typedef enum {
    HEALTH_STATUS_EXCELLENT = 0,
    HEALTH_STATUS_GOOD = 1,
    HEALTH_STATUS_FAIR = 2,
    HEALTH_STATUS_POOR = 3,
    HEALTH_STATUS_CRITICAL = 4
} health_status_t;

typedef enum {
    HEALTH_INDICATOR_CPU = 0,
    HEALTH_INDICATOR_MEMORY = 1,
    HEALTH_INDICATOR_DISK = 2,
    HEALTH_INDICATOR_NETWORK = 3,
    HEALTH_INDICATOR_THREADS = 4,
    HEALTH_INDICATOR_CONNECTIONS = 5,
    HEALTH_INDICATOR_LATENCY = 6,
    HEALTH_INDICATOR_THROUGHPUT = 7
} health_indicator_t;

typedef struct {
    health_indicator_t indicator_type;
    int current_value;
    int threshold_critical;
    int threshold_warning;
    health_status_t current_status;
    unsigned long last_update_time;
    char indicator_name[64];
} health_indicator_entry_t;

typedef struct {
    int indicator_count;
    health_indicator_entry_t indicators[MAX_HEALTH_INDICATORS];
    health_status_t overall_health;
    unsigned long monitor_interval_ms;
    int health_score; // 0-100 scale
    unsigned long last_evaluation_time;
    int critical_events_count;
    int warning_events_count;
    struct resource_manager_context* resource_manager_ref;
} health_monitor_context_t;

// Function declarations
int init_health_monitor(health_monitor_context_t* ctx);
int register_health_indicator(health_monitor_context_t* ctx, health_indicator_t type, const char* name, int warning_threshold, int critical_threshold);
int update_health_indicator(health_monitor_context_t* ctx, const char* indicator_name, int current_value);
int evaluate_system_health(health_monitor_context_t* ctx);
int get_overall_health_status(health_monitor_context_t* ctx);
int trigger_health_optimization(health_monitor_context_t* ctx);
int get_health_report(health_monitor_context_t* ctx, char* report_buffer, int buffer_size);
int set_resource_manager_reference(health_monitor_context_t* ctx, struct resource_manager_context* res_mgr);
void cleanup_health_monitor(health_monitor_context_t* ctx);

#endif // SYSTEM_HEALTH_MONITOR_H