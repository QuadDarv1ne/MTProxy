/*
 * Performance Correlation Engine Implementation for MTProxy
 * Provides correlation between various performance metrics and system diagnostics
 */

#include "performance-correlation-engine.h"

// Helper function to compare strings (since we can't use standard library)
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

// Helper function for simple integer to string conversion
static int int_to_string(int value, char* str, int max_len) {
    if (max_len <= 0) return 0;
    
    int temp = value;
    int digits = 0;
    
    if (temp == 0) {
        if (max_len < 2) return 0;
        str[0] = '0';
        str[1] = '\0';
        return 1;
    }
    
    if (temp < 0) {
        if (max_len < 2) return 0;
        str[0] = '-';
        str++;
        max_len--;
        temp = -temp;
    }
    
    int temp_val = temp;
    while (temp_val > 0) {
        temp_val /= 10;
        digits++;
    }
    
    if (digits >= max_len) return 0;
    
    str[digits] = '\0';
    int pos = digits - 1;
    while (temp > 0 && pos >= 0) {
        str[pos--] = '0' + (temp % 10);
        temp /= 10;
    }
    
    return digits;
}

// Helper function for simple float to string conversion (limited precision)
static int float_to_string(float value, char* str, int max_len) {
    if (max_len <= 0) return 0;
    
    int int_part = (int)value;
    float frac_part = value - int_part;
    
    if (frac_part < 0) frac_part = -frac_part;
    
    int int_len = int_to_string(int_part, str, max_len);
    if (int_len <= 0) return 0;
    
    if (int_len >= max_len - 1) return int_len;
    
    str[int_len] = '.';
    int pos = int_len + 1;
    
    // Add 2 decimal places
    frac_part *= 100;
    int frac_int = (int)frac_part;
    
    if (frac_int > 99) frac_int = 99;
    
    if (pos >= max_len - 2) return int_len;
    
    str[pos] = '0' + (frac_int / 10);
    str[pos + 1] = '0' + (frac_int % 10);
    str[pos + 2] = '\0';
    
    return pos + 2;
}

// Helper function for simple string formatting (simplified snprintf)
static int simple_sprintf(char* str, int size, const char* format, int val1, int val2, float val3) {
    int pos = 0;
    int fmt_pos = 0;
    
    while (format[fmt_pos] != '\0' && pos < size - 1) {
        if (format[fmt_pos] == '%' && format[fmt_pos + 1] != '\0') {
            fmt_pos++; // Move past %
            
            if (format[fmt_pos] == 'd') {
                char temp_str[16];
                int len = int_to_string(val1, temp_str, 16);
                if (len > 0 && pos + len < size) {
                    int i;
                    for (i = 0; i < len; i++) {
                        str[pos++] = temp_str[i];
                    }
                }
            } else if (format[fmt_pos] == 'f') {
                char temp_str[16];
                int len = float_to_string(val3, temp_str, 16);
                if (len > 0 && pos + len < size) {
                    int i;
                    for (i = 0; i < len; i++) {
                        str[pos++] = temp_str[i];
                    }
                }
            } else if (format[fmt_pos] == 's') {
                // We'll handle this case differently since we don't have full format support
            }
        } else if (format[fmt_pos] == '{') {
            // Handle special markers for our specific format needs
            fmt_pos++; // Skip '{'
            if (format[fmt_pos] == '1' && format[fmt_pos + 1] == '}') {
                char temp_str[16];
                int len = int_to_string(val1, temp_str, 16);
                if (len > 0 && pos + len < size) {
                    int i;
                    for (i = 0; i < len; i++) {
                        str[pos++] = temp_str[i];
                    }
                    fmt_pos++; // Skip '}'
                }
            } else if (format[fmt_pos] == '2' && format[fmt_pos + 1] == '}') {
                char temp_str[16];
                int len = int_to_string(val2, temp_str, 16);
                if (len > 0 && pos + len < size) {
                    int i;
                    for (i = 0; i < len; i++) {
                        str[pos++] = temp_str[i];
                    }
                    fmt_pos++; // Skip '}'
                }
            } else if (format[fmt_pos] == '3' && format[fmt_pos + 1] == '}') {
                char temp_str[16];
                int len = float_to_string(val3, temp_str, 16);
                if (len > 0 && pos + len < size) {
                    int i;
                    for (i = 0; i < len; i++) {
                        str[pos++] = temp_str[i];
                    }
                    fmt_pos++; // Skip '}'
                }
            } else {
                str[pos++] = '{'; // Put back the '{'
                str[pos++] = format[fmt_pos]; // Put back the next char
            }
        } else {
            str[pos++] = format[fmt_pos];
        }
        fmt_pos++;
    }
    
    str[pos] = '\0';
    return pos;
}

