/*
    MTProxy Cluster Manager
    Реализация системы управления кластером
*/

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <pthread.h>
#endif

#include "system/cluster/cluster-manager.h"
#include "common/utils.h"

// ============================================================================
// Глобальные переменные
// ============================================================================

static struct {
    bool initialized;
    bool running;
    char cluster_name[CLUSTER_MAX_NAME_LEN];
    char local_node_name[CLUSTER_MAX_NAME_LEN];

    cluster_node_config_t nodes[CLUSTER_MAX_NODES];
    cluster_node_info_t node_status[CLUSTER_MAX_NODES];
    int node_count;

    cluster_role_t local_role;
    char leader_name[CLUSTER_MAX_NAME_LEN];
    int64_t leader_last_seen;

    int listen_port;
    int64_t start_time;
    int64_t last_heartbeat;

    cluster_stats_t stats;

    // Callback функции
    cluster_node_change_callback_t node_change_callback;
    cluster_leader_change_callback_t leader_change_callback;
    cluster_message_handler_t message_handlers[16];

    // Потоки
#ifdef _WIN32
    HANDLE heartbeat_thread;
    HANDLE listener_thread;
#else
    pthread_t heartbeat_thread;
    pthread_t listener_thread;
#endif

    // Синхронизация
    bool auto_failover_enabled;
    int election_timeout_ms;
    int heartbeat_interval_ms;
} g_cluster = {0};

// ============================================================================
// Внутренние функции
// ============================================================================

static int64_t get_current_time_ms(void) {
#ifdef _WIN32
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return (int64_t)((uli.QuadPart - 116444736000000000ULL) / 10000);
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

static cluster_node_config_t* find_node_config(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < g_cluster.node_count; i++) {
        if (strcmp(g_cluster.nodes[i].name, name) == 0) {
            return &g_cluster.nodes[i];
        }
    }
    return NULL;
}

static cluster_node_info_t* find_node_status(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < g_cluster.node_count; i++) {
        if (strcmp(g_cluster.nodes[i].name, name) == 0) {
            return &g_cluster.node_status[i];
        }
    }
    return NULL;
}

static int send_tcp_message(const char *host, int port, const cluster_message_t *msg) {
    if (!host || !msg) return -1;
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        struct hostent *he = gethostbyname(host);
        if (!he) return -1;
        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    // Таймаут 3 секунды
#ifdef _WIN32
    DWORD timeout = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }
    
    // Отправка сообщения
    send(sock, (const char*)msg, sizeof(cluster_message_t), 0);
    
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    
    return 0;
}

static void update_node_status(const char *name, cluster_node_state_t new_status) {
    cluster_node_info_t *status = find_node_status(name);
    if (!status) return;

    cluster_node_state_t old_status = status->status;
    if (old_status == new_status) return;

    status->status = new_status;
    status->last_seen = get_current_time_ms();

    kprintf("[CLUSTER] Node %s status changed: %s -> %s\n",
            name,
            cluster_node_status_to_string(old_status),
            cluster_node_status_to_string(new_status));

    // Callback
    if (g_cluster.node_change_callback) {
        g_cluster.node_change_callback(name, old_status, new_status);
    }
}

