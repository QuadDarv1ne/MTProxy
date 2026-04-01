/*
 * socks5.h - SOCKS5 клиент/сервер с аутентификацией
 * Поддержка SOCKS4/SOCKS5, методы аутентификации
 */

#ifndef __SOCKS5_H__
#define __SOCKS5_H__

#include <stdint.h>
#include <pthread.h>

#define SOCKS5_PORT 1080
#define SOCKS5_MAX_USERS 64
#define SOCKS5_MAX_USERNAME_LEN 256
#define SOCKS5_MAX_PASSWORD_LEN 256

/* SOCKS5 команды */
#define SOCKS_CMD_CONNECT    0x01
#define SOCKS_CMD_BIND       0x02
#define SOCKS_CMD_UDP_ASSOC  0x03

/* SOCKS5 типы адресов */
#define SOCKS_ATYP_IPV4  0x01
#define SOCKS_ATYP_DOMAIN 0x03
#define SOCKS_ATYP_IPV6  0x04

/* SOCKS5 методы аутентификации */
#define SOCKS_AUTH_NONE    0x00
#define SOCKS_AUTH_GSSAPI  0x01
#define SOCKS_AUTH_USERPASS 0x02
#define SOCKS_AUTH_NO_ACCEPT 0xFF

/* SOCKS5 ответы */
#define SOCKS_REP_SUCCESS     0x00
#define SOCKS_REP_FAILURE     0x01
#define SOCKS_REP_NOT_ALLOWED 0x02
#define SOCKS_REP_NET_UNREACH 0x03
#define SOCKS_REP_HOST_UNREACH 0x04
#define SOCKS_REP_CONN_REFUSED 0x05
#define SOCKS_REP_TTL_EXPIRED  0x06

/* Пользователь для аутентификации */
typedef struct {
    char username[SOCKS5_MAX_USERNAME_LEN];
    char password[SOCKS5_MAX_PASSWORD_LEN];
    int enabled;
    uint64_t bytes_sent;
    uint64_t bytes_received;
} socks5_user_t;

/* Статистика SOCKS5 */
typedef struct {
    uint64_t total_connections;
    uint64_t active_connections;
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    uint64_t auth_failures;
    uint64_t failed_connections;
} socks5_stats_t;

/* Конфигурация SOCKS5 сервера */
typedef struct {
    int port;
    char bind_address[64];
    int auth_enabled;
    int allow_udp;
    char allowed_ips[256][64];
    int allowed_count;
    char denied_ips[256][64];
    int denied_count;
} socks5_config_t;

/* Контекст SOCKS5 сервера */
typedef struct {
    int server_fd;
    int running;
    pthread_t thread;
    socks5_config_t config;
    socks5_user_t users[SOCKS5_MAX_USERS];
    int user_count;
    socks5_stats_t stats;
    pthread_mutex_t lock;
} socks5_server_t;

/* Инициализация SOCKS5 сервера */
int socks5_server_init(socks5_server_t *server, socks5_config_t *config);

/* Запуск сервера */
int socks5_server_start(socks5_server_t *server);

/* Остановка сервера */
int socks5_server_stop(socks5_server_t *server);

/* Освобождение ресурсов */
void socks5_server_destroy(socks5_server_t *server);

/* Добавление пользователя */
int socks5_add_user(socks5_server_t *server, const char *username, const char *password);

/* Удаление пользователя */
int socks5_remove_user(socks5_server_t *server, const char *username);

/* Проверка аутентификации */
int socks5_authenticate(socks5_server_t *server, const char *username, const char *password);

/* Получение статистики */
void socks5_get_stats(socks5_server_t *server, socks5_stats_t *stats);

/* Экспорт статистики в JSON */
int socks5_export_json(socks5_server_t *server, char *buffer, size_t buffer_size);

/* SOCKS5 клиент */
typedef struct {
    int fd;
    char proxy_host[256];
    int proxy_port;
    char username[SOCKS5_MAX_USERNAME_LEN];
    char password[SOCKS5_MAX_PASSWORD_LEN];
    int auth_method;
} socks5_client_t;

/* Инициализация SOCKS5 клиента */
int socks5_client_init(socks5_client_t *client, const char *proxy_host, int proxy_port);

/* Установка аутентификации */
int socks5_client_set_auth(socks5_client_t *client, const char *username, const char *password);

/* Подключение через SOCKS5 */
int socks5_client_connect(socks5_client_t *client, const char *dest_host, int dest_port);

/* Закрытие подключения */
void socks5_client_close(socks5_client_t *client);

#endif /* __SOCKS5_H__ */
