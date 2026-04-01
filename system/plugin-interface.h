/*
 * Интерфейс плагинов для MTProxy
 * Определение API для динамической загрузки и управления плагинами
 */

#ifndef _PLUGIN_INTERFACE_H_
#define _PLUGIN_INTERFACE_H_

#include "modular-architecture.h"

// Все необходимые типы определены в modular-architecture.h

// Менеджер плагинов
typedef struct {
    plugin_interface_t **plugins;
    int plugin_count;
    int max_plugins;
    void *plugin_directory;
    void *active_sessions;
    void *resource_manager;
    int initialized;
    plugin_stats_t global_stats;
} plugin_manager_t;

// Функции инициализации плагинов
int plugin_manager_init(plugin_manager_t *manager);
int plugin_manager_load_plugin(plugin_manager_t *manager, const char *plugin_path);
int plugin_manager_unload_plugin(plugin_manager_t *manager, const char *plugin_name);
int plugin_manager_execute_plugin(plugin_manager_t *manager, 
                                const char *plugin_name, 
                                void *input, 
                                void *output, 
                                size_t *output_size);
int plugin_manager_reload_plugin(plugin_manager_t *manager, const char *plugin_name);
void plugin_manager_cleanup(plugin_manager_t *manager);

// Функции управления плагинами
int plugin_manager_list_plugins(plugin_manager_t *manager, 
                              plugin_interface_t **plugins, 
                              int *count);
int plugin_manager_get_plugin(plugin_manager_t *manager, 
                            const char *plugin_name, 
                            plugin_interface_t **plugin);
int plugin_manager_configure_plugin(plugin_manager_t *manager, 
                                 const char *plugin_name, 
                                 void *config);
int plugin_manager_set_plugin_parameter(plugin_manager_t *manager, 
                                     const char *plugin_name, 
                                     const char *param_name, 
                                     void *value);
int plugin_manager_get_plugin_parameter(plugin_manager_t *manager, 
                                     const char *plugin_name, 
                                     const char *param_name, 
                                     void **value);

// Функции статистики
plugin_stats_t plugin_manager_get_plugin_stats(plugin_manager_t *manager, 
                                            const char *plugin_name);
plugin_stats_t plugin_manager_get_global_stats(plugin_manager_t *manager);
void plugin_manager_reset_plugin_stats(plugin_manager_t *manager, 
                                    const char *plugin_name);
void plugin_manager_reset_global_stats(plugin_manager_t *manager);

// Вспомогательные функции
int plugin_manager_is_available(void);
int plugin_manager_validate_plugin_path(const char *plugin_path);
int plugin_manager_check_compatibility(plugin_interface_t *plugin, 
                                    int required_api_version);

#endif