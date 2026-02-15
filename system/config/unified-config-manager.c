/*
 * Unified Configuration Manager Implementation for MTProxy
 * Manages configuration for all advanced systems
 */

#include "unified-config-manager.h"

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

// Helper function to convert string to integer
static int string_to_int(const char* str) {
    int result = 0;
    int sign = 1;
    int i = 0;
    
    // Check for negative sign
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    }
    
    // Convert digits
    while (str[i] != '\0' && str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return result * sign;
}

// Helper function to convert string to float (simplified)
static float string_to_float(const char* str) {
    int integer_part = 0;
    int fractional_part = 0;
    int divisor = 1;
    int sign = 1;
    int i = 0;
    
    // Check for negative sign
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }
    
    // Parse integer part
    while (str[i] != '\0' && str[i] != '.') {
        if (str[i] >= '0' && str[i] <= '9') {
            integer_part = integer_part * 10 + (str[i] - '0');
        }
        i++;
    }
    
    // Parse fractional part
    if (str[i] == '.') {
        i++; // Skip the dot
        while (str[i] != '\0') {
            if (str[i] >= '0' && str[i] <= '9') {
                fractional_part = fractional_part * 10 + (str[i] - '0');
                divisor *= 10;
            }
            i++;
        }
    }
    
    float result = (float)integer_part + ((float)fractional_part / (float)divisor);
    return result * sign;
}

// Initialize the unified config manager
int init_unified_config_manager(unified_config_manager_t* cfg_mgr, const char* config_file_path) {
    if (!cfg_mgr || !config_file_path) {
        return -1;
    }
    
    // Initialize all fields to default values
    cfg_mgr->entry_count = 0;
    cfg_mgr->auto_save_enabled = 0;
    cfg_mgr->last_save_time = 0;
    
    // Copy config file path with bounds checking
    string_copy(cfg_mgr->config_file_path, config_file_path, CONFIG_FILE_PATH_MAX);
    
    // Initialize all config entries
    int i;
    for (i = 0; i < MAX_CONFIG_ENTRIES; i++) {
        config_entry_t* entry = &cfg_mgr->entries[i];
        entry->key[0] = '\0';
        entry->type = CONFIG_TYPE_INT; // Default value
        entry->module = CONFIG_MODULE_DIAGNOSTIC; // Default value
        entry->value.int_val = 0;
        entry->is_modified = 0;
    }
    
    return 0;
}

// Load configuration from file (placeholder implementation)
int load_config_from_file(unified_config_manager_t* cfg_mgr) {
    if (!cfg_mgr) {
        return -1;
    }
    
    // In a real implementation, this would read from the config file
    // For now, we'll just return success
    return 0;
}

// Save configuration to file (placeholder implementation)
int save_config_to_file(unified_config_manager_t* cfg_mgr) {
    if (!cfg_mgr) {
        return -1;
    }
    
    // In a real implementation, this would write to the config file
    // For now, we'll just return success
    return 0;
}

// Set integer configuration value
int set_config_value_int(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, int value) {
    if (!cfg_mgr || !key) {
        return -1;
    }
    
    // Look for existing entry
    int entry_idx = -1;
    int i;
    for (i = 0; i < cfg_mgr->entry_count; i++) {
        if (cfg_mgr->entries[i].module == module && string_compare(cfg_mgr->entries[i].key, key) == 0) {
            entry_idx = i;
            break;
        }
    }
    
    // If entry doesn't exist, create a new one
    if (entry_idx == -1) {
        if (cfg_mgr->entry_count >= MAX_CONFIG_ENTRIES) {
            return -1; // No space for new entries
        }
        
        entry_idx = cfg_mgr->entry_count;
        cfg_mgr->entry_count++;
        
        // Set entry properties
        string_copy(cfg_mgr->entries[entry_idx].key, key, MAX_CONFIG_KEY_LENGTH);
        cfg_mgr->entries[entry_idx].module = module;
        cfg_mgr->entries[entry_idx].type = CONFIG_TYPE_INT;
    }
    
    // Set the value
    cfg_mgr->entries[entry_idx].value.int_val = value;
    cfg_mgr->entries[entry_idx].is_modified = 1;
    
    return 0;
}

