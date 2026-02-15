/*
 * real-time-monitoring-dashboard.h
 * Real-time Monitoring Dashboard for MTProxy
 *
 * Comprehensive monitoring dashboard with live metrics, charts, and alerts.
 */

#ifndef REAL_TIME_MONITORING_DASHBOARD_H
#define REAL_TIME_MONITORING_DASHBOARD_H

#include <stdint.h>
#include <stdbool.h>

// Dashboard metric types
typedef enum {
    DASHBOARD_METRIC_CPU_USAGE = 0,
    DASHBOARD_METRIC_MEMORY_USAGE = 1,
    DASHBOARD_METRIC_NETWORK_IN = 2,
    DASHBOARD_METRIC_NETWORK_OUT = 3,
    DASHBOARD_METRIC_ACTIVE_CONNECTIONS = 4,
    DASHBOARD_METRIC_REQUESTS_PER_SECOND = 5,
    DASHBOARD_METRIC_AVG_RESPONSE_TIME = 6,
    DASHBOARD_METRIC_ERROR_RATE = 7,
    DASHBOARD_METRIC_CACHE_HIT_RATIO = 8,
    DASHBOARD_METRIC_CRYPTO_OPERATIONS = 9,
    DASHBOARD_METRIC_UPTIME = 10,
    DASHBOARD_METRIC_THREADS = 11,
    DASHBOARD_METRIC_OPEN_FILES = 12,
    DASHBOARD_METRIC_DISK_USAGE = 13
} dashboard_metric_type_t;

// Chart types
typedef enum {
    CHART_TYPE_LINE = 0,
    CHART_TYPE_BAR = 1,
    CHART_TYPE_GAUGE = 2,
    CHART_TYPE_PIE = 3,
    CHART_TYPE_HEATMAP = 4,
    CHART_TYPE_SCATTER = 5
} chart_type_t;

// Alert severity
typedef enum {
    ALERT_SEVERITY_LOW = 0,
    ALERT_SEVERITY_MEDIUM = 1,
    ALERT_SEVERITY_HIGH = 2,
    ALERT_SEVERITY_CRITICAL = 3
} alert_severity_t;

// Alert types
typedef enum {
    ALERT_TYPE_HIGH_CPU = 0,
    ALERT_TYPE_HIGH_MEMORY = 1,
    ALERT_TYPE_HIGH_LATENCY = 2,
    ALERT_TYPE_LOW_CACHE_HIT = 3,
    ALERT_TYPE_CONNECTION_DROPPED = 4,
    ALERT_TYPE_ERROR_RATE_SPIKE = 5,
    ALERT_TYPE_DISK_SPACE_LOW = 6,
    ALERT_TYPE_SECURITY_EVENT = 7
} alert_type_t;

// Dashboard widget types
typedef enum {
    WIDGET_TYPE_CHART = 0,
    WIDGET_TYPE_GAUGE = 1,
    WIDGET_TYPE_TABLE = 2,
    WIDGET_TYPE_ALERT_LIST = 3,
    WIDGET_TYPE_STATUS_INDICATOR = 4,
    WIDGET_TYPE_TEXT_PANEL = 5,
    WIDGET_TYPE_METRIC_CARD = 6
} widget_type_t;

// Dashboard layout positions
typedef enum {
    LAYOUT_POSITION_TOP = 0,
    LAYOUT_POSITION_BOTTOM = 1,
    LAYOUT_POSITION_LEFT = 2,
    LAYOUT_POSITION_RIGHT = 3,
    LAYOUT_POSITION_CENTER = 4,
    LAYOUT_POSITION_FULLSCREEN = 5
} layout_position_t;

// Metric data point
typedef struct {
    uint64_t timestamp;
    double value;
    double min_value;
    double max_value;
    double avg_value;
    double percentile_95;
    double percentile_99;
} metric_data_point_t;

// Dashboard metric
typedef struct {
    dashboard_metric_type_t metric_type;
    char metric_name[64];
    char unit[16];
    double current_value;
    double min_value;
    double max_value;
    double avg_value;
    double warning_threshold;
    double critical_threshold;
    bool is_enabled;
    uint64_t last_updated;
    metric_data_point_t* history;
    int history_count;
    int max_history_points;
    chart_type_t preferred_chart_type;
    bool show_on_dashboard;
    char description[128];
} dashboard_metric_t;

