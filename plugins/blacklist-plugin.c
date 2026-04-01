/*
 * IP Blacklist Plugin for MTProxy
 * Плагин для блокировки подключений по чёрному/белому списку IP
 *
 * Компиляция:
 *   gcc -shared -fPIC -o blacklist-plugin.so blacklist-plugin.c
 *
 * Установка:
 *   cp blacklist-plugin.so /usr/lib/mtproxy/plugins/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/plugin-system.h"

/* ============================================================================
 * Конфигурация
 * ============================================================================ */

#define MAX_IP_ENTRIES 1000

typedef struct {
    char ip[64];
    char description[256];
    time_t added_at;
} ip_entry_t;

typedef struct {
    ip_entry_t blacklist[MAX_IP_ENTRIES];
    int blacklist_count;
    
    ip_entry_t whitelist[MAX_IP_ENTRIES];
    int whitelist_count;
    
    int total_blocked;
    int total_allowed;
    bool whitelist_mode;  // true = разрешены только из whitelist
} blacklist_plugin_data_t;

/* ============================================================================
 * Вспомогательные функции
 * ============================================================================ */

static bool ip_in_list(const char *ip, ip_entry_t *list, int count) {
    for (int i = 0; i < count; i++) {
        // Проверка точного совпадения
        if (strcmp(list[i].ip, ip) == 0) {
            return true;
        }
        
        // Проверка маски (например, 192.168.1.*)
        const char *list_ip = list[i].ip;
        size_t ip_len = strlen(ip);
        size_t list_ip_len = strlen(list_ip);
        
        if (list_ip_len > 0 && list_ip[list_ip_len - 1] == '*') {
            size_t prefix_len = list_ip_len - 1;
            if (ip_len >= prefix_len && 
                strncmp(ip, list_ip, prefix_len) == 0) {
                return true;
            }
        }
    }
    return false;
}

static int add_to_list(ip_entry_t *list, int *count, const char *ip, 
                       const char *description) {
    if (*count >= MAX_IP_ENTRIES) {
        return -1;  // Список полон
    }
    
    // Проверка на дубликат
    if (ip_in_list(ip, list, *count)) {
        return -2;  // Уже в списке
    }
    
    ip_entry_t *entry = &list[*count];
    strncpy(entry->ip, ip, sizeof(entry->ip) - 1);
    strncpy(entry->description, description ? description : "", 
            sizeof(entry->description) - 1);
    entry->added_at = time(NULL);
    (*count)++;
    
    return 0;
}

static int remove_from_list(ip_entry_t *list, int *count, const char *ip) {
    for (int i = 0; i < *count; i++) {
        if (strcmp(list[i].ip, ip) == 0) {
            // Сдвиг элементов
            for (int j = i; j < *count - 1; j++) {
                list[j] = list[j + 1];
            }
            (*count)--;
            return 0;
        }
    }
    return -1;  // Не найдено
}

/* ============================================================================
 * Хуки
 * ============================================================================ */

static plugin_result_t on_connection_accept(plugin_hook_context_t *ctx,
                                            void *plugin_data) {
    blacklist_plugin_data_t *data = (blacklist_plugin_data_t *)plugin_data;
    
    if (!ctx || !ctx->client_ip || !data) {
        return PLUGIN_OK;
    }
    
    const char *ip = ctx->client_ip;
    
    // Whitelist режим: разрешены только из whitelist
    if (data->whitelist_mode) {
        if (ip_in_list(ip, data->whitelist, data->whitelist_count)) {
            data->total_allowed++;
            return PLUGIN_OK;
        } else {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                     "IP %s not in whitelist (whitelist mode enabled)", ip);
            strncpy(ctx->result_data, msg, sizeof(ctx->result_data) - 1);
            ctx->result_code = 403;
            return PLUGIN_REJECT;
        }
    }
    
    // Проверка whitelist (если не пустой)
    if (data->whitelist_count > 0 && ip_in_list(ip, data->whitelist, 
                                                 data->whitelist_count)) {
        data->total_allowed++;
        return PLUGIN_OK;  // Разрешено (в whitelist)
    }
    
    // Проверка blacklist
    if (ip_in_list(ip, data->blacklist, data->blacklist_count)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "IP %s is blacklisted", ip);
        strncpy(ctx->result_data, msg, sizeof(ctx->result_data) - 1);
        ctx->result_code = 403;
        
        data->total_blocked++;
        return PLUGIN_REJECT;  // Заблокировано
    }
    
    data->total_allowed++;
    return PLUGIN_OK;  // Разрешено (не в blacklist)
}

/* ============================================================================
 * Экспортируемые функции
 * ============================================================================ */

PLUGIN_DECLARE_INFO(
    "blacklist-plugin",
    "IP blacklist/whitelist plugin",
    "1.0.0",
    "MTProxy Team",
    "Apache-2.0"
)