static void perform_heartbeat(void) {
    if (!g_cluster.running || g_cluster.node_count == 0) {
        return;
    }
    
    int64_t now = get_current_time_ms();
    
    // Отправка heartbeat всем узлам
    cluster_message_t msg = {0};
    msg.type = CLUSTER_MSG_HEARTBEAT;
    snprintf(msg.source, sizeof(msg.source), "%s", g_cluster.local_node_name);
    msg.timestamp = now;
    msg.message_id = g_cluster.stats.total_messages_sent + 1;
    
    for (int i = 0; i < g_cluster.node_count; i++) {
        if (!g_cluster.nodes[i].enabled) continue;
        if (strcmp(g_cluster.nodes[i].name, g_cluster.local_node_name) == 0) continue;
        
        snprintf(msg.destination, sizeof(msg.destination), "%s", g_cluster.nodes[i].name);
        
        int result = send_tcp_message(g_cluster.nodes[i].host, 
                                       g_cluster.nodes[i].management_port, &msg);
        
        if (result == 0) {
            g_cluster.stats.total_messages_sent++;
            // Обновление статуса узла
            cluster_node_status_t *status = &g_cluster.node_status[i];
            if (status->status == CLUSTER_NODE_OFFLINE || 
                status->status == CLUSTER_NODE_FAILED) {
                update_node_status(g_cluster.nodes[i].name, CLUSTER_NODE_ONLINE);
            }
            status->last_seen = now;
        } else {
            // Узел не отвечает
            if (status->status == CLUSTER_NODE_ONLINE) {
                // Первый сбой
                update_node_status(g_cluster.nodes[i].name, CLUSTER_NODE_DEGRADED);
            } else if (status->status == CLUSTER_NODE_DEGRADED) {
                // Второй сбой - помечаем как failed
                update_node_status(g_cluster.nodes[i].name, CLUSTER_NODE_FAILED);
                
                // Auto failover
                if (g_cluster.auto_failover_enabled) {
                    cluster_handle_node_failure(g_cluster.nodes[i].name);
                }
            }
        }
    }
    
    g_cluster.last_heartbeat = now;
}

static void check_leader_alive(void) {
    if (!g_cluster.running) return;
    if (g_cluster.local_role == CLUSTER_ROLE_LEADER) return;
    if (g_cluster.leader_name[0] == '\0') return;
    
    int64_t now = get_current_time_ms();
    int64_t timeout = g_cluster.election_timeout_ms * 3;  // 3x timeout
    
    if ((now - g_cluster.leader_last_seen) > timeout) {
        kprintf("[CLUSTER] Leader %s is dead, starting election\n", g_cluster.leader_name);
        
        // Лидер не отвечает
        g_cluster.leader_name[0] = '\0';
        
        if (g_cluster.auto_failover_enabled) {
            cluster_start_election();
        }
    }
}

#ifdef _WIN32
static DWORD WINAPI heartbeat_thread_func(LPVOID arg)
#else
static void* heartbeat_thread_func(void *arg)
#endif
{
    kprintf("[CLUSTER] Heartbeat thread started (interval: %d ms)\n", 
            g_cluster.heartbeat_interval_ms);
    
    while (g_cluster.running) {
#ifdef _WIN32
        Sleep(g_cluster.heartbeat_interval_ms);
#else
        struct timespec ts;
        ts.tv_sec = g_cluster.heartbeat_interval_ms / 1000;
        ts.tv_nsec = (g_cluster.heartbeat_interval_ms % 1000) * 1000000;
        nanosleep(&ts, NULL);
#endif
        
        if (g_cluster.running) {
            perform_heartbeat();
            check_leader_alive();
        }
    }
    
    kprintf("[CLUSTER] Heartbeat thread stopped\n");
    return 0;
}

#ifdef _WIN32
static DWORD WINAPI listener_thread_func(LPVOID arg)
#else
static void* listener_thread_func(void *arg)
#endif
{
    kprintf("[CLUSTER] Listener thread started (port: %d)\n", g_cluster.listen_port);
    
    // Создание серверного сокета
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(g_cluster.listen_port);
    
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        kprintf("[CLUSTER] Failed to create listener socket\n");
        return 0;
    }
    
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        kprintf("[CLUSTER] Failed to bind to port %d\n", g_cluster.listen_port);
#ifdef _WIN32
        closesocket(server_sock);
#else
        close(server_sock);
#endif
        return 0;
    }
    
    listen(server_sock, 10);
    
    while (g_cluster.running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) continue;
        
        // Получение сообщения
        cluster_message_t msg = {0};
        int received = recv(client_sock, (char*)&msg, sizeof(msg), 0);
        
        if (received > 0) {
            // Обработка сообщения
            cluster_message_type_t type = msg.type;
            
            if (type < 16 && g_cluster.message_handlers[type]) {
                g_cluster.message_handlers[type](&msg);
            } else {
                // Обработка по умолчанию
                switch (type) {
                    case CLUSTER_MSG_HEARTBEAT:
                        // Ответ на heartbeat
                        g_cluster.stats.total_messages_received++;
                        break;
                        
                    case CLUSTER_MSG_ELECTION_REQUEST:
                        // Обработка запроса на выборы
                        g_cluster.stats.total_messages_received++;
                        break;
                        
                    case CLUSTER_MSG_CONFIG_SYNC:
                        // Синхронизация конфигурации
                        g_cluster.stats.total_messages_received++;
                        break;
                        
                    default:
                        break;
                }
            }
        }
        
