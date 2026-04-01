/*
 * Модульная архитектура для MTProxy
 * Поддержка плагинов и унифицированного API для управления компонентами
 */

#ifndef _MODULAR_ARCHITECTURE_H_
#define _MODULAR_ARCHITECTURE_H_

#include <stdint.h>

// Типы модулей
typedef enum {
    MODULE_TYPE_CORE = 0,
    MODULE_TYPE_NETWORK = 1,
    MODULE_TYPE_CRYPTO = 2,
    MODULE_TYPE_PROTOCOL = 3,
    MODULE_TYPE_SECURITY = 4,
    MODULE_TYPE_MONITORING = 5,
    MODULE_TYPE_PLUGIN = 6
} module_type_t;

// Состояния модуля
typedef enum {
    MODULE_STATE_UNLOADED = 0,
    MODULE_STATE_LOADED = 1,
    MODULE_STATE_INITIALIZED = 2,
    MODULE_STATE_ACTIVE = 3,
    MODULE_STATE_SUSPENDED = 4,
    MODULE_STATE_ERROR = 5
} module_state_t;

// Типы плагинов
typedef enum {
    PLUGIN_TYPE_PROTOCOL = 0,
    PLUGIN_TYPE_FILTER = 1,
    PLUGIN_TYPE_ENCODER = 2,
    PLUGIN_TYPE_DECODER = 3,
    PLUGIN_TYPE_AUTHENTICATOR = 4,
    PLUGIN_TYPE_ANALYZER = 5,
    PLUGIN_TYPE_OPTIMIZER = 6
} plugin_type_t;

// Статистика модульной архитектуры
typedef struct {
    long long total_modules_loaded;
    long long total_plugins_loaded;
    long long active_modules;
    long long active_plugins;
    long long api_calls_processed;
    double average_api_response_time;
    int total_module_types;
    long long last_activity_time;
} modular_architecture_stats_t;

// Конфигурация модульной архитектуры
typedef struct {
    int enable_modular_architecture;
    int enable_plugin_system;
    int max_loaded_modules;
    int max_loaded_plugins;
    int enable_dynamic_loading;
    int enable_hot_swapping;
    int enable_module_sandboxing;
    int sandbox_memory_limit_kb;
    int enable_api_validation;
    int api_timeout_ms;
    int enable_module_dependencies;
    int enable_circular_dependency_detection;
    int enable_security_scanning;
    int scan_timeout_ms;
    int enable_performance_monitoring;
    int performance_log_interval_ms;
} modular_architecture_config_t;

// Функции жизненного цикла модуля
typedef int (*module_init_func_t)(void *params);
typedef int (*module_start_func_t)(void);
typedef int (*module_stop_func_t)(void);
typedef int (*module_cleanup_func_t)(void);
typedef int (*module_process_func_t)(void *input, void *output);
typedef int (*module_configure_func_t)(void *config);

// Описание модуля
typedef struct module_descriptor {
    char name[256];
    char version[32];
    char description[512];
    module_type_t type;
    module_state_t state;
    module_init_func_t init_func;
    module_start_func_t start_func;
    module_stop_func_t stop_func;
    module_cleanup_func_t cleanup_func;
    module_process_func_t process_func;
    module_configure_func_t configure_func;
    void *handle;  // Для динамических библиотек
    struct module_descriptor *dependencies;
    int dependency_count;
    void *private_data;
    long long load_timestamp;
} module_descriptor_t;

// Интерфейс плагина
typedef struct {
    char name[256];
    char version[32];
    char author[128];
    plugin_type_t type;
    int api_version;
    int (*init)(void *config);
    int (*execute)(void *input, void *output);
    int (*cleanup)(void);
    int (*configure)(void *settings);
    int (*validate)(void *data);
    int (*get_capabilities)(void);
    int (*set_parameter)(const char *param_name, void *value);
    int (*get_parameter)(const char *param_name, void **value);
    void *plugin_data;
    void *reserved;  // Для будущего использования
} plugin_interface_t;

// Статус плагина
typedef enum {
    PLUGIN_STATUS_UNLOADED = 0,
    PLUGIN_STATUS_LOADED = 1,
    PLUGIN_STATUS_INITIALIZED = 2,
    PLUGIN_STATUS_ACTIVE = 3,
    PLUGIN_STATUS_ERROR = 4
} plugin_status_t;

// Возможности плагина
typedef enum {
    PLUGIN_CAPABILITY_PROCESSING = 1,
    PLUGIN_CAPABILITY_ENCRYPTION = 2,
    PLUGIN_CAPABILITY_DECRYPTION = 4,
    PLUGIN_CAPABILITY_FILTERING = 8,
    PLUGIN_CAPABILITY_LOGGING = 16,
    PLUGIN_CAPABILITY_ANALYTICS = 32,
    PLUGIN_CAPABILITY_MODIFICATION = 64
} plugin_capability_t;

