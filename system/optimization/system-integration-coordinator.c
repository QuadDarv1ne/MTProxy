/*
 * System Integration Coordinator Implementation for MTProxy
 * Coordinates interactions between all advanced systems
 */

#include "system-integration-coordinator.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

// Helper function to compare strings
static int string_compare(const char* str1, const char* str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return (str1[i] < str2[i]) ? -1 : 1;
        }
        i++;
    }
    
    if (str1[i] == '\0' && str2[i] == '\0') {
        return 0;
    }
    
    return (str1[i] == '\0') ? -1 : 1;
}

// Helper function to get string length
static int string_length(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

// Helper function to copy string with bounds checking
static void string_copy(char* dest, const char* src, int max_len) {
    int i = 0;
    while (i < max_len - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Initialize the integration coordinator
int init_integration_coordinator(integration_coordinator_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    // Initialize all fields to default values
    ctx->system_count = 0;
    ctx->status = COORDINATION_STATUS_IDLE;
    ctx->last_coordination_time = 0;
    ctx->coordination_interval_ms = COORDINATION_INTERVAL_MS;
    ctx->health_check_interval_ms = HEALTH_CHECK_INTERVAL_MS;
    ctx->total_interactions = 0;
    ctx->error_count = 0;
    ctx->system_efficiency_score = 1.0f;
    
    // Initialize all registered systems
    int i;
    for (i = 0; i < MAX_REGISTERED_SYSTEMS; i++) {
        registered_system_t* sys = &ctx->systems[i];
        sys->system_type = SYSTEM_TYPE_DIAGNOSTIC; // Default value
        sys->system_context = 0;
        sys->is_active = 0;
        sys->last_interaction_time = 0;
        sys->priority_level = 0;
        sys->system_name[0] = '\0';
    }
    
    return 0;
}

// Register a system with the coordinator
int register_system(integration_coordinator_context_t* ctx, system_type_t type, void* context, const char* name, int priority) {
    if (!ctx || !name || ctx->system_count >= MAX_REGISTERED_SYSTEMS) {
        return -1;
    }
    
    registered_system_t* sys = &ctx->systems[ctx->system_count];
    
    sys->system_type = type;
    sys->system_context = context;
    sys->is_active = 1;
    sys->last_interaction_time = 0;
    sys->priority_level = priority;
    
    // Copy system name with bounds checking
    string_copy(sys->system_name, name, 64);
    
    ctx->system_count++;
    return 0;
}

// Unregister a system from the coordinator
int unregister_system(integration_coordinator_context_t* ctx, const char* system_name) {
    if (!ctx || !system_name) {
        return -1;
    }
    
    int sys_idx = -1;
    int i;
    for (i = 0; i < ctx->system_count; i++) {
        if (string_compare(ctx->systems[i].system_name, system_name) == 0) {
            sys_idx = i;
            break;
        }
    }
    
    if (sys_idx == -1) {
        return -1;
    }
    
    // Shift remaining systems down
    for (i = sys_idx; i < ctx->system_count - 1; i++) {
        ctx->systems[i] = ctx->systems[i + 1];
    }
    
    ctx->system_count--;
    return 0;
}

// Coordinate interactions between registered systems
int coordinate_systems(integration_coordinator_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    if (ctx->system_count == 0) {
        ctx->status = COORDINATION_STATUS_IDLE;
        return 0;
    }
    
    ctx->status = COORDINATION_STATUS_ACTIVE;
    
    // Sort systems by priority (higher priority first)
    int i, j;
    for (i = 0; i < ctx->system_count - 1; i++) {
        for (j = 0; j < ctx->system_count - 1 - i; j++) {
            if (ctx->systems[j].priority_level < ctx->systems[j + 1].priority_level) {
                // Swap systems
                registered_system_t temp = ctx->systems[j];
                ctx->systems[j] = ctx->systems[j + 1];
                ctx->systems[j + 1] = temp;
            }
        }
    }
    
    // Process each active system
    int processed_count = 0;
    for (i = 0; i < ctx->system_count; i++) {
        registered_system_t* sys = &ctx->systems[i];
        if (sys->is_active) {
            // Update interaction time
            sys->last_interaction_time = 0; // In real implementation, this would be current time
            
            // Perform system-specific coordination
            // This is where we would integrate with specific system APIs
            processed_count++;
        }
    }
    
    ctx->total_interactions += processed_count;
    ctx->last_coordination_time = 0; // In real implementation, this would be current time
    
    return processed_count;
}

// Trigger health check for all registered systems
int trigger_health_check(integration_coordinator_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    int healthy_count = 0;
    int i;
    for (i = 0; i < ctx->system_count; i++) {
        registered_system_t* sys = &ctx->systems[i];
        
        // In a real implementation, we would call the specific health check function for each system
        // For now, we'll just count active systems as healthy
        if (sys->is_active) {
            healthy_count++;
        }
    }
    
    // Update efficiency score based on health
    if (ctx->system_count > 0) {
        ctx->system_efficiency_score = (float)healthy_count / (float)ctx->system_count;
    } else {
        ctx->system_efficiency_score = 1.0f;
    }
    
    if (healthy_count < ctx->system_count) {
        ctx->status = COORDINATION_STATUS_DEGRADED;
    }
    
    return healthy_count;
}

// Get status of a specific system
int get_system_status(integration_coordinator_context_t* ctx, const char* system_name) {
    if (!ctx || !system_name) {
        return -1;
    }
    
    int i;
    for (i = 0; i < ctx->system_count; i++) {
        if (string_compare(ctx->systems[i].system_name, system_name) == 0) {
            return ctx->systems[i].is_active ? 1 : 0;
        }
    }
    
    return -1; // System not found
}

// Send coordination message between systems
int send_coordination_message(integration_coordinator_context_t* ctx, system_type_t source, system_type_t destination, const char* message_type, void* payload) {
    if (!ctx || !message_type) {
        return -1;
    }
    
    // In a real implementation, this would route messages between systems
    // For now, we'll just increment the interaction counter
    ctx->total_interactions++;
    
    return 0;
}

// Calculate overall system efficiency
float calculate_system_efficiency(integration_coordinator_context_t* ctx) {
    if (!ctx || ctx->system_count == 0) {
        return 0.0f;
    }
    
    // Calculate efficiency based on various factors
    float base_efficiency = (float)(ctx->system_count - ctx->error_count) / (float)ctx->system_count;
    
    // Adjust based on interaction frequency
    if (ctx->total_interactions > 0) {
        // In a real implementation, we'd normalize this based on time
        base_efficiency *= 0.8f; // Placeholder adjustment
    }
    
    // Clamp to valid range
    if (base_efficiency < 0.0f) base_efficiency = 0.0f;
    if (base_efficiency > 1.0f) base_efficiency = 1.0f;
    
    ctx->system_efficiency_score = base_efficiency;
    return base_efficiency;
}

// Schedule a coordination task (placeholder implementation)
int schedule_coordination_task(integration_coordinator_context_t* ctx, void (*task_func)(), unsigned long delay_ms) {
    if (!ctx || !task_func) {
        return -1;
    }
    
    // In a real implementation, this would schedule a delayed task
    // For now, we'll just return success
    return 0;
}

// Cleanup the integration coordinator
void cleanup_integration_coordinator(integration_coordinator_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    // Reset all fields to default values
    ctx->system_count = 0;
    ctx->status = COORDINATION_STATUS_IDLE;
    ctx->last_coordination_time = 0;
    ctx->coordination_interval_ms = COORDINATION_INTERVAL_MS;
    ctx->health_check_interval_ms = HEALTH_CHECK_INTERVAL_MS;
    ctx->total_interactions = 0;
    ctx->error_count = 0;
    ctx->system_efficiency_score = 1.0f;
    
    // Initialize all registered systems
    int i;
    for (i = 0; i < MAX_REGISTERED_SYSTEMS; i++) {
        registered_system_t* sys = &ctx->systems[i];
        sys->system_type = SYSTEM_TYPE_DIAGNOSTIC; // Default value
        sys->system_context = 0;
        sys->is_active = 0;
        sys->last_interaction_time = 0;
        sys->priority_level = 0;
        sys->system_name[0] = '\0';
    }
}

// Find system index by name
static int find_system_index(integration_coordinator_context_t* ctx, const char* system_name) {
    if (!ctx || !system_name) {
        return -1;
    }
    
    int i;
    for (i = 0; i < ctx->system_count; i++) {
        if (string_compare(ctx->systems[i].system_name, system_name) == 0) {
            return i;
        }
    }
    
    return -1;
}

// Get system by type
registered_system_t* get_system_by_type(integration_coordinator_context_t* ctx, system_type_t type) {
    if (!ctx) {
        return NULL;
    }
    
    int i;
    for (i = 0; i < ctx->system_count; i++) {
        if (ctx->systems[i].system_type == type) {
            return &ctx->systems[i];
        }
    }
    
    return NULL;
}

// Update system activity status
int update_system_activity(integration_coordinator_context_t* ctx, const char* system_name, int is_active) {
    if (!ctx || !system_name) {
        return -1;
    }
    
    int idx = find_system_index(ctx, system_name);
    if (idx == -1) {
        return -1;
    }
    
    ctx->systems[idx].is_active = is_active;
    ctx->systems[idx].last_interaction_time = 0; // In real implementation, this would be current time
    
    return 0;
}