#ifdef _WIN32
        closesocket(client_sock);
#else
        close(client_sock);
#endif
    }
    
#ifdef _WIN32
    closesocket(server_sock);
#else
    close(server_sock);
#endif
    
    kprintf("[CLUSTER] Listener thread stopped\n");
    return 0;
}

// ============================================================================
// Инициализация и очистка
// ============================================================================

int cluster_init(const char *cluster_name, const char *local_node_name) {
    if (!cluster_name || !local_node_name) {
        return -1;
    }
    
    if (g_cluster.initialized) {
        kprintf("[CLUSTER] Already initialized\n");
        return 0;
    }
    
    memset(&g_cluster, 0, sizeof(g_cluster));
    
    snprintf(g_cluster.cluster_name, sizeof(g_cluster.cluster_name), "%s", cluster_name);
    snprintf(g_cluster.local_node_name, sizeof(g_cluster.local_node_name), "%s", local_node_name);
    
    // Добавляем локальный узел
    cluster_node_config_t *local = &g_cluster.nodes[0];
    snprintf(local->name, sizeof(local->name), "%s", local_node_name);
    local->role = CLUSTER_ROLE_STANDALONE;
    local->priority = 1;
    local->enabled = true;
    
    cluster_node_status_t *status = &g_cluster.node_status[0];
    snprintf(status->name, sizeof(status->name), "%s", local_node_name);
    status->status = CLUSTER_NODE_STARTING;
    status->role = CLUSTER_ROLE_STANDALONE;
    
    g_cluster.node_count = 1;
    g_cluster.local_role = CLUSTER_ROLE_STANDALONE;
    
    g_cluster.start_time = get_current_time_ms();
    g_cluster.election_timeout_ms = 5000;
    g_cluster.heartbeat_interval_ms = 2000;
    g_cluster.auto_failover_enabled = true;
    
    g_cluster.initialized = true;
    
    kprintf("[CLUSTER] Initialized: cluster=%s, node=%s\n", cluster_name, local_node_name);
    return 0;
}

void cluster_cleanup(void) {
    if (!g_cluster.initialized) {
        return;
    }
    
    cluster_stop();
    
    memset(&g_cluster, 0, sizeof(g_cluster));
    kprintf("[CLUSTER] Cleaned up\n");
}

bool cluster_is_initialized(void) {
    return g_cluster.initialized;
}

// ============================================================================
// Управление узлами
// ============================================================================

int cluster_add_node(const char *name, const char *host, int port, int management_port) {
    if (!g_cluster.initialized || !name || !host) {
        return -1;
    }
    
    if (g_cluster.node_count >= CLUSTER_MAX_NODES) {
        kprintf("[CLUSTER] Error: Maximum nodes reached\n");
        return -1;
    }
    
    if (find_node_config(name) != NULL) {
        kprintf("[CLUSTER] Error: Node already exists: %s\n", name);
        return -1;
    }
    
    cluster_node_config_t *node = &g_cluster.nodes[g_cluster.node_count];
    memset(node, 0, sizeof(cluster_node_config_t));
    
    snprintf(node->name, sizeof(node->name), "%s", name);
    snprintf(node->host, sizeof(node->host), "%s", host);
    node->port = port;
    node->management_port = management_port > 0 ? management_port : port;
    node->role = CLUSTER_ROLE_FOLLOWER;
    node->priority = 1;
    node->enabled = true;
    
    cluster_node_status_t *status = &g_cluster.node_status[g_cluster.node_count];
    memset(status, 0, sizeof(cluster_node_status_t));
    snprintf(status->name, sizeof(status->name), "%s", name);
    status->status = CLUSTER_NODE_OFFLINE;
    status->role = CLUSTER_ROLE_FOLLOWER;
    
    g_cluster.node_count++;
    g_cluster.stats.total_nodes++;
    
    kprintf("[CLUSTER] Node added: %s at %s:%d\n", name, host, port);
    return 0;
}

