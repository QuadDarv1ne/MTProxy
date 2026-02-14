/*
 * Унифицированный API для MTProxy
 * Единый интерфейс для управления всеми компонентами системы
 */

#ifndef _UNIFIED_API_H_
#define _UNIFIED_API_H_

#include "modular-architecture.h"
#include "plugin-interface.h"

// Типы API операций
typedef enum {
    API_OP_LOAD_MODULE = 0,
    API_OP_UNLOAD_MODULE = 1,
    API_OP_EXECUTE_PLUGIN = 2,
    API_OP_CONFIGURE_COMPONENT = 3,
    API_OP_GET_STATS = 4,
    API_OP_LIST_COMPONENTS = 5,
    API_OP_RELOAD_COMPONENT = 6,
    API_OP_REGISTER_CALLBACK = 7,
    API_OP_UNREGISTER_CALLBACK = 8
} api_operation_type_t;

// Статусы API
typedef enum {
    API_STATUS_SUCCESS = 0,
    API_STATUS_ERROR = 1,
    API_STATUS_INVALID_PARAMS = 2,
    API_STATUS_COMPONENT_NOT_FOUND = 3,
    API_STATUS_PERMISSION_DENIED = 4,
    API_STATUS_TIMEOUT = 5,
    API_STATUS_NOT_IMPLEMENTED = 6
} api_status_t;

// Параметры для загрузки модуля
typedef struct {
    char module_path[512];
    char module_name[256];
    module_type_t module_type;
    void *init_params;
} api_load_module_params_t;

// Параметры для выполнения плагина
typedef struct {
    char plugin_name[256];
    void *input_data;
    size_t input_size;
    void *output_data;
    size_t output_size;
    int timeout_ms;
} api_execute_plugin_params_t;

// Параметры для конфигурации компонента
typedef struct {
    char component_name[256];
    char property_name[128];
    void *property_value;
    size_t value_size;
} api_configure_params_t;

// Параметры для получения статистики
typedef struct {
    char component_name[256];
    int stat_type;  // 0: module stats, 1: plugin stats, 2: global stats
} api_get_stats_params_t;

// Общий параметр для API вызова
typedef union {
    api_load_module_params_t load_module;
    api_execute_plugin_params_t execute_plugin;
    api_configure_params_t configure;
    api_get_stats_params_t get_stats;
    char component_name[256];  // Для операций, требующих только имя компонента
} api_params_t;

// Результат API вызова
typedef struct {
    api_status_t status;
    int error_code;
    char error_message[512];
    void *result_data;
    size_t result_size;
    double execution_time_ms;
} api_result_t;

// Функция обратного вызова для асинхронных операций
typedef void (*api_callback_t)(api_result_t *result, void *user_data);

// Унифицированная функция API
api_status_t unified_api_call(api_operation_type_t operation, 
                            const api_params_t *params, 
                            api_result_t *result);

// Упрощенные функции для часто используемых операций
api_status_t unified_load_module(const char *module_path, 
                               module_type_t type, 
                               void *init_params);
api_status_t unified_unload_module(const char *module_name);
api_status_t unified_execute_plugin(const char *plugin_name, 
                                 void *input, 
                                 void *output, 
                                 size_t *output_size);
api_status_t unified_configure_component(const char *component_name, 
                                      const char *property, 
                                      void *value, 
                                      size_t value_size);
api_status_t unified_get_component_stats(const char *component_name, 
                                      void *stats_buffer, 
                                      size_t buffer_size);

// Функции управления компонентами
int unified_list_modules(module_descriptor_t **modules, int *count);
int unified_list_plugins(plugin_interface_t **plugins, int *count);
int unified_get_module_status(const char *module_name, module_state_t *state);
int unified_get_plugin_status(const char *plugin_name, plugin_status_t *status);

// Функции обратного вызова
int unified_register_callback(const char *event_type, api_callback_t callback, void *user_data);
int unified_unregister_callback(const char *event_type, api_callback_t callback);

// Инициализация и очистка
int unified_api_init(void);
void unified_api_cleanup(void);

// Проверка состояния
int unified_api_is_initialized(void);
int unified_api_is_component_loaded(const char *component_name);

#endif