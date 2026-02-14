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

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

#include "common/config-manager.h"
#include "common/kprintf.h"
#include "common/common-stats.h"

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

static struct config_manager_stats config_stats = {0};

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
    int is_sensitive; // Для паролей и ключей
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
    pthread_mutex_t config_mutex;
    pthread_t reload_thread;
    int reload_thread_running;
};

// Global configuration context
static struct config_context global_config_ctx = {0};

// Built-in configuration sections
static const struct builtin_config_section {
    const char *name;
    const char *description;
} builtin_sections[] = {
    {"network", "Network configuration parameters"},
    {"security", "Security-related settings"},
    {"performance", "Performance tuning parameters"},
    {"logging", "Logging configuration"},
    {"monitoring", "Monitoring and profiling settings"},
    {"advanced", "Advanced/experimental features"}
};

#define BUILTIN_SECTION_COUNT (sizeof(builtin_sections) / sizeof(builtin_sections[0]))

// Configuration validation rules
struct config_validation_rule {
    const char *param_name;
    int (*validator)(const void *value, const char *param_name);
    const char *error_message;
};

// Инициализация configuration manager
int config_manager_init(const char *config_file_path) {
    if (global_config_ctx.sections) {
        return 0; // Уже инициализирован
    }
    
    pthread_mutex_init(&global_config_ctx.config_mutex, NULL);
    
    // Инициализация секций
    global_config_ctx.section_capacity = BUILTIN_SECTION_COUNT + 10;
    global_config_ctx.sections = calloc(global_config_ctx.section_capacity, 
                                       sizeof(struct config_section));
    if (!global_config_ctx.sections) {
        return -1;
    }
    
    // Создание builtin секций
    for (int i = 0; i < BUILTIN_SECTION_COUNT; i++) {
        strncpy(global_config_ctx.sections[i].name, 
                builtin_sections[i].name, 
                sizeof(global_config_ctx.sections[i].name) - 1);
        global_config_ctx.section_count++;
    }
    
    // Установка пути к конфигу
    if (config_file_path) {
        strncpy(global_config_ctx.config_file_path, 
                config_file_path, 
                sizeof(global_config_ctx.config_file_path) - 1);
    } else {
        strcpy(global_config_ctx.config_file_path, "/etc/mtproxy.conf");
    }
    
    global_config_ctx.auto_reload_enabled = 1;
    global_config_ctx.validation_enabled = 1;
    global_config_ctx.reload_thread_running = 0;
    
    vkprintf(1, "Configuration manager initialized with %d builtin sections\n", 
             BUILTIN_SECTION_COUNT);
    
    return 0;
}

// Создание новой секции конфигурации
int config_manager_create_section(const char *section_name, const char *description) {
    pthread_mutex_lock(&global_config_ctx.config_mutex);
    
    // Проверка существования
    for (int i = 0; i < global_config_ctx.section_count; i++) {
        if (strcmp(global_config_ctx.sections[i].name, section_name) == 0) {
            pthread_mutex_unlock(&global_config_ctx.config_mutex);
            return -1; // Секция уже существует
        }
    }
    
    // Расширение массива если нужно
    if (global_config_ctx.section_count >= global_config_ctx.section_capacity) {
        int new_capacity = global_config_ctx.section_capacity * 2;
        struct config_section *new_sections = realloc(global_config_ctx.sections,
                                                     new_capacity * sizeof(struct config_section));
        if (!new_sections) {
            pthread_mutex_unlock(&global_config_ctx.config_mutex);
            return -1;
        }
        global_config_ctx.sections = new_sections;
        global_config_ctx.section_capacity = new_capacity;
    }
    
    // Создание новой секции
    struct config_section *new_section = &global_config_ctx.sections[global_config_ctx.section_count];
    strncpy(new_section->name, section_name, sizeof(new_section->name) - 1);
    new_section->parameters = NULL;
    new_section->param_count = 0;
    new_section->param_capacity = 0;
    new_section->last_updated = time(NULL);
    
    if (description) {
        // TODO: Сохранить описание
    }
    
    global_config_ctx.section_count++;
    pthread_mutex_unlock(&global_config_ctx.config_mutex);
    
    vkprintf(2, "Created configuration section: %s\n", section_name);
    return 0;
}

