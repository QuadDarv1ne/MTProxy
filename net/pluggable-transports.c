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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "net/pluggable-transports.h"
#include "common/kprintf.h"
#include "common/common-stats.h"
#include "net/shadowsocks-advanced.h"

// Статистика для pluggable transports
struct pt_stats {
    long long transport_registrations;
    long long transport_activations;
    long long transport_deactivations;
    long long data_transmitted;
    long long transport_switches;
    long long plugin_loads;
    long long plugin_unloads;
};

static struct pt_stats pt_statistics = {0};

// Registry для pluggable transports
#define MAX_TRANSPORTS 32
#define MAX_PLUGINS 16

// Transport plugin interface
struct transport_plugin {
    char name[64];
    enum transport_type type;
    transport_init_func init;
    transport_cleanup_func cleanup;
    transport_send_func send;
    transport_receive_func receive;
    transport_configure_func configure;
    void *plugin_data;
    int is_loaded;
    int priority;
};

// Transport registry
static struct transport_plugin transport_registry[MAX_TRANSPORTS];
static int registered_transports = 0;
static pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;

// Активные транспорты
static struct transport_plugin *active_transports[MAX_TRANSPORTS];
static int active_transport_count = 0;

// Pluggable transport manager context
struct pt_manager_context {
    struct transport_plugin *current_transport;
    unsigned char session_key[32];
    time_t last_switch;
    int auto_switch_enabled;
    int load_balancing_enabled;
};

// Built-in transport implementations
static int tcp_transport_init(void *config);
static int tcp_transport_cleanup(void *data);
static int tcp_transport_send(void *data, const unsigned char *buf, int len);
static int tcp_transport_receive(void *data, unsigned char *buf, int len);
static int tcp_transport_configure(void *data, const char *config_str);

static int udp_transport_init(void *config);
static int udp_transport_cleanup(void *data);
static int udp_transport_send(void *data, const unsigned char *buf, int len);
static int udp_transport_receive(void *data, unsigned char *buf, int len);
static int udp_transport_configure(void *data, const char *config_str);

static int websocket_transport_init(void *config);
static int websocket_transport_cleanup(void *data);
static int websocket_transport_send(void *data, const unsigned char *buf, int len);
static int websocket_transport_receive(void *data, unsigned char *buf, int len);
static int websocket_transport_configure(void *data, const char *config_str);

// Инициализация pluggable transports системы
int pt_manager_init(void) {
    pthread_mutex_init(&registry_mutex, NULL);
    
    // Регистрация built-in транспортов
    struct transport_plugin tcp_plugin = {
        .name = "tcp",
        .type = TRANSPORT_TCP,
        .init = tcp_transport_init,
        .cleanup = tcp_transport_cleanup,
        .send = tcp_transport_send,
        .receive = tcp_transport_receive,
        .configure = tcp_transport_configure,
        .priority = 100
    };
    
    struct transport_plugin udp_plugin = {
        .name = "udp",
        .type = TRANSPORT_UDP,
        .init = udp_transport_init,
        .cleanup = udp_transport_cleanup,
        .send = udp_transport_send,
        .receive = udp_transport_receive,
        .configure = udp_transport_configure,
        .priority = 80
    };
    
    struct transport_plugin ws_plugin = {
        .name = "websocket",
        .type = TRANSPORT_WEBSOCKET,
        .init = websocket_transport_init,
        .cleanup = websocket_transport_cleanup,
        .send = websocket_transport_send,
        .receive = websocket_transport_receive,
        .configure = websocket_transport_configure,
        .priority = 90
    };
    
    pt_register_transport(&tcp_plugin);
    pt_register_transport(&udp_plugin);
    pt_register_transport(&ws_plugin);
    
    vkprintf(1, "Pluggable transports manager initialized with %d built-in transports\n", 
             registered_transports);
    return 0;
}

