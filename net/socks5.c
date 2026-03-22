/*
 * socks5.c - SOCKS5 клиент/сервер с аутентификацией
 * Поддержка SOCKS4/SOCKS5, методы аутентификации
 */

#include "socks5.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

/* Внутренние функции сервера */
static void *socks5_server_thread(void *arg);
static int socks5_handle_client(int client_fd, socks5_server_t *server);
static int socks5_process_request(int fd, socks5_server_t *server);

/* Инициализация SOCKS5 сервера */
int socks5_server_init(socks5_server_t *server, socks5_config_t *config) {
    if (!server) {
        return -1;
    }
    
    memset(server, 0, sizeof(socks5_server_t));
    
    if (config) {
        memcpy(&server->config, config, sizeof(socks5_config_t));
    } else {
        /* Конфигурация по умолчанию */
        server->config.port = SOCKS5_PORT;
        strcpy(server->config.bind_address, "0.0.0.0");
        server->config.auth_enabled = 0;
        server->config.allow_udp = 1;
    }
    
    if (pthread_mutex_init(&server->lock, NULL) != 0) {
        return -1;
    }
    
    server->server_fd = -1;
    server->running = 0;
    server->user_count = 0;
    
    return 0;
}

/* Запуск сервера */
int socks5_server_start(socks5_server_t *server) {
    if (!server || server->running) {
        return -1;
    }
    
    /* Создаём сокет */
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        return -1;
    }
    
    /* Устанавливаем опции */
    int opt = 1;
    setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Bind */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server->config.port);
    addr.sin_addr.s_addr = inet_addr(server->config.bind_address);
    
    if (bind(server->server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(server->server_fd);
        server->server_fd = -1;
        return -1;
    }
    
    /* Listen */
    if (listen(server->server_fd, 128) < 0) {
        close(server->server_fd);
        server->server_fd = -1;
        return -1;
    }
    
    server->running = 1;
    
    /* Запускаем поток */
    if (pthread_create(&server->thread, NULL, socks5_server_thread, server) != 0) {
        server->running = 0;
        close(server->server_fd);
        server->server_fd = -1;
        return -1;
    }
    
    return 0;
}

/* Поток сервера */
static void *socks5_server_thread(void *arg) {
    socks5_server_t *server = (socks5_server_t *)arg;
    
    while (server->running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server->server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (server->running) {
                continue;
            }
            break;
        }
        
        /* Обрабатываем клиента в том же потоке (для простоты) */
        socks5_handle_client(client_fd, server);
        close(client_fd);
    }
    
    return NULL;
}