// Set float configuration value
int set_config_value_float(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, float value) {
    if (!cfg_mgr || !key) {
        return -1;
    }
    
    // Look for existing entry
    int entry_idx = -1;
    int i;
    for (i = 0; i < cfg_mgr->entry_count; i++) {
        if (cfg_mgr->entries[i].module == module && string_compare(cfg_mgr->entries[i].key, key) == 0) {
            entry_idx = i;
            break;
        }
    }
    
    // If entry doesn't exist, create a new one
    if (entry_idx == -1) {
        if (cfg_mgr->entry_count >= MAX_CONFIG_ENTRIES) {
            return -1; // No space for new entries
        }
        
        entry_idx = cfg_mgr->entry_count;
        cfg_mgr->entry_count++;
        
        // Set entry properties
        string_copy(cfg_mgr->entries[entry_idx].key, key, MAX_CONFIG_KEY_LENGTH);
        cfg_mgr->entries[entry_idx].module = module;
        cfg_mgr->entries[entry_idx].type = CONFIG_TYPE_FLOAT;
    }
    
    // Set the value
    cfg_mgr->entries[entry_idx].value.float_val = value;
    cfg_mgr->entries[entry_idx].is_modified = 1;
    
    return 0;
}

// Set string configuration value
int set_config_value_string(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, const char* value) {
    if (!cfg_mgr || !key || !value) {
        return -1;
    }
    
    // Look for existing entry
    int entry_idx = -1;
    int i;
    for (i = 0; i < cfg_mgr->entry_count; i++) {
        if (cfg_mgr->entries[i].module == module && string_compare(cfg_mgr->entries[i].key, key) == 0) {
            entry_idx = i;
            break;
        }
    }
    
    // If entry doesn't exist, create a new one
    if (entry_idx == -1) {
        if (cfg_mgr->entry_count >= MAX_CONFIG_ENTRIES) {
            return -1; // No space for new entries
        }
        
        entry_idx = cfg_mgr->entry_count;
        cfg_mgr->entry_count++;
        
        // Set entry properties
        string_copy(cfg_mgr->entries[entry_idx].key, key, MAX_CONFIG_KEY_LENGTH);
        cfg_mgr->entries[entry_idx].module = module;
        cfg_mgr->entries[entry_idx].type = CONFIG_TYPE_STRING;
    }
    
    // Set the value
    string_copy(cfg_mgr->entries[entry_idx].value.str_val, value, MAX_CONFIG_VALUE_LENGTH);
    cfg_mgr->entries[entry_idx].is_modified = 1;
    
    return 0;
}

// Set boolean configuration value
int set_config_value_bool(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, int value) {
    if (!cfg_mgr || !key) {
        return -1;
    }
    
    // Look for existing entry
    int entry_idx = -1;
    int i;
    for (i = 0; i < cfg_mgr->entry_count; i++) {
        if (cfg_mgr->entries[i].module == module && string_compare(cfg_mgr->entries[i].key, key) == 0) {
            entry_idx = i;
            break;
        }
    }
    
    // If entry doesn't exist, create a new one
    if (entry_idx == -1) {
        if (cfg_mgr->entry_count >= MAX_CONFIG_ENTRIES) {
            return -1; // No space for new entries
        }
        
        entry_idx = cfg_mgr->entry_count;
        cfg_mgr->entry_count++;
        
        // Set entry properties
        string_copy(cfg_mgr->entries[entry_idx].key, key, MAX_CONFIG_KEY_LENGTH);
        cfg_mgr->entries[entry_idx].module = module;
        cfg_mgr->entries[entry_idx].type = CONFIG_TYPE_BOOL;
    }
    
    // Set the value
    cfg_mgr->entries[entry_idx].value.bool_val = value;
    cfg_mgr->entries[entry_idx].is_modified = 1;
    
    return 0;
}

// Get integer configuration value
int get_config_value_int(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, int* value) {
    if (!cfg_mgr || !key || !value) {
        return -1;
    }
    
    // Look for the entry
    int i;
    for (i = 0; i < cfg_mgr->entry_count; i++) {
        if (cfg_mgr->entries[i].module == module && string_compare(cfg_mgr->entries[i].key, key) == 0) {
            if (cfg_mgr->entries[i].type == CONFIG_TYPE_INT) {
                *value = cfg_mgr->entries[i].value.int_val;
                return 0;
            } else {
                // Type mismatch
                return -1;
            }
        }
    }
    
    // Entry not found
    return -1;
}

// Get float configuration value
int get_config_value_float(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, float* value) {
    if (!cfg_mgr || !key || !value) {
        return -1;
    }
    
    // Look for the entry
    int i;
    for (i = 0; i < cfg_mgr->entry_count; i++) {
        if (cfg_mgr->entries[i].module == module && string_compare(cfg_mgr->entries[i].key, key) == 0) {
            if (cfg_mgr->entries[i].type == CONFIG_TYPE_FLOAT) {
                *value = cfg_mgr->entries[i].value.float_val;
                return 0;
            } else {
                // Type mismatch
                return -1;
            }
        }
    }
    
    // Entry not found
    return -1;
}

