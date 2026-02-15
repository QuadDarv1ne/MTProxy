/*
 * Resource Optimization Manager for MTProxy
 * Manages system resources across all advanced systems
 */

#ifndef RESOURCE_OPTIMIZATION_MANAGER_H
#define RESOURCE_OPTIMIZATION_MANAGER_H

// Forward declarations to avoid circular dependencies
struct diagnostic_context;
struct monitoring_context;
struct debug_framework_context;
struct correlation_engine_context;
struct integration_coordinator_context;

#define MAX_MONITORED_RESOURCES 32
#define RESOURCE_SAMPLING_INTERVAL_MS 100
#define OPTIMIZATION_THRESHOLD_PERCENT 85

typedef enum {
    RESOURCE_TYPE_CPU = 0,
    RESOURCE_TYPE_MEMORY = 1,
    RESOURCE_TYPE_NETWORK = 2,
    RESOURCE_TYPE_DISK = 3,
    RESOURCE_TYPE_THREAD = 4,
    RESOURCE_TYPE_CONNECTION = 5
} resource_type_t;

typedef enum {
    RESOURCE_PRIORITY_LOW = 0,
    RESOURCE_PRIORITY_MEDIUM = 1,
    RESOURCE_PRIORITY_HIGH = 2,
    RESOURCE_PRIORITY_CRITICAL = 3
} resource_priority_t;

typedef struct {
    resource_type_t type;
    unsigned long current_usage;
    unsigned long peak_usage;
    unsigned long limit;
    resource_priority_t priority;
    int is_overloaded;
    unsigned long last_update_time;
    char resource_name[64];
} resource_entry_t;

typedef struct {
    int resource_count;
    resource_entry_t resources[MAX_MONITORED_RESOURCES];
    unsigned long sampling_interval_ms;
    int optimization_enabled;
    int current_optimization_level;
    unsigned long last_optimization_time;
    int total_resources_saved;
    float efficiency_improvement_ratio;
} resource_manager_context_t;

// Function declarations
int init_resource_manager(resource_manager_context_t* ctx);
int register_resource(resource_manager_context_t* ctx, resource_type_t type, unsigned long limit, const char* name, resource_priority_t priority);
int update_resource_usage(resource_manager_context_t* ctx, const char* resource_name, unsigned long current_usage);
int optimize_resources(resource_manager_context_t* ctx);
int get_resource_status(resource_manager_context_t* ctx, const char* resource_name, resource_entry_t* entry);
int trigger_resource_reallocation(resource_manager_context_t* ctx);
int calculate_resource_efficiency(resource_manager_context_t* ctx, float* efficiency_score);
int set_optimization_level(resource_manager_context_t* ctx, int level);
void cleanup_resource_manager(resource_manager_context_t* ctx);

#endif // RESOURCE_OPTIMIZATION_MANAGER_H