int cluster_remove_node(const char *name) {
    if (!g_cluster.initialized || !name) {
        return -1;
    }
    
    int idx = -1;
    for (int i = 0; i < g_cluster.node_count; i++) {
        if (strcmp(g_cluster.nodes[i].name, name) == 0) {
            idx = i;
            break;
        }
    }
    
    if (idx < 0) return -1;
    
    // Сдвиг массивов
    for (int i = idx; i < g_cluster.node_count - 1; i++) {
        g_cluster.nodes[i] = g_cluster.nodes[i + 1];
        g_cluster.node_status[i] = g_cluster.node_status[i + 1];
    }
    
    g_cluster.node_count--;
    g_cluster.stats.total_nodes--;
    
    kprintf("[CLUSTER] Node removed: %s\n", name);
    return 0;
}

const cluster_node_config_t* cluster_get_node_config(const char *name) {
    return find_node_config(name);
}

const cluster_node_status_t* cluster_get_node_status(const char *name) {
    return find_node_status(name);
}

int cluster_get_all_nodes(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return -1;
    
    int offset = 0;
    for (int i = 0; i < g_cluster.node_count; i++) {
        if (i > 0) {
            offset += snprintf(buffer + offset, buffer_size - offset, ", ");
        }
        offset += snprintf(buffer + offset, buffer_size - offset, "%s", 
                           g_cluster.nodes[i].name);
    }
    
    return g_cluster.node_count;
}

// ============================================================================
// Запуск и остановка
// ============================================================================

int cluster_start(int listen_port) {
    if (!g_cluster.initialized) {
        return -1;
    }
    
    if (g_cluster.running) {
        kprintf("[CLUSTER] Already running\n");
        return 0;
    }
    
    g_cluster.listen_port = listen_port > 0 ? listen_port : 9000;
    g_cluster.running = true;
    
    // Запуск потока heartbeat
#ifdef _WIN32
    g_cluster.heartbeat_thread = CreateThread(NULL, 0, heartbeat_thread_func, NULL, 0, NULL);
    g_cluster.listener_thread = CreateThread(NULL, 0, listener_thread_func, NULL, 0, NULL);
    
    if (!g_cluster.heartbeat_thread || !g_cluster.listener_thread) {
        g_cluster.running = false;
        return -1;
    }
#else
    if (pthread_create(&g_cluster.heartbeat_thread, NULL, heartbeat_thread_func, NULL) != 0) {
        g_cluster.running = false;
        return -1;
    }
    
    if (pthread_create(&g_cluster.listener_thread, NULL, listener_thread_func, NULL) != 0) {
        g_cluster.running = false;
        return -1;
    }
#endif
    
    kprintf("[CLUSTER] Started on port %d\n", listen_port);
    return 0;
}

void cluster_stop(void) {
    if (!g_cluster.running) {
        return;
    }
    
    g_cluster.running = false;
    
#ifdef _WIN32
    if (g_cluster.heartbeat_thread) {
        WaitForSingleObject(g_cluster.heartbeat_thread, INFINITE);
        CloseHandle(g_cluster.heartbeat_thread);
    }
    if (g_cluster.listener_thread) {
        WaitForSingleObject(g_cluster.listener_thread, INFINITE);
        CloseHandle(g_cluster.listener_thread);
    }
#else
    pthread_join(g_cluster.heartbeat_thread, NULL);
    pthread_join(g_cluster.listener_thread, NULL);
#endif
    
    kprintf("[CLUSTER] Stopped\n");
}

bool cluster_is_running(void) {
    return g_cluster.running;
}

// ============================================================================
// Лидерство и выборы
// ============================================================================

const char* cluster_get_leader(void) {
    if (g_cluster.leader_name[0] == '\0') {
        return NULL;
    }
    return g_cluster.leader_name;
}

cluster_role_t cluster_get_local_role(void) {
    return g_cluster.local_role;
}

