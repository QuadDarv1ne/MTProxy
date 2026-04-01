/*
 * real-time-monitoring-dashboard.c
 * Real-time Monitoring Dashboard Implementation for MTProxy
 */

#include "real-time-monitoring-dashboard.h"

// Simple implementations for standard functions
static void* simple_malloc(size_t size) {
    static char heap[8192*1024]; // 8MB heap
    static size_t heap_offset = 0;
    
    if (heap_offset + size > sizeof(heap)) {
        return 0;
    }
    
    void *ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

static void simple_free(void *ptr) {
    // Simple free simulation
}

static void simple_memset(void *ptr, int value, size_t num) {
    char *p = (char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (char)value;
    }
}

static void simple_memcpy(void *dest, const void *src, size_t num) {
    char *d = (char*)dest;
    const char *s = (const char*)src;
    for (size_t i = 0; i < num; i++) {
        d[i] = s[i];
    }
}

static int simple_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

static size_t simple_strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

// Global context and callbacks
static dashboard_context_t* g_dashboard = 0;
static metric_update_callback_t g_metric_callback = 0;
static alert_trigger_callback_t g_alert_callback = 0;
static snapshot_update_callback_t g_snapshot_callback = 0;
static widget_render_callback_t g_widget_callback = 0;
static dashboard_event_callback_t g_event_callback = 0;

// Get current timestamp
static uint64_t get_current_timestamp_ms(void) {
    static uint64_t base_time = 1000000;
    base_time += 100;
    return base_time;
}

// Initialize dashboard
int init_dashboard(dashboard_context_t* ctx) {
    if (!ctx) return -1;
    
    // Zero initialize
    simple_memset(ctx, 0, sizeof(dashboard_context_t));
    
    // Default configuration
    dashboard_config_t default_config = {
        .enable_real_time_updates = 1,
        .enable_auto_refresh = 1,
        .refresh_interval_ms = 1000,
        .enable_alerts = 1,
        .enable_metric_history = 1,
        .max_history_points = 1000,
        .enable_export = 1,
        .enable_themes = 1,
        .theme_name = "default",
        .enable_annotations = 1,
        .enable_zoom_pan = 1,
        .enable_tooltip = 1,
        .enable_legend = 1,
        .enable_grid_lines = 1,
        .opacity = 1.0,
        .enable_animation = 1,
        .animation_duration_ms = 500,
        .enable_dark_mode = 0,
        .enable_responsive_layout = 1,
        .enable_fullscreen_mode = 1,
        .enable_user_preferences = 1,
        .enable_access_control = 0,
        .max_widgets = 50,
        .enable_widget_sharing = 1,
        .enable_template_system = 1,
        .enable_custom_dashboards = 1,
        .max_dashboards = 10,
        .enable_data_export = 1,
        .enable_screenshot = 1,
        .enable_print_view = 1,
        .enable_share_link = 1,
        .enable_embed_mode = 0
    };
    
    return init_dashboard_with_config(ctx, &default_config);
}

// Initialize with custom configuration
int init_dashboard_with_config(dashboard_context_t* ctx, const dashboard_config_t* config) {
    if (!ctx) return -1;
    
    // Zero initialize
    simple_memset(ctx, 0, sizeof(dashboard_context_t));
    
    // Apply configuration
    if (config) {
        ctx->config = *config;
    }
    
    // Initialize metrics
    ctx->max_metrics = 32;  // Support 32 different metrics
    ctx->metrics = (dashboard_metric_t*)simple_malloc(
        sizeof(dashboard_metric_t) * ctx->max_metrics);
    ctx->metric_count = 0;
    
    // Initialize metrics with default values
    const char* metric_names[] = {
        "CPU Usage", "Memory Usage", "Network In", "Network Out",
        "Active Connections", "Requests/sec", "Avg Response Time", "Error Rate",
        "Cache Hit Ratio", "Crypto Ops/sec", "Uptime", "Threads",
        "Open Files", "Disk Usage"
    };
    
    const char* metric_units[] = {
        "%", "%", "Mbps", "Mbps", "count", "req/s", "ms", "%",
        "%", "ops/s", "s", "count", "count", "%"
    };
    
    double warning_thresholds[] = {
        75.0, 80.0, 80.0, 80.0, 800, 8000, 100.0, 2.0,
        85.0, 8000, 0.0, 50, 200, 80.0
    };
    
    double critical_thresholds[] = {
        90.0, 90.0, 95.0, 95.0, 950, 9500, 500.0, 5.0,
        75.0, 9500, 0.0, 100, 500, 95.0
    };
    
    for (int i = 0; i < 14; i++) {
        dashboard_metric_t* metric = &ctx->metrics[ctx->metric_count];
        metric->metric_type = (dashboard_metric_type_t)i;
        
        // Set metric name
        int name_len = 0;
        const char* name = metric_names[i];
        while (name_len < 63 && name[name_len]) {
            metric->metric_name[name_len] = name[name_len];
            name_len++;
        }
        metric->metric_name[name_len] = '\0';
        
        // Set unit
        int unit_len = 0;
        const char* unit = metric_units[i];
        while (unit_len < 15 && unit[unit_len]) {
            metric->unit[unit_len] = unit[unit_len];
            unit_len++;
        }
        metric->unit[unit_len] = '\0';
        
        metric->current_value = 0.0;
        metric->warning_threshold = warning_thresholds[i];
        metric->critical_threshold = critical_thresholds[i];
        metric->is_enabled = 1;
        metric->last_updated = get_current_timestamp_ms();
        metric->max_history_points = ctx->config.max_history_points;
        metric->history = (metric_data_point_t*)simple_malloc(
            sizeof(metric_data_point_t) * metric->max_history_points);
        metric->history_count = 0;
        metric->preferred_chart_type = (i < 8) ? CHART_TYPE_LINE : CHART_TYPE_GAUGE;
        metric->show_on_dashboard = 1;
        
        // Set description
        const char* descriptions[] = {
            "CPU utilization percentage", "Memory utilization percentage", 
            "Network input bandwidth", "Network output bandwidth",
            "Number of active connections", "Requests processed per second",
            "Average response time in milliseconds", "Error rate percentage",
            "Cache hit ratio percentage", "Crypto operations per second",
            "System uptime in seconds", "Number of active threads",
            "Number of open files", "Disk usage percentage"
        };
        
        int desc_len = 0;
        const char* desc = descriptions[i];
        while (desc_len < 127 && desc[desc_len]) {
            metric->description[desc_len] = desc[desc_len];
            desc_len++;
        }
        metric->description[desc_len] = '\0';
        
        ctx->metric_count++;
    }
    
    // Initialize alerts
    ctx->max_active_alerts = 100;
    ctx->active_alerts = (alert_definition_t*)simple_malloc(
        sizeof(alert_definition_t) * ctx->max_active_alerts);
    ctx->active_alert_count = 0;
    
    ctx->max_resolved_alerts = 1000;
    ctx->resolved_alerts = (alert_definition_t*)simple_malloc(
        sizeof(alert_definition_t) * ctx->max_resolved_alerts);
    ctx->resolved_alert_count = 0;
    
    // Initialize widgets
    ctx->max_widgets = ctx->config.max_widgets;
    ctx->widgets = (dashboard_widget_t*)simple_malloc(
        sizeof(dashboard_widget_t) * ctx->max_widgets);
    ctx->widget_count = 0;
    
    // Initialize views
    ctx->max_views = ctx->config.max_dashboards;
    ctx->views = (dashboard_view_t*)simple_malloc(
        sizeof(dashboard_view_t) * ctx->max_views);
    ctx->view_count = 1;  // Default view
    
    // Initialize default view
    dashboard_view_t* default_view = &ctx->views[0];
    default_view->view_id = 1;
    default_view->is_default_view = 1;
    default_view->is_public = 1;
    default_view->created_at = get_current_timestamp_ms();
    default_view->last_modified = get_current_timestamp_ms();
    default_view->max_widgets = 20;
    default_view->widget_count = 0;
    default_view->widgets = (dashboard_widget_t*)simple_malloc(
        sizeof(dashboard_widget_t) * default_view->max_widgets);
    
    int view_name_len = 0;
    const char* view_name = "Default Dashboard";
    while (view_name_len < 63 && view_name[view_name_len]) {
        default_view->view_name[view_name_len] = view_name[view_name_len];
        view_name_len++;
    }
    default_view->view_name[view_name_len] = '\0';
    
    // Initialize snapshots
    ctx->max_snapshots = 1000;
    ctx->snapshot_history = (live_metrics_snapshot_t*)simple_malloc(
        sizeof(live_metrics_snapshot_t) * ctx->max_snapshots);
    ctx->snapshot_count = 0;
    ctx->last_snapshot_time = get_current_timestamp_ms();
    
    // Initialize sessions
    ctx->max_sessions = 50;
    ctx->active_sessions = (dashboard_session_t*)simple_malloc(
        sizeof(dashboard_session_t) * ctx->max_sessions);
    ctx->active_session_count = 0;
    
    // Initialize current snapshot with default values
    simple_memset(&ctx->current_snapshot, 0, sizeof(live_metrics_snapshot_t));
    ctx->current_snapshot.snapshot_id = 1;
    ctx->current_snapshot.timestamp = get_current_timestamp_ms();
    ctx->current_snapshot.cpu_usage_percent = 25.0;
    ctx->current_snapshot.memory_usage_percent = 45.0;
    ctx->current_snapshot.network_in_mbps = 10.0;
    ctx->current_snapshot.network_out_mbps = 8.0;
    ctx->current_snapshot.active_connections = 150;
    ctx->current_snapshot.total_connections = 1000;
    ctx->current_snapshot.failed_connections = 5;
    ctx->current_snapshot.avg_response_time_ms = 45.0;
    ctx->current_snapshot.p95_response_time_ms = 85.0;
    ctx->current_snapshot.p99_response_time_ms = 120.0;
    ctx->current_snapshot.requests_per_second = 1200;
    ctx->current_snapshot.error_rate_percent = 0.5;
    ctx->current_snapshot.cache_hit_ratio = 95.0;
    ctx->current_snapshot.current_rss_kb = 65536;  // 64MB
    ctx->current_snapshot.peak_rss_kb = 81920;     // 80MB
    ctx->current_snapshot.virtual_memory_kb = 131072; // 128MB
    ctx->current_snapshot.open_files_count = 65;
    ctx->current_snapshot.threads_count = 12;
    ctx->current_snapshot.crypto_operations_per_second = 6500;
    ctx->current_snapshot.uptime_seconds = 3600;  // 1 hour
    ctx->current_snapshot.disk_usage_percent = 45.0;
    ctx->current_snapshot.total_disk_space_kb = 1048576;  // 1GB
    ctx->current_snapshot.available_disk_space_kb = 576716; // ~0.5GB
    ctx->current_snapshot.temperature_celsius = 45.0;
    ctx->current_snapshot.dropped_packets = 0;
    ctx->current_snapshot.corrupted_packets = 0;
    ctx->current_snapshot.encryption_time_avg_ms = 0.12;
    ctx->current_snapshot.decryption_time_avg_ms = 0.08;
    ctx->current_snapshot.system_healthy = 1;
    ctx->current_snapshot.system_health_score = 95.0;
    ctx->current_snapshot.active_alerts_count = 0;
    ctx->current_snapshot.resolved_alerts_count = 0;
    ctx->current_snapshot.last_health_check = get_current_timestamp_ms();
    
    // Initialize performance tracking
    ctx->total_snapshots_collected = 0;
    ctx->total_alerts_generated = 0;
    ctx->total_widgets_rendered = 0;
    ctx->total_data_points_processed = 0;
    ctx->average_render_time_ms = 10.0;
    ctx->average_data_collection_time_ms = 5.0;
    
    // Initialize user management
    ctx->total_users_connected = 0;
    ctx->peak_concurrent_users = 0;
    ctx->peak_concurrent_users_time = get_current_timestamp_ms();
    
    // Initialize export settings
    ctx->export_in_progress = 0;
    ctx->last_export_time = 0;
    ctx->export_format_csv = 1;
    ctx->export_format_json = 1;
    ctx->export_format_pdf = 0;
    ctx->export_format_png = 0;
    
    // Initialize theme settings
    int theme_len = 0;
    const char* theme = ctx->config.theme_name;
    while (theme_len < 31 && theme[theme_len]) {
        ctx->current_theme[theme_len] = theme[theme_len];
        theme_len++;
    }
    ctx->current_theme[theme_len] = '\0';
    ctx->dark_mode_enabled = ctx->config.enable_dark_mode;
    ctx->dashboard_opacity = ctx->config.opacity;
    ctx->animations_enabled = ctx->config.enable_animation;
    ctx->animation_duration_ms = ctx->config.animation_duration_ms;
    
    // Initialize state
    ctx->dashboard_active = 1;
    ctx->real_time_updates_active = ctx->config.enable_real_time_updates;
    ctx->auto_refresh_active = ctx->config.enable_auto_refresh;
    ctx->data_collection_active = 1;
    ctx->alert_monitoring_active = ctx->config.enable_alerts;
    ctx->active_components = 0;
    
    // Initialize statistics
    ctx->uptime_seconds = 0;
    ctx->start_time = get_current_timestamp_ms();
    ctx->availability_percentage = 100.0;
    ctx->total_downtime_seconds = 0;
    ctx->last_downtime_start = 0;
    
    // Set system state
    ctx->initialized = 1;
    ctx->active = 1;
    ctx->initialization_time = get_current_timestamp_ms();
    
    // Set dashboard ID and version
    const char* dashboard_id = "MTProxy-Monitoring-Dashboard-v1.0";
    int id_len = 0;
    while (id_len < 63 && dashboard_id[id_len]) {
        ctx->dashboard_id[id_len] = dashboard_id[id_len];
        id_len++;
    }
    ctx->dashboard_id[id_len] = '\0';
    
    const char* version = "1.0.0";
    int ver_len = 0;
    while (ver_len < 31 && version[ver_len]) {
        ctx->version_string[ver_len] = version[ver_len];
        ver_len++;
    }
    ctx->version_string[ver_len] = '\0';
    
    g_dashboard = ctx;
    return 0;
}

// Cleanup dashboard
void cleanup_dashboard(dashboard_context_t* ctx) {
    if (!ctx) return;
    
    // Free allocated memory
    if (ctx->metrics) {
        for (int i = 0; i < ctx->metric_count; i++) {
            if (ctx->metrics[i].history) {
                simple_free(ctx->metrics[i].history);
            }
        }
        simple_free(ctx->metrics);
    }
    
    if (ctx->active_alerts) {
        simple_free(ctx->active_alerts);
    }
    
    if (ctx->resolved_alerts) {
        simple_free(ctx->resolved_alerts);
    }
    
    if (ctx->widgets) {
        simple_free(ctx->widgets);
    }
    
    if (ctx->views) {
        for (int i = 0; i < ctx->view_count; i++) {
            if (ctx->views[i].widgets) {
                simple_free(ctx->views[i].widgets);
            }
        }
        simple_free(ctx->views);
    }
    
    if (ctx->snapshot_history) {
        simple_free(ctx->snapshot_history);
    }
    
    if (ctx->active_sessions) {
        simple_free(ctx->active_sessions);
    }
    
    // Reset context
    simple_memset(ctx, 0, sizeof(dashboard_context_t));
    
    if (g_dashboard == ctx) {
        g_dashboard = 0;
    }
}

// Update metric value
int update_metric_value(dashboard_context_t* ctx, dashboard_metric_type_t metric_type, 
                       double new_value) {
    if (!ctx || !ctx->initialized) return -1;
    
    // Find the metric
    for (int i = 0; i < ctx->metric_count; i++) {
        if (ctx->metrics[i].metric_type == metric_type) {
            dashboard_metric_t* metric = &ctx->metrics[i];
            
            // Update current value
            metric->current_value = new_value;
            metric->last_updated = get_current_timestamp_ms();
            
            // Store in history (circular buffer)
            int hist_idx = metric->history_count % metric->max_history_points;
            metric->history[hist_idx].timestamp = get_current_timestamp_ms();
            metric->history[hist_idx].value = new_value;
            metric->history[hist_idx].avg_value = new_value;  // Simplified
            metric->history[hist_idx].min_value = new_value * 0.9;  // Simplified
            metric->history[hist_idx].max_value = new_value * 1.1;  // Simplified
            metric->history[hist_idx].percentile_95 = new_value * 0.95;  // Simplified
            metric->history[hist_idx].percentile_99 = new_value * 0.99;  // Simplified
            
            metric->history_count++;
            
            // Update the corresponding value in current snapshot
            switch (metric_type) {
                case DASHBOARD_METRIC_CPU_USAGE:
                    ctx->current_snapshot.cpu_usage_percent = new_value;
                    break;
                case DASHBOARD_METRIC_MEMORY_USAGE:
                    ctx->current_snapshot.memory_usage_percent = new_value;
                    break;
                case DASHBOARD_METRIC_NETWORK_IN:
                    ctx->current_snapshot.network_in_mbps = new_value;
                    break;
                case DASHBOARD_METRIC_NETWORK_OUT:
                    ctx->current_snapshot.network_out_mbps = new_value;
                    break;
                case DASHBOARD_METRIC_ACTIVE_CONNECTIONS:
                    ctx->current_snapshot.active_connections = (uint64_t)new_value;
                    break;
                case DASHBOARD_METRIC_REQUESTS_PER_SECOND:
                    ctx->current_snapshot.requests_per_second = (uint64_t)new_value;
                    break;
                case DASHBOARD_METRIC_AVG_RESPONSE_TIME:
                    ctx->current_snapshot.avg_response_time_ms = new_value;
                    break;
                case DASHBOARD_METRIC_ERROR_RATE:
                    ctx->current_snapshot.error_rate_percent = new_value;
                    break;
                case DASHBOARD_METRIC_CACHE_HIT_RATIO:
                    ctx->current_snapshot.cache_hit_ratio = new_value;
                    break;
                case DASHBOARD_METRIC_DISK_USAGE:
                    ctx->current_snapshot.disk_usage_percent = new_value;
                    break;
                default:
                    break;
            }
            
            // Check for alerts
            if (ctx->alert_monitoring_active) {
                if (new_value >= metric->critical_threshold) {
                    // Trigger critical alert
                    char title[128];
                    int title_len = 0;
                    const char* base_title = "Critical Threshold Exceeded: ";
                    while (title_len < 63 && base_title[title_len]) {
                        title[title_len] = base_title[title_len];
                        title_len++;
                    }
                    
                    int name_len = 0;
                    while (title_len < 127 && name_len < 63 && metric->metric_name[name_len]) {
                        title[title_len] = metric->metric_name[name_len];
                        title_len++;
                        name_len++;
                    }
                    title[title_len] = '\0';
                    
                    trigger_alert(ctx, ALERT_TYPE_HIGH_CPU + (int)metric_type % 8, 
                                title, metric->description, new_value, metric->critical_threshold);
                } else if (new_value >= metric->warning_threshold) {
                    // Trigger warning alert
                    char title[128];
                    int title_len = 0;
                    const char* base_title = "Warning Threshold Approaching: ";
                    while (title_len < 63 && base_title[title_len]) {
                        title[title_len] = base_title[title_len];
                        title_len++;
                    }
                    
                    int name_len = 0;
                    while (title_len < 127 && name_len < 63 && metric->metric_name[name_len]) {
                        title[title_len] = metric->metric_name[name_len];
                        title_len++;
                        name_len++;
                    }
                    title[title_len] = '\0';
                    
                    trigger_alert(ctx, ALERT_TYPE_HIGH_CPU + (int)metric_type % 8, 
                                title, metric->description, new_value, metric->warning_threshold);
                }
            }
            
            // Call metric callback
            if (g_metric_callback) {
                g_metric_callback(metric_type, new_value);
            }
            
            return 0;
        }
    }
    
    return -1;  // Metric not found
}

// Trigger alert
int trigger_alert(dashboard_context_t* ctx, alert_type_t alert_type, 
                 const char* title, const char* description, double current_value, 
                 double threshold_value) {
    if (!ctx || !ctx->initialized) return -1;
    
    if (ctx->active_alert_count >= ctx->max_active_alerts) return -1;
    
    alert_definition_t* alert = &ctx->active_alerts[ctx->active_alert_count];
    static uint64_t alert_counter = 1;
    
    alert->alert_id = alert_counter++;
    alert->alert_type = alert_type;
    alert->severity = (current_value >= threshold_value * 1.2) ? 
                     ALERT_SEVERITY_CRITICAL : ALERT_SEVERITY_MEDIUM;
    alert->timestamp = get_current_timestamp_ms();
    
    // Set title
    if (title) {
        int title_len = 0;
        while (title_len < 127 && title[title_len]) {
            alert->title[title_len] = title[title_len];
            title_len++;
        }
        alert->title[title_len] = '\0';
    } else {
        const char* default_titles[] = {
            "High CPU Usage", "High Memory Usage", "High Latency", "Low Cache Hit Ratio",
            "Connection Dropped", "Error Rate Spike", "Low Disk Space", "Security Event"
        };
        
        int title_idx = alert_type % 8;
        int title_len = 0;
        const char* def_title = default_titles[title_idx];
        while (title_len < 127 && def_title[title_len]) {
            alert->title[title_len] = def_title[title_len];
            title_len++;
        }
        alert->title[title_len] = '\0';
    }
    
    // Set description
    if (description) {
        int desc_len = 0;
        while (desc_len < 255 && description[desc_len]) {
            alert->description[desc_len] = description[desc_len];
            desc_len++;
        }
        alert->description[desc_len] = '\0';
    } else {
        const char* default_desc = "System metric has exceeded configured threshold";
        int desc_len = 0;
        while (desc_len < 255 && default_desc[desc_len]) {
            alert->description[desc_len] = default_desc[desc_len];
            desc_len++;
        }
        alert->description[desc_len] = '\0';
    }
    
    // Set affected component based on alert type
    const char* components[] = {
        "CPU Subsystem", "Memory Manager", "Network Layer", "Cache System",
        "Connection Pool", "Request Handler", "Storage System", "Security Module"
    };
    
    int comp_idx = alert_type % 8;
    int comp_len = 0;
    const char* comp = components[comp_idx];
    while (comp_len < 63 && comp[comp_len]) {
        alert->affected_component[comp_len] = comp[comp_len];
        comp_len++;
    }
    alert->affected_component[comp_len] = '\0';
    
    alert->current_value = current_value;
    alert->threshold_value = threshold_value;
    alert->is_active = 1;
    alert->acknowledged = 0;
    alert->resolved = 0;
    alert->confidence_score = 90.0 + (alert_type * 2);  // Vary confidence
    
    // Set suggested action
    const char* actions[] = {
        "Monitor CPU usage and consider scaling resources",
        "Check memory usage and optimize allocations", 
        "Investigate performance bottlenecks",
        "Review cache configuration and sizing",
        "Check connection handling and timeouts",
        "Review error logs and fix underlying issues",
        "Free up disk space or expand storage",
        "Review security logs and investigate threats"
    };
    
    int action_idx = alert_type % 8;
    int action_len = 0;
    const char* action = actions[action_idx];
    while (action_len < 255 && action[action_len]) {
        alert->suggested_action[action_len] = action[action_len];
        action_len++;
    }
    alert->suggested_action[action_len] = '\0';
    
    alert->auto_resolve_timeout = get_current_timestamp_ms() + 300000;  // 5 minutes
    
    ctx->active_alert_count++;
    ctx->total_alerts_generated++;
    ctx->current_snapshot.active_alerts_count = ctx->active_alert_count;
    
    // Call alert callback
    if (g_alert_callback) {
        g_alert_callback(alert);
    }
    
    return 0;
}

// Collect live snapshot
int collect_live_snapshot(dashboard_context_t* ctx) {
    if (!ctx || !ctx->initialized) return -1;
    
    // Update the snapshot with current values from metrics
    ctx->current_snapshot.snapshot_id++;
    ctx->current_snapshot.timestamp = get_current_timestamp_ms();
    
    // The metrics are already updated in update_metric_value, so we just need to update derived values
    ctx->current_snapshot.system_health_score = 100.0 - 
        (ctx->current_snapshot.cpu_usage_percent / 100.0 * 10) -
        (ctx->current_snapshot.memory_usage_percent / 100.0 * 10) -
        (ctx->current_snapshot.error_rate_percent * 5) +
        (ctx->current_snapshot.cache_hit_ratio / 100.0 * 15);  // Reward high cache hit
    
    if (ctx->current_snapshot.system_health_score < 0) {
        ctx->current_snapshot.system_health_score = 0;
    }
    if (ctx->current_snapshot.system_health_score > 100) {
        ctx->current_snapshot.system_health_score = 100;
    }
    
    ctx->current_snapshot.system_healthy = (ctx->current_snapshot.system_health_score >= 70.0);
    ctx->current_snapshot.last_health_check = get_current_timestamp_ms();
    ctx->current_snapshot.active_alerts_count = ctx->active_alert_count;
    
    // Store in history (circular buffer)
    int snap_idx = ctx->snapshot_count % ctx->max_snapshots;
    ctx->snapshot_history[snap_idx] = ctx->current_snapshot;
    ctx->snapshot_count++;
    
    ctx->total_snapshots_collected++;
    ctx->last_snapshot_time = get_current_timestamp_ms();
    
    // Call snapshot callback
    if (g_snapshot_callback) {
        g_snapshot_callback(&ctx->current_snapshot);
    }
    
    return 0;
}

// Get dashboard statistics
void get_dashboard_statistics(dashboard_context_t* ctx, uint64_t* total_snapshots, 
                            uint64_t* total_alerts, uint64_t* total_widgets_rendered,
                            double* availability) {
    if (!ctx) return;
    
    if (total_snapshots) *total_snapshots = ctx->total_snapshots_collected;
    if (total_alerts) *total_alerts = ctx->total_alerts_generated;
    if (total_widgets_rendered) *total_widgets_rendered = ctx->total_widgets_rendered;
    if (availability) *availability = ctx->availability_percentage;
}

// Utility functions
const char* dashboard_metric_type_to_string(dashboard_metric_type_t metric_type) {
    switch (metric_type) {
        case DASHBOARD_METRIC_CPU_USAGE: return "CPU Usage";
        case DASHBOARD_METRIC_MEMORY_USAGE: return "Memory Usage";
        case DASHBOARD_METRIC_NETWORK_IN: return "Network In";
        case DASHBOARD_METRIC_NETWORK_OUT: return "Network Out";
        case DASHBOARD_METRIC_ACTIVE_CONNECTIONS: return "Active Connections";
        case DASHBOARD_METRIC_REQUESTS_PER_SECOND: return "Requests Per Second";
        case DASHBOARD_METRIC_AVG_RESPONSE_TIME: return "Avg Response Time";
        case DASHBOARD_METRIC_ERROR_RATE: return "Error Rate";
        case DASHBOARD_METRIC_CACHE_HIT_RATIO: return "Cache Hit Ratio";
        case DASHBOARD_METRIC_CRYPTO_OPERATIONS: return "Crypto Operations";
        case DASHBOARD_METRIC_UPTIME: return "Uptime";
        case DASHBOARD_METRIC_THREADS: return "Threads";
        case DASHBOARD_METRIC_OPEN_FILES: return "Open Files";
        case DASHBOARD_METRIC_DISK_USAGE: return "Disk Usage";
        default: return "Unknown";
    }
}

const char* chart_type_to_string(chart_type_t chart_type) {
    switch (chart_type) {
        case CHART_TYPE_LINE: return "Line";
        case CHART_TYPE_BAR: return "Bar";
        case CHART_TYPE_GAUGE: return "Gauge";
        case CHART_TYPE_PIE: return "Pie";
        case CHART_TYPE_HEATMAP: return "Heatmap";
        case CHART_TYPE_SCATTER: return "Scatter";
        default: return "Unknown";
    }
}

const char* alert_severity_to_string(alert_severity_t severity) {
    switch (severity) {
        case ALERT_SEVERITY_LOW: return "Low";
        case ALERT_SEVERITY_MEDIUM: return "Medium";
        case ALERT_SEVERITY_HIGH: return "High";
        case ALERT_SEVERITY_CRITICAL: return "Critical";
        default: return "Unknown";
    }
}

const char* alert_type_to_string(alert_type_t alert_type) {
    switch (alert_type) {
        case ALERT_TYPE_HIGH_CPU: return "High CPU";
        case ALERT_TYPE_HIGH_MEMORY: return "High Memory";
        case ALERT_TYPE_HIGH_LATENCY: return "High Latency";
        case ALERT_TYPE_LOW_CACHE_HIT: return "Low Cache Hit";
        case ALERT_TYPE_CONNECTION_DROPPED: return "Connection Dropped";
        case ALERT_TYPE_ERROR_RATE_SPIKE: return "Error Rate Spike";
        case ALERT_TYPE_DISK_SPACE_LOW: return "Low Disk Space";
        case ALERT_TYPE_SECURITY_EVENT: return "Security Event";
        default: return "Unknown";
    }
}

const char* widget_type_to_string(widget_type_t widget_type) {
    switch (widget_type) {
        case WIDGET_TYPE_CHART: return "Chart";
        case WIDGET_TYPE_GAUGE: return "Gauge";
        case WIDGET_TYPE_TABLE: return "Table";
        case WIDGET_TYPE_ALERT_LIST: return "Alert List";
        case WIDGET_TYPE_STATUS_INDICATOR: return "Status Indicator";
        case WIDGET_TYPE_TEXT_PANEL: return "Text Panel";
        case WIDGET_TYPE_METRIC_CARD: return "Metric Card";
        default: return "Unknown";
    }
}

// Callback registration
void register_metric_update_callback(metric_update_callback_t callback) {
    g_metric_callback = callback;
}

void register_alert_trigger_callback(alert_trigger_callback_t callback) {
    g_alert_callback = callback;
}

void register_snapshot_update_callback(snapshot_update_callback_t callback) {
    g_snapshot_callback = callback;
}

void register_widget_render_callback(widget_render_callback_t callback) {
    g_widget_callback = callback;
}

void register_dashboard_event_callback(dashboard_event_callback_t callback) {
    g_event_callback = callback;
}