/*
 * Performance Correlation Engine for MTProxy
 * Provides correlation between various performance metrics and system diagnostics
 */

#ifndef PERFORMANCE_CORRELATION_ENGINE_H
#define PERFORMANCE_CORRELATION_ENGINE_H

// Forward declarations to avoid circular dependencies
struct diagnostic_result;
struct metric_data;
struct debug_trace;

#define MAX_CORRELATION_GROUPS 64
#define MAX_METRICS_PER_GROUP 128
#define CORRELATION_THRESHOLD 0.7f
#define CORRELATION_WINDOW_SIZE 1000  // Number of samples to consider

typedef enum {
    CORRELATION_TYPE_NONE = 0,
    CORRELATION_TYPE_DIRECT = 1,
    CORRELATION_TYPE_INVERSE = 2,
    CORRELATION_TYPE_COMPLEX = 3,
    CORRELATION_TYPE_CYCLIC = 4
} correlation_type_t;

typedef enum {
    CORRELATION_SOURCE_DIAGNOSTICS = 0,
    CORRELATION_SOURCE_MONITORING = 1,
    CORRELATION_SOURCE_DEBUGGING = 2,
    CORRELATION_SOURCE_PERFORMANCE = 3,
    CORRELATION_SOURCE_NETWORK = 4,
    CORRELATION_SOURCE_MEMORY = 5,
    CORRELATION_SOURCE_CPU = 6
} correlation_source_t;

typedef struct {
    char metric_name[64];
    correlation_source_t source_type;
    float correlation_coefficient;
    unsigned long timestamp;
    void* data_ptr;
} correlation_metric_t;

typedef struct {
    char group_name[128];
    correlation_type_t correlation_type;
    int metric_count;
    correlation_metric_t metrics[MAX_METRICS_PER_GROUP];
    float correlation_matrix[MAX_METRICS_PER_GROUP][MAX_METRICS_PER_GROUP];
    unsigned long last_updated;
} correlation_group_t;

typedef struct {
    int group_count;
    correlation_group_t groups[MAX_CORRELATION_GROUPS];
    float correlation_threshold;
    int window_size;
    unsigned long last_analysis_time;
    int total_correlations_found;
} correlation_engine_context_t;

// Function declarations
int init_correlation_engine(correlation_engine_context_t* ctx);
int add_correlation_group(correlation_engine_context_t* ctx, const char* group_name, correlation_type_t type);
int add_metric_to_group(correlation_engine_context_t* ctx, const char* group_name, const char* metric_name, correlation_source_t source, void* data_ptr);
int analyze_correlations(correlation_engine_context_t* ctx);
float calculate_correlation_coefficient(float* series_a, float* series_b, int size);
int generate_correlation_report(correlation_engine_context_t* ctx, char* report_buffer, int buffer_size);
int detect_performance_anomalies(correlation_engine_context_t* ctx, correlation_metric_t* anomalies, int max_anomalies);
int predict_performance_impact(correlation_engine_context_t* ctx, const char* affected_metric, float predicted_change, float* impact_result);
int register_correlation_callback(correlation_engine_context_t* ctx, void (*callback)(correlation_metric_t*, float));
void cleanup_correlation_engine(correlation_engine_context_t* ctx);

#endif // PERFORMANCE_CORRELATION_ENGINE_H