int cluster_start_election(void) {
    if (!g_cluster.running) {
        return -1;
    }
    
    kprintf("[CLUSTER] Starting leader election\n");
    
    g_cluster.local_role = CLUSTER_ROLE_CANDIDATE;
    
    // Отправка запроса на выборы всем узлам
    cluster_message_t msg = {0};
    msg.type = CLUSTER_MSG_ELECTION_REQUEST;
    snprintf(msg.source, sizeof(msg.source), "%s", g_cluster.local_node_name);
    msg.timestamp = get_current_time_ms();
    
    int votes = 1;  // Голос за себя
    int total_responses = 0;
    
    for (int i = 0; i < g_cluster.node_count; i++) {
        if (strcmp(g_cluster.nodes[i].name, g_cluster.local_node_name) == 0) continue;
        if (!g_cluster.nodes[i].enabled) continue;
        if (g_cluster.node_status[i].status != CLUSTER_NODE_ONLINE) continue;
        
        snprintf(msg.destination, sizeof(msg.destination), "%s", g_cluster.nodes[i].name);
        send_tcp_message(g_cluster.nodes[i].host, g_cluster.nodes[i].management_port, &msg);
        total_responses++;
    }
    
    // Упрощённая логика выборов
    // В реальной реализации нужно ждать ответы и считать голоса
    if (total_responses == 0 || votes > total_responses / 2) {
        cluster_become_leader();
    }
    
    return 0;
}

int cluster_become_leader(void) {
    if (!g_cluster.initialized) {
        return -1;
    }
    
    char old_leader[CLUSTER_MAX_NAME_LEN];
    snprintf(old_leader, sizeof(old_leader), "%s", g_cluster.leader_name);
    
    g_cluster.local_role = CLUSTER_ROLE_LEADER;
    snprintf(g_cluster.leader_name, sizeof(g_cluster.leader_name), "%s", g_cluster.local_node_name);
    g_cluster.leader_last_seen = get_current_time_ms();
    
    kprintf("[CLUSTER] %s became leader\n", g_cluster.local_node_name);
    
    // Уведомление всех узлов
    cluster_message_t msg = {0};
    msg.type = CLUSTER_MSG_ELECTION_RESPONSE;
    snprintf(msg.source, sizeof(msg.source), "%s", g_cluster.local_node_name);
    snprintf(msg.payload, sizeof(msg.payload), "leader=%s", g_cluster.local_node_name);
    
    cluster_broadcast_message(CLUSTER_MSG_ELECTION_RESPONSE, msg.payload, 
                              (int)strlen(msg.payload));
    
    // Callback
    if (g_cluster.leader_change_callback) {
        g_cluster.leader_change_callback(old_leader, g_cluster.local_node_name);
    }
    
    return 0;
}

int cluster_step_down(void) {
    if (g_cluster.local_role != CLUSTER_ROLE_LEADER) {
        return -1;
    }
    
    kprintf("[CLUSTER] %s stepping down as leader\n", g_cluster.local_node_name);
    
    g_cluster.local_role = CLUSTER_ROLE_FOLLOWER;
    g_cluster.leader_name[0] = '\0';
    
    // Уведомление
    cluster_broadcast_message(CLUSTER_MSG_ELECTION_REQUEST, "", 0);
    
    return 0;
}

// ============================================================================
// Синхронизация конфигурации
// ============================================================================

int cluster_sync_config(const char *config_data) {
    if (!g_cluster.running || !config_data) {
        return -1;
    }
    
    kprintf("[CLUSTER] Syncing config (%zu bytes)\n", strlen(config_data));
    
    // Отправка конфигурации всем узлам
    cluster_message_t msg = {0};
    msg.type = CLUSTER_MSG_CONFIG_SYNC;
    snprintf(msg.source, sizeof(msg.source), "%s", g_cluster.local_node_name);
    snprintf(msg.payload, sizeof(msg.payload), "%s", config_data);
    msg.payload_size = (int)strlen(config_data);
    
    int sent = 0;
    for (int i = 0; i < g_cluster.node_count; i++) {
        if (strcmp(g_cluster.nodes[i].name, g_cluster.local_node_name) == 0) continue;
        if (!g_cluster.nodes[i].enabled) continue;
        
        snprintf(msg.destination, sizeof(msg.destination), "%s", g_cluster.nodes[i].name);
        if (send_tcp_message(g_cluster.nodes[i].host, g_cluster.nodes[i].management_port, &msg) == 0) {
            sent++;
        }
    }
    
    g_cluster.stats.config_sync_count++;
    g_cluster.stats.total_messages_sent += sent;
    
    kprintf("[CLUSTER] Config synced to %d nodes\n", sent);
    return 0;
}

