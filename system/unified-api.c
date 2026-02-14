/*
 * Реализация унифицированного API для MTProxy
 * Единый интерфейс для управления всеми компонентами системы
 */

#include "unified-api.h"

// Глобальные переменные для унифицированного API
static modular_architecture_context_t g_modular_arch;
static plugin_manager_t g_plugin_mgr;
static int g_api_initialized = 0;

// Простая реализация strcpy для совместимости с MTProxy
static void simple_strcpy(char *dest, const char *src) {
    while ((*dest++ = *src++));
}

// Простая реализация strcmp для совместимости с MTProxy
static int simple_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Инициализация унифицированного API
int unified_api_init(void) {
    // Инициализация модульной архитектуры
    if (modular_arch_init(&g_modular_arch) != 0) {
        return -1;
    }
    
    // Инициализация менеджера плагинов
    if (plugin_manager_init(&g_plugin_mgr) != 0) {
        return -1;
    }
    
    g_api_initialized = 1;
    return 0;
}

// Очистка унифицированного API
void unified_api_cleanup(void) {
    if (!g_api_initialized) {
        return;
    }
    
    // Очистка менеджера плагинов
    plugin_manager_cleanup(&g_plugin_mgr);
    
    // Очистка модульной архитектуры
    modular_arch_cleanup(&g_modular_arch);
    
    g_api_initialized = 0;
}

// Унифицированный вызов API
api_status_t unified_api_call(api_operation_type_t operation, 
                            const api_params_t *params, 
                            api_result_t *result) {
    if (!g_api_initialized) {
        if (result) {
            result->status = API_STATUS_ERROR;
            result->error_code = 1;
            simple_strcpy(result->error_message, "API not initialized");
        }
        return API_STATUS_ERROR;
    }
    
    if (!result) {
        return API_STATUS_INVALID_PARAMS;
    }
    
    // Инициализация результата
    result->status = API_STATUS_SUCCESS;
    result->error_code = 0;
    result->error_message[0] = '\0';
    result->result_data = 0;
    result->result_size = 0;
    result->execution_time_ms = 0.0;
    
    // Выполнение операции в зависимости от типа
    switch (operation) {
        case API_OP_LOAD_MODULE:
            if (params) {
                if (modular_arch_load_module(&g_modular_arch, 
                                           params->load_module.module_path, 
                                           0) == 0) {  // В реальной реализации нужно передать дескриптор
                    result->status = API_STATUS_SUCCESS;
                } else {
                    result->status = API_STATUS_ERROR;
                    simple_strcpy(result->error_message, "Failed to load module");
                }
            } else {
                result->status = API_STATUS_INVALID_PARAMS;
                simple_strcpy(result->error_message, "Invalid parameters for load module");
            }
            break;
            
        case API_OP_EXECUTE_PLUGIN:
            if (params) {
                size_t output_size = 0;
                if (plugin_manager_execute_plugin(&g_plugin_mgr,
                                               params->execute_plugin.plugin_name,
                                               params->execute_plugin.input_data,
                                               params->execute_plugin.output_data,
                                               &output_size) == 0) {
                    result->status = API_STATUS_SUCCESS;
                    result->result_size = output_size;
                } else {
                    result->status = API_STATUS_ERROR;
                    simple_strcpy(result->error_message, "Failed to execute plugin");
                }
            } else {
                result->status = API_STATUS_INVALID_PARAMS;
                simple_strcpy(result->error_message, "Invalid parameters for execute plugin");
            }
            break;
            
        case API_OP_CONFIGURE_COMPONENT:
            if (params) {
                // В реальной реализации: настройка компонента
                result->status = API_STATUS_SUCCESS;
            } else {
                result->status = API_STATUS_INVALID_PARAMS;
                simple_strcpy(result->error_message, "Invalid parameters for configure component");
            }
            break;
            
        case API_OP_GET_STATS:
            if (params) {
                // В реальной реализации: получение статистики компонента
                result->status = API_STATUS_SUCCESS;
            } else {
                result->status = API_STATUS_INVALID_PARAMS;
                simple_strcpy(result->error_message, "Invalid parameters for get stats");
            }
            break;
            
        case API_OP_LIST_COMPONENTS:
            // В реальной реализации: список всех компонентов
            result->status = API_STATUS_SUCCESS;
            break;
            
        case API_OP_UNLOAD_MODULE:
            if (params) {
                if (modular_arch_unload_module(&g_modular_arch, 
                                             params->component_name) == 0) {
                    result->status = API_STATUS_SUCCESS;
                } else {
                    result->status = API_STATUS_ERROR;
                    simple_strcpy(result->error_message, "Failed to unload module");
                }
            } else {
                result->status = API_STATUS_INVALID_PARAMS;
                simple_strcpy(result->error_message, "Invalid parameters for unload module");
            }
            break;
            
        case API_OP_RELOAD_COMPONENT:
            // В реальной реализации: перезагрузка компонента
            result->status = API_STATUS_NOT_IMPLEMENTED;
            simple_strcpy(result->error_message, "Reload component not implemented");
            break;
            
        case API_OP_REGISTER_CALLBACK:
        case API_OP_UNREGISTER_CALLBACK:
            // В реальной реализации: регистрация/отмена регистрации callback'ов
            result->status = API_STATUS_NOT_IMPLEMENTED;
            simple_strcpy(result->error_message, "Callback registration not implemented");
            break;
            
        default:
            result->status = API_STATUS_ERROR;
            simple_strcpy(result->error_message, "Unknown operation");
            break;
    }
    
    return result->status;
}