// Регистрация параметра конфигурации
int config_manager_register_parameter(
    const char *section_name,
    const char *param_name,
    enum config_param_type type,
    void *value_ptr,
    size_t value_size,
    int is_runtime_modifiable,
    const char *default_value,
    const char *description) {
    
    pthread_mutex_lock(&global_config_ctx.config_mutex);
    
    // Поиск секции
    struct config_section *section = NULL;
    for (int i = 0; i < global_config_ctx.section_count; i++) {
        if (strcmp(global_config_ctx.sections[i].name, section_name) == 0) {
            section = &global_config_ctx.sections[i];
            break;
        }
    }
    
    if (!section) {
        pthread_mutex_unlock(&global_config_ctx.config_mutex);
        return -1;
    }
    
    // Проверка существования параметра
    for (int i = 0; i < section->param_count; i++) {
        if (strcmp(section->parameters[i].name, param_name) == 0) {
            pthread_mutex_unlock(&global_config_ctx.config_mutex);
            return -1; // Параметр уже существует
        }
    }
    
    // Расширение массива параметров если нужно
    if (section->param_count >= section->param_capacity) {
        int new_capacity = section->param_capacity == 0 ? 16 : section->param_capacity * 2;
        struct config_parameter *new_params = realloc(section->parameters,
                                                     new_capacity * sizeof(struct config_parameter));
        if (!new_params) {
            pthread_mutex_unlock(&global_config_ctx.config_mutex);
            return -1;
        }
        section->parameters = new_params;
        section->param_capacity = new_capacity;
    }
    
    // Создание нового параметра
    struct config_parameter *new_param = &section->parameters[section->param_count];
    strncpy(new_param->name, param_name, sizeof(new_param->name) - 1);
    if (description) {
        strncpy(new_param->description, description, sizeof(new_param->description) - 1);
    }
    new_param->type = type;
    new_param->value_ptr = value_ptr;
    new_param->value_size = value_size;
    new_param->is_runtime_modifiable = is_runtime_modifiable;
    new_param->is_sensitive = (strstr(param_name, "password") != NULL || 
                              strstr(param_name, "secret") != NULL ||
                              strstr(param_name, "key") != NULL);
    new_param->last_modified = time(NULL);
    new_param->version = 1;
    
    if (default_value) {
        strncpy(new_param->default_value, default_value, sizeof(new_param->default_value) - 1);
    }
    
    section->param_count++;
    section->last_updated = time(NULL);
    
    pthread_mutex_unlock(&global_config_ctx.config_mutex);
    
    vkprintf(3, "Registered config parameter: %s.%s (type %d)\n", 
             section_name, param_name, type);
    return 0;
}

// Установка значения параметра
int config_manager_set_parameter(
    const char *section_name,
    const char *param_name,
    const void *value,
    size_t value_size) {
    
    pthread_mutex_lock(&global_config_ctx.config_mutex);
    
    struct config_parameter *param = config_manager_find_parameter(section_name, param_name);
    if (!param) {
        pthread_mutex_unlock(&global_config_ctx.config_mutex);
        return -1;
    }
    
    // Проверка возможности runtime изменения
    if (!param->is_runtime_modifiable) {
        vkprintf(2, "Parameter %s.%s is not runtime modifiable\n", section_name, param_name);
        pthread_mutex_unlock(&global_config_ctx.config_mutex);
        return -1;
    }
    
    // Проверка размера
    if (value_size != param->value_size) {
        vkprintf(2, "Value size mismatch for parameter %s.%s\n", section_name, param_name);
        pthread_mutex_unlock(&global_config_ctx.config_mutex);
        return -1;
    }
    
    // Копирование значения
    memcpy(param->value_ptr, value, value_size);
    param->last_modified = time(NULL);
    param->version++;
    
    // Обновление времени секции
    struct config_section *section = config_manager_find_section(section_name);
    if (section) {
        section->last_updated = time(NULL);
    }
    
    config_stats.runtime_changes++;
    pthread_mutex_unlock(&global_config_ctx.config_mutex);
    
    vkprintf(2, "Updated parameter: %s.%s\n", section_name, param_name);
    return 0;
}