/* Обработка клиента */
static int socks5_handle_client(int client_fd, socks5_server_t *server) {
    unsigned char buffer[512];
    
    /* Читаем приветствие */
    int n = read(client_fd, buffer, 2);
    if (n != 2) {
        return -1;
    }
    
    int version = buffer[0];
    int nmethods = buffer[1];
    
    if (version != 0x05) {
        /* SOCKS4 не поддерживаем */
        return -1;
    }
    
    /* Читаем методы аутентификации */
    n = read(client_fd, buffer, nmethods);
    if (n != nmethods) {
        return -1;
    }
    
    /* Выбираем метод */
    int method = SOCKS_AUTH_NO_ACCEPT;
    
    if (!server->config.auth_enabled) {
        method = SOCKS_AUTH_NONE;
    } else {
        /* Ищем метод USER/PASS */
        for (int i = 0; i < nmethods; i++) {
            if (buffer[i] == SOCKS_AUTH_USERPASS) {
                method = SOCKS_AUTH_USERPASS;
                break;
            }
        }
    }
    
    /* Отправляем выбранный метод */
    unsigned char reply[2] = {0x05, (unsigned char)method};
    write(client_fd, reply, 2);
    
    if (method == SOCKS_AUTH_NO_ACCEPT) {
        return -1;
    }
    
    /* Аутентификация */
    if (method == SOCKS_AUTH_USERPASS) {
        /* Читаем username/password */
        n = read(client_fd, buffer, 2);
        if (n != 2 || buffer[0] != 0x01) {
            server->stats.auth_failures++;
            return -1;
        }
        
        int ulen = buffer[1];
        n = read(client_fd, buffer, ulen);
        if (n != ulen) {
            server->stats.auth_failures++;
            return -1;
        }
        
        char username[256];
        strncpy(username, (char *)buffer, ulen);
        username[ulen] = '\0';
        
        /* Читаем длину пароля */
        n = read(client_fd, buffer, 1);
        if (n != 1) {
            server->stats.auth_failures++;
            return -1;
        }
        
        int plen = buffer[0];
        n = read(client_fd, buffer, plen);
        if (n != plen) {
            server->stats.auth_failures++;
            return -1;
        }
        
        char password[256];
        strncpy(password, (char *)buffer, plen);
        password[plen] = '\0';
        
        /* Проверяем */
        if (!socks5_authenticate(server, username, password)) {
            /* Отправляем failure */
            unsigned char auth_reply[2] = {0x01, 0x01};
            write(client_fd, auth_reply, 2);
            server->stats.auth_failures++;
            return -1;
        }
        
        /* Отправляем success */
        unsigned char auth_reply[2] = {0x01, 0x00};
        write(client_fd, auth_reply, 2);
    }
    
    /* Обрабатываем запрос */
    return socks5_process_request(client_fd, server);
}

