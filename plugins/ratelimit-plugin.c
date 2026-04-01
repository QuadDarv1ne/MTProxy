/*
 * Rate Limiting Plugin for MTProxy
 * Плагин для ограничения количества подключений с одного IP
 *
 * Компиляция:
 *   gcc -shared -fPIC -o ratelimit-plugin.so ratelimit-plugin.c
 *
 * Установка:
 *   cp ratelimit-plugin.so /usr/lib/mtproxy/plugins/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/plugin-system.h"

/* ============================================================================
 * Конфигурация
 * ============================================================================ */

#define MAX_IP_ENTRIES 10000
#define DEFAULT_MAX_CONNECTIONS 10
#define DEFAULT_WINDOW_SECONDS 60

typedef struct {
    char ip[64];
    int connection_count;
    time_t window_start;
} ip_rate_entry_t;

typedef struct {
    ip_rate_entry_t entries[MAX_IP_ENTRIES];
    int entry_count;
    int max_connections;
    int window_seconds;
    int total_blocked;
} ratelimit_plugin_data_t;

/* ============================================================================
 * Вспомогательные функции
 * ============================================================================ */

static ip_rate_entry_t* find_or_create_entry(ratelimit_plugin_data_t *data, 
                                              const char *ip) {
    time_t now = time(NULL);
    
    // Поиск существующей записи
    for (int i = 0; i < data->entry_count; i++) {
        if (strcmp(data->entries[i].ip, ip) == 0) {
            // Сброс окна если истекло
            if (now - data->entries[i].window_start >= data->window_seconds) {
                data->entries[i].connection_count = 0;
                data->entries[i].window_start = now;
            }
            return &data->entries[i];
        }
    }
    
    // Создание новой записи
    if (data->entry_count < MAX_IP_ENTRIES) {
        ip_rate_entry_t *entry = &data->entries[data->entry_count++];
        strncpy(entry->ip, ip, sizeof(entry->ip) - 1);
        entry->connection_count = 0;
        entry->window_start = now;
        return entry;
    }
    
    return NULL;
}

/* ============================================================================
 * Хуки
 * ============================================================================ */

static plugin_result_t on_connection_accept(plugin_hook_context_t *ctx,
                                            void *plugin_data) {
    ratelimit_plugin_data_t *data = (ratelimit_plugin_data_t *)plugin_data;
    
    if (!ctx || !ctx->client_ip || !data) {
        return PLUGIN_OK;
    }
    
    // Поиск или создание записи для IP
    ip_rate_entry_t *entry = find_or_create_entry(data, ctx->client_ip);
    if (!entry) {
        // Превышен максимум записей, пропускаем
        return PLUGIN_OK;
    }
    
    // Увеличение счётчика
    entry->connection_count++;
    
    // Проверка лимита
    if (entry->connection_count > data->max_connections) {
        char msg[256];
        snprintf(msg, sizeof(msg), 
                 "Rate limit exceeded: %d connections from %s (max: %d per %ds)",
                 entry->connection_count, ctx->client_ip, 
                 data->max_connections, data->window_seconds);
        
        strncpy(ctx->result_data, msg, sizeof(ctx->result_data) - 1);
        ctx->result_code = 429;
        
        data->total_blocked++;
        
        return PLUGIN_REJECT;  // Блокировать подключение
    }
    
    return PLUGIN_OK;  // Разрешить подключение
}

/* ============================================================================
 * Экспортируемые функции
 * ============================================================================ */

PLUGIN_DECLARE_INFO(
    "ratelimit-plugin",
    "Rate limiting plugin - limits connections per IP",
    "1.0.0",
    "MTProxy Team",
    "Apache-2.0"
)

int plugin_init(const plugin_config_t *config, void **plugin_data) {
    if (!plugin_data) {
        return PLUGIN_ERROR;
    }
    
    // Выделение памяти
    ratelimit_plugin_data_t *data = calloc(1, sizeof(ratelimit_plugin_data_t));
    if (!data) {
        return PLUGIN_ERROR;
    }
    
    // Конфигурация по умолчанию
    data->max_connections = DEFAULT_MAX_CONNECTIONS;
    data->window_seconds = DEFAULT_WINDOW_SECONDS;
    data->entry_count = 0;
    data->total_blocked = 0;
    
    // Чтение конфигурации из файла
    if (config && config->config_path) {
        FILE *fp = fopen(config->config_path, "r");
        if (fp) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "max_connections=", 16) == 0) {
                    data->max_connections = atoi(line + 16);
                } else if (strncmp(line, "window_seconds=", 15) == 0) {
                    data->window_seconds = atoi(line + 15);
                }
            }
            fclose(fp);
        }
    }
    
    *plugin_data = data;
    
    printf("[ratelimit-plugin] Initialized (max_conn=%d, window=%ds)\n",
           data->max_connections, data->window_seconds);
    
    return PLUGIN_OK;
}

void plugin_shutdown(void *plugin_data) {
    ratelimit_plugin_data_t *data = (ratelimit_plugin_data_t *)plugin_data;
    
    if (!data) {
        return;
    }
    
    printf("[ratelimit-plugin] Shutdown. Total blocked: %d\n", 
           data->total_blocked);
    
    free(data);
}

int plugin_register_hook(plugin_hook_type_t hook_type,
                        plugin_hook_callback_t callback,
                        int priority,
                        void *plugin_data) {
    return PLUGIN_OK;
}

void plugin_get_stats(plugin_stats_t *stats) {
    if (!stats) {
        return;
    }
    
    ratelimit_plugin_data_t *data = (ratelimit_plugin_data_t *)stats->plugin_data;
    if (!data) {
        return;
    }
    
    memset(stats, 0, sizeof(plugin_stats_t));
    stats->total_requests = data->entry_count;
    stats->total_blocked = data->total_blocked;
    stats->plugin_data = data;
}

/* ============================================================================
 * Точки входа
 * ============================================================================ */

__attribute__((constructor))
static void plugin_on_load(void) {
    printf("[ratelimit-plugin] Plugin loaded\n");
}

__attribute__((destructor))
static void plugin_on_unload(void) {
    printf("[ratelimit-plugin] Plugin unloaded\n");
}
