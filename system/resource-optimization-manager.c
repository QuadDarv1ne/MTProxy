/*
 * Resource Optimization Manager Implementation for MTProxy
 * Manages system resources across all advanced systems
 */

#include "resource-optimization-manager.h"

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

// Helper function to calculate absolute value
static int int_abs(int value) {
    return (value < 0) ? -value : value;
}

// Initialize the resource manager
int init_resource_manager(resource_manager_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    // Initialize all fields to default values
    ctx->resource_count = 0;
    ctx->sampling_interval_ms = RESOURCE_SAMPLING_INTERVAL_MS;
    ctx->optimization_enabled = 1;
    ctx->current_optimization_level = 2; // Medium
    ctx->last_optimization_time = 0;
    ctx->total_resources_saved = 0;
    ctx->efficiency_improvement_ratio = 1.0f;
    
    // Initialize all resource entries
    int i;
    for (i = 0; i < MAX_MONITORED_RESOURCES; i++) {
        resource_entry_t* res = &ctx->resources[i];
        res->type = RESOURCE_TYPE_CPU; // Default value
        res->current_usage = 0;
        res->peak_usage = 0;
        res->limit = 0;
        res->priority = RESOURCE_PRIORITY_MEDIUM;
        res->is_overloaded = 0;
        res->last_update_time = 0;
        res->resource_name[0] = '\0';
    }
    
    return 0;
}

// Register a resource with the manager
int register_resource(resource_manager_context_t* ctx, resource_type_t type, unsigned long limit, const char* name, resource_priority_t priority) {
    if (!ctx || !name || ctx->resource_count >= MAX_MONITORED_RESOURCES) {
        return -1;
    }
    
    resource_entry_t* res = &ctx->resources[ctx->resource_count];
    
    res->type = type;
    res->current_usage = 0;
    res->peak_usage = 0;
    res->limit = limit;
    res->priority = priority;
    res->is_overloaded = 0;
    res->last_update_time = 0;
    
    // Copy resource name with bounds checking
    string_copy(res->resource_name, name, 64);
    
    ctx->resource_count++;
    return 0;
}

// Update resource usage
int update_resource_usage(resource_manager_context_t* ctx, const char* resource_name, unsigned long current_usage) {
    if (!ctx || !resource_name) {
        return -1;
    }
    
    int res_idx = -1;
    int i;
    for (i = 0; i < ctx->resource_count; i++) {
        if (string_compare(ctx->resources[i].resource_name, resource_name) == 0) {
            res_idx = i;
            break;
        }
    }
    
    if (res_idx == -1) {
        return -1;
    }
    
    resource_entry_t* res = &ctx->resources[res_idx];
    res->current_usage = current_usage;
    
    // Update peak usage if necessary
    if (current_usage > res->peak_usage) {
        res->peak_usage = current_usage;
    }
    
    // Check if resource is overloaded (usage exceeds 85% of limit)
    if (res->limit > 0) {
        unsigned long threshold = (res->limit * OPTIMIZATION_THRESHOLD_PERCENT) / 100;
        res->is_overloaded = (current_usage > threshold) ? 1 : 0;
    } else {
        res->is_overloaded = 0;
    }
    
    res->last_update_time = 0; // In real implementation, this would be current time
    
    return 0;
}

// Optimize resources based on current usage
int optimize_resources(resource_manager_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    int optimizations_performed = 0;
    int i;
    for (i = 0; i < ctx->resource_count; i++) {
        resource_entry_t* res = &ctx->resources[i];
        
        if (res->is_overloaded) {
            // Perform optimization based on priority
            if (res->priority >= RESOURCE_PRIORITY_HIGH) {
                // For high priority resources, consider increasing limit or reducing usage
                // In a real implementation, this would involve system-level optimizations
                optimizations_performed++;
            } else {
                // For lower priority resources, reduce usage where possible
                // In a real implementation, this would involve throttling or deferring operations
                optimizations_performed++;
            }
        }
    }
    
    ctx->last_optimization_time = 0; // In real implementation, this would be current time
    ctx->total_resources_saved += optimizations_performed;
    
    return optimizations_performed;
}

// Get resource status
int get_resource_status(resource_manager_context_t* ctx, const char* resource_name, resource_entry_t* entry) {
    if (!ctx || !resource_name || !entry) {
        return -1;
    }
    
    int i;
    for (i = 0; i < ctx->resource_count; i++) {
        if (string_compare(ctx->resources[i].resource_name, resource_name) == 0) {
            *entry = ctx->resources[i];
            return 0;
        }
    }
    
    return -1; // Resource not found
}