// Alert definition
typedef struct {
    uint64_t alert_id;
    alert_type_t alert_type;
    alert_severity_t severity;
    uint64_t timestamp;
    char title[128];
    char description[256];
    char affected_component[64];
    double current_value;
    double threshold_value;
    bool is_active;
    bool acknowledged;
    uint64_t acknowledged_at;
    char acknowledged_by[64];
    bool resolved;
    uint64_t resolved_at;
    char suggested_action[256];
    double confidence_score;
    bool auto_resolved;
    uint64_t auto_resolve_timeout;
} alert_definition_t;

// Dashboard widget
typedef struct {
    uint64_t widget_id;
    widget_type_t widget_type;
    char title[128];
    layout_position_t position;
    int x;
    int y;
    int width;
    int height;
    dashboard_metric_type_t* associated_metrics;
    int metric_count;
    chart_type_t chart_type;
    bool is_visible;
    uint64_t last_refresh;
    char custom_css[256];
    bool auto_refresh;
    uint64_t refresh_interval_ms;
    char custom_config[512];
} dashboard_widget_t;

// Dashboard configuration
typedef struct {
    bool enable_real_time_updates;
    bool enable_auto_refresh;
    uint64_t refresh_interval_ms;
    bool enable_alerts;
    bool enable_metric_history;
    int max_history_points;
    bool enable_export;
    bool enable_themes;
    char theme_name[32];
    bool enable_annotations;
    bool enable_zoom_pan;
    bool enable_tooltip;
    bool enable_legend;
    bool enable_grid_lines;
    double opacity;
    bool enable_animation;
    uint64_t animation_duration_ms;
    bool enable_dark_mode;
    bool enable_responsive_layout;
    bool enable_fullscreen_mode;
    bool enable_user_preferences;
    bool enable_access_control;
    int max_widgets;
    bool enable_widget_sharing;
    bool enable_template_system;
    bool enable_custom_dashboards;
    int max_dashboards;
    bool enable_data_export;
    bool enable_screenshot;
    bool enable_print_view;
    bool enable_share_link;
    bool enable_embed_mode;
} dashboard_config_t;

// Dashboard view
typedef struct {
    uint64_t view_id;
    char view_name[64];
    char description[256];
    dashboard_widget_t* widgets;
    int widget_count;
    int max_widgets;
    bool is_default_view;
    bool is_public;
    uint64_t created_at;
    uint64_t last_modified;
    char owner[64];
    bool is_locked;
    char permissions[128];
    bool auto_save;
    uint64_t auto_save_interval_ms;
} dashboard_view_t;

// Live metrics snapshot
typedef struct {
    uint64_t snapshot_id;
    uint64_t timestamp;
    double cpu_usage_percent;
    double memory_usage_percent;
    double network_in_mbps;
    double network_out_mbps;
    uint64_t active_connections;
    uint64_t total_connections;
    uint64_t failed_connections;
    double avg_response_time_ms;
    double p95_response_time_ms;
    double p99_response_time_ms;
    uint64_t requests_per_second;
    double error_rate_percent;
    double cache_hit_ratio;
    uint64_t current_rss_kb;
    uint64_t peak_rss_kb;
    uint64_t virtual_memory_kb;
    uint64_t open_files_count;
    uint64_t threads_count;
    double crypto_operations_per_second;
    uint64_t uptime_seconds;
    double disk_usage_percent;
    uint64_t total_disk_space_kb;
    uint64_t available_disk_space_kb;
    double temperature_celsius;
    uint64_t dropped_packets;
    uint64_t corrupted_packets;
    double encryption_time_avg_ms;
    double decryption_time_avg_ms;
    bool system_healthy;
    double system_health_score;
    int active_alerts_count;
    int resolved_alerts_count;
    uint64_t last_health_check;
} live_metrics_snapshot_t;

// Dashboard session
typedef struct {
    uint64_t session_id;
    uint64_t user_id;
    char username[64];
    uint64_t connected_at;
    uint64_t last_activity;
    uint64_t disconnected_at;
    bool is_active;
    char ip_address[48];
    char user_agent[256];
    bool has_admin_privileges;
    bool can_modify_dashboard;
    bool can_create_widgets;
    bool can_manage_alerts;
    uint64_t last_viewed_dashboard;
    char current_view[64];
    bool is_mobile_device;
    double screen_resolution_width;
    double screen_resolution_height;
    bool auto_refresh_enabled;
    uint64_t refresh_interval_override_ms;
} dashboard_session_t;

