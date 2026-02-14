/*
 * Реализация интерфейса плагинов для MTProxy
 * Определение API для динамической загрузки и управления плагинами
 */

#include "plugin-interface.h"

// Простая реализация strcmp для совместимости с MTProxy
static int simple_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Глобальный менеджер плагинов
static plugin_manager_t g_plugin_manager = {0};

// Инициализация менеджера плагинов
int plugin_manager_init(plugin_manager_t *manager) {
    if (!manager) {
        return -1;
    }
    
    // Инициализация структуры менеджера
    manager->plugins = 0;  // В реальной реализации выделение памяти под массив плагинов
    manager->plugin_count = 0;
    manager->max_plugins = 50;  // Максимум 50 плагинов
    manager->plugin_directory = 0;
    manager->active_sessions = 0;
    manager->resource_manager = 0;
    manager->initialized = 0;
    
    // Инициализация глобальной статистики
    manager->global_stats.executions_count = 0;
    manager->global_stats.bytes_processed = 0;
    manager->global_stats.errors_count = 0;
    manager->global_stats.current_status = PLUGIN_STATUS_INITIALIZED;
    manager->global_stats.average_execution_time = 0.0;
    manager->global_stats.last_execution_time = 0;
    manager->global_stats.active_sessions = 0;
    
    manager->initialized = 1;
    
    // Копирование в глобальный менеджер
    g_plugin_manager = *manager;
    
    return 0;
}

// Загрузка плагина
int plugin_manager_load_plugin(plugin_manager_t *manager, const char *plugin_path) {
    if (!manager || !plugin_path || !manager->initialized) {
        return -1;
    }
    
    if (manager->plugin_count >= manager->max_plugins) {
        return -1;  // Достигнуто максимальное количество плагинов
    }
    
    // В реальной реализации: загрузка плагина из файла/библиотеки
    // и инициализация его интерфейса
    
    // Обновление статистики
    manager->plugin_count++;
    manager->global_stats.executions_count++;
    
    // Копирование в глобальный менеджер
    g_plugin_manager = *manager;
    
    return 0;
}

// Выгрузка плагина
int plugin_manager_unload_plugin(plugin_manager_t *manager, const char *plugin_name) {
    if (!manager || !plugin_name || !manager->initialized) {
        return -1;
    }
    
    // В реальной реализации: найти плагин по имени и выгрузить его
    // вызвать cleanup функцию плагина
    
    // Обновление статистики
    if (manager->plugin_count > 0) {
        manager->plugin_count--;
    }
    
    // Копирование в глобальный менеджер
    g_plugin_manager = *manager;
    
    return 0;
}

// Выполнение плагина
int plugin_manager_execute_plugin(plugin_manager_t *manager, 
                                const char *plugin_name, 
                                void *input, 
                                void *output, 
                                size_t *output_size) {
    if (!manager || !plugin_name || !manager->initialized) {
        return -1;
    }
    
    // В реальной реализации: найти плагин по имени и выполнить его
    // с предоставленными входными данными
    
    // Обновление статистики
    manager->global_stats.executions_count++;
    if (input) {
        manager->global_stats.bytes_processed += 1024;  // Условный размер
    }
    
    // Установка размера выходных данных
    if (output_size) {
        *output_size = 1024;  // Условный размер
    }
    
    // Копирование в глобальный менеджер
    g_plugin_manager = *manager;
    
    return 0;
}

// Перезагрузка плагина
int plugin_manager_reload_plugin(plugin_manager_t *manager, const char *plugin_name) {
    if (!manager || !plugin_name || !manager->initialized) {
        return -1;
    }
    
    // В реальной реализации: выгрузить и заново загрузить плагин
    // с сохранением конфигурации
    
    return 0;
}

// Очистка менеджера плагинов
void plugin_manager_cleanup(plugin_manager_t *manager) {
    if (!manager) {
        return;
    }
    
    // Выгрузить все плагины
    for (int i = 0; i < manager->plugin_count; i++) {
        if (manager->plugins[i] && manager->plugins[i]->cleanup) {
            manager->plugins[i]->cleanup();
        }
    }
    
    // Освободить ресурсы
    manager->plugins = 0;
    manager->plugin_directory = 0;
    manager->active_sessions = 0;
    manager->resource_manager = 0;
    
    // Сброс менеджера
    manager->plugin_count = 0;
    manager->initialized = 0;
    
    // Сброс глобальной статистики
    manager->global_stats.executions_count = 0;
    manager->global_stats.bytes_processed = 0;
    manager->global_stats.errors_count = 0;
    manager->global_stats.average_execution_time = 0.0;
    manager->global_stats.last_execution_time = 0;
    manager->global_stats.active_sessions = 0;
}

// Список плагинов
int plugin_manager_list_plugins(plugin_manager_t *manager, 
                              plugin_interface_t **plugins, 
                              int *count) {
    if (!manager || !plugins || !count || !manager->initialized) {
        return -1;
    }
    
    *count = manager->plugin_count;
    
    // Копирование указателей на плагины
    for (int i = 0; i < manager->plugin_count && i < manager->max_plugins; i++) {
        plugins[i] = manager->plugins[i];
    }
    
    return 0;
}