int cluster_get_config(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return -1;
    }
    
    // Возвращаем заглушку
    snprintf(buffer, buffer_size, "{\"cluster\":\"%s\",\"node\":\"%s\"}", 
             g_cluster.cluster_name, g_cluster.local_node_name);
    
    return 0;
}

int cluster_request_config(void) {
    if (!g_cluster.running) {
        return -1;
    }
    
    // Запрос конфигурации у лидера
    if (g_cluster.leader_name[0] == '\0') {
        return -1;
    }
    
    cluster_node_config_t *leader = find_node_config(g_cluster.leader_name);
    if (!leader) {
        return -1;
    }
    
    cluster_message_t msg = {0};
    msg.type = CLUSTER_MSG_CONFIG_SYNC;
    snprintf(msg.source, sizeof(msg.source), "%s", g_cluster.local_node_name);
    snprintf(msg.destination, sizeof(msg.destination), "%s", g_cluster.leader_name);
    snprintf(msg.payload, sizeof(msg.payload), "request_config=true");
    
    send_tcp_message(leader->host, leader->management_port, &msg);
    
    return 0;
}

// ============================================================================
// Балансировка нагрузки
// ============================================================================

const char* cluster_get_least_loaded_node(void) {
    if (g_cluster.node_count == 0) {
        return NULL;
    }
    
    const char *least_loaded = NULL;
    double min_load = 101.0;
    
    for (int i = 0; i < g_cluster.node_count; i++) {
        if (!g_cluster.nodes[i].enabled) continue;
        if (g_cluster.node_status[i].status != CLUSTER_NODE_ONLINE) continue;
        
        cluster_node_status_t *status = &g_cluster.node_status[i];
        if (status->max_connections == 0) continue;
        
        double load = (double)status->active_connections / status->max_connections * 100.0;
        
        if (load < min_load) {
            min_load = load;
            least_loaded = g_cluster.nodes[i].name;
        }
    }
    
    return least_loaded;
}

int cluster_rebalance_connections(const char *source_node, int connections_count) {
    if (!source_node || connections_count <= 0) {
        return -1;
    }
    
    kprintf("[CLUSTER] Rebalancing %d connections from %s\n", connections_count, source_node);
    
    // Поиск наименее загруженных узлов
    const char *target = cluster_get_least_loaded_node();
    if (!target || strcmp(target, source_node) == 0) {
        return -1;
    }
    
    // Отправка команды на перераспределение
    cluster_message_t msg = {0};
    msg.type = CLUSTER_MSG_LOAD_BALANCE;
    snprintf(msg.source, sizeof(msg.source), "%s", g_cluster.local_node_name);
    snprintf(msg.payload, sizeof(msg.payload), 
             "source=%s,target=%s,count=%d", source_node, target, connections_count);
    
    cluster_broadcast_message(CLUSTER_MSG_LOAD_BALANCE, msg.payload, (int)strlen(msg.payload));
    
    return 0;
}

double cluster_get_avg_load_percent(void) {
    if (g_cluster.node_count == 0) {
        return 0.0;
    }
    
    double total_load = 0.0;
    int online_count = 0;
    
    for (int i = 0; i < g_cluster.node_count; i++) {
        if (!g_cluster.nodes[i].enabled) continue;
        if (g_cluster.node_status[i].status != CLUSTER_NODE_ONLINE) continue;
        
        cluster_node_status_t *status = &g_cluster.node_status[i];
        if (status->max_connections > 0) {
            double load = (double)status->active_connections / status->max_connections * 100.0;
            total_load += load;
            online_count++;
        }
    }
    
    if (online_count == 0) {
        return 0.0;
    }
    
    return total_load / online_count;
}

// ============================================================================
// Failover
// ============================================================================

