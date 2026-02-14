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

#ifndef __PLUGGABLE_TRANSPORTS_H__
#define __PLUGGABLE_TRANSPORTS_H__

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct pt_manager_context;
struct transport_plugin;

// Transport types (дублируем из shadowsocks-advanced.h)
enum transport_type {
    TRANSPORT_TCP = 0,
    TRANSPORT_UDP,
    TRANSPORT_WEBSOCKET,
    TRANSPORT_QUIC,
    TRANSPORT_HTTP2
};

// Transport plugin function types
typedef int (*transport_init_func)(void *config);
typedef int (*transport_cleanup_func)(void *data);
typedef int (*transport_send_func)(void *data, const unsigned char *buf, int len);
typedef int (*transport_receive_func)(void *data, unsigned char *buf, int len);
typedef int (*transport_configure_func)(void *data, const char *config_str);

// Transport information structure
struct transport_info {
    char name[64];
    enum transport_type type;
    int is_active;
    int priority;
};

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

// Инициализация pluggable transports системы
int pt_manager_init(void);

// Регистрация нового transport plugin
int pt_register_transport(const struct transport_plugin *plugin);

// Загрузка transport plugin по имени
int pt_load_transport(const char *name, void *config);

// Выгрузка transport plugin
int pt_unload_transport(const char *name);

// Создание pluggable transport manager
struct pt_manager_context *pt_manager_create(const unsigned char *session_key);

// Отправка данных через pluggable transport
int pt_manager_send(struct pt_manager_context *ctx, const unsigned char *data, int len);

// Получение данных через pluggable transport
int pt_manager_receive(struct pt_manager_context *ctx, unsigned char *buffer, int buf_len);

// Настройка transport параметров
int pt_manager_configure_transport(struct pt_manager_context *ctx, 
                                  const char *transport_name, 
                                  const char *config_str);

// Автоматическое переключение transport
void pt_auto_switch_transport(struct pt_manager_context *ctx);

// Load balancing между транспортами
struct transport_plugin *pt_select_load_balanced_transport(void);

// Получение списка доступных transports
int pt_get_available_transports(struct transport_info *transports, int max_count);

// Получение статистики
void pt_manager_get_stats(struct pt_stats *stats);

// Вывод статистики
void pt_manager_print_stats(void);

// Очистка pluggable transport manager
void pt_manager_destroy(struct pt_manager_context *ctx);

// Очистка системы transports
void pt_manager_cleanup(void);

// Built-in transport plugin structure
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

#ifdef __cplusplus
}
#endif

#endif // __PLUGGABLE_TRANSPORTS_H__