// Get string configuration value
int get_config_value_string(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, char* value, int max_len) {
    if (!cfg_mgr || !key || !value || max_len <= 0) {
        return -1;
    }
    
    // Look for the entry
    int i;
    for (i = 0; i < cfg_mgr->entry_count; i++) {
        if (cfg_mgr->entries[i].module == module && string_compare(cfg_mgr->entries[i].key, key) == 0) {
            if (cfg_mgr->entries[i].type == CONFIG_TYPE_STRING) {
                string_copy(value, cfg_mgr->entries[i].value.str_val, max_len);
                return 0;
            } else {
                // Type mismatch
                return -1;
            }
        }
    }
    
    // Entry not found
    return -1;
}

// Get boolean configuration value
int get_config_value_bool(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, int* value) {
    if (!cfg_mgr || !key || !value) {
        return -1;
    }
    
    // Look for the entry
    int i;
    for (i = 0; i < cfg_mgr->entry_count; i++) {
        if (cfg_mgr->entries[i].module == module && string_compare(cfg_mgr->entries[i].key, key) == 0) {
            if (cfg_mgr->entries[i].type == CONFIG_TYPE_BOOL) {
                *value = cfg_mgr->entries[i].value.bool_val;
                return 0;
            } else {
                // Type mismatch
                return -1;
            }
        }
    }
    
    // Entry not found
    return -1;
}

// Register a new configuration entry
int register_config_entry(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key, config_type_t type) {
    if (!cfg_mgr || !key || cfg_mgr->entry_count >= MAX_CONFIG_ENTRIES) {
        return -1;
    }
    
    // Check if entry already exists
    int i;
    for (i = 0; i < cfg_mgr->entry_count; i++) {
        if (cfg_mgr->entries[i].module == module && string_compare(cfg_mgr->entries[i].key, key) == 0) {
            // Entry already exists
            return -1;
        }
    }
    
    // Create new entry
    config_entry_t* entry = &cfg_mgr->entries[cfg_mgr->entry_count];
    
    string_copy(entry->key, key, MAX_CONFIG_KEY_LENGTH);
    entry->module = module;
    entry->type = type;
    entry->is_modified = 0;
    
    // Initialize value based on type
    switch (type) {
        case CONFIG_TYPE_INT:
            entry->value.int_val = 0;
            break;
        case CONFIG_TYPE_FLOAT:
            entry->value.float_val = 0.0f;
            break;
        case CONFIG_TYPE_STRING:
            entry->value.str_val[0] = '\0';
            break;
        case CONFIG_TYPE_BOOL:
            entry->value.bool_val = 0;
            break;
    }
    
    cfg_mgr->entry_count++;
    return 0;
}

// Apply configuration to a specific module
int apply_config_to_module(unified_config_manager_t* cfg_mgr, config_module_t module) {
    if (!cfg_mgr) {
        return -1;
    }
    
    // In a real implementation, this would apply all configuration values
    // for the specified module to that module's internal state
    // For now, we'll just return success
    return 0;
}

// Cleanup the config manager
void cleanup_config_manager(unified_config_manager_t* cfg_mgr) {
    if (!cfg_mgr) {
        return;
    }
    
    // Reset all fields to default values
    cfg_mgr->entry_count = 0;
    cfg_mgr->auto_save_enabled = 0;
    cfg_mgr->last_save_time = 0;
    
    // Initialize all config entries
    int i;
    for (i = 0; i < MAX_CONFIG_ENTRIES; i++) {
        config_entry_t* entry = &cfg_mgr->entries[i];
        entry->key[0] = '\0';
        entry->type = CONFIG_TYPE_INT; // Default value
        entry->module = CONFIG_MODULE_DIAGNOSTIC; // Default value
        entry->value.int_val = 0;
        entry->is_modified = 0;
    }
}

// Find entry index by module and key
static int find_entry_index(unified_config_manager_t* cfg_mgr, config_module_t module, const char* key) {
    if (!cfg_mgr || !key) {
        return -1;
    }
    
    int i;
    for (i = 0; i < cfg_mgr->entry_count; i++) {
        if (cfg_mgr->entries[i].module == module && string_compare(cfg_mgr->entries[i].key, key) == 0) {
            return i;
        }
    }
    
    return -1;
}

// Get the number of configuration entries for a module
int get_config_count_for_module(unified_config_manager_t* cfg_mgr, config_module_t module) {
    if (!cfg_mgr) {
        return -1;
    }
    
    int count = 0;
    int i;
    for (i = 0; i < cfg_mgr->entry_count; i++) {
        if (cfg_mgr->entries[i].module == module) {
            count++;
        }
    }
    
    return count;
}