int cluster_handle_node_failure(const char *failed_node) {
    if (!failed_node) {
        return -1;
    }
    
    kprintf("[CLUSTER] Handling failure of node: %s\n", failed_node);
    
    g_cluster.stats.failover_count++;
    
    // Если это лидер, начинаем выборы
    if (strcmp(failed_node, g_cluster.leader_name) == 0) {
        kprintf("[CLUSTER] Leader failed, starting election\n");
        cluster_start_election();
    }
    
    // Перераспределение подключений
    cluster_node_status_t *status = find_node_status(failed_node);
    if (status && status->active_connections > 0) {
        cluster_rebalance_connections(failed_node, status->active_connections);
    }
    
    return 0;
}

int cluster_recovery_node(const char *node_name) {
    if (!node_name) {
        return -1;
    }
    
    kprintf("[CLUSTER] Recovering node: %s\n", node_name);
    
    cluster_node_status_t *status = find_node_status(node_name);
    if (!status) {
        return -1;
    }
    
    // Сброс статуса
    status->status = CLUSTER_NODE_STARTING;
    status->last_seen = get_current_time_ms();
    
    // Запрос конфигурации у лидера
    cluster_request_config();
    
    return 0;
}

void cluster_set_auto_failover(bool enabled) {
    g_cluster.auto_failover_enabled = enabled;
    kprintf("[CLUSTER] Auto failover %s\n", enabled ? "enabled" : "disabled");
}

// ============================================================================
// Отправка сообщений
// ============================================================================

int cluster_send_message(const char *destination, cluster_message_type_t type,
                         const char *payload, int payload_size) {
    if (!destination || !payload) {
        return -1;
    }
    
    cluster_node_config_t *node = find_node_config(destination);
    if (!node) {
        return -1;
    }
    
    cluster_message_t msg = {0};
    msg.type = type;
    snprintf(msg.source, sizeof(msg.source), "%s", g_cluster.local_node_name);
    snprintf(msg.destination, sizeof(msg.destination), "%s", destination);
    msg.timestamp = get_current_time_ms();
    msg.message_id = ++g_cluster.stats.total_messages_sent;
    
    if (payload_size > CLUSTER_MAX_MESSAGE_LEN) {
        payload_size = CLUSTER_MAX_MESSAGE_LEN;
    }
    memcpy(msg.payload, payload, payload_size);
    msg.payload_size = payload_size;
    
    return send_tcp_message(node->host, node->management_port, &msg);
}

int cluster_broadcast_message(cluster_message_type_t type,
                              const char *payload, int payload_size) {
    int sent = 0;
    
    for (int i = 0; i < g_cluster.node_count; i++) {
        if (strcmp(g_cluster.nodes[i].name, g_cluster.local_node_name) == 0) continue;
        if (!g_cluster.nodes[i].enabled) continue;
        
        if (cluster_send_message(g_cluster.nodes[i].name, type, payload, payload_size) == 0) {
            sent++;
        }
    }
    
    return sent;
}

int cluster_set_message_handler(cluster_message_type_t type, 
                                 cluster_message_handler_t handler) {
    if (type >= 16) {
        return -1;
    }
    
    g_cluster.message_handlers[type] = handler;
    return 0;
}

// ============================================================================
// Callback функции
// ============================================================================

void cluster_set_node_change_callback(cluster_node_change_callback_t callback) {
    g_cluster.node_change_callback = callback;
}

void cluster_set_leader_change_callback(cluster_leader_change_callback_t callback) {
    g_cluster.leader_change_callback = callback;
}

// ============================================================================
// Статистика и мониторинг
// ============================================================================

int cluster_get_stats(cluster_stats_t *stats) {
    if (!g_cluster.initialized || !stats) {
        return -1;
    }
    
    memcpy(stats, &g_cluster.stats, sizeof(cluster_stats_t));
    stats->local_role = g_cluster.local_role;
    snprintf(stats->leader_name, sizeof(stats->leader_name), "%s", g_cluster.leader_name);
    stats->cluster_uptime_ms = get_current_time_ms() - g_cluster.start_time;
    stats->avg_load_percent = cluster_get_avg_load_percent();
    
    // Подсчёт онлайн узлов
    stats->online_nodes = 0;
    stats->offline_nodes = 0;
    stats->degraded_nodes = 0;
    
    for (int i = 0; i < g_cluster.node_count; i++) {
        switch (g_cluster.node_status[i].status) {
            case CLUSTER_NODE_ONLINE:
                stats->online_nodes++;
                break;
            case CLUSTER_NODE_OFFLINE:
            case CLUSTER_NODE_FAILED:
                stats->offline_nodes++;
                break;
            case CLUSTER_NODE_DEGRADED:
                stats->degraded_nodes++;
                break;
            default:
                break;
        }
    }
    
    return 0;
}