// Main dashboard context
typedef struct {
    // Configuration
    dashboard_config_t config;
    
    // Metrics management
    dashboard_metric_t* metrics;
    int metric_count;
    int max_metrics;
    
    // Alerts management
    alert_definition_t* active_alerts;
    int active_alert_count;
    int max_active_alerts;
    alert_definition_t* resolved_alerts;
    int resolved_alert_count;
    int max_resolved_alerts;
    
    // Widgets management
    dashboard_widget_t* widgets;
    int widget_count;
    int max_widgets;
    
    // Views management
    dashboard_view_t* views;
    int view_count;
    int max_views;
    uint64_t current_view_id;
    
    // Live data
    live_metrics_snapshot_t current_snapshot;
    live_metrics_snapshot_t* snapshot_history;
    int snapshot_count;
    int max_snapshots;
    uint64_t last_snapshot_time;
    
    // Sessions
    dashboard_session_t* active_sessions;
    int active_session_count;
    int max_sessions;
    
    // Performance tracking
    uint64_t total_snapshots_collected;
    uint64_t total_alerts_generated;
    uint64_t total_widgets_rendered;
    uint64_t total_data_points_processed;
    double average_render_time_ms;
    double average_data_collection_time_ms;
    uint64_t last_render_time;
    uint64_t last_data_collection_time;
    
    // User management
    int total_users_connected;
    int peak_concurrent_users;
    uint64_t peak_concurrent_users_time;
    
    // Export and reporting
    bool export_in_progress;
    uint64_t last_export_time;
    char last_export_filename[256];
    bool export_format_csv;
    bool export_format_json;
    bool export_format_pdf;
    bool export_format_png;
    
    // Theme and appearance
    char current_theme[32];
    bool dark_mode_enabled;
    double dashboard_opacity;
    bool animations_enabled;
    uint64_t animation_duration_ms;
    
    // State management
    bool dashboard_active;
    bool real_time_updates_active;
    bool auto_refresh_active;
    bool data_collection_active;
    bool alert_monitoring_active;
    int active_components;
    
    // Statistics
    uint64_t uptime_seconds;
    uint64_t start_time;
    double availability_percentage;
    uint64_t total_downtime_seconds;
    uint64_t last_downtime_start;
    
    // State
    int initialized;
    int active;
    uint64_t initialization_time;
    char dashboard_id[64];
    char version_string[32];
} dashboard_context_t;

// Callback function types
typedef void (*metric_update_callback_t)(dashboard_metric_type_t metric_type, double new_value);
typedef void (*alert_trigger_callback_t)(const alert_definition_t* alert);
typedef void (*snapshot_update_callback_t)(const live_metrics_snapshot_t* snapshot);
typedef void (*widget_render_callback_t)(uint64_t widget_id, const char* rendered_html);
typedef void (*dashboard_event_callback_t)(const char* event_type, const char* event_data);

// Function declarations

// Initialization and cleanup
int init_dashboard(dashboard_context_t* ctx);
int init_dashboard_with_config(dashboard_context_t* ctx, const dashboard_config_t* config);
void cleanup_dashboard(dashboard_context_t* ctx);

// Configuration management
void get_dashboard_config(dashboard_context_t* ctx, dashboard_config_t* config);
int set_dashboard_config(dashboard_context_t* ctx, const dashboard_config_t* config);
int enable_real_time_updates(dashboard_context_t* ctx, bool enable);
int set_refresh_interval(dashboard_context_t* ctx, uint64_t interval_ms);

// Metric management
int add_dashboard_metric(dashboard_context_t* ctx, dashboard_metric_type_t metric_type,
                       const char* name, const char* unit, double warning_threshold, 
                       double critical_threshold);
int update_metric_value(dashboard_context_t* ctx, dashboard_metric_type_t metric_type, 
                       double new_value);
int get_metric_value(dashboard_context_t* ctx, dashboard_metric_type_t metric_type, 
                    double* value);
int get_metric_history(dashboard_context_t* ctx, dashboard_metric_type_t metric_type,
                      metric_data_point_t* history, int max_points);
int set_metric_thresholds(dashboard_context_t* ctx, dashboard_metric_type_t metric_type,
                         double warning, double critical);

// Alert management
int trigger_alert(dashboard_context_t* ctx, alert_type_t alert_type, 
                 const char* title, const char* description, double current_value, 
                 double threshold_value);
int acknowledge_alert(dashboard_context_t* ctx, uint64_t alert_id, const char* acknowledged_by);
int resolve_alert(dashboard_context_t* ctx, uint64_t alert_id);
int get_active_alerts(dashboard_context_t* ctx, alert_definition_t* alerts, int max_alerts);
int get_alert_history(dashboard_context_t* ctx, alert_definition_t* alerts, int max_alerts);

// Widget management
int add_widget(dashboard_context_t* ctx, widget_type_t widget_type, const char* title,
              layout_position_t position, int x, int y, int width, int height);
