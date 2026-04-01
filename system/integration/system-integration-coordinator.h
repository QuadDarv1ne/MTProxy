/*
 * System Integration Coordinator for MTProxy
 * Coordinates interactions between all advanced systems
 */

#ifndef SYSTEM_INTEGRATION_COORDINATOR_H
#define SYSTEM_INTEGRATION_COORDINATOR_H

// Forward declarations to avoid circular dependencies
struct diagnostic_context;
struct monitoring_context;
struct debug_framework_context;
struct correlation_engine_context;

#define MAX_REGISTERED_SYSTEMS 16
#define COORDINATION_INTERVAL_MS 100
#define HEALTH_CHECK_INTERVAL_MS 5000

typedef enum {
    SYSTEM_TYPE_DIAGNOSTIC = 0,
    SYSTEM_TYPE_MONITORING = 1,
    SYSTEM_TYPE_DEBUGGING = 2,
    SYSTEM_TYPE_CORRELATION = 3,
    SYSTEM_TYPE_OPTIMIZER = 4,
    SYSTEM_TYPE_LOAD_BALANCER = 5,
    SYSTEM_TYPE_CACHE_MANAGER = 6,
    SYSTEM_TYPE_SECURITY = 7
} system_type_t;

typedef enum {
    COORDINATION_STATUS_IDLE = 0,
    COORDINATION_STATUS_ACTIVE = 1,
    COORDINATION_STATUS_ERROR = 2,
    COORDINATION_STATUS_DEGRADED = 3
} coordination_status_t;

typedef struct {
    system_type_t system_type;
    void* system_context;
    int is_active;
    unsigned long last_interaction_time;
    int priority_level;
    char system_name[64];
} registered_system_t;

typedef struct {
    int system_count;
    registered_system_t systems[MAX_REGISTERED_SYSTEMS];
    coordination_status_t status;
    unsigned long last_coordination_time;
    unsigned long coordination_interval_ms;
    unsigned long health_check_interval_ms;
    int total_interactions;
    int error_count;
    float system_efficiency_score;
} integration_coordinator_context_t;

// Function declarations
int init_integration_coordinator(integration_coordinator_context_t* ctx);
int register_system(integration_coordinator_context_t* ctx, system_type_t type, void* context, const char* name, int priority);
int unregister_system(integration_coordinator_context_t* ctx, const char* system_name);
int coordinate_systems(integration_coordinator_context_t* ctx);
int trigger_health_check(integration_coordinator_context_t* ctx);
int get_system_status(integration_coordinator_context_t* ctx, const char* system_name);
int send_coordination_message(integration_coordinator_context_t* ctx, system_type_t source, system_type_t destination, const char* message_type, void* payload);
float calculate_system_efficiency(integration_coordinator_context_t* ctx);
int schedule_coordination_task(integration_coordinator_context_t* ctx, void (*task_func)(), unsigned long delay_ms);
void cleanup_integration_coordinator(integration_coordinator_context_t* ctx);

#endif // SYSTEM_INTEGRATION_COORDINATOR_H