// Получение плагина
int plugin_manager_get_plugin(plugin_manager_t *manager, 
                            const char *plugin_name, 
                            plugin_interface_t **plugin) {
    if (!manager || !plugin_name || !plugin || !manager->initialized) {
        return -1;
    }
    
    // В реальной реализации: найти плагин по имени
    for (int i = 0; i < manager->plugin_count; i++) {
        if (manager->plugins[i] && 
            !simple_strcmp(manager->plugins[i]->config.name, plugin_name)) {
            *plugin = manager->plugins[i];
            return 0;
        }
    }
    
    return -1;  // Плагин не найден
}

// Конфигурация плагина
int plugin_manager_configure_plugin(plugin_manager_t *manager, 
                                 const char *plugin_name, 
                                 void *config) {
    if (!manager || !plugin_name || !config || !manager->initialized) {
        return -1;
    }
    
    plugin_interface_t *plugin = 0;
    if (plugin_manager_get_plugin(manager, plugin_name, &plugin) != 0) {
        return -1;  // Плагин не найден
    }
    
    if (plugin && plugin->configure) {
        return plugin->configure(config);
    }
    
    return -1;
}

// Установка параметра плагина
int plugin_manager_set_plugin_parameter(plugin_manager_t *manager, 
                                     const char *plugin_name, 
                                     const char *param_name, 
                                     void *value) {
    if (!manager || !plugin_name || !param_name || !manager->initialized) {
        return -1;
    }
    
    plugin_interface_t *plugin = 0;
    if (plugin_manager_get_plugin(manager, plugin_name, &plugin) != 0) {
        return -1;  // Плагин не найден
    }
    
    if (plugin && plugin->set_parameter) {
        return plugin->set_parameter(param_name, value);
    }
    
    return -1;
}

// Получение параметра плагина
int plugin_manager_get_plugin_parameter(plugin_manager_t *manager, 
                                     const char *plugin_name, 
                                     const char *param_name, 
                                     void **value) {
    if (!manager || !plugin_name || !param_name || !value || !manager->initialized) {
        return -1;
    }
    
    plugin_interface_t *plugin = 0;
    if (plugin_manager_get_plugin(manager, plugin_name, &plugin) != 0) {
        return -1;  // Плагин не найден
    }
    
    if (plugin && plugin->get_parameter) {
        return plugin->get_parameter(param_name, value);
    }
    
    return -1;
}

// Получение статистики плагина
plugin_stats_t plugin_manager_get_plugin_stats(plugin_manager_t *manager, 
                                            const char *plugin_name) {
    plugin_interface_t *plugin = 0;
    if (!manager || !plugin_name || plugin_manager_get_plugin(manager, plugin_name, &plugin) != 0) {
        // Возвращаем пустую статистику
        plugin_stats_t empty_stats = {0};
        return empty_stats;
    }
    
    return plugin->stats;
}

// Получение глобальной статистики
plugin_stats_t plugin_manager_get_global_stats(plugin_manager_t *manager) {
    if (!manager) {
        return g_plugin_manager.global_stats;
    }
    return manager->global_stats;
}

// Сброс статистики плагина
void plugin_manager_reset_plugin_stats(plugin_manager_t *manager, 
                                    const char *plugin_name) {
    plugin_interface_t *plugin = 0;
    if (!manager || !plugin_name || plugin_manager_get_plugin(manager, plugin_name, &plugin) != 0) {
        return;
    }
    
    plugin->stats.executions_count = 0;
    plugin->stats.bytes_processed = 0;
    plugin->stats.errors_count = 0;
    plugin->stats.average_execution_time = 0.0;
    plugin->stats.last_execution_time = 0;
    plugin->stats.active_sessions = 0;
}

// Сброс глобальной статистики
void plugin_manager_reset_global_stats(plugin_manager_t *manager) {
    if (!manager) {
        manager = &g_plugin_manager;
    }
    
    manager->global_stats.executions_count = 0;
    manager->global_stats.bytes_processed = 0;
    manager->global_stats.errors_count = 0;
    manager->global_stats.average_execution_time = 0.0;
    manager->global_stats.last_execution_time = 0;
    manager->global_stats.active_sessions = 0;
}

// Проверка доступности
int plugin_manager_is_available(void) {
    return 1;  // Для совместимости с MTProxy
}

// Валидация пути к плагину
int plugin_manager_validate_plugin_path(const char *plugin_path) {
    if (!plugin_path) {
        return 0;
    }
    
    // В реальной реализации: проверить существование файла и его формат
    // Для совместимости с MTProxy просто проверим, что строка не пустая
    return (plugin_path[0] != '\0') ? 1 : 0;
}

// Проверка совместимости
int plugin_manager_check_compatibility(plugin_interface_t *plugin, 
                                    int required_api_version) {
    if (!plugin) {
        return 0;
    }
    
    // Проверить версию API плагина
    return (plugin->config.api_version >= required_api_version) ? 1 : 0;
}