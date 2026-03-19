/*
    This file is part of Mtproto-proxy Library.

    Mtproto-proxy Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Mtproto-proxy Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Mtproto-proxy Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2024-2026 MTProto Proxy Enhanced Project
              2024-2026 Dupley Maxim Igorevich (Maestro7IT)
*/

#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Configuration statistics
struct config_manager_stats {
    long long total_config_loads;
    long long config_reload_count;
    long long validation_errors;
    long long migration_operations;
    long long runtime_changes;
    long long config_cache_hits;
    long long config_cache_misses;
};

// Configuration parameter types
enum config_param_type {
    CONFIG_TYPE_INT = 0,
    CONFIG_TYPE_LONG,
    CONFIG_TYPE_DOUBLE,
    CONFIG_TYPE_STRING,
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_ENUM,
    CONFIG_TYPE_ARRAY,
    CONFIG_TYPE_OBJECT
};

// Configuration change events
enum config_change_event {
    CONFIG_EVENT_CREATED = 0,
    CONFIG_EVENT_MODIFIED = 1,
    CONFIG_EVENT_DELETED = 2,
    CONFIG_EVENT_RELOADED = 3
};

// Validation result codes
enum config_validation_result {
    CONFIG_VALID = 0,
    CONFIG_INVALID_VALUE = 1,
    CONFIG_OUT_OF_RANGE = 2,
    CONFIG_INVALID_TYPE = 3,
    CONFIG_DEPENDENCY_ERROR = 4,
    CONFIG_SYNTAX_ERROR = 5
};

// Configuration parameter structure
struct config_parameter {
    char name[128];
    char description[256];
    enum config_param_type type;
    void *value_ptr;
    size_t value_size;
    int is_runtime_modifiable;
    int is_sensitive;
    char default_value[256];
    char min_value[64];
    char max_value[64];
    time_t last_modified;
    int version;
    
    // Расширенные поля
    char unit[32];                          // Единица измерения (ms, bytes, %)
    int (*validator_func)(const void *val); // Пользовательская валидация
    void *validator_data;                   // Данные для валидатора
    char depends_on[128];                   // Зависимый параметр
    int is_deprecated;                      // Помечен как устаревший
    char replacement[128];                  // Замена для deprecated параметра
};

// Configuration change history entry
struct config_change_entry {
    time_t timestamp;
    char section[64];
    char parameter[128];
    char old_value[256];
    char new_value[256];
    char changed_by[64];
    enum config_change_event event_type;
};

// Configuration change history
struct config_change_history {
    struct config_change_entry *entries;
    int entry_count;
    int max_entries;
    int current_index;
};

// Callback function type for config changes
typedef void (*config_change_callback_t)(
    const char *section,
    const char *param_name,
    enum config_change_event event,
    void *user_data
);

// Callback registration
struct config_callback_entry {
    config_change_callback_t callback;
    void *user_data;
    int is_active;
};

// Configuration section
struct config_section {
    char name[64];
    char description[256];
    struct config_parameter *parameters;
    int param_count;
    int param_capacity;
    time_t last_updated;
    
    // Расширенные поля
    struct config_callback_entry *callbacks;
    int callback_count;
    int max_callbacks;
};

// Configuration context
struct config_context {
    struct config_section *sections;
    int section_count;
    int section_capacity;
    char config_file_path[512];
    time_t last_file_modified;
    int auto_reload_enabled;
    int validation_enabled;
    pthread_mutex_t config_mutex;
    pthread_t reload_thread;
    int reload_thread_running;
    
    // Расширенные поля
    struct config_change_history *change_history;
    struct config_callback_entry *global_callbacks;
    int global_callback_count;
    int max_global_callbacks;
    int config_version;
    char last_error[256];
    int json_pretty_print;
    int case_sensitive_keys;
};

// Инициализация configuration manager
int config_manager_init(const char *config_file_path);

// Создание новой секции конфигурации
int config_manager_create_section(const char *section_name, const char *description);

// Регистрация параметра конфигурации
int config_manager_register_parameter(
    const char *section_name,
    const char *param_name,
    enum config_param_type type,
    void *value_ptr,
    size_t value_size,
    int is_runtime_modifiable,
    const char *default_value,
    const char *description);

