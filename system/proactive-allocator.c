/*
 * proactive-allocator.c
 * Proactive Resource Allocation System Implementation
 */

#include "proactive-allocator.h"

// Global context and callbacks
static proactive_allocator_ctx_t* g_allocator_ctx = NULL;
static allocation_callback_t g_allocation_callback = NULL;
static deallocation_callback_t g_deallocation_callback = NULL;
static resource_pressure_callback_t g_pressure_callback = NULL;
static allocation_stats_callback_t g_stats_callback = NULL;
static resource_availability_callback_t g_availability_callback = NULL;

// Simple time function
static uint64_t get_timestamp_ms_internal(void) {
    static uint64_t counter = 8000000;
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
const char* resource_type_to_string(resource_type_t type) {
    switch (type) {
        case RESOURCE_TYPE_CPU: return "CPU";
        case RESOURCE_TYPE_MEMORY: return "MEMORY";
        case RESOURCE_TYPE_NETWORK_BANDWIDTH: return "NETWORK_BANDWIDTH";
        case RESOURCE_TYPE_DISK_IO: return "DISK_IO";
        case RESOURCE_TYPE_CONNECTIONS: return "CONNECTIONS";
        case RESOURCE_TYPE_THREADS: return "THREADS";
        case RESOURCE_TYPE_CRYPTO_BUFFERS: return "CRYPTO_BUFFERS";
        case RESOURCE_TYPE_CACHE_MEMORY: return "CACHE_MEMORY";
        default: return "INVALID";
    }
}

const char* allocation_strategy_to_string(allocation_strategy_t strategy) {
    switch (strategy) {
        case STRATEGY_CONSERVATIVE: return "CONSERVATIVE";
        case STRATEGY_AGGRESSIVE: return "AGGRESSIVE";
        case STRATEGY_BALANCED: return "BALANCED";
        case STRATEGY_PREDICTIVE: return "PREDICTIVE";
        case STRATEGY_ADAPTIVE: return "ADAPTIVE";
        default: return "INVALID";
    }
}

// Initialization functions
int init_proactive_allocator(proactive_allocator_ctx_t* ctx) {
    if (!ctx) return -1;
    
    // Default configuration
    proactive_config_t default_config = {
        .enable_proactive_allocation = 1,
        .default_strategy = STRATEGY_PREDICTIVE,
        .prediction_horizon_seconds = 300,
        .safety_margin_percent = 20.0,
        .reallocation_interval_seconds = 60,
        .max_concurrent_allocations = 1000,
        .minimum_allocation_unit = 1024,
        .maximum_allocation_unit = 1048576,
        .enable_overcommit = 1,
        .overcommit_ratio = 1.5,
        .garbage_collection_interval_seconds = 300,
        .garbage_collection_threshold = 10.0,
        .enable_resource_sharing = 1,
        .sharing_efficiency_target = 80,
        .enable_priority_scheduling = 1,
        .high_priority_threshold = 80
    };
    
    return init_proactive_allocator_with_config(ctx, &default_config);
}

int init_proactive_allocator_with_config(proactive_allocator_ctx_t* ctx, const proactive_config_t* config) {
    if (!ctx || !config) return -1;
    
    // Initialize context
    ctx->config = *config;
    ctx->last_allocation_time = get_timestamp_ms_internal();
    ctx->last_reallocation_time = get_timestamp_ms_internal();
    ctx->last_garbage_collection_time = get_timestamp_ms_internal();
    ctx->is_optimizing = 0;
    ctx->current_strategy = config->default_strategy;
    ctx->active_algorithm_index = 0;
    ctx->pool_count = 0;
    ctx->pending_request_count = 0;
    ctx->active_allocation_count = 0;
    ctx->forecast_count = 0;
    ctx->efficiency_history_index = 0;
    
    // Initialize statistics
    ctx->stats.total_allocations = 0;
    ctx->stats.successful_allocations = 0;
    ctx->stats.failed_allocations = 0;
    ctx->stats.preempted_allocations = 0;
    ctx->stats.reallocated_resources = 0;
    ctx->stats.allocation_success_rate = 0.0;
    ctx->stats.average_resource_utilization = 0.0;
    ctx->stats.resource_efficiency_score = 0.0;
    ctx->stats.cost_effectiveness_ratio = 0.0;
    ctx->stats.peak_allocation_time = 0;
    ctx->stats.last_optimization_time = get_timestamp_ms_internal();
    ctx->stats.optimization_gain_percent = 0.0;
    
    // Allocate request buffer
    ctx->pending_requests = (resource_request_t*)malloc(sizeof(resource_request_t) * 10000);
    if (!ctx->pending_requests) return -1;
    
    // Allocate allocation buffer
    ctx->active_allocations = (resource_allocation_t*)malloc(sizeof(resource_allocation_t) * 10000);
    if (!ctx->active_allocations) {
        free(ctx->pending_requests);
        return -1;
    }
    
    // Allocate forecast buffer
    ctx->demand_forecasts = (demand_forecast_t*)malloc(sizeof(demand_forecast_t) * 1000);
    if (!ctx->demand_forecasts) {
        free(ctx->pending_requests);
        free(ctx->active_allocations);
        return -1;
    }
    
    // Initialize allocation algorithms (simplified)
    for (int i = 0; i < 5; i++) {
        ctx->allocation_algorithms[i] = NULL;
    }
    
    // Initialize resource pools with default capacities
    create_resource_pool(ctx, RESOURCE_TYPE_CPU, 1000000);  // 1M units
    create_resource_pool(ctx, RESOURCE_TYPE_MEMORY, 1073741824);  // 1GB
    create_resource_pool(ctx, RESOURCE_TYPE_NETWORK_BANDWIDTH, 1000000000);  // 1Gbps
    create_resource_pool(ctx, RESOURCE_TYPE_CONNECTIONS, 10000);  // 10K connections
    
    g_allocator_ctx = ctx;
    return 0;
}

void cleanup_proactive_allocator(proactive_allocator_ctx_t* ctx) {
    if (!ctx) return;
    
    // Free allocated memory
    if (ctx->pending_requests) {
        free(ctx->pending_requests);
        ctx->pending_requests = NULL;
    }
    
    if (ctx->active_allocations) {
        free(ctx->active_allocations);
        ctx->active_allocations = NULL;
    }
    
    if (ctx->demand_forecasts) {
        free(ctx->demand_forecasts);
        ctx->demand_forecasts = NULL;
    }
    
    // Clean up allocation algorithms
    for (int i = 0; i < 5; i++) {
        if (ctx->allocation_algorithms[i]) {
            // In a real implementation, we would properly clean up algorithm resources
            ctx->allocation_algorithms[i] = NULL;
        }
    }
    
    if (g_allocator_ctx == ctx) {
        g_allocator_ctx = NULL;
    }
}

// Configuration management
void get_proactive_config(proactive_allocator_ctx_t* ctx, proactive_config_t* config) {
    if (!ctx || !config) return;
    *config = ctx->config;
}

int set_proactive_config(proactive_allocator_ctx_t* ctx, const proactive_config_t* config) {
    if (!ctx || !config) return -1;
    ctx->config = *config;
    return 0;
}

// Resource pool management
int create_resource_pool(proactive_allocator_ctx_t* ctx, resource_type_t type, uint64_t capacity) {
    if (!ctx || ctx->pool_count >= 8) return -1;
    
    resource_pool_t* pool = &ctx->resource_pools[ctx->pool_count];
    
    pool->type = type;
    pool->total_capacity = capacity;
    pool->currently_allocated = 0;
    pool->available_capacity = capacity;
    pool->reserved_capacity = 0;
    pool->utilization_percentage = 0.0;
    pool->allocation_count = 0;
    pool->deallocation_count = 0;
    pool->failed_allocations = 0;
    pool->average_allocation_time_ms = 0.0;
    pool->average_utilization_rate = 0.0;
    pool->last_update_time = get_timestamp_ms_internal();
    
    ctx->pool_count++;
    return 0;
}

int update_resource_pool_capacity(proactive_allocator_ctx_t* ctx, resource_type_t type, uint64_t new_capacity) {
    if (!ctx) return -1;
    
    for (int i = 0; i < ctx->pool_count; i++) {
        if (ctx->resource_pools[i].type == type) {
            uint64_t old_capacity = ctx->resource_pools[i].total_capacity;
            ctx->resource_pools[i].total_capacity = new_capacity;
            ctx->resource_pools[i].available_capacity += (new_capacity - old_capacity);
            ctx->resource_pools[i].last_update_time = get_timestamp_ms_internal();
            return 0;
        }
    }
    
    return -1; // Pool not found
}

int get_resource_pool_info(proactive_allocator_ctx_t* ctx, resource_type_t type, resource_pool_t* pool_info) {
    if (!ctx || !pool_info) return -1;
    
    for (int i = 0; i < ctx->pool_count; i++) {
        if (ctx->resource_pools[i].type == type) {
            *pool_info = ctx->resource_pools[i];
            return 0;
        }
    }
    
    return -1; // Pool not found
}

uint64_t get_available_resources(proactive_allocator_ctx_t* ctx, resource_type_t type) {
    if (!ctx) return 0;
    
    for (int i = 0; i < ctx->pool_count; i++) {
        if (ctx->resource_pools[i].type == type) {
            return ctx->resource_pools[i].available_capacity;
        }
    }
    
    return 0;
}

uint64_t get_total_resources(proactive_allocator_ctx_t* ctx, resource_type_t type) {
    if (!ctx) return 0;
    
    for (int i = 0; i < ctx->pool_count; i++) {
        if (ctx->resource_pools[i].type == type) {
            return ctx->resource_pools[i].total_capacity;
        }
    }
    
    return 0;
}

// Resource request management
uint64_t request_resources(proactive_allocator_ctx_t* ctx, const resource_request_t* request) {
    if (!ctx || !request || ctx->pending_request_count >= 10000) return 0;
    
    uint64_t request_id = ctx->pending_request_count + 1;
    
    ctx->pending_requests[ctx->pending_request_count] = *request;
    ctx->pending_requests[ctx->pending_request_count].request_id = request_id;
    ctx->pending_requests[ctx->pending_request_count].request_time = get_timestamp_ms_internal();
    
    // Copy requester_id and purpose strings
    int i = 0;
    while (i < 63 && request->requester_id[i]) {
        ctx->pending_requests[ctx->pending_request_count].requester_id[i] = request->requester_id[i];
        i++;
    }
    ctx->pending_requests[ctx->pending_request_count].requester_id[i] = '\0';
    
    i = 0;
    while (i < 127 && request->purpose[i]) {
        ctx->pending_requests[ctx->pending_request_count].purpose[i] = request->purpose[i];
        i++;
    }
    ctx->pending_requests[ctx->pending_request_count].purpose[i] = '\0';
    
    ctx->pending_request_count++;
    
    return request_id;
}

int cancel_resource_request(proactive_allocator_ctx_t* ctx, uint64_t request_id) {
    if (!ctx) return -1;
    
    for (int i = 0; i < ctx->pending_request_count; i++) {
        if (ctx->pending_requests[i].request_id == request_id) {
            // Shift remaining requests
            for (int j = i; j < ctx->pending_request_count - 1; j++) {
                ctx->pending_requests[j] = ctx->pending_requests[j + 1];
            }
            ctx->pending_request_count--;
            return 0;
        }
    }
    
    return -1; // Request not found
}

// Resource allocation functions
int allocate_resources(proactive_allocator_ctx_t* ctx, uint64_t request_id) {
    if (!ctx) return -1;
    
    // Find the request
    resource_request_t* request = NULL;
    for (int i = 0; i < ctx->pending_request_count; i++) {
        if (ctx->pending_requests[i].request_id == request_id) {
            request = &ctx->pending_requests[i];
            break;
        }
    }
    
    if (!request) return -1;
    
    // Check if allocation is feasible
    uint64_t available = get_available_resources(ctx, request->resource_type);
    if (available < request->minimum_required) {
        ctx->stats.failed_allocations++;
        return -1;
    }
    
    // Determine allocation amount
    uint64_t allocation_amount = request->requested_amount;
    if (allocation_amount > available) {
        allocation_amount = available;
    }
    if (allocation_amount > request->maximum_acceptable) {
        allocation_amount = request->maximum_acceptable;
    }
    
    // Create allocation
    if (ctx->active_allocation_count < 10000) {
        resource_allocation_t* allocation = &ctx->active_allocations[ctx->active_allocation_count];
        
        allocation->allocation_id = ctx->active_allocation_count + 1;
        allocation->request = *request;
        allocation->allocated_amount = allocation_amount;
        allocation->allocation_time = get_timestamp_ms_internal();
        allocation->expiration_time = allocation->allocation_time + request->deadline_ms;
        allocation->is_active = 1;
        allocation->utilization_efficiency = 0.0;
        allocation->usage_count = 0;
        allocation->last_access_time = allocation->allocation_time;
        
        ctx->active_allocation_count++;
        
        // Update resource pool
        for (int i = 0; i < ctx->pool_count; i++) {
            if (ctx->resource_pools[i].type == request->resource_type) {
                ctx->resource_pools[i].currently_allocated += allocation_amount;
                ctx->resource_pools[i].available_capacity -= allocation_amount;
                ctx->resource_pools[i].allocation_count++;
                ctx->resource_pools[i].utilization_percentage = 
                    (double)ctx->resource_pools[i].currently_allocated / ctx->resource_pools[i].total_capacity * 100.0;
                ctx->resource_pools[i].last_update_time = get_timestamp_ms_internal();
                break;
            }
        }
        
        // Update statistics
        ctx->stats.total_allocations++;
        ctx->stats.successful_allocations++;
        ctx->stats.allocation_success_rate = 
            (double)ctx->stats.successful_allocations / ctx->stats.total_allocations * 100.0;
        
        // Call allocation callback
        if (g_allocation_callback) {
            g_allocation_callback(allocation);
        }
        
        // Remove from pending requests
        cancel_resource_request(ctx, request_id);
        
        return 0;
    }
    
    return -1; // No space for new allocation
}

int deallocate_resources(proactive_allocator_ctx_t* ctx, uint64_t allocation_id) {
    if (!ctx) return -1;
    
    for (int i = 0; i < ctx->active_allocation_count; i++) {
        if (ctx->active_allocations[i].allocation_id == allocation_id) {
            resource_allocation_t* allocation = &ctx->active_allocations[i];
            
            if (!allocation->is_active) return -1;
            
            // Update resource pool
            for (int j = 0; j < ctx->pool_count; j++) {
                if (ctx->resource_pools[j].type == allocation->request.resource_type) {
                    ctx->resource_pools[j].currently_allocated -= allocation->allocated_amount;
                    ctx->resource_pools[j].available_capacity += allocation->allocated_amount;
                    ctx->resource_pools[j].deallocation_count++;
                    ctx->resource_pools[j].utilization_percentage = 
                        (double)ctx->resource_pools[j].currently_allocated / ctx->resource_pools[j].total_capacity * 100.0;
                    ctx->resource_pools[j].last_update_time = get_timestamp_ms_internal();
                    break;
                }
            }
            
            // Call deallocation callback
            if (g_deallocation_callback) {
                g_deallocation_callback(allocation);
            }
            
            allocation->is_active = 0;
            return 0;
        }
    }
    
    return -1; // Allocation not found
}

int reallocate_resources(proactive_allocator_ctx_t* ctx) {
    if (!ctx) return -1;
    
    uint64_t current_time = get_timestamp_ms_internal();
    
    // Check for expired allocations
    for (int i = 0; i < ctx->active_allocation_count; i++) {
        if (ctx->active_allocations[i].is_active && 
            ctx->active_allocations[i].expiration_time < current_time) {
            deallocate_resources(ctx, ctx->active_allocations[i].allocation_id);
            ctx->stats.reallocated_resources++;
        }
    }
    
    // Perform optimization based on current strategy
    optimize_resource_allocation(ctx);
    
    ctx->last_reallocation_time = current_time;
    return 0;
}

int optimize_resource_allocation(proactive_allocator_ctx_t* ctx) {
    if (!ctx) return -1;
    
    ctx->is_optimizing = 1;
    ctx->stats.last_optimization_time = get_timestamp_ms_internal();
    
    // Simple optimization: consolidate small allocations
    // In a real implementation, this would use more sophisticated algorithms
    
    // Update efficiency scores
    for (int i = 0; i < ctx->active_allocation_count; i++) {
        if (ctx->active_allocations[i].is_active) {
            // Calculate efficiency based on utilization
            double efficiency = ctx->active_allocations[i].utilization_efficiency;
            if (ctx->efficiency_history_index < 1000) {
                ctx->resource_efficiency_history[ctx->efficiency_history_index] = efficiency;
                ctx->efficiency_history_index++;
            }
        }
    }
    
    ctx->is_optimizing = 0;
    return 0;
}

// Proactive allocation
int perform_proactive_allocation_cycle(proactive_allocator_ctx_t* ctx) {
    if (!ctx) return -1;
    
    // Generate demand forecasts for each resource type
    for (int i = 0; i < ctx->pool_count; i++) {
        resource_type_t type = ctx->resource_pools[i].type;
        demand_forecast_t* forecast = generate_demand_forecast(ctx, type, ctx->config.prediction_horizon_seconds);
        if (forecast && ctx->forecast_count < 1000) {
            ctx->demand_forecasts[ctx->forecast_count] = *forecast;
            ctx->forecast_count++;
        }
    }
    
    // Pre-allocate resources based on forecasts
    if (ctx->config.enable_proactive_allocation) {
        for (int i = 0; i < ctx->forecast_count; i++) {
            demand_forecast_t* forecast = &ctx->demand_forecasts[i];
            uint64_t safety_margin = (uint64_t)(forecast->forecasted_demand * ctx->config.safety_margin_percent / 100.0);
            uint64_t required_amount = forecast->forecasted_demand + safety_margin;
            
            // Check if we need to pre-allocate
            uint64_t available = get_available_resources(ctx, forecast->resource_type);
            if (available < required_amount) {
                // Create a proactive allocation request
                resource_request_t request = {0};
                request.resource_type = forecast->resource_type;
                request.requested_amount = required_amount - available;
                request.minimum_required = request.requested_amount;
                request.maximum_acceptable = request.requested_amount * 2;
                request.priority = 50; // Medium priority
                request.deadline_ms = ctx->config.prediction_horizon_seconds * 1000;
                request.is_preemptible = 1;
                
                int req_len = 0;
                const char* req_id = "proactive_allocator";
                while (req_len < 63 && req_id[req_len]) {
                    request.requester_id[req_len] = req_id[req_len];
                    req_len++;
                }
                request.requester_id[req_len] = '\0';
                
                req_len = 0;
                const char* purpose = "Proactive resource allocation";
                while (req_len < 127 && purpose[req_len]) {
                    request.purpose[req_len] = purpose[req_len];
                    req_len++;
                }
                request.purpose[req_len] = '\0';
                
                uint64_t request_id = request_resources(ctx, &request);
                if (request_id > 0) {
                    allocate_resources(ctx, request_id);
                }
            }
        }
    }
    
    return 0;
}

demand_forecast_t* generate_demand_forecast(proactive_allocator_ctx_t* ctx, resource_type_t type, int horizon_seconds) {
    if (!ctx) return NULL;
    
    static demand_forecast_t forecast;
    
    // Simple forecasting based on current utilization and trends
    uint64_t current_utilization = 0;
    uint64_t total_capacity = 0;
    
    for (int i = 0; i < ctx->pool_count; i++) {
        if (ctx->resource_pools[i].type == type) {
            current_utilization = ctx->resource_pools[i].currently_allocated;
            total_capacity = ctx->resource_pools[i].total_capacity;
            break;
        }
    }
    
    // Simple trend analysis (in a real implementation, this would use ML)
    double utilization_ratio = (double)current_utilization / total_capacity;
    double trend_slope = 0.01; // Small increasing trend
    
    forecast.resource_type = type;
    forecast.forecasted_demand = (uint64_t)(current_utilization * (1.0 + trend_slope * horizon_seconds / 60.0));
    forecast.confidence_interval_min = (uint64_t)(forecast.forecasted_demand * 0.8);
    forecast.confidence_interval_max = (uint64_t)(forecast.forecasted_demand * 1.2);
    forecast.confidence_level = 0.85;
    forecast.forecast_time = get_timestamp_ms_internal();
    forecast.validity_period_seconds = horizon_seconds;
    
    int method_len = 0;
    const char* method = "trend_analysis";
    while (method_len < 63 && method[method_len]) {
        forecast.forecast_method[method_len] = method[method_len];
        method_len++;
    }
    forecast.forecast_method[method_len] = '\0';
    
    forecast.trend_slope = trend_slope;
    forecast.is_increasing_trend = (trend_slope > 0);
    
    return &forecast;
}

// Resource pressure monitoring
resource_pressure_t* monitor_resource_pressure(proactive_allocator_ctx_t* ctx, resource_type_t type) {
    if (!ctx) return NULL;
    
    static resource_pressure_t pressure;
    
    for (int i = 0; i < ctx->pool_count; i++) {
        if (ctx->resource_pools[i].type == type) {
            pressure.resource_type = type;
            pressure.current_pressure = ctx->resource_pools[i].utilization_percentage;
            pressure.predicted_pressure = pressure.current_pressure * 1.1; // Simple prediction
            pressure.pressure_trend = 0.5; // Slight increasing trend
            pressure.pressure_timestamp = get_timestamp_ms_internal();
            pressure.is_critical = (pressure.current_pressure > 90.0);
            
            int desc_len = 0;
            const char* desc = "Resource pressure monitoring";
            while (desc_len < 250 && desc[desc_len]) {
                pressure.pressure_description[desc_len] = desc[desc_len];
                desc_len++;
            }
            pressure.pressure_description[desc_len] = '\0';
            
            pressure.mitigation_recommendation_score = pressure.is_critical ? 90.0 : 30.0;
            
            // Call pressure callback
            if (g_pressure_callback) {
                g_pressure_callback(&pressure);
            }
            
            return &pressure;
        }
    }
    
    return NULL;
}

bool is_resource_critical(proactive_allocator_ctx_t* ctx, resource_type_t type) {
    resource_pressure_t* pressure = monitor_resource_pressure(ctx, type);
    return pressure ? pressure->is_critical : false;
}

// Statistics and reporting
allocation_stats_t get_allocation_statistics(proactive_allocator_ctx_t* ctx) {
    if (!ctx) {
        allocation_stats_t empty_stats = {0};
        return empty_stats;
    }
    return ctx->stats;
}

void reset_allocation_statistics(proactive_allocator_ctx_t* ctx) {
    if (!ctx) return;
    
    ctx->stats.total_allocations = 0;
    ctx->stats.successful_allocations = 0;
    ctx->stats.failed_allocations = 0;
    ctx->stats.preempted_allocations = 0;
    ctx->stats.reallocated_resources = 0;
    ctx->stats.allocation_success_rate = 0.0;
    ctx->stats.average_resource_utilization = 0.0;
    ctx->stats.resource_efficiency_score = 0.0;
    ctx->stats.cost_effectiveness_ratio = 0.0;
    ctx->stats.peak_allocation_time = 0;
    ctx->stats.last_optimization_time = get_timestamp_ms_internal();
    ctx->stats.optimization_gain_percent = 0.0;
}

// Callback registration
void register_allocation_callback(allocation_callback_t callback) {
    g_allocation_callback = callback;
}

void register_deallocation_callback(deallocation_callback_t callback) {
    g_deallocation_callback = callback;
}

void register_resource_pressure_callback(resource_pressure_callback_t callback) {
    g_pressure_callback = callback;
}

void register_allocation_stats_callback(allocation_stats_callback_t callback) {
    g_stats_callback = callback;
}

void register_resource_availability_callback(resource_availability_callback_t callback) {
    g_availability_callback = callback;
}

// Integration functions
int integrate_with_predictive_optimizer(proactive_allocator_ctx_t* ctx) {
    return 0;
}

int integrate_with_auto_scaler(proactive_allocator_ctx_t* ctx) {
    return 0;
}

int integrate_with_memory_manager(proactive_allocator_ctx_t* ctx) {
    return 0;
}

int apply_proactive_allocations(proactive_allocator_ctx_t* ctx) {
    return 0;
}