// Регистрация нового transport plugin
int pt_register_transport(const struct transport_plugin *plugin) {
    if (!plugin || registered_transports >= MAX_TRANSPORTS) {
        return -1;
    }
    
    pthread_mutex_lock(&registry_mutex);
    
    // Проверяем дубликаты
    for (int i = 0; i < registered_transports; i++) {
        if (strcmp(transport_registry[i].name, plugin->name) == 0) {
            pthread_mutex_unlock(&registry_mutex);
            return -1;
        }
    }
    
    // Регистрируем новый transport
    memcpy(&transport_registry[registered_transports], plugin, sizeof(struct transport_plugin));
    transport_registry[registered_transports].is_loaded = 1;
    registered_transports++;
    
    pt_statistics.transport_registrations++;
    pthread_mutex_unlock(&registry_mutex);
    
    vkprintf(2, "Registered transport plugin: %s (type %d)\n", 
             plugin->name, plugin->type);
    return 0;
}

// Загрузка transport plugin по имени
int pt_load_transport(const char *name, void *config) {
    pthread_mutex_lock(&registry_mutex);
    
    for (int i = 0; i < registered_transports; i++) {
        if (strcmp(transport_registry[i].name, name) == 0 && 
            transport_registry[i].is_loaded) {
            
            // Инициализируем transport
            if (transport_registry[i].init && 
                transport_registry[i].init(config) != 0) {
                pthread_mutex_unlock(&registry_mutex);
                return -1;
            }
            
            // Добавляем в активные транспорты
            active_transports[active_transport_count] = &transport_registry[i];
            active_transport_count++;
            
            pt_statistics.transport_activations++;
            pt_statistics.plugin_loads++;
            
            pthread_mutex_unlock(&registry_mutex);
            vkprintf(2, "Loaded transport plugin: %s\n", name);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&registry_mutex);
    return -1;
}

// Выгрузка transport plugin
int pt_unload_transport(const char *name) {
    pthread_mutex_lock(&registry_mutex);
    
    for (int i = 0; i < active_transport_count; i++) {
        if (strcmp(active_transports[i]->name, name) == 0) {
            // Очищаем transport
            if (active_transports[i]->cleanup) {
                active_transports[i]->cleanup(active_transports[i]->plugin_data);
            }
            
            // Удаляем из активных транспортов
            for (int j = i; j < active_transport_count - 1; j++) {
                active_transports[j] = active_transports[j + 1];
            }
            active_transport_count--;
            
            pt_statistics.transport_deactivations++;
            pt_statistics.plugin_unloads++;
            
            pthread_mutex_unlock(&registry_mutex);
            vkprintf(2, "Unloaded transport plugin: %s\n", name);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&registry_mutex);
    return -1;
}

// Создание pluggable transport manager
struct pt_manager_context *pt_manager_create(const unsigned char *session_key) {
    struct pt_manager_context *ctx = calloc(1, sizeof(struct pt_manager_context));
    if (!ctx) {
        return NULL;
    }
    
    if (session_key) {
        memcpy(ctx->session_key, session_key, 32);
    }
    
    ctx->auto_switch_enabled = 1;
    ctx->load_balancing_enabled = 1;
    ctx->last_switch = time(NULL);
    
    return ctx;
}

// Отправка данных через pluggable transport
int pt_manager_send(struct pt_manager_context *ctx, const unsigned char *data, int len) {
    if (!ctx || !data || len <= 0) {
        return -1;
    }
    
    // Выбираем активный transport
    if (!ctx->current_transport && active_transport_count > 0) {
        ctx->current_transport = active_transports[0];
    }
    
    if (!ctx->current_transport) {
        return -1;
    }
    
    // Отправляем данные
    int result = -1;
    if (ctx->current_transport->send) {
        result = ctx->current_transport->send(ctx->current_transport->plugin_data, data, len);
    }
    
    if (result > 0) {
        pt_statistics.data_transmitted += result;
    }
    
    return result;
}

// Получение данных через pluggable transport
int pt_manager_receive(struct pt_manager_context *ctx, unsigned char *buffer, int buf_len) {
    if (!ctx || !buffer || buf_len <= 0) {
        return -1;
    }
    
    // Выбираем активный transport
    if (!ctx->current_transport && active_transport_count > 0) {
        ctx->current_transport = active_transports[0];
    }
    
    if (!ctx->current_transport) {
        return -1;
    }
    
    // Получаем данные
    int result = -1;
    if (ctx->current_transport->receive) {
        result = ctx->current_transport->receive(ctx->current_transport->plugin_data, buffer, buf_len);
    }
    
    if (result > 0) {
        pt_statistics.data_transmitted += result;
    }
    
    return result;
}

// Автоматическое переключение transport
static void pt_auto_switch_transport(struct pt_manager_context *ctx) {
    if (!ctx->auto_switch_enabled || active_transport_count <= 1) {
        return;
    }
    
    time_t now = time(NULL);
    if (now - ctx->last_switch < 300) { // 5 минут минимальный интервал
        return;
    }
    
    // Выбираем следующий transport по приоритету
    int current_index = -1;
    for (int i = 0; i < active_transport_count; i++) {
        if (active_transports[i] == ctx->current_transport) {
            current_index = i;
            break;
        }
    }
    
    if (current_index >= 0) {
        int next_index = (current_index + 1) % active_transport_count;
        ctx->current_transport = active_transports[next_index];
        ctx->last_switch = now;
        pt_statistics.transport_switches++;
        
        vkprintf(2, "Auto-switched to transport: %s\n", 
                 ctx->current_transport->name);
    }
}

// Load balancing между транспортами
static struct transport_plugin *pt_select_load_balanced_transport(void) {
    if (active_transport_count <= 0) {
        return NULL;
    }
    
    // Простой round-robin алгоритм
    static int current_index = 0;
    struct transport_plugin *selected = active_transports[current_index];
    current_index = (current_index + 1) % active_transport_count;
    
    return selected;
}

// Настройка transport параметров
int pt_manager_configure_transport(struct pt_manager_context *ctx, 
                                  const char *transport_name, 
                                  const char *config_str) {
    if (!ctx || !transport_name) {
        return -1;
    }
    
    pthread_mutex_lock(&registry_mutex);
    
    for (int i = 0; i < active_transport_count; i++) {
        if (strcmp(active_transports[i]->name, transport_name) == 0) {
            if (active_transports[i]->configure) {
                int result = active_transports[i]->configure(
                    active_transports[i]->plugin_data, config_str);
                pthread_mutex_unlock(&registry_mutex);
                return result;
            }
            break;
        }
    }
    
    pthread_mutex_unlock(&registry_mutex);
    return -1;
}

// Получение списка доступных transports
int pt_get_available_transports(struct transport_info *transports, int max_count) {
    if (!transports || max_count <= 0) {
        return -1;
    }
    
    pthread_mutex_lock(&registry_mutex);
    
    int count = (registered_transports < max_count) ? registered_transports : max_count;
    for (int i = 0; i < count; i++) {
        strncpy(transports[i].name, transport_registry[i].name, 
                sizeof(transports[i].name) - 1);
        transports[i].name[sizeof(transports[i].name) - 1] = '\0';
        transports[i].type = transport_registry[i].type;
        transports[i].is_active = 0;
        
        // Проверяем активность
        for (int j = 0; j < active_transport_count; j++) {
            if (active_transports[j] == &transport_registry[i]) {
                transports[i].is_active = 1;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&registry_mutex);
    return count;
}

// Получение статистики
void pt_manager_get_stats(struct pt_stats *stats) {
    if (stats) {
        memcpy(stats, &pt_statistics, sizeof(struct pt_stats));
    }
}

// Вывод статистики
void pt_manager_print_stats(void) {
    vkprintf(1, "Pluggable Transports Statistics:\n");
    vkprintf(1, "  Transport Registrations: %lld\n", pt_statistics.transport_registrations);
    vkprintf(1, "  Transport Activations: %lld\n", pt_statistics.transport_activations);
    vkprintf(1, "  Transport Deactivations: %lld\n", pt_statistics.transport_deactivations);
    vkprintf(1, "  Data Transmitted: %lld bytes\n", pt_statistics.data_transmitted);
    vkprintf(1, "  Transport Switches: %lld\n", pt_statistics.transport_switches);
    vkprintf(1, "  Plugin Loads: %lld\n", pt_statistics.plugin_loads);
    vkprintf(1, "  Plugin Unloads: %lld\n", pt_statistics.plugin_unloads);
    vkprintf(1, "  Registered Transports: %d\n", registered_transports);
    vkprintf(1, "  Active Transports: %d\n", active_transport_count);
}

// Очистка pluggable transport manager
void pt_manager_destroy(struct pt_manager_context *ctx) {
    if (ctx) {
        free(ctx);
    }
}

// Очистка системы transports
void pt_manager_cleanup(void) {
    // Выгружаем все активные transports
    while (active_transport_count > 0) {
        pt_unload_transport(active_transports[0]->name);
    }
    
    // Очищаем registry
    registered_transports = 0;
    
    pthread_mutex_destroy(&registry_mutex);
    vkprintf(1, "Pluggable transports manager cleaned up\n");
}

// Built-in TCP transport implementation
static int tcp_transport_init(void *config) {
    // Инициализация TCP транспорта
    vkprintf(2, "TCP transport initialized\n");
    return 0;
}

static int tcp_transport_cleanup(void *data) {
    // Очистка TCP транспорта
    vkprintf(2, "TCP transport cleaned up\n");
    return 0;
}

static int tcp_transport_send(void *data, const unsigned char *buf, int len) {
    // Отправка через TCP (заглушка)
    vkprintf(3, "TCP send: %d bytes\n", len);
    return len;
}

static int tcp_transport_receive(void *data, unsigned char *buf, int len) {
    // Получение через TCP (заглушка)
    vkprintf(3, "TCP receive: %d bytes\n", len);
    return len;
}

static int tcp_transport_configure(void *data, const char *config_str) {
    // Конфигурация TCP транспорта
    vkprintf(2, "TCP transport configured: %s\n", config_str ? config_str : "default");
    return 0;
}

// Built-in UDP transport implementation
static int udp_transport_init(void *config) {
    vkprintf(2, "UDP transport initialized\n");
    return 0;
}

static int udp_transport_cleanup(void *data) {
    vkprintf(2, "UDP transport cleaned up\n");
    return 0;
}

static int udp_transport_send(void *data, const unsigned char *buf, int len) {
    vkprintf(3, "UDP send: %d bytes\n", len);
    return len;
}

static int udp_transport_receive(void *data, unsigned char *buf, int len) {
    vkprintf(3, "UDP receive: %d bytes\n", len);
    return len;
}

static int udp_transport_configure(void *data, const char *config_str) {
    vkprintf(2, "UDP transport configured: %s\n", config_str ? config_str : "default");
    return 0;
}

// Built-in WebSocket transport implementation
static int websocket_transport_init(void *config) {
    vkprintf(2, "WebSocket transport initialized\n");
    return 0;
}

static int websocket_transport_cleanup(void *data) {
    vkprintf(2, "WebSocket transport cleaned up\n");
    return 0;
}

static int websocket_transport_send(void *data, const unsigned char *buf, int len) {
    vkprintf(3, "WebSocket send: %d bytes\n", len);
    return len;
}

static int websocket_transport_receive(void *data, unsigned char *buf, int len) {
    vkprintf(3, "WebSocket receive: %d bytes\n", len);
    return len;
}

static int websocket_transport_configure(void *data, const char *config_str) {
    vkprintf(2, "WebSocket transport configured: %s\n", config_str ? config_str : "default");
    return 0;
}