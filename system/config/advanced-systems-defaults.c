/*
 * Default Configuration Values for Advanced Systems Implementation
 * Sets up default configuration for all advanced systems
 */

#include "advanced-systems-defaults.h"

// Initialize default configuration values for all advanced systems
int init_advanced_systems_defaults(unified_config_manager_t* cfg_mgr) {
    if (!cfg_mgr) {
        return -1;
    }
    
    int result = 0;
    
    // Register and set diagnostic system defaults
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_DIAGNOSTIC, "enabled", CONFIG_TYPE_BOOL) != 0) result = -1;
    if (set_config_value_bool(cfg_mgr, CONFIG_MODULE_DIAGNOSTIC, "enabled", DIAGNOSTIC_ENABLED_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_DIAGNOSTIC, "log_level", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_DIAGNOSTIC, "log_level", DIAGNOSTIC_LOG_LEVEL_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_DIAGNOSTIC, "sampling_rate", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_DIAGNOSTIC, "sampling_rate", DIAGNOSTIC_SAMPLING_RATE_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_DIAGNOSTIC, "timeout_ms", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_DIAGNOSTIC, "timeout_ms", DIAGNOSTIC_TIMEOUT_MS_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_DIAGNOSTIC, "max_reports", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_DIAGNOSTIC, "max_reports", DIAGNOSTIC_MAX_REPORTS_DEFAULT) != 0) result = -1;
    
    // Register and set monitoring system defaults
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_MONITORING, "enabled", CONFIG_TYPE_BOOL) != 0) result = -1;
    if (set_config_value_bool(cfg_mgr, CONFIG_MODULE_MONITORING, "enabled", MONITORING_ENABLED_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_MONITORING, "update_interval_ms", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_MONITORING, "update_interval_ms", MONITORING_UPDATE_INTERVAL_MS_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_MONITORING, "retention_hours", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_MONITORING, "retention_hours", MONITORING_RETENTION_HOURS_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_MONITORING, "alert_threshold", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_MONITORING, "alert_threshold", MONITORING_ALERT_THRESHOLD_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_MONITORING, "max_metrics", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_MONITORING, "max_metrics", MONITORING_MAX_METRICS_DEFAULT) != 0) result = -1;
    
    // Register and set debugging framework defaults
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_DEBUGGING, "enabled", CONFIG_TYPE_BOOL) != 0) result = -1;
    if (set_config_value_bool(cfg_mgr, CONFIG_MODULE_DEBUGGING, "enabled", DEBUGGING_ENABLED_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_DEBUGGING, "log_level", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_DEBUGGING, "log_level", DEBUGGING_LOG_LEVEL_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_DEBUGGING, "max_traces", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_DEBUGGING, "max_traces", DEBUGGING_MAX_TRACES_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_DEBUGGING, "breakpoint_limit", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_DEBUGGING, "breakpoint_limit", DEBUGGING_BREAKPOINT_LIMIT_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_DEBUGGING, "variable_watch_limit", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_DEBUGGING, "variable_watch_limit", DEBUGGING_VARIABLE_WATCH_LIMIT_DEFAULT) != 0) result = -1;
    
    // Register and set optimization system defaults
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_OPTIMIZATION, "enabled", CONFIG_TYPE_BOOL) != 0) result = -1;
    if (set_config_value_bool(cfg_mgr, CONFIG_MODULE_OPTIMIZATION, "enabled", OPTIMIZATION_ENABLED_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_OPTIMIZATION, "level", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_OPTIMIZATION, "level", OPTIMIZATION_LEVEL_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_OPTIMIZATION, "sampling_interval_ms", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_OPTIMIZATION, "sampling_interval_ms", OPTIMIZATION_SAMPLING_INTERVAL_MS_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_OPTIMIZATION, "resource_threshold", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_OPTIMIZATION, "resource_threshold", OPTIMIZATION_RESOURCE_THRESHOLD_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_OPTIMIZATION, "autotune_enabled", CONFIG_TYPE_BOOL) != 0) result = -1;
    if (set_config_value_bool(cfg_mgr, CONFIG_MODULE_OPTIMIZATION, "autotune_enabled", OPTIMIZATION_AUTOTUNE_ENABLED_DEFAULT) != 0) result = -1;
    
    // Register and set health monitor defaults
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_HEALTH_MONITOR, "enabled", CONFIG_TYPE_BOOL) != 0) result = -1;
    if (set_config_value_bool(cfg_mgr, CONFIG_MODULE_HEALTH_MONITOR, "enabled", HEALTH_MONITOR_ENABLED_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_HEALTH_MONITOR, "interval_ms", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_HEALTH_MONITOR, "interval_ms", HEALTH_MONITOR_INTERVAL_MS_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_HEALTH_MONITOR, "critical_threshold", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_HEALTH_MONITOR, "critical_threshold", HEALTH_CRITICAL_THRESHOLD_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_HEALTH_MONITOR, "warning_threshold", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_HEALTH_MONITOR, "warning_threshold", HEALTH_WARNING_THRESHOLD_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_HEALTH_MONITOR, "retention_minutes", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_HEALTH_MONITOR, "retention_minutes", HEALTH_RETENTION_MINUTES_DEFAULT) != 0) result = -1;
    
    // Register and set resource manager defaults
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_RESOURCE_MGMT, "enabled", CONFIG_TYPE_BOOL) != 0) result = -1;
    if (set_config_value_bool(cfg_mgr, CONFIG_MODULE_RESOURCE_MGMT, "enabled", RESOURCE_MANAGER_ENABLED_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_RESOURCE_MGMT, "sampling_interval_ms", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_RESOURCE_MGMT, "sampling_interval_ms", RESOURCE_SAMPLING_INTERVAL_MS_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_RESOURCE_MGMT, "optimization_threshold", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_RESOURCE_MGMT, "optimization_threshold", RESOURCE_OPTIMIZATION_THRESHOLD_DEFAULT) != 0) result = -1;
    
    if (register_config_entry(cfg_mgr, CONFIG_MODULE_RESOURCE_MGMT, "max_allocated_percent", CONFIG_TYPE_INT) != 0) result = -1;
    if (set_config_value_int(cfg_mgr, CONFIG_MODULE_RESOURCE_MGMT, "max_allocated_percent", RESOURCE_MAX_ALLOCATED_DEFAULT) != 0) result = -1;
    
    return result;
}