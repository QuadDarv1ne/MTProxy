/*
 * auto-scaler.c
 * Auto-Scaling Optimization Component Implementation
 */

#include "auto-scaler.h"

// Global context and callbacks
static auto_scaler_ctx_t* g_auto_scaler_ctx = 0;
static scaling_decision_callback_t g_decision_callback = 0;
static resource_query_callback_t g_query_callback = 0;
static resource_adjust_callback_t g_adjust_callback = 0;
static scaling_event_callback_t g_event_callback = 0;

// Simple time function
static uint64_t get_timestamp_ms_internal(void) {
    static uint64_t counter = 2000000;
    return counter++;
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

// Utility function implementations
const char* scaling_policy_to_string(scaling_policy_t policy) {
    switch (policy) {
        case SCALING_POLICY_CONSERVATIVE: return "CONSERVATIVE";
        case SCALING_POLICY_AGGRESSIVE: return "AGGRESSIVE";
        case SCALING_POLICY_ADAPTIVE: return "ADAPTIVE";
        case SCALING_POLICY_CUSTOM: return "CUSTOM";
        default: return "INVALID";
    }
}

const char* resource_type_to_string(resource_type_t type) {
    switch (type) {
        case RESOURCE_TYPE_THREADS: return "THREADS";
        case RESOURCE_TYPE_CONNECTIONS: return "CONNECTIONS";
        case RESOURCE_TYPE_MEMORY: return "MEMORY";
        case RESOURCE_TYPE_BANDWIDTH: return "BANDWIDTH";
        case RESOURCE_TYPE_CPU: return "CPU";
        default: return "INVALID";
    }
}

const char* scaling_action_to_string(scaling_action_t action) {
    switch (action) {
        case SCALING_ACTION_NONE: return "NONE";
        case SCALING_ACTION_SCALE_UP: return "SCALE_UP";
        case SCALING_ACTION_SCALE_DOWN: return "SCALE_DOWN";
        case SCALING_ACTION_MAINTAIN: return "MAINTAIN";
        default: return "INVALID";
    }
}

// Initialization functions
int init_auto_scaler(auto_scaler_ctx_t* ctx) {
    if (!ctx) return -1;
    
    // Default configuration
    auto_scaler_config_t default_config = {
        .enable_auto_scaling = 1,
        .policy = SCALING_POLICY_ADAPTIVE,
        .target_utilization = 70,
        .scale_up_threshold = 85,
        .scale_down_threshold = 30,
        .cooldown_period_seconds = 60,
        .evaluation_interval_seconds = 30,
        .scale_up_multiplier = 1.5,
        .scale_down_multiplier = 0.8,
        .enable_predictive_scaling = 1,
        .prediction_window_seconds = 300,
        .enable_hysteresis = 1,
        .hysteresis_threshold = 5
    };
    
    // Set min/max resources
    for (int i = 0; i < 5; i++) {
        default_config.min_resources[i] = 1;
        default_config.max_resources[i] = 1000;
    }
    
    return init_auto_scaler_with_config(ctx, &default_config);
}

int init_auto_scaler_with_config(auto_scaler_ctx_t* ctx, const auto_scaler_config_t* config) {
    if (!ctx || !config) return -1;
    
    // Initialize context
    ctx->config = *config;
    ctx->last_evaluation_time = get_timestamp_ms_internal();
    ctx->last_scaling_time = 0;
    ctx->is_scaling_in_progress = 0;
    ctx->scaling_cooldown_counter = 0;
    ctx->decision_history_index = 0;
    ctx->trend_index = 0;
    ctx->resource_manager = 0;
    
    // Initialize statistics
    ctx->stats.total_scaling_events = 0;
    ctx->stats.scale_up_events = 0;
    ctx->stats.scale_down_events = 0;
    ctx->stats.no_action_events = 0;
    ctx->stats.failed_scaling_attempts = 0;
    ctx->stats.average_scaling_latency_ms = 0.0;
    ctx->stats.scaling_accuracy = 0.0;
    ctx->stats.last_scaling_time = 0;
    
    // Initialize resources
    for (int i = 0; i < 5; i++) {
        ctx->resources[i].type = (resource_type_t)i;
        ctx->resources[i].current_value = config->min_resources[i];
        ctx->resources[i].target_value = config->min_resources[i];
        ctx->resources[i].min_value = config->min_resources[i];
        ctx->resources[i].max_value = config->max_resources[i];
        ctx->resources[i].utilization_percent = 0.0;
        ctx->resources[i].last_updated = get_timestamp_ms_internal();
        ctx->stats.current_resource_levels[i] = config->min_resources[i];
    }
    
    // Initialize utilization trend
    for (int i = 0; i < 60; i++) {
        ctx->utilization_trend[i] = 50.0; // Default 50% utilization
    }
    
    g_auto_scaler_ctx = ctx;
    return 0;
}

void cleanup_auto_scaler(auto_scaler_ctx_t* ctx) {
    if (!ctx) return;
    
    if (g_auto_scaler_ctx == ctx) {
        g_auto_scaler_ctx = 0;
    }
}

// Configuration management
void get_auto_scaler_config(auto_scaler_ctx_t* ctx, auto_scaler_config_t* config) {
    if (!ctx || !config) return;
    *config = ctx->config;
}

int set_auto_scaler_config(auto_scaler_ctx_t* ctx, const auto_scaler_config_t* config) {
    if (!ctx || !config) return -1;
    ctx->config = *config;
    return 0;
}

// Resource management
int register_resource_manager(auto_scaler_ctx_t* ctx, void* resource_manager) {
    if (!ctx) return -1;
    ctx->resource_manager = resource_manager;
    return 0;
}

int update_resource_metrics(auto_scaler_ctx_t* ctx, resource_type_t type, 
                           int current_value, int max_value) {
    if (!ctx || type < 0 || type >= 5) return -1;
    
    resource_metrics_t* resource = &ctx->resources[type];
    resource->current_value = current_value;
    resource->max_value = max_value;
    resource->last_updated = get_timestamp_ms_internal();
    
    // Calculate utilization percentage
    if (max_value > 0) {
        resource->utilization_percent = (double)current_value / max_value * 100.0;
    } else {
        resource->utilization_percent = 0.0;
    }
    
    // Update current resource levels in stats
    ctx->stats.current_resource_levels[type] = current_value;
    
    return 0;
}

int get_current_resource_level(auto_scaler_ctx_t* ctx, resource_type_t type) {
    if (!ctx || type < 0 || type >= 5) return -1;
    return ctx->resources[type].current_value;
}

int get_target_resource_level(auto_scaler_ctx_t* ctx, resource_type_t type) {
    if (!ctx || type < 0 || type >= 5) return -1;
    return ctx->resources[type].target_value;
}

// Scaling decision making
scaling_decision_t evaluate_scaling_needs(auto_scaler_ctx_t* ctx) {
    scaling_decision_t decision = {0};
    decision.timestamp = get_timestamp_ms_internal();
    
    if (!ctx) {
        decision.action = SCALING_ACTION_NONE;
        return decision;
    }
    
    // Check if auto-scaling is enabled
    if (!ctx->config.enable_auto_scaling) {
        decision.action = SCALING_ACTION_NONE;
        int reason_len = 0;
        const char* reason = "Auto-scaling disabled";
        while (reason_len < 250 && reason[reason_len]) {
            decision.reason[reason_len] = reason[reason_len];
            reason_len++;
        }
        decision.reason[reason_len] = '\0';
        return decision;
    }
    
    // Check cooldown period
    if (ctx->scaling_cooldown_counter > 0) {
        ctx->scaling_cooldown_counter--;
        decision.action = SCALING_ACTION_NONE;
        int reason_len = 0;
        const char* reason = "In cooldown period";
        while (reason_len < 250 && reason[reason_len]) {
            decision.reason[reason_len] = reason[reason_len];
            reason_len++;
        }
        decision.reason[reason_len] = '\0';
        return decision;
    }
    
    // Evaluate each resource type
    for (int i = 0; i < 5; i++) {
        resource_type_t type = (resource_type_t)i;
        resource_metrics_t* resource = &ctx->resources[i];
        
        scaling_action_t action = determine_scaling_action(ctx, type);
        
        if (action != SCALING_ACTION_NONE && action != SCALING_ACTION_MAINTAIN) {
            decision.action = action;
            decision.resource_type = type;
            decision.current_value = resource->current_value;
            
            // Calculate new value based on action
            double multiplier = (action == SCALING_ACTION_SCALE_UP) ? 
                ctx->config.scale_up_multiplier : ctx->config.scale_down_multiplier;
            
            int new_value = (int)(resource->current_value * multiplier);
            
            // Apply bounds checking
            if (new_value < ctx->config.min_resources[i]) {
                new_value = ctx->config.min_resources[i];
            }
            if (new_value > ctx->config.max_resources[i]) {
                new_value = ctx->config.max_resources[i];
            }
            
            decision.new_value = new_value;
            decision.adjustment_amount = new_value - resource->current_value;
            decision.confidence_score = 0.8; // Default confidence
            
            // Set reason
            int reason_len = 0;
            const char* action_str = (action == SCALING_ACTION_SCALE_UP) ? "Scale up" : "Scale down";
            const char* resource_str = resource_type_to_string(type);
            
            // Simple reason string construction
            const char* reason_parts[] = {action_str, " ", resource_str, " due to utilization"};
            for (int part = 0; part < 4 && reason_len < 240; part++) {
                const char* part_str = reason_parts[part];
                int part_len = 0;
                while (part_str[part_len] && reason_len < 240) {
                    decision.reason[reason_len++] = part_str[part_len++];
                }
            }
            decision.reason[reason_len] = '\0';
            
            break; // Take first scaling action
        }
    }
    
    // If no action needed
    if (decision.action == 0) {
        decision.action = SCALING_ACTION_NONE;
        int reason_len = 0;
        const char* reason = "System within target utilization range";
        while (reason_len < 250 && reason[reason_len]) {
            decision.reason[reason_len] = reason[reason_len];
            reason_len++;
        }
        decision.reason[reason_len] = '\0';
    }
    
    return decision;
}

int execute_scaling_decision(auto_scaler_ctx_t* ctx, const scaling_decision_t* decision) {
    if (!ctx || !decision) return -1;
    
    if (decision->action == SCALING_ACTION_NONE) {
        ctx->stats.no_action_events++;
        return 0; // Success - no action needed
    }
    
    // Call resource adjustment callback
    if (g_adjust_callback) {
        int result = g_adjust_callback(decision->resource_type, decision->new_value);
        if (result == 0) {
            // Update resource metrics
            update_resource_metrics(ctx, decision->resource_type, 
                                  decision->new_value, ctx->resources[decision->resource_type].max_value);
            
            // Update target value
            ctx->resources[decision->resource_type].target_value = decision->new_value;
            
            // Update statistics
            ctx->stats.total_scaling_events++;
            if (decision->action == SCALING_ACTION_SCALE_UP) {
                ctx->stats.scale_up_events++;
            } else if (decision->action == SCALING_ACTION_SCALE_DOWN) {
                ctx->stats.scale_down_events++;
            }
            
            ctx->stats.last_scaling_time = get_timestamp_ms_internal();
            ctx->last_scaling_time = ctx->stats.last_scaling_time;
            
            // Start cooldown period
            ctx->scaling_cooldown_counter = ctx->config.cooldown_period_seconds;
            ctx->is_scaling_in_progress = 0;
            
            // Call decision callback
            if (g_decision_callback) {
                g_decision_callback(decision);
            }
            
            return 0; // Success
        } else {
            ctx->stats.failed_scaling_attempts++;
            return -1; // Failed to execute scaling
        }
    }
    
    return -1; // No adjustment callback registered
}

scaling_action_t determine_scaling_action(auto_scaler_ctx_t* ctx, resource_type_t type) {
    if (!ctx || type < 0 || type >= 5) return SCALING_ACTION_NONE;
    
    resource_metrics_t* resource = &ctx->resources[type];
    double utilization = resource->utilization_percent;
    
    // Apply hysteresis if enabled
    if (ctx->config.enable_hysteresis) {
        // Simple hysteresis implementation
        if (utilization > ctx->config.scale_up_threshold + ctx->config.hysteresis_threshold) {
            return SCALING_ACTION_SCALE_UP;
        } else if (utilization < ctx->config.scale_down_threshold - ctx->config.hysteresis_threshold) {
            return SCALING_ACTION_SCALE_DOWN;
        } else {
            return SCALING_ACTION_MAINTAIN;
        }
    } else {
        // Basic threshold-based scaling
        if (utilization > ctx->config.scale_up_threshold) {
            return SCALING_ACTION_SCALE_UP;
        } else if (utilization < ctx->config.scale_down_threshold) {
            return SCALING_ACTION_SCALE_DOWN;
        } else {
            return SCALING_ACTION_MAINTAIN;
        }
    }
}

// Performance monitoring
double get_current_system_utilization(auto_scaler_ctx_t* ctx) {
    if (!ctx) return 0.0;
    
    double total_utilization = 0.0;
    int active_resources = 0;
    
    for (int i = 0; i < 5; i++) {
        if (ctx->resources[i].max_value > 0) {
            total_utilization += ctx->resources[i].utilization_percent;
            active_resources++;
        }
    }
    
    return active_resources > 0 ? total_utilization / active_resources : 0.0;
}

double get_average_utilization(auto_scaler_ctx_t* ctx, int window_size) {
    if (!ctx || window_size <= 0 || window_size > 60) return 0.0;
    
    double sum = 0.0;
    int count = 0;
    int current_index = ctx->trend_index;
    
    for (int i = 0; i < window_size && i < 60; i++) {
        int index = (current_index - i - 1 + 60) % 60;
        sum += ctx->utilization_trend[index];
        count++;
    }
    
    return count > 0 ? sum / count : 0.0;
}

// Control functions
int start_auto_scaling(auto_scaler_ctx_t* ctx) {
    if (!ctx) return -1;
    ctx->config.enable_auto_scaling = 1;
    ctx->last_evaluation_time = get_timestamp_ms_internal();
    return 0;
}

int stop_auto_scaling(auto_scaler_ctx_t* ctx) {
    if (!ctx) return -1;
    ctx->config.enable_auto_scaling = 0;
    return 0;
}

bool is_auto_scaling_active(auto_scaler_ctx_t* ctx) {
    if (!ctx) return 0;
    return ctx->config.enable_auto_scaling && !ctx->is_scaling_in_progress;
}

int force_scaling_evaluation(auto_scaler_ctx_t* ctx) {
    if (!ctx) return -1;
    
    scaling_decision_t decision = evaluate_scaling_needs(ctx);
    return execute_scaling_decision(ctx, &decision);
}

// Statistics and reporting
auto_scaler_stats_t get_auto_scaler_statistics(auto_scaler_ctx_t* ctx) {
    if (!ctx) {
        auto_scaler_stats_t empty_stats = {0};
        return empty_stats;
    }
    return ctx->stats;
}

void reset_auto_scaler_statistics(auto_scaler_ctx_t* ctx) {
    if (!ctx) return;
    
    ctx->stats.total_scaling_events = 0;
    ctx->stats.scale_up_events = 0;
    ctx->stats.scale_down_events = 0;
    ctx->stats.no_action_events = 0;
    ctx->stats.failed_scaling_attempts = 0;
    ctx->stats.average_scaling_latency_ms = 0.0;
    ctx->stats.scaling_accuracy = 0.0;
    ctx->stats.last_scaling_time = 0;
}

// Callback registration
void register_scaling_decision_callback(scaling_decision_callback_t callback) {
    g_decision_callback = callback;
}

void register_resource_query_callback(resource_query_callback_t callback) {
    g_query_callback = callback;
}

void register_resource_adjust_callback(resource_adjust_callback_t callback) {
    g_adjust_callback = callback;
}

void register_scaling_event_callback(scaling_event_callback_t callback) {
    g_event_callback = callback;
}

// Integration functions
int integrate_with_performance_monitor(auto_scaler_ctx_t* ctx) {
    // Placeholder for integration
    return 0;
}

int integrate_with_resource_manager(auto_scaler_ctx_t* ctx) {
    // Placeholder for integration
    return 0;
}

int apply_auto_scaling(auto_scaler_ctx_t* ctx) {
    // Placeholder for application
    return 0;
}

int verify_scaling_operations(auto_scaler_ctx_t* ctx) {
    // Placeholder for verification
    return 0;
}