// Упрощенные функции для часто используемых операций

api_status_t unified_load_module(const char *module_path, 
                               module_type_t type, 
                               void *init_params) {
    if (!g_api_initialized) {
        return API_STATUS_ERROR;
    }
    
    api_params_t params;
    api_result_t result;
    
    simple_strcpy(params.load_module.module_path, module_path ? module_path : "");
    params.load_module.module_type = type;
    params.load_module.init_params = init_params;
    
    return unified_api_call(API_OP_LOAD_MODULE, &params, &result);
}

api_status_t unified_unload_module(const char *module_name) {
    if (!g_api_initialized) {
        return API_STATUS_ERROR;
    }
    
    api_params_t params;
    api_result_t result;
    
    simple_strcpy(params.component_name, module_name ? module_name : "");
    
    return unified_api_call(API_OP_UNLOAD_MODULE, &params, &result);
}

api_status_t unified_execute_plugin(const char *plugin_name, 
                                 void *input, 
                                 void *output, 
                                 size_t *output_size) {
    if (!g_api_initialized) {
        return API_STATUS_ERROR;
    }
    
    api_params_t params;
    api_result_t result;
    
    simple_strcpy(params.execute_plugin.plugin_name, plugin_name ? plugin_name : "");
    params.execute_plugin.input_data = input;
    params.execute_plugin.output_data = output;
    params.execute_plugin.timeout_ms = 5000; // 5 секунд по умолчанию
    
    api_status_t status = unified_api_call(API_OP_EXECUTE_PLUGIN, &params, &result);
    
    if (output_size && result.result_size > 0) {
        *output_size = result.result_size;
    }
    
    return status;
}

api_status_t unified_configure_component(const char *component_name, 
                                      const char *property, 
                                      void *value, 
                                      size_t value_size) {
    if (!g_api_initialized) {
        return API_STATUS_ERROR;
    }
    
    api_params_t params;
    api_result_t result;
    
    simple_strcpy(params.configure.component_name, component_name ? component_name : "");
    simple_strcpy(params.configure.property_name, property ? property : "");
    params.configure.property_value = value;
    params.configure.value_size = value_size;
    
    return unified_api_call(API_OP_CONFIGURE_COMPONENT, &params, &result);
}

api_status_t unified_get_component_stats(const char *component_name, 
                                      void *stats_buffer, 
                                      size_t buffer_size) {
    if (!g_api_initialized) {
        return API_STATUS_ERROR;
    }
    
    api_params_t params;
    api_result_t result;
    
    simple_strcpy(params.get_stats.component_name, component_name ? component_name : "");
    params.get_stats.stat_type = 0; // По умолчанию статистика модуля
    
    // В реальной реализации: получить статистику и скопировать в буфер
    api_status_t status = unified_api_call(API_OP_GET_STATS, &params, &result);
    
    if (status == API_STATUS_SUCCESS && stats_buffer && result.result_data) {
        // Копирование результата в буфер (в реальной реализации)
    }
    
    return status;
}

// Функции управления компонентами

int unified_list_modules(module_descriptor_t **modules, int *count) {
    if (!g_api_initialized) {
        return -1;
    }
    
    return modular_arch_list_modules(&g_modular_arch, modules, count);
}

int unified_list_plugins(plugin_interface_t **plugins, int *count) {
    if (!g_api_initialized) {
        return -1;
    }
    
    return plugin_manager_list_plugins(&g_plugin_mgr, plugins, count);
}

int unified_get_module_status(const char *module_name, module_state_t *state) {
    if (!g_api_initialized || !module_name || !state) {
        return -1;
    }
    
    module_descriptor_t *desc = 0;
    if (modular_arch_get_module(&g_modular_arch, module_name, &desc) == 0) {
        *state = desc->state;
        return 0;
    }
    
    return -1;
}

int unified_get_plugin_status(const char *plugin_name, plugin_status_t *status) {
    if (!g_api_initialized || !plugin_name || !status) {
        return -1;
    }
    
    // Временно возвращаем успешный результат, так как точное поле статуса
    // в plugin_interface_t зависит от внутренней реализации
    (void)plugin_name; // Подавляем предупреждение о неиспользуемой переменной
    *status = PLUGIN_STATUS_ACTIVE; // Временное значение
    return 0;
}

// Функции обратного вызова (в реальной реализации)
int unified_register_callback(const char *event_type, api_callback_t callback, void *user_data) {
    if (!g_api_initialized) {
        return -1;
    }
    
    // В реальной реализации: регистрация callback'а
    return 0;
}

int unified_unregister_callback(const char *event_type, api_callback_t callback) {
    if (!g_api_initialized) {
        return -1;
    }
    
    // В реальной реализации: отмена регистрации callback'а
    return 0;
}

// Проверка состояния
int unified_api_is_initialized(void) {
    return g_api_initialized;
}

int unified_api_is_component_loaded(const char *component_name) {
    if (!g_api_initialized || !component_name) {
        return 0;
    }
    
    // Проверить, является ли компонент модулем
    module_descriptor_t *module_desc = 0;
    if (modular_arch_get_module(&g_modular_arch, component_name, &module_desc) == 0) {
        return (module_desc->state == MODULE_STATE_ACTIVE);
    }
    
    // Временно возвращаем 0, так как точное поле статуса
    // в plugin_interface_t зависит от внутренней реализации
    (void)component_name; // Подавляем предупреждение о неиспользуемой переменной
    return 0;
}