/* Обработка запроса клиента */
static int socks5_process_request(int fd, socks5_server_t *server) {
    unsigned char buffer[512];
    
    /* Читаем запрос */
    int n = read(fd, buffer, 4);
    if (n != 4) {
        server->stats.failed_connections++;
        return -1;
    }
    
    int version = buffer[0];
    int cmd = buffer[1];
    int atype = buffer[3];
    
    if (version != 0x05) {
        server->stats.failed_connections++;
        return -1;
    }
    
    /* Поддерживаем только CONNECT */
    if (cmd != SOCKS_CMD_CONNECT) {
        unsigned char reply[] = {0x05, SOCKS_REP_FAILURE, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        write(fd, reply, 10);
        server->stats.failed_connections++;
        return -1;
    }
    
    /* Читаем адрес */
    char dest_host[256];
    int dest_port = 0;
    
    if (atype == SOCKS_ATYP_IPV4) {
        n = read(fd, buffer, 4);
        if (n != 4) {
            return -1;
        }
        snprintf(dest_host, sizeof(dest_host), "%d.%d.%d.%d", 
                 buffer[0], buffer[1], buffer[2], buffer[3]);
        
        n = read(fd, buffer, 2);
        if (n != 2) {
            return -1;
        }
        dest_port = (buffer[0] << 8) | buffer[1];
    } else if (atype == SOCKS_ATYP_DOMAIN) {
        n = read(fd, buffer, 1);
        if (n != 1) {
            return -1;
        }
        int dlen = buffer[0];
        
        n = read(fd, buffer, dlen);
        if (n != dlen) {
            return -1;
        }
        strncpy(dest_host, (char *)buffer, dlen);
        dest_host[dlen] = '\0';
        
        n = read(fd, buffer, 2);
        if (n != 2) {
            return -1;
        }
        dest_port = (buffer[0] << 8) | buffer[1];
    } else if (atype == SOCKS_ATYP_IPV6) {
        /* IPv6 не поддерживаем */
        unsigned char reply[] = {0x05, SOCKS_REP_FAILURE, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        write(fd, reply, 10);
        server->stats.failed_connections++;
        return -1;
    } else {
        server->stats.failed_connections++;
        return -1;
    }
    
    /* Подключаемся к назначению */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", dest_port);
    
    if (getaddrinfo(dest_host, port_str, &hints, &res) != 0) {
        unsigned char reply[] = {0x05, SOCKS_REP_HOST_UNREACH, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        write(fd, reply, 10);
        server->stats.failed_connections++;
        return -1;
    }
    
    int dest_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (dest_fd < 0) {
        freeaddrinfo(res);
        unsigned char reply[] = {0x05, SOCKS_REP_FAILURE, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        write(fd, reply, 10);
        server->stats.failed_connections++;
        return -1;
    }
    
    if (connect(dest_fd, res->ai_addr, res->ai_addrlen) < 0) {
        close(dest_fd);
        freeaddrinfo(res);
        unsigned char reply[] = {0x05, SOCKS_REP_CONN_REFUSED, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        write(fd, reply, 10);
        server->stats.failed_connections++;
        return -1;
    }
    
    freeaddrinfo(res);
    
    /* Отправляем успех */
    unsigned char reply[] = {0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    write(fd, reply, 10);
    
    server->stats.total_connections++;
    server->stats.active_connections++;
    
    /* Передаём данные (простой proxy) */
    fd_set fds;
    char data[4096];
    
    while (1) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        FD_SET(dest_fd, &fds);
        
        struct timeval tv = {1, 0};
        int ret = select((fd > dest_fd ? fd : dest_fd) + 1, &fds, NULL, NULL, &tv);
        
        if (ret < 0) {
            break;
        }
        
        if (ret == 0) {
            if (!server->running) {
                break;
            }
            continue;
        }
        
        if (FD_ISSET(fd, &fds)) {
            n = read(fd, data, sizeof(data));
            if (n <= 0) {
                break;
            }
            
            int sent = write(dest_fd, data, n);
            if (sent > 0) {
                pthread_mutex_lock(&server->lock);
                server->stats.total_bytes_received += sent;
                pthread_mutex_unlock(&server->lock);
            }
        }
        
        if (FD_ISSET(dest_fd, &fds)) {
            n = read(dest_fd, data, sizeof(data));
            if (n <= 0) {
                break;
            }
            
            int sent = write(fd, data, n);
            if (sent > 0) {
                pthread_mutex_lock(&server->lock);
                server->stats.total_bytes_sent += sent;
                pthread_mutex_unlock(&server->lock);
            }
        }
    }
    
    close(dest_fd);
    
    pthread_mutex_lock(&server->lock);
    if (server->stats.active_connections > 0) {
        server->stats.active_connections--;
    }
    pthread_mutex_unlock(&server->lock);
    
    return 0;
}

/* Остановка сервера */
int socks5_server_stop(socks5_server_t *server) {
    if (!server || !server->running) {
        return -1;
    }
    
    server->running = 0;
    
    if (server->server_fd >= 0) {
        shutdown(server->server_fd, SHUT_RDWR);
        close(server->server_fd);
        server->server_fd = -1;
    }
    
    pthread_join(server->thread, NULL);
    
    return 0;
}

/* Освобождение ресурсов */
void socks5_server_destroy(socks5_server_t *server) {
    if (!server) {
        return;
    }
    
    if (server->running) {
        socks5_server_stop(server);
    }
    
    pthread_mutex_destroy(&server->lock);
}

/* Добавление пользователя */
int socks5_add_user(socks5_server_t *server, const char *username, const char *password) {
    if (!server || !username || !password) {
        return -1;
    }
    
    pthread_mutex_lock(&server->lock);
    
    if (server->user_count >= SOCKS5_MAX_USERS) {
        pthread_mutex_unlock(&server->lock);
        return -1;
    }
    
    /* Проверяем, нет ли уже такого */
    for (int i = 0; i < server->user_count; i++) {
        if (strcmp(server->users[i].username, username) == 0) {
            pthread_mutex_unlock(&server->lock);
            return -1;
        }
    }
    
    /* Добавляем */
    socks5_user_t *user = &server->users[server->user_count];
    strncpy(user->username, username, SOCKS5_MAX_USERNAME_LEN - 1);
    strncpy(user->password, password, SOCKS5_MAX_PASSWORD_LEN - 1);
    user->enabled = 1;
    user->bytes_sent = 0;
    user->bytes_received = 0;
    
    server->user_count++;
    
    pthread_mutex_unlock(&server->lock);
    
    return 0;
}

/* Удаление пользователя */
int socks5_remove_user(socks5_server_t *server, const char *username) {
    if (!server || !username) {
        return -1;
    }
    
    pthread_mutex_lock(&server->lock);
    
    for (int i = 0; i < server->user_count; i++) {
        if (strcmp(server->users[i].username, username) == 0) {
            /* Сдвигаем массив */
            for (int j = i; j < server->user_count - 1; j++) {
                server->users[j] = server->users[j + 1];
            }
            server->user_count--;
            pthread_mutex_unlock(&server->lock);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&server->lock);
    return -1;
}

/* Проверка аутентификации */
int socks5_authenticate(socks5_server_t *server, const char *username, const char *password) {
    if (!server || !username || !password) {
        return 0;
    }
    
    pthread_mutex_lock(&server->lock);
    
    for (int i = 0; i < server->user_count; i++) {
        socks5_user_t *user = &server->users[i];
        if (!user->enabled) {
            continue;
        }
        
        if (strcmp(user->username, username) == 0 && 
            strcmp(user->password, password) == 0) {
            pthread_mutex_unlock(&server->lock);
            return 1;
        }
    }
    
    pthread_mutex_unlock(&server->lock);
    return 0;
}

/* Получение статистики */
void socks5_get_stats(socks5_server_t *server, socks5_stats_t *stats) {
    if (!server || !stats) {
        return;
    }
    
    pthread_mutex_lock(&server->lock);
    memcpy(stats, &server->stats, sizeof(socks5_stats_t));
    pthread_mutex_unlock(&server->lock);
}

/* Экспорт статистики в JSON */
int socks5_export_json(socks5_server_t *server, char *buffer, size_t buffer_size) {
    if (!server || !buffer || buffer_size < 512) {
        return -1;
    }
    
    pthread_mutex_lock(&server->lock);
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "{\n");
    offset += snprintf(buffer + offset, buffer_size - offset, "  \"stats\": {\n");
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"total_connections\": %llu,\n", 
                       (unsigned long long)server->stats.total_connections);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"active_connections\": %llu,\n", 
                       (unsigned long long)server->stats.active_connections);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"total_bytes_sent\": %llu,\n", 
                       (unsigned long long)server->stats.total_bytes_sent);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"total_bytes_received\": %llu,\n", 
                       (unsigned long long)server->stats.total_bytes_received);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"auth_failures\": %llu,\n", 
                       (unsigned long long)server->stats.auth_failures);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"failed_connections\": %llu\n", 
                       (unsigned long long)server->stats.failed_connections);
    offset += snprintf(buffer + offset, buffer_size - offset, "  },\n");
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "  \"users\": %d,\n", server->user_count);
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "  \"auth_enabled\": %s\n", 
                       server->config.auth_enabled ? "true" : "false");
    offset += snprintf(buffer + offset, buffer_size - offset, "}\n");
    
    pthread_mutex_unlock(&server->lock);
    
    return offset;
}