// Получение значения параметра
int config_manager_get_parameter(
    const char *section_name,
    const char *param_name,
    void *value_out,
    size_t value_size) {
    
    pthread_mutex_lock(&global_config_ctx.config_mutex);
    
    struct config_parameter *param = config_manager_find_parameter(section_name, param_name);
    if (!param) {
        pthread_mutex_unlock(&global_config_ctx.config_mutex);
        return -1;
    }
    
    // Проверка размера
    if (value_size != param->value_size) {
        pthread_mutex_unlock(&global_config_ctx.config_mutex);
        return -1;
    }
    
    // Копирование значения
    memcpy(value_out, param->value_ptr, value_size);
    config_stats.config_cache_hits++;
    
    pthread_mutex_unlock(&global_config_ctx.config_mutex);
    return 0;
}

// Вспомогательные функции поиска
static struct config_section *config_manager_find_section(const char *section_name) {
    for (int i = 0; i < global_config_ctx.section_count; i++) {
        if (strcmp(global_config_ctx.sections[i].name, section_name) == 0) {
            return &global_config_ctx.sections[i];
        }
    }
    return NULL;
}

static struct config_parameter *config_manager_find_parameter(
    const char *section_name, 
    const char *param_name) {
    
    struct config_section *section = config_manager_find_section(section_name);
    if (!section) {
        return NULL;
    }
    
    for (int i = 0; i < section->param_count; i++) {
        if (strcmp(section->parameters[i].name, param_name) == 0) {
            return &section->parameters[i];
        }
    }
    return NULL;
}

// Загрузка конфигурации из файла
int config_manager_load_from_file(const char *file_path) {
    if (!file_path) {
        file_path = global_config_ctx.config_file_path;
    }
    
    FILE *file = fopen(file_path, "r");
    if (!file) {
        vkprintf(1, "Cannot open config file: %s\n", file_path);
        return -1;
    }
    
    char line[1024];
    char current_section[64] = "";
    
    while (fgets(line, sizeof(line), file)) {
        // Удаление комментариев и whitespace
        char *comment = strchr(line, '#');
        if (comment) {
            *comment = '\0';
        }
        
        // Пропуск пустых строк
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') {
            trimmed++;
        }
        if (*trimmed == '\0' || *trimmed == '\n') {
            continue;
        }
        
        // Обработка секции [section]
        if (*trimmed == '[') {
            char *end_bracket = strchr(trimmed, ']');
            if (end_bracket) {
                *end_bracket = '\0';
                strncpy(current_section, trimmed + 1, sizeof(current_section) - 1);
                vkprintf(3, "Processing section: %s\n", current_section);
            }
            continue;
        }
        
        // Обработка параметра key = value
        char *equals = strchr(trimmed, '=');
        if (equals && current_section[0] != '\0') {
            *equals = '\0';
            char *key = trimmed;
            char *value = equals + 1;
            
            // Trim whitespace
            while (*key == ' ' || *key == '\t') key++;
            char *key_end = key + strlen(key) - 1;
            while (key_end > key && (*key_end == ' ' || *key_end == '\t' || *key_end == '\n')) {
                *key_end = '\0';
                key_end--;
            }
            
            while (*value == ' ' || *value == '\t') value++;
            char *value_end = value + strlen(value) - 1;
            while (value_end > value && (*value_end == ' ' || *value_end == '\t' || *value_end == '\n')) {
                *value_end = '\0';
                value_end--;
            }
            
            // Установка параметра
            config_manager_set_parameter_string(current_section, key, value);
        }
    }
    
    fclose(file);
    config_stats.total_config_loads++;
    
    struct stat st;
    if (stat(file_path, &st) == 0) {
        global_config_ctx.last_file_modified = st.st_mtime;
    }
    
    vkprintf(1, "Configuration loaded from: %s\n", file_path);
    return 0;
}