// Square root approximation using Newton's method
static float sqrt_approx(float x) {
    if (x <= 0) return 0.0f;
    if (x == 1.0f) return 1.0f;
    
    float guess = x * 0.5f; // Initial guess
    int iterations = 10; // Number of iterations
    
    for (int i = 0; i < iterations; i++) {
        guess = 0.5f * (guess + x / guess);
    }
    
    return guess;
}

// Initialize the correlation engine
int init_correlation_engine(correlation_engine_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    // Initialize all fields to zero/default values
    ctx->group_count = 0;
    ctx->correlation_threshold = CORRELATION_THRESHOLD;
    ctx->window_size = CORRELATION_WINDOW_SIZE;
    ctx->last_analysis_time = 0;
    ctx->total_correlations_found = 0;
    
    // Initialize all correlation groups
    int i, j, k;
    for (i = 0; i < MAX_CORRELATION_GROUPS; i++) {
        correlation_group_t* group = &ctx->groups[i];
        group->group_name[0] = '\0';
        group->correlation_type = CORRELATION_TYPE_NONE;
        group->metric_count = 0;
        
        for (j = 0; j < MAX_METRICS_PER_GROUP; j++) {
            correlation_metric_t* metric = &group->metrics[j];
            metric->metric_name[0] = '\0';
            metric->source_type = CORRELATION_SOURCE_PERFORMANCE;
            metric->correlation_coefficient = 0.0f;
            metric->timestamp = 0;
            metric->data_ptr = 0;
            
            for (k = 0; k < MAX_METRICS_PER_GROUP; k++) {
                group->correlation_matrix[j][k] = 0.0f;
            }
        }
        group->last_updated = 0;
    }
    
    return 0;
}

// Add a correlation group
int add_correlation_group(correlation_engine_context_t* ctx, const char* group_name, correlation_type_t type) {
    if (!ctx || !group_name || ctx->group_count >= MAX_CORRELATION_GROUPS) {
        return -1;
    }
    
    correlation_group_t* group = &ctx->groups[ctx->group_count];
    
    // Copy group name with bounds checking
    int i = 0;
    while (i < 127 && group_name[i] != '\0') {
        group->group_name[i] = group_name[i];
        i++;
    }
    group->group_name[i] = '\0';
    
    group->correlation_type = type;
    group->metric_count = 0;
    group->last_updated = 0;
    
    ctx->group_count++;
    return 0;
}

// Add a metric to a correlation group
int add_metric_to_group(correlation_engine_context_t* ctx, const char* group_name, const char* metric_name, correlation_source_t source, void* data_ptr) {
    if (!ctx || !group_name || !metric_name) {
        return -1;
    }
    
    // Find the group
    int group_idx = -1;
    int i;
    for (i = 0; i < ctx->group_count; i++) {
        if (string_compare(ctx->groups[i].group_name, group_name) == 0) {
            group_idx = i;
            break;
        }
    }
    
    if (group_idx == -1) {
        return -1;
    }
    
    correlation_group_t* group = &ctx->groups[group_idx];
    if (group->metric_count >= MAX_METRICS_PER_GROUP) {
        return -1;
    }
    
    correlation_metric_t* metric = &group->metrics[group->metric_count];
    
    // Copy metric name with bounds checking
    int j = 0;
    while (j < 63 && metric_name[j] != '\0') {
        metric->metric_name[j] = metric_name[j];
        j++;
    }
    metric->metric_name[j] = '\0';
    
    metric->source_type = source;
    metric->correlation_coefficient = 0.0f;
    metric->timestamp = 0;
    metric->data_ptr = data_ptr;
    
    group->metric_count++;
    group->last_updated = 0;
    
    return 0;
}