// Установка значения параметра
int config_manager_set_parameter(
    const char *section_name,
    const char *param_name,
    const void *value,
    size_t value_size);

// Получение значения параметра
int config_manager_get_parameter(
    const char *section_name,
    const char *param_name,
    void *value_out,
    size_t value_size);

// Установка строкового параметра
int config_manager_set_parameter_string(
    const char *section_name,
    const char *param_name,
    const char *value_string);

// Загрузка конфигурации из файла
int config_manager_load_from_file(const char *file_path);

// Получение статистики
void config_manager_get_stats(struct config_manager_stats *stats);

// Вывод статистики
void config_manager_print_stats(void);

// Очистка configuration manager
void config_manager_cleanup(void);

// Расширенные функции
int config_manager_save_to_file(const char *file_path);
int config_manager_validate_config(void);
int config_manager_migrate_config(int from_version, int to_version);
int config_manager_enable_auto_reload(int enable);
int config_manager_get_parameter_info(const char *section_name,
                                     const char *param_name,
                                     struct config_parameter *info_out);
int config_manager_list_sections(char section_names[][64], int max_sections);
int config_manager_list_parameters(const char *section_name,
                                  char param_names[][128],
                                  int max_parameters);

// Расширенные функции управления конфигурацией
int config_manager_register_callback(const char *section_name,
                                    const char *param_name,
                                    config_change_callback_t callback,
                                    void *user_data);
int config_manager_unregister_callback(const char *section_name,
                                       const char *param_name,
                                       config_change_callback_t callback);
int config_manager_notify_change(const char *section_name,
                                const char *param_name,
                                enum config_change_event event);

// Функции истории изменений
int config_manager_get_change_history(struct config_change_entry *entries,
                                     int max_entries,
                                     time_t from_time,
                                     time_t to_time);
int config_manager_clear_change_history(void);
int config_manager_get_change_count(time_t from_time, time_t to_time);

// JSON экспорт/импорт
int config_manager_export_to_json(const char *filename, int include_sensitive);
int config_manager_import_from_json(const char *filename);
char* config_manager_export_to_json_string(int include_sensitive);
int config_manager_import_from_json_string(const char *json_string);

// Валидация
int config_manager_validate_parameter(const char *section_name,
                                     const char *param_name,
                                     const void *value);
int config_manager_set_validator(const char *section_name,
                                const char *param_name,
                                int (*validator)(const void*),
                                void *validator_data);
int config_manager_validate_dependencies(void);

// Утилиты
int config_manager_backup_config(const char *backup_path);
int config_manager_restore_config(const char *backup_path);
int config_manager_diff_configs(const char *file1, const char *file2,
                               char *diff_output, size_t output_size);
int config_manager_get_config_version(void);
const char* config_manager_get_last_error(void);
void config_manager_clear_last_error(void);

// Горячая перезагрузка конфигурации
int config_manager_hot_reload(const char *new_config_path);
int config_manager_schedule_reload(time_t delay_seconds);
int config_manager_is_reload_needed(void);

// Массовые операции
int config_manager_begin_batch(void);
int config_manager_commit_batch(void);
int config_manager_rollback_batch(void);
int config_manager_is_batch_mode(void);

// Вспомогательные макросы
#define CONFIG_REGISTER_INT(section, name, ptr, modifiable, default_val, desc) \
    config_manager_register_parameter(section, name, CONFIG_TYPE_INT, ptr, \
                                    sizeof(int), modifiable, #default_val, desc)

#define CONFIG_REGISTER_STRING(section, name, ptr, modifiable, default_val, desc) \
    config_manager_register_parameter(section, name, CONFIG_TYPE_STRING, ptr, \
                                    strlen(ptr) + 1, modifiable, default_val, desc)

#define CONFIG_REGISTER_BOOL(section, name, ptr, modifiable, default_val, desc) \
    config_manager_register_parameter(section, name, CONFIG_TYPE_BOOL, ptr, \
                                    sizeof(int), modifiable, #default_val, desc)

#define CONFIG_REGISTER_DOUBLE(section, name, ptr, modifiable, default_val, desc) \
    config_manager_register_parameter(section, name, CONFIG_TYPE_DOUBLE, ptr, \
                                    sizeof(double), modifiable, #default_val, desc)

#ifdef __cplusplus
}
#endif

#endif // __CONFIG_MANAGER_H__