/* SOCKS5 клиент */

/* Инициализация клиента */
int socks5_client_init(socks5_client_t *client, const char *proxy_host, int proxy_port) {
    if (!client || !proxy_host) {
        return -1;
    }
    
    memset(client, 0, sizeof(socks5_client_t));
    strncpy(client->proxy_host, proxy_host, sizeof(client->proxy_host) - 1);
    client->proxy_port = proxy_port;
    client->fd = -1;
    client->auth_method = SOCKS_AUTH_NONE;
    
    return 0;
}

/* Установка аутентификации */
int socks5_client_set_auth(socks5_client_t *client, const char *username, const char *password) {
    if (!client || !username || !password) {
        return -1;
    }
    
    strncpy(client->username, username, sizeof(client->username) - 1);
    strncpy(client->password, password, sizeof(client->password) - 1);
    client->auth_method = SOCKS_AUTH_USERPASS;
    
    return 0;
}

/* Подключение через SOCKS5 */
int socks5_client_connect(socks5_client_t *client, const char *dest_host, int dest_port) {
    if (!client || !dest_host) {
        return -1;
    }
    
    /* Подключаемся к прокси */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", client->proxy_port);
    
    if (getaddrinfo(client->proxy_host, port_str, &hints, &res) != 0) {
        return -1;
    }
    
    client->fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (client->fd < 0) {
        freeaddrinfo(res);
        return -1;
    }
    
    if (connect(client->fd, res->ai_addr, res->ai_addrlen) < 0) {
        close(client->fd);
        freeaddrinfo(res);
        client->fd = -1;
        return -1;
    }
    
    freeaddrinfo(res);
    
    /* SOCKS5 приветствие */
    unsigned char hello[3];
    hello[0] = 0x05;
    hello[1] = client->auth_method == SOCKS_AUTH_USERPASS ? 0x01 : 0x00;
    hello[2] = client->auth_method == SOCKS_AUTH_USERPASS ? SOCKS_AUTH_USERPASS : SOCKS_AUTH_NONE;
    
    write(client->fd, hello, 3);
    
    /* Читаем ответ */
    unsigned char reply[2];
    int n = read(client->fd, reply, 2);
    if (n != 2 || reply[0] != 0x05) {
        close(client->fd);
        client->fd = -1;
        return -1;
    }
    
    /* Аутентификация если нужна */
    if (reply[1] == SOCKS_AUTH_USERPASS) {
        /* Отправляем username/password */
        int ulen = strlen(client->username);
        int plen = strlen(client->password);
        
        unsigned char auth[515];
        int pos = 0;
        auth[pos++] = 0x01;
        auth[pos++] = ulen;
        memcpy(auth + pos, client->username, ulen);
        pos += ulen;
        auth[pos++] = plen;
        memcpy(auth + pos, client->password, plen);
        pos += plen;
        
        write(client->fd, auth, pos);
        
        /* Читаем ответ */
        n = read(client->fd, reply, 2);
        if (n != 2 || reply[1] != 0x00) {
            close(client->fd);
            client->fd = -1;
            return -1;
        }
    } else if (reply[1] != SOCKS_AUTH_NONE) {
        close(client->fd);
        client->fd = -1;
        return -1;
    }
    
    /* Отправляем запрос на подключение */
    unsigned char request[512];
    int req_len = 0;
    
    request[req_len++] = 0x05;
    request[req_len++] = SOCKS_CMD_CONNECT;
    request[req_len++] = 0x00;
    
    /* Определяем тип адреса */
    struct in_addr addr;
    if (inet_pton(AF_INET, dest_host, &addr) == 1) {
        /* IPv4 */
        request[req_len++] = SOCKS_ATYP_IPV4;
        memcpy(request + req_len, &addr, 4);
        req_len += 4;
    } else {
        /* Domain name */
        int dlen = strlen(dest_host);
        request[req_len++] = SOCKS_ATYP_DOMAIN;
        request[req_len++] = dlen;
        memcpy(request + req_len, dest_host, dlen);
        req_len += dlen;
    }
    
    /* Порт */
    request[req_len++] = (dest_port >> 8) & 0xFF;
    request[req_len++] = dest_port & 0xFF;
    
    write(client->fd, request, req_len);
    
    /* Читаем ответ */
    n = read(client->fd, reply, 10);
    if (n < 10 || reply[1] != SOCKS_REP_SUCCESS) {
        close(client->fd);
        client->fd = -1;
        return -1;
    }
    
    return client->fd;
}

/* Закрытие подключения */
void socks5_client_close(socks5_client_t *client) {
    if (!client) {
        return;
    }
    
    if (client->fd >= 0) {
        close(client->fd);
        client->fd = -1;
    }
}