// Analyze correlations between metrics
int analyze_correlations(correlation_engine_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    int i, j, k;
    for (i = 0; i < ctx->group_count; i++) {
        correlation_group_t* group = &ctx->groups[i];
        if (group->metric_count < 2) {
            continue;
        }
        
        // Calculate correlation matrix for this group
        for (j = 0; j < group->metric_count; j++) {
            for (k = 0; k < group->metric_count; k++) {
                if (j == k) {
                    group->correlation_matrix[j][k] = 1.0f;
                } else {
                    // In a real implementation, we would extract actual data values
                    // For now, we'll simulate with placeholder calculation
                    group->correlation_matrix[j][k] = 0.0f;
                    
                    // Count correlations that exceed threshold
                    if (group->correlation_matrix[j][k] > ctx->correlation_threshold ||
                        group->correlation_matrix[j][k] < -ctx->correlation_threshold) {
                        ctx->total_correlations_found++;
                    }
                }
            }
        }
        
        group->last_updated = 0;
    }
    
    ctx->last_analysis_time = 0;
    return 0;
}

// Calculate correlation coefficient between two data series
float calculate_correlation_coefficient(float* series_a, float* series_b, int size) {
    if (!series_a || !series_b || size <= 0) {
        return 0.0f;
    }
    
    if (size == 1) {
        return 1.0f;
    }
    
    // Calculate means
    float mean_a = 0.0f, mean_b = 0.0f;
    int i;
    for (i = 0; i < size; i++) {
        mean_a += series_a[i];
        mean_b += series_b[i];
    }
    mean_a /= size;
    mean_b /= size;
    
    // Calculate numerator and denominators for correlation coefficient
    float numerator = 0.0f, sum_sq_a = 0.0f, sum_sq_b = 0.0f;
    for (i = 0; i < size; i++) {
        float diff_a = series_a[i] - mean_a;
        float diff_b = series_b[i] - mean_b;
        
        numerator += diff_a * diff_b;
        sum_sq_a += diff_a * diff_a;
        sum_sq_b += diff_b * diff_b;
    }
    
    // Calculate correlation coefficient
    float denominator = sqrt_approx(sum_sq_a * sum_sq_b);
    if (denominator == 0.0f) {
        return 0.0f;
    }
    
    return numerator / denominator;
}