int remove_widget(dashboard_context_t* ctx, uint64_t widget_id);
int update_widget_position(dashboard_context_t* ctx, uint64_t widget_id, int x, int y);
int update_widget_size(dashboard_context_t* ctx, uint64_t widget_id, int width, int height);
int get_widget(dashboard_context_t* ctx, uint64_t widget_id, dashboard_widget_t* widget);

// View management
int create_view(dashboard_context_t* ctx, const char* name, const char* description);
int switch_view(dashboard_context_t* ctx, uint64_t view_id);
int delete_view(dashboard_context_t* ctx, uint64_t view_id);
int get_current_view(dashboard_context_t* ctx, dashboard_view_t* view);
int add_widget_to_view(dashboard_context_t* ctx, uint64_t view_id, uint64_t widget_id);

// Data collection and snapshots
int collect_live_snapshot(dashboard_context_t* ctx);
live_metrics_snapshot_t get_current_snapshot(dashboard_context_t* ctx);
int get_snapshot_history(dashboard_context_t* ctx, live_metrics_snapshot_t* snapshots, 
                        int max_snapshots);
int export_snapshot_data(dashboard_context_t* ctx, const char* filename, 
                        uint64_t start_time, uint64_t end_time);

// Session management
int create_dashboard_session(dashboard_context_t* ctx, const char* username, 
                           const char* ip_address, const char* user_agent);
int end_dashboard_session(dashboard_context_t* ctx, uint64_t session_id);
int get_active_sessions(dashboard_context_t* ctx, dashboard_session_t* sessions, 
                      int max_sessions);

// Rendering and display
int render_dashboard(dashboard_context_t* ctx, char* output_buffer, int max_buffer_size);
int render_widget(dashboard_context_t* ctx, uint64_t widget_id, char* output_buffer, 
                 int max_buffer_size);
int render_metric_chart(dashboard_context_t* ctx, dashboard_metric_type_t metric_type,
                       chart_type_t chart_type, char* output_buffer, int max_buffer_size);

// Export and reporting
int export_dashboard_data(dashboard_context_t* ctx, const char* filename, 
                         const char* format);
int generate_report(dashboard_context_t* ctx, const char* report_type, 
                   char* report_buffer, int max_buffer_size);
int take_screenshot(dashboard_context_t* ctx, const char* filename);

// Theme and appearance
int set_theme(dashboard_context_t* ctx, const char* theme_name);
int toggle_dark_mode(dashboard_context_t* ctx, bool enable);
int set_dashboard_opacity(dashboard_context_t* ctx, double opacity);

// Statistics and monitoring
void get_dashboard_statistics(dashboard_context_t* ctx, uint64_t* total_snapshots, 
                            uint64_t* total_alerts, uint64_t* total_widgets_rendered,
                            double* availability);
int get_system_health_status(dashboard_context_t* ctx, double* health_score);
int get_performance_metrics(dashboard_context_t* ctx, double* render_time, 
                          double* collection_time);

// Callback registration
void register_metric_update_callback(metric_update_callback_t callback);
void register_alert_trigger_callback(alert_trigger_callback_t callback);
void register_snapshot_update_callback(snapshot_update_callback_t callback);
void register_widget_render_callback(widget_render_callback_t callback);
void register_dashboard_event_callback(dashboard_event_callback_t callback);

// Integration functions
int integrate_with_diagnostic_system(dashboard_context_t* ctx);
int integrate_with_performance_monitor(dashboard_context_t* ctx);
int integrate_with_security_system(dashboard_context_t* ctx);
int apply_dashboard_recommendations(dashboard_context_t* ctx);
int verify_dashboard_integrity(dashboard_context_t* ctx);

// Utility functions
const char* dashboard_metric_type_to_string(dashboard_metric_type_t metric_type);
const char* chart_type_to_string(chart_type_t chart_type);
const char* alert_severity_to_string(alert_severity_t severity);
const char* alert_type_to_string(alert_type_t alert_type);
const char* widget_type_to_string(widget_type_t widget_type);
const char* layout_position_to_string(layout_position_t position);
dashboard_metric_type_t string_to_dashboard_metric_type(const char* str);
chart_type_t string_to_chart_type(const char* str);
alert_severity_t string_to_alert_severity(const char* str);
alert_type_t string_to_alert_type(const char* str);
widget_type_t string_to_widget_type(const char* str);
layout_position_t string_to_layout_position(const char* str);
int format_metric_value(double value, const char* unit, char* formatted_output, int max_length);
int generate_widget_id(dashboard_context_t* ctx);

#endif // REAL_TIME_MONITORING_DASHBOARD_H