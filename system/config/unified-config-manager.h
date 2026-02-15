/*
 * Unified Configuration Manager for MTProxy
 * Manages configuration for all advanced systems
 */

#ifndef UNIFIED_CONFIG_MANAGER_H
#define UNIFIED_CONFIG_MANAGER_H

#define MAX_CONFIG_ENTRIES 256
#define MAX_CONFIG_KEY_LENGTH 128
#define MAX_CONFIG_VALUE_LENGTH 512
#define CONFIG_FILE_PATH_MAX 256

typedef enum {
    CONFIG_TYPE_INT = 0,
    CONFIG_TYPE_FLOAT = 1,
    CONFIG_TYPE_STRING = 2,
    CONFIG_TYPE_BOOL = 3
} config_type_t;

typedef enum {
    CONFIG_MODULE_DIAGNOSTIC = 0,
    CONFIG_MODULE_MONITORING = 1,
    CONFIG_MODULE_DEBUGGING = 2,
    CONFIG_MODULE_OPTIMIZATION = 3,
    CONFIG_MODULE_INTEGRATION = 4,
    CONFIG_MODULE_RESOURCE_MGMT = 5,
    CONFIG_MODULE_HEALTH_MONITOR = 6
} config_module_t;

typedef struct {
    char key[MAX_CONFIG_KEY_LENGTH];
    config_type_t type;
    config_module_t module;
    union {
        int int_val;
        float float_val;
        char str_val[MAX_CONFIG_VALUE_LENGTH];
        int bool_val;
    } value;
    int is_modified;
} config_entry_t;

typedef struct {
    config_entry_t entries[MAX_CONFIG_ENTRIES];
    int entry_count;
    char config_file_path[CONFIG_FILE_PATH_MAX];
    int auto_save_enabled;
    unsigned long last_save_time;
} unified_config_manager_t;

// Function declarations
int init_unified_config_manager(unified_config_manager_t* cfg_mgr, const char* config_file_path);
int load_config_from_file(unified_config_manager_t* cfg_mgr);
int save_config_to_file(unified_config_manager_t* cfg_mgr);
int set_config_value_int(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, int value);
int set_config_value_float(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, float value);
int set_config_value_string(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, const char* value);
int set_config_value_bool(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, int value);
int get_config_value_int(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, int* value);
int get_config_value_float(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, float* value);
int get_config_value_string(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, char* value, int max_len);
int get_config_value_bool(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, int* value);
int register_config_entry(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, config_type_t type);
int apply_config_to_module(unified_config_manager_t* cfg_mgr, config_module_t module);
void cleanup_config_manager(unified_config_manager_t* cfg_mgr);

#endif // UNIFIED_CONFIG_MANAGER_H