// Установка строкового параметра
int config_manager_set_parameter_string(
    const char *section_name,
    const char *param_name,
    const char *value_string) {
    
    struct config_parameter *param = config_manager_find_parameter(section_name, param_name);
    if (!param) {
        return -1;
    }
    
    // Конвертация строки в соответствующий тип
    switch (param->type) {
        case CONFIG_TYPE_INT: {
            int int_val = atoi(value_string);
            return config_manager_set_parameter(section_name, param_name, &int_val, sizeof(int));
        }
        
        case CONFIG_TYPE_LONG: {
            long long_val = atoll(value_string);
            return config_manager_set_parameter(section_name, param_name, &long_val, sizeof(long));
        }
        
        case CONFIG_TYPE_DOUBLE: {
            double double_val = atof(value_string);
            return config_manager_set_parameter(section_name, param_name, &double_val, sizeof(double));
        }
        
        case CONFIG_TYPE_STRING: {
            return config_manager_set_parameter(section_name, param_name, value_string, 
                                              strlen(value_string) + 1);
        }
        
        case CONFIG_TYPE_BOOL: {
            int bool_val = (strcasecmp(value_string, "true") == 0 || 
                           strcasecmp(value_string, "yes") == 0 || 
                           atoi(value_string) != 0);
            return config_manager_set_parameter(section_name, param_name, &bool_val, sizeof(int));
        }
        
        default:
            return -1;
    }
}

// Получение статистики
void config_manager_get_stats(struct config_manager_stats *stats) {
    if (stats) {
        memcpy(stats, &config_stats, sizeof(struct config_manager_stats));
    }
}

// Вывод статистики
void config_manager_print_stats(void) {
    vkprintf(1, "Configuration Manager Statistics:\n");
    vkprintf(1, "  Total Config Loads: %lld\n", config_stats.total_config_loads);
    vkprintf(1, "  Config Reload Count: %lld\n", config_stats.config_reload_count);
    vkprintf(1, "  Validation Errors: %lld\n", config_stats.validation_errors);
    vkprintf(1, "  Migration Operations: %lld\n", config_stats.migration_operations);
    vkprintf(1, "  Runtime Changes: %lld\n", config_stats.runtime_changes);
    vkprintf(1, "  Config Cache Hits: %lld\n", config_stats.config_cache_hits);
    vkprintf(1, "  Config Cache Misses: %lld\n", config_stats.config_cache_misses);
    vkprintf(1, "  Sections: %d\n", global_config_ctx.section_count);
    
    // Вывод параметров по секциям
    for (int i = 0; i < global_config_ctx.section_count; i++) {
        struct config_section *section = &global_config_ctx.sections[i];
        vkprintf(1, "  Section '%s': %d parameters\n", section->name, section->param_count);
    }
}

// Очистка configuration manager
void config_manager_cleanup(void) {
    pthread_mutex_lock(&global_config_ctx.config_mutex);
    
    // Очистка секций
    for (int i = 0; i < global_config_ctx.section_count; i++) {
        struct config_section *section = &global_config_ctx.sections[i];
        if (section->parameters) {
            free(section->parameters);
        }
    }
    
    if (global_config_ctx.sections) {
        free(global_config_ctx.sections);
    }
    
    pthread_mutex_unlock(&global_config_ctx.config_mutex);
    pthread_mutex_destroy(&global_config_ctx.config_mutex);
    
    memset(&config_stats, 0, sizeof(config_stats));
    memset(&global_config_ctx, 0, sizeof(global_config_ctx));
    
    vkprintf(1, "Configuration manager cleaned up\n");
}