// Trigger resource reallocation
int trigger_resource_reallocation(resource_manager_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    // In a real implementation, this would trigger system-wide resource reallocation
    // For now, we'll just optimize resources
    return optimize_resources(ctx);
}

// Calculate resource efficiency
int calculate_resource_efficiency(resource_manager_context_t* ctx, float* efficiency_score) {
    if (!ctx || !efficiency_score) {
        return -1;
    }
    
    if (ctx->resource_count == 0) {
        *efficiency_score = 1.0f;
        return 0;
    }
    
    // Calculate efficiency based on usage patterns
    unsigned long total_limit = 0;
    unsigned long total_current_usage = 0;
    int i;
    for (i = 0; i < ctx->resource_count; i++) {
        resource_entry_t* res = &ctx->resources[i];
        total_limit += res->limit;
        total_current_usage += res->current_usage;
    }
    
    if (total_limit == 0) {
        *efficiency_score = 1.0f;
        return 0;
    }
    
    // Efficiency is higher when usage is closer to optimal (not too low, not too high)
    float usage_ratio = (float)total_current_usage / (float)total_limit;
    
    // Optimal usage is around 70-85%, so we penalize both under and over usage
    if (usage_ratio <= 0.85f && usage_ratio >= 0.50f) {
        // Good range, close to optimal
        *efficiency_score = 0.9f + (0.1f * (1.0f - (float)int_abs((int)(usage_ratio * 100) - 70) / 35.0f));
    } else if (usage_ratio < 0.50f) {
        // Underutilized
        *efficiency_score = 0.5f + (0.4f * (usage_ratio / 0.50f));
    } else {
        // Overutilized
        *efficiency_score = 0.9f - (0.4f * ((usage_ratio - 0.85f) / 0.15f));
    }
    
    // Clamp to valid range
    if (*efficiency_score < 0.0f) *efficiency_score = 0.0f;
    if (*efficiency_score > 1.0f) *efficiency_score = 1.0f;
    
    ctx->efficiency_improvement_ratio = *efficiency_score;
    return 0;
}

// Set optimization level
int set_optimization_level(resource_manager_context_t* ctx, int level) {
    if (!ctx || level < 0 || level > 3) {
        return -1;
    }
    
    ctx->current_optimization_level = level;
    
    // In a real implementation, this would adjust optimization aggressiveness
    return 0;
}

// Cleanup the resource manager
void cleanup_resource_manager(resource_manager_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    // Reset all fields to default values
    ctx->resource_count = 0;
    ctx->sampling_interval_ms = RESOURCE_SAMPLING_INTERVAL_MS;
    ctx->optimization_enabled = 1;
    ctx->current_optimization_level = 2; // Medium
    ctx->last_optimization_time = 0;
    ctx->total_resources_saved = 0;
    ctx->efficiency_improvement_ratio = 1.0f;
    
    // Initialize all resource entries
    int i;
    for (i = 0; i < MAX_MONITORED_RESOURCES; i++) {
        resource_entry_t* res = &ctx->resources[i];
        res->type = RESOURCE_TYPE_CPU; // Default value
        res->current_usage = 0;
        res->peak_usage = 0;
        res->limit = 0;
        res->priority = RESOURCE_PRIORITY_MEDIUM;
        res->is_overloaded = 0;
        res->last_update_time = 0;
        res->resource_name[0] = '\0';
    }
}

// Find resource index by name
static int find_resource_index(resource_manager_context_t* ctx, const char* resource_name) {
    if (!ctx || !resource_name) {
        return -1;
    }
    
    int i;
    for (i = 0; i < ctx->resource_count; i++) {
        if (string_compare(ctx->resources[i].resource_name, resource_name) == 0) {
            return i;
        }
    }
    
    return -1;
}

// Get resource overload status
int is_resource_overloaded(resource_manager_context_t* ctx, const char* resource_name) {
    if (!ctx || !resource_name) {
        return -1;
    }
    
    int idx = find_resource_index(ctx, resource_name);
    if (idx == -1) {
        return -1;
    }
    
    return ctx->resources[idx].is_overloaded;
}

// Get total optimization savings
int get_total_resources_saved(resource_manager_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    return ctx->total_resources_saved;
}