// Статистика плагина
typedef struct {
    long long executions_count;
    long long bytes_processed;
    long long errors_count;
    plugin_status_t current_status;
    double average_execution_time;
    long long last_execution_time;
    int active_sessions;
} plugin_stats_t;

// Конфигурация плагина
typedef struct {
    char name[256];
    char version[32];
    char description[512];
    plugin_type_t type;
    int api_version;
    int capabilities;
    int max_concurrent_sessions;
    int timeout_ms;
    int enable_logging;
    int log_level;
    int enable_monitoring;
    int monitoring_interval_ms;
    int enable_resource_limiting;
    int memory_limit_kb;
    int cpu_limit_percent;
} plugin_config_t;

// Теперь определяем структуру плагина с полной информацией
typedef struct {
    plugin_interface_t interface;
    plugin_config_t config;
    plugin_stats_t stats;
    plugin_status_t status;
} full_plugin_info_t;

// Контекст модульной архитектуры
typedef struct {
    modular_architecture_config_t config;
    modular_architecture_stats_t stats;
    module_descriptor_t **loaded_modules;
    plugin_interface_t **loaded_plugins;
    int module_count;
    int plugin_count;
    int max_modules;
    int max_plugins;
    void *api_registry;
    void *dependency_resolver;
    void *module_loader;
    void *plugin_manager;
    int initialized;
    long long last_activity_time;
    void *mutex;  // Для многопоточной безопасности
} modular_architecture_context_t;

// Структура для вызова API
typedef struct {
    char module_name[256];
    char function_name[256];
    void *input_params;
    size_t input_size;
    void *output_params;
    size_t output_size;
    int timeout_ms;
} api_call_request_t;

// Результат вызова API
typedef struct {
    int success;
    int error_code;
    char error_message[512];
    void *result_data;
    size_t result_size;
    double execution_time;
} api_call_result_t;

// Функции инициализации архитектуры
int modular_arch_init(modular_architecture_context_t *ctx);
int modular_arch_init_with_config(modular_architecture_context_t *ctx, 
                                const modular_architecture_config_t *config);
void modular_arch_cleanup(modular_architecture_context_t *ctx);

// Функции управления модулями
int modular_arch_load_module(modular_architecture_context_t *ctx, 
                           const char *module_path, 
                           module_descriptor_t *desc);
int modular_arch_unload_module(modular_architecture_context_t *ctx, 
                            const char *module_name);
int modular_arch_get_module(modular_architecture_context_t *ctx, 
                          const char *module_name, 
                          module_descriptor_t **desc);
int modular_arch_list_modules(modular_architecture_context_t *ctx, 
                            module_descriptor_t **modules, 
                            int *count);
int modular_arch_module_operation(modular_architecture_context_t *ctx, 
                               const char *module_name, 
                               const char *operation, 
                               void *params);

// Функции управления плагинами
int modular_arch_load_plugin(modular_architecture_context_t *ctx, 
                           const char *plugin_path, 
                           plugin_type_t type);
int modular_arch_unload_plugin(modular_architecture_context_t *ctx, 
                            const char *plugin_name);
int modular_arch_execute_plugin(modular_architecture_context_t *ctx, 
                              const char *plugin_name, 
                              void *input, 
                              void *output);

// Функции унифицированного API
int modular_arch_api_call(modular_architecture_context_t *ctx, 
                        const api_call_request_t *request, 
                        api_call_result_t *result);
int modular_arch_register_api_function(const char *module_name, 
                                     const char *func_name, 
                                     void *func_ptr);
int modular_arch_unregister_api_function(const char *module_name, 
                                       const char *func_name);

// Функции зависимостей
int modular_arch_resolve_dependencies(modular_architecture_context_t *ctx);
int modular_arch_check_module_compatibility(modular_architecture_context_t *ctx, 
                                          const char *module_name, 
                                          const char *required_version);

// Функции статистики
modular_architecture_stats_t modular_arch_get_stats(modular_architecture_context_t *ctx);
void modular_arch_reset_stats(modular_architecture_context_t *ctx);

// Функции конфигурации
void modular_arch_get_config(modular_architecture_context_t *ctx, 
                           modular_architecture_config_t *config);
int modular_arch_update_config(modular_architecture_context_t *ctx, 
                             const modular_architecture_config_t *new_config);

// Вспомогательные функции
int modular_arch_is_available(void);
int modular_arch_validate_module(const module_descriptor_t *desc);
int modular_arch_validate_plugin(const plugin_interface_t *plugin);
int modular_arch_sanitize_input(void *input, size_t size);

#endif