int plugin_init(const plugin_config_t *config, void **plugin_data) {
    if (!plugin_data) {
        return PLUGIN_ERROR;
    }
    
    // Выделение памяти
    blacklist_plugin_data_t *data = calloc(1, sizeof(blacklist_plugin_data_t));
    if (!data) {
        return PLUGIN_ERROR;
    }
    
    data->blacklist_count = 0;
    data->whitelist_count = 0;
    data->total_blocked = 0;
    data->total_allowed = 0;
    data->whitelist_mode = false;
    
    // Чтение конфигурации
    if (config && config->config_path) {
        FILE *fp = fopen(config->config_path, "r");
        if (fp) {
            char line[512];
            while (fgets(line, sizeof(line), fp)) {
                // Удаление newline
                line[strcspn(line, "\n")] = 0;
                
                if (strncmp(line, "blacklist=", 10) == 0) {
                    add_to_list(data->blacklist, &data->blacklist_count, 
                               line + 10, "from config");
                } else if (strncmp(line, "whitelist=", 10) == 0) {
                    add_to_list(data->whitelist, &data->whitelist_count, 
                               line + 10, "from config");
                } else if (strcmp(line, "whitelist_mode=true") == 0) {
                    data->whitelist_mode = true;
                }
            }
            fclose(fp);
        }
    }
    
    *plugin_data = data;
    
    printf("[blacklist-plugin] Initialized (blacklist=%d, whitelist=%d, mode=%s)\n",
           data->blacklist_count, data->whitelist_count,
           data->whitelist_mode ? "whitelist" : "blacklist");
    
    return PLUGIN_OK;
}

void plugin_shutdown(void *plugin_data) {
    blacklist_plugin_data_t *data = (blacklist_plugin_data_t *)plugin_data;
    
    if (!data) {
        return;
    }
    
    printf("[blacklist-plugin] Shutdown. Blocked: %d, Allowed: %d\n",
           data->total_blocked, data->total_allowed);
    
    free(data);
}

/* ============================================================================
 * API для управления списками (вызывается из основного приложения)
 * ============================================================================ */

int plugin_blacklist_add(void *plugin_data, const char *ip, 
                         const char *description) {
    blacklist_plugin_data_t *data = (blacklist_plugin_data_t *)plugin_data;
    if (!data) return PLUGIN_ERROR;
    
    int ret = add_to_list(data->blacklist, &data->blacklist_count, ip, description);
    if (ret == 0) {
        printf("[blacklist-plugin] Added to blacklist: %s (%s)\n", ip, description);
    }
    return ret;
}

int plugin_blacklist_remove(void *plugin_data, const char *ip) {
    blacklist_plugin_data_t *data = (blacklist_plugin_data_t *)plugin_data;
    if (!data) return PLUGIN_ERROR;
    
    int ret = remove_from_list(data->blacklist, &data->blacklist_count, ip);
    if (ret == 0) {
        printf("[blacklist-plugin] Removed from blacklist: %s\n", ip);
    }
    return ret;
}

int plugin_whitelist_add(void *plugin_data, const char *ip, 
                         const char *description) {
    blacklist_plugin_data_t *data = (blacklist_plugin_data_t *)plugin_data;
    if (!data) return PLUGIN_ERROR;
    
    int ret = add_to_list(data->whitelist, &data->whitelist_count, ip, description);
    if (ret == 0) {
        printf("[blacklist-plugin] Added to whitelist: %s (%s)\n", ip, description);
    }
    return ret;
}

int plugin_whitelist_remove(void *plugin_data, const char *ip) {
    blacklist_plugin_data_t *data = (blacklist_plugin_data_t *)plugin_data;
    if (!data) return PLUGIN_ERROR;
    
    int ret = remove_from_list(data->whitelist, &data->whitelist_count, ip);
    if (ret == 0) {
        printf("[blacklist-plugin] Removed from whitelist: %s\n", ip);
    }
    return ret;
}

void plugin_get_stats(plugin_stats_t *stats) {
    if (!stats) {
        return;
    }
    
    blacklist_plugin_data_t *data = (blacklist_plugin_data_t *)stats->plugin_data;
    if (!data) {
        return;
    }
    
    memset(stats, 0, sizeof(plugin_stats_t));
    stats->total_requests = data->total_allowed + data->total_blocked;
    stats->total_blocked = data->total_blocked;
    stats->plugin_data = data;
}

/* ============================================================================
 * Точки входа
 * ============================================================================ */

__attribute__((constructor))
static void plugin_on_load(void) {
    printf("[blacklist-plugin] Plugin loaded\n");
}

__attribute__((destructor))
static void plugin_on_unload(void) {
    printf("[blacklist-plugin] Plugin unloaded\n");
}