int cluster_get_stats_string(char *buffer, size_t buffer_size) {
    if (!g_cluster.initialized || !buffer || buffer_size < 256) {
        return -1;
    }
    
    cluster_stats_t stats;
    cluster_get_stats(&stats);
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "Cluster Statistics:\n");
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Cluster: %s, Local Node: %s\n",
                       g_cluster.cluster_name, g_cluster.local_node_name);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Nodes: %d (Online: %d, Offline: %d, Degraded: %d)\n",
                       stats.total_nodes, stats.online_nodes, 
                       stats.offline_nodes, stats.degraded_nodes);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Role: %s, Leader: %s\n",
                       cluster_role_to_string(stats.local_role),
                       stats.leader_name[0] ? stats.leader_name : "none");
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Messages: Sent %llu, Received %llu\n",
                       (unsigned long long)stats.total_messages_sent,
                       (unsigned long long)stats.total_messages_received);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Avg Load: %.1f%%, Failovers: %llu\n",
                       stats.avg_load_percent,
                       (unsigned long long)stats.failover_count);
    
    return 0;
}

void cluster_reset_stats(void) {
    if (!g_cluster.initialized) return;
    
    memset(&g_cluster.stats, 0, sizeof(g_cluster.stats));
    g_cluster.stats.total_nodes = g_cluster.node_count;
    kprintf("[CLUSTER] Statistics reset\n");
}

int64_t cluster_get_uptime(void) {
    if (!g_cluster.initialized) return 0;
    return get_current_time_ms() - g_cluster.start_time;
}

int cluster_get_online_count(void) {
    int count = 0;
    for (int i = 0; i < g_cluster.node_count; i++) {
        if (g_cluster.node_status[i].status == CLUSTER_NODE_ONLINE) {
            count++;
        }
    }
    return count;
}

bool cluster_has_quorum(void) {
    int online = cluster_get_online_count();
    return online > g_cluster.node_count / 2;
}

// ============================================================================
// Утилиты
// ============================================================================

const char* cluster_role_to_string(cluster_role_t role) {
    switch (role) {
        case CLUSTER_ROLE_LEADER: return "Leader";
        case CLUSTER_ROLE_FOLLOWER: return "Follower";
        case CLUSTER_ROLE_CANDIDATE: return "Candidate";
        case CLUSTER_ROLE_STANDALONE: return "Standalone";
        default: return "Unknown";
    }
}

const char* cluster_node_status_to_string(cluster_node_state_t status) {
    switch (status) {
        case CLUSTER_NODE_OFFLINE: return "Offline";
        case CLUSTER_NODE_STARTING: return "Starting";
        case CLUSTER_NODE_ONLINE: return "Online";
        case CLUSTER_NODE_DEGRADED: return "Degraded";
        case CLUSTER_NODE_FAILED: return "Failed";
        default: return "Unknown";
    }
}

const char* cluster_message_type_to_string(cluster_message_type_t type) {
    switch (type) {
        case CLUSTER_MSG_HEARTBEAT: return "Heartbeat";
        case CLUSTER_MSG_ELECTION_REQUEST: return "ElectionRequest";
        case CLUSTER_MSG_ELECTION_RESPONSE: return "ElectionResponse";
        case CLUSTER_MSG_CONFIG_SYNC: return "ConfigSync";
        case CLUSTER_MSG_CONFIG_ACK: return "ConfigAck";
        case CLUSTER_MSG_STATUS_REQUEST: return "StatusRequest";
        case CLUSTER_MSG_STATUS_RESPONSE: return "StatusResponse";
        case CLUSTER_MSG_LOAD_BALANCE: return "LoadBalance";
        case CLUSTER_MSG_FAILOVER: return "Failover";
        default: return "Unknown";
    }
}
