/*
 * MTProxy Integration Layer
 * Main integration point for all advanced systems
 */

#ifndef MTPROXY_INTEGRATION_LAYER_H
#define MTPROXY_INTEGRATION_LAYER_H

// Forward declarations to avoid circular dependencies
struct diagnostic_context;
struct monitoring_context;
struct debug_framework_context;
struct correlation_engine_context;
struct integration_coordinator_context;

#define MAX_ACTIVE_SESSIONS 1000
#define INTEGRATION_LAYER_VERSION 1.0f

typedef enum {
    INTEGRATION_MODE_NORMAL = 0,
    INTEGRATION_MODE_DIAGNOSTIC = 1,
    INTEGRATION_MODE_MONITORING = 2,
    INTEGRATION_MODE_DEBUGGING = 3,
    INTEGRATION_MODE_ANALYTICS = 4
} integration_mode_t;

typedef struct {
    struct diagnostic_context* diagnostic_ctx;
    struct monitoring_context* monitoring_ctx;
    struct debug_framework_context* debug_ctx;
    struct correlation_engine_context* correlation_ctx;
    struct integration_coordinator_context* coordinator_ctx;
    
    integration_mode_t current_mode;
    int is_initialized;
    int is_running;
    unsigned long startup_time;
    float integration_score;
    
    // Statistics
    unsigned long total_requests_processed;
    unsigned long total_errors_detected;
    unsigned long total_anomalies_found;
    float average_response_time;
} integration_layer_context_t;

// Function declarations
int init_integration_layer(integration_layer_context_t* ctx);
int start_integration_layer(integration_layer_context_t* ctx);
int stop_integration_layer(integration_layer_context_t* ctx);
int set_integration_mode(integration_layer_context_t* ctx, integration_mode_t mode);
int process_request_with_diagnostics(integration_layer_context_t* ctx, void* request_data);
int trigger_comprehensive_analysis(integration_layer_context_t* ctx);
int get_integration_statistics(integration_layer_context_t* ctx, char* stats_buffer, int buffer_size);
int register_with_coordinator(integration_layer_context_t* ctx);
void cleanup_integration_layer(integration_layer_context_t* ctx);

#endif // MTPROXY_INTEGRATION_LAYER_H