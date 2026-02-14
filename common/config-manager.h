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
    CONFIG_TYPE_ENUM
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
};

// Configuration section
struct config_section {
    char name[64];
    struct config_parameter *parameters;
    int param_count;
    int param_capacity;
    time_t last_updated;
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