// Generate correlation report
int generate_correlation_report(correlation_engine_context_t* ctx, char* report_buffer, int buffer_size) {
    if (!ctx || !report_buffer || buffer_size <= 0) {
        return -1;
    }
    
    // Start the report
    int pos = 0;
    const char* header = "Performance Correlation Report\n";
    int header_len = string_length(header);
    
    if (pos + header_len >= buffer_size) {
        return -1;
    }
    
    int i;
    for (i = 0; i < header_len; i++) {
        report_buffer[pos++] = header[i];
    }
    
    // Add correlation statistics
    char temp_buf[256];
    int temp_len = simple_sprintf(temp_buf, sizeof(temp_buf), 
                                 "Total Groups: {1}\nTotal Correlations Found: {2}\nThreshold: {3}\n\n", 
                                 ctx->group_count, ctx->total_correlations_found, (float)ctx->correlation_threshold);
    
    if (pos + temp_len >= buffer_size) {
        return -1;
    }
    
    for (i = 0; i < temp_len && pos < buffer_size - 1; i++) {
        report_buffer[pos++] = temp_buf[i];
    }
    
    // Add group details
    int g, m1, m2;
    for (g = 0; g < ctx->group_count; g++) {
        correlation_group_t* group = &ctx->groups[g];
        
        // Create group header
        int name_len = string_length(group->group_name);
        if (pos + name_len + 30 >= buffer_size) {
            break;
        }
        
        // Add "Group: " prefix
        const char* group_prefix = "Group: ";
        int prefix_len = string_length(group_prefix);
        for (i = 0; i < prefix_len && pos < buffer_size - 1; i++) {
            report_buffer[pos++] = group_prefix[i];
        }
        
        // Add group name
        for (i = 0; i < name_len && pos < buffer_size - 1; i++) {
            report_buffer[pos++] = group->group_name[i];
        }
        
        // Add metrics count
        char metrics_info[64];
        int metrics_len = simple_sprintf(metrics_info, sizeof(metrics_info), " ({1} metrics)\n", group->metric_count, 0, 0.0f);
        for (i = 0; i < metrics_len && pos < buffer_size - 1; i++) {
            report_buffer[pos++] = metrics_info[i];
        }
        
        // Add metric correlations
        for (m1 = 0; m1 < group->metric_count && m1 < 10; m1++) {  // Limit to first 10 metrics for brevity
            for (m2 = m1 + 1; m2 < group->metric_count && m2 < 10; m2++) {
                float corr = group->correlation_matrix[m1][m2];
                if (corr > ctx->correlation_threshold || corr < -ctx->correlation_threshold) {
                    // Create correlation string
                    char metric1_name[65], metric2_name[65], corr_str[10];
                    int m1_len = (string_length(group->metrics[m1].metric_name) < 64) ? 
                                string_length(group->metrics[m1].metric_name) : 64;
                    int m2_len = (string_length(group->metrics[m2].metric_name) < 64) ? 
                                string_length(group->metrics[m2].metric_name) : 64;
                    
                    for (i = 0; i < m1_len; i++) metric1_name[i] = group->metrics[m1].metric_name[i];
                    metric1_name[m1_len] = '\0';
                    for (i = 0; i < m2_len; i++) metric2_name[i] = group->metrics[m2].metric_name[i];
                    metric2_name[m2_len] = '\0';
                    
                    int corr_len = float_to_string(corr, corr_str, 10);
                    
                    char corr_line[256];
                    int line_len = 0;
                    
                    // Build correlation line manually
                    const char* arrow = " <-> ";
                    const char* colon = ": ";
                    const char* nl = "\n";
                    
                    int base_pos = 0;
                    // Add spaces for indentation
                    corr_line[base_pos++] = ' ';
                    corr_line[base_pos++] = ' ';
                    
                    // Add first metric name
                    for (i = 0; i < m1_len && base_pos < 250; i++) {
                        corr_line[base_pos++] = metric1_name[i];
                    }
                    
                    // Add arrow
                    int arrow_len = string_length(arrow);
                    for (i = 0; i < arrow_len && base_pos < 250; i++) {
                        corr_line[base_pos++] = arrow[i];
                    }
                    
                    // Add second metric name
                    for (i = 0; i < m2_len && base_pos < 250; i++) {
                        corr_line[base_pos++] = metric2_name[i];
                    }
                    
                    // Add colon
                    int colon_len = string_length(colon);
                    for (i = 0; i < colon_len && base_pos < 250; i++) {
                        corr_line[base_pos++] = colon[i];
                    }
                    
                    // Add correlation value
                    for (i = 0; i < corr_len && base_pos < 250; i++) {
                        corr_line[base_pos++] = corr_str[i];
                    }
                    
                    // Add newline
                    int nl_len = string_length(nl);
                    for (i = 0; i < nl_len && base_pos < 250; i++) {
                        corr_line[base_pos++] = nl[i];
                    }
                    
                    corr_line[base_pos] = '\0';
                    line_len = base_pos;
                    
                    if (pos + line_len >= buffer_size) {
                        break;
                    }
                    
                    for (i = 0; i < line_len && pos < buffer_size - 1; i++) {
                        report_buffer[pos++] = corr_line[i];
                    }
                }
            }
        }
        
        if (pos >= buffer_size - 1) {
            break;
        }
        
        report_buffer[pos++] = '\n';
    }
    
    report_buffer[pos] = '\0';
    return pos;
}

// Detect performance anomalies
int detect_performance_anomalies(correlation_engine_context_t* ctx, correlation_metric_t* anomalies, int max_anomalies) {
    if (!ctx || !anomalies || max_anomalies <= 0) {
        return -1;
    }
    
    int anomaly_count = 0;
    
    int g, m;
    for (g = 0; g < ctx->group_count && anomaly_count < max_anomalies; g++) {
        correlation_group_t* group = &ctx->groups[g];
        
        for (m = 0; m < group->metric_count && anomaly_count < max_anomalies; m++) {
            correlation_metric_t* metric = &group->metrics[m];
            
            // In a real implementation, we would check for anomalies based on historical data
            // For now, we'll simulate detection by checking correlation coefficients
            if (metric->correlation_coefficient > 0.9f || metric->correlation_coefficient < -0.9f) {
                anomalies[anomaly_count] = *metric;
                anomaly_count++;
            }
        }
    }
    
    return anomaly_count;
}

// Predict performance impact
int predict_performance_impact(correlation_engine_context_t* ctx, const char* affected_metric, float predicted_change, float* impact_result) {
    if (!ctx || !affected_metric || !impact_result) {
        return -1;
    }
    
    // Find the metric in our correlation groups
    int g, m;
    for (g = 0; g < ctx->group_count; g++) {
        correlation_group_t* group = &ctx->groups[g];
        
        for (m = 0; m < group->metric_count; m++) {
            if (string_compare(group->metrics[m].metric_name, affected_metric) == 0) {
                // Calculate potential impact based on correlations
                float total_impact = 0.0f;
                int correlated_metrics = 0;
                
                int other_m;
                for (other_m = 0; other_m < group->metric_count; other_m++) {
                    if (m != other_m) {
                        float correlation = group->correlation_matrix[m][other_m];
                        
                        // Only consider strong correlations
                        if (correlation > ctx->correlation_threshold || 
                            correlation < -ctx->correlation_threshold) {
                            
                            total_impact += correlation * predicted_change;
                            correlated_metrics++;
                        }
                    }
                }
                
                if (correlated_metrics > 0) {
                    *impact_result = total_impact / correlated_metrics;
                } else {
                    *impact_result = 0.0f;
                }
                
                return 0;
            }
        }
    }
    
    return -1; // Metric not found
}

// Register correlation callback
int register_correlation_callback(correlation_engine_context_t* ctx, void (*callback)(correlation_metric_t*, float)) {
    if (!ctx || !callback) {
        return -1;
    }
    
    // In a real implementation, we would store the callback
    // For now, we'll just return success
    return 0;
}

// Cleanup correlation engine
void cleanup_correlation_engine(correlation_engine_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    // Reset all fields to zero/default values
    ctx->group_count = 0;
    ctx->correlation_threshold = CORRELATION_THRESHOLD;
    ctx->window_size = CORRELATION_WINDOW_SIZE;
    ctx->last_analysis_time = 0;
    ctx->total_correlations_found = 0;
    
    int i, j, k;
    for (i = 0; i < MAX_CORRELATION_GROUPS; i++) {
        correlation_group_t* group = &ctx->groups[i];
        group->group_name[0] = '\0';
        group->correlation_type = CORRELATION_TYPE_NONE;
        group->metric_count = 0;
        
        for (j = 0; j < MAX_METRICS_PER_GROUP; j++) {
            correlation_metric_t* metric = &group->metrics[j];
            metric->metric_name[0] = '\0';
            metric->source_type = CORRELATION_SOURCE_PERFORMANCE;
            metric->correlation_coefficient = 0.0f;
            metric->timestamp = 0;
            metric->data_ptr = 0;
            
            for (k = 0; k < MAX_METRICS_PER_GROUP; k++) {
                group->correlation_matrix[j][k] = 0.0f;
            }
        }
        group->last_updated = 0;
    }
}