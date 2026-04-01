/*
    MTProxy Load Balancer
    Реализация системы балансировки нагрузки
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

#include "system/cluster/load-balancer.h"
#include "common/utils.h"
#include "common/kprintf.h"

// ============================================================================
// Глобальные переменные
// ============================================================================

static struct {
    bool initialized;
    bool running;
    load_balancer_algorithm_t algorithm;
    
    load_balancer_backend_config_t backends[LOAD_BALANCER_MAX_BACKENDS];
    load_balancer_backend_stats_t backend_stats[LOAD_BALANCER_MAX_BACKENDS];
    int backend_count;
    
    // Для Round Robin
    int rr_index;
    
    // Статистика
    load_balancer_stats_t stats;
    int64_t start_time;
    uint64_t last_second_requests;
    int64_t last_second_time;
    
    // Настройки
    int rebalance_threshold_percent;
    bool auto_rebalance;
    
    // Health check
    load_balancer_health_check_callback_t health_check_callback;
    bool health_check_running;
#ifdef _WIN32
    HANDLE health_check_thread;
#else
    pthread_t health_check_thread;
#endif
    
    // Callback
    load_balancer_backend_change_callback_t backend_change_callback;
} g_load_balancer = {0};

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

static load_balancer_backend_config_t* find_backend(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        if (strcmp(g_load_balancer.backends[i].name, name) == 0) {
            return &g_load_balancer.backends[i];
        }
    }
    return NULL;
}

static load_balancer_backend_stats_t* find_backend_stats(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        if (strcmp(g_load_balancer.backends[i].name, name) == 0) {
            return &g_load_balancer.backend_stats[i];
        }
    }
    return NULL;
}

static void update_backend_status(const char *name, load_balancer_backend_status_t new_status) {
    load_balancer_backend_stats_t *stats = find_backend_stats(name);
    if (!stats) return;
    
    load_balancer_backend_status_t old_status = stats->status;
    if (old_status == new_status) return;
    
    stats->status = new_status;
    
    kprintf("[LB] Backend %s status changed: %s -> %s\n",
            name,
            load_balancer_backend_status_to_string(old_status),
            load_balancer_backend_status_to_string(new_status));
    
    // Callback
    if (g_load_balancer.backend_change_callback) {
        g_load_balancer.backend_change_callback(name, old_status, new_status);
    }
}

static int tcp_health_check(const char *host, int port, int timeout_ms) {
    if (!host) return -1;
    
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
    
#ifdef _WIN32
    DWORD timeout = timeout_ms;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
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
    
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    
    return 0;
}

// ============================================================================
// Алгоритмы балансировки
// ============================================================================

static const char* select_round_robin(void) {
    if (g_load_balancer.backend_count == 0) return NULL;
    
    int start_index = g_load_balancer.rr_index;
    int checked = 0;
    
    while (checked < g_load_balancer.backend_count) {
        load_balancer_backend_config_t *backend = &g_load_balancer.backends[g_load_balancer.rr_index];
        load_balancer_backend_stats_t *stats = &g_load_balancer.backend_stats[g_load_balancer.rr_index];
        
        g_load_balancer.rr_index = (g_load_balancer.rr_index + 1) % g_load_balancer.backend_count;
        
        if (backend->enabled && stats->status == LOAD_BALANCER_BACKEND_ONLINE) {
            if (stats->active_connections < backend->max_connections) {
                return backend->name;
            }
        }
        
        checked++;
    }
    
    // Все узлы перегружены, возвращаем первый доступный
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        if (g_load_balancer.backends[i].enabled && 
            g_load_balancer.backend_stats[i].status == LOAD_BALANCER_BACKEND_ONLINE) {
            return g_load_balancer.backends[i].name;
        }
    }
    
    return NULL;
}

static const char* select_least_connections(void) {
    const char *selected = NULL;
    int min_connections = INT32_MAX;
    
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        if (!g_load_balancer.backends[i].enabled) continue;
        if (g_load_balancer.backend_stats[i].status != LOAD_BALANCER_BACKEND_ONLINE) continue;
        
        if (g_load_balancer.backend_stats[i].active_connections < min_connections) {
            min_connections = g_load_balancer.backend_stats[i].active_connections;
            selected = g_load_balancer.backends[i].name;
        }
    }
    
    return selected;
}

static const char* select_least_load(void) {
    const char *selected = NULL;
    double min_load = 101.0;
    
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        if (!g_load_balancer.backends[i].enabled) continue;
        if (g_load_balancer.backend_stats[i].status != LOAD_BALANCER_BACKEND_ONLINE) continue;
        
        if (g_load_balancer.backend_stats[i].load_percent < min_load) {
            min_load = g_load_balancer.backend_stats[i].load_percent;
            selected = g_load_balancer.backends[i].name;
        }
    }
    
    return selected;
}

static const char* select_weighted(void) {
    const char *selected = NULL;
    double max_weighted_score = -1.0;
    
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        if (!g_load_balancer.backends[i].enabled) continue;
        if (g_load_balancer.backend_stats[i].status != LOAD_BALANCER_BACKEND_ONLINE) continue;
        
        double weight = g_load_balancer.backends[i].weight;
        double load = g_load_balancer.backend_stats[i].load_percent;
        
        // Score = weight / (load + 1)
        double score = weight / (load + 1.0);
        
        if (score > max_weighted_score) {
            max_weighted_score = score;
            selected = g_load_balancer.backends[i].name;
        }
    }
    
    return selected;
}

static const char* select_ip_hash(const char *client_ip) {
    if (!client_ip || g_load_balancer.backend_count == 0) return NULL;
    
    int index = load_balancer_ip_hash(client_ip, g_load_balancer.backend_count);
    
    // Проверяем, доступен ли узел
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        int try_index = (index + i) % g_load_balancer.backend_count;
        
        if (g_load_balancer.backends[try_index].enabled &&
            g_load_balancer.backend_stats[try_index].status == LOAD_BALANCER_BACKEND_ONLINE) {
            return g_load_balancer.backends[try_index].name;
        }
    }
    
    return NULL;
}

static const char* select_random(void) {
    if (g_load_balancer.backend_count == 0) return NULL;
    
    int start = rand() % g_load_balancer.backend_count;
    
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        int index = (start + i) % g_load_balancer.backend_count;
        
        if (g_load_balancer.backends[index].enabled &&
            g_load_balancer.backend_stats[index].status == LOAD_BALANCER_BACKEND_ONLINE) {
            return g_load_balancer.backends[index].name;
        }
    }
    
    return NULL;
}

// ============================================================================
// Инициализация и очистка
// ============================================================================

int load_balancer_init(load_balancer_algorithm_t algorithm) {
    if (g_load_balancer.initialized) {
        return 0;
    }
    
    memset(&g_load_balancer, 0, sizeof(g_load_balancer));
    
    g_load_balancer.algorithm = algorithm;
    g_load_balancer.start_time = get_current_time_ms();
    g_load_balancer.rebalance_threshold_percent = 80;
    g_load_balancer.auto_rebalance = true;
    g_load_balancer.rr_index = 0;
    
    g_load_balancer.initialized = true;
    
    kprintf("[LB] Load Balancer initialized (algorithm: %s)\n",
            load_balancer_algorithm_to_string(algorithm));
    
    return 0;
}

void load_balancer_cleanup(void) {
    if (!g_load_balancer.initialized) {
        return;
    }
    
    load_balancer_stop_health_checks();
    
    memset(&g_load_balancer, 0, sizeof(g_load_balancer));
    kprintf("[LB] Load Balancer cleaned up\n");
}

bool load_balancer_is_initialized(void) {
    return g_load_balancer.initialized;
}

// ============================================================================
// Управление backend узлами
// ============================================================================

int load_balancer_add_backend(const char *name, const char *host, int port, double weight) {
    if (!g_load_balancer.initialized || !name || !host) {
        return -1;
    }
    
    if (g_load_balancer.backend_count >= LOAD_BALANCER_MAX_BACKENDS) {
        kprintf("[LB] Error: Maximum backends reached\n");
        return -1;
    }
    
    if (find_backend(name) != NULL) {
        kprintf("[LB] Error: Backend already exists: %s\n", name);
        return -1;
    }
    
    load_balancer_backend_config_t *backend = &g_load_balancer.backends[g_load_balancer.backend_count];
    memset(backend, 0, sizeof(load_balancer_backend_config_t));
    
    snprintf(backend->name, sizeof(backend->name), "%s", name);
    snprintf(backend->host, sizeof(backend->host), "%s", host);
    backend->port = port;
    backend->weight = weight > 0 ? weight : 1.0;
    backend->max_connections = 10000;
    backend->max_load_percent = 90;
    backend->enabled = true;
    
    load_balancer_backend_stats_t *stats = &g_load_balancer.backend_stats[g_load_balancer.backend_count];
    memset(stats, 0, sizeof(load_balancer_backend_stats_t));
    snprintf(stats->name, sizeof(stats->name), "%s", name);
    stats->status = LOAD_BALANCER_BACKEND_OFFLINE;
    
    g_load_balancer.backend_count++;
    g_load_balancer.stats.total_backends++;
    g_load_balancer.stats.active_backends++;
    
    kprintf("[LB] Backend added: %s at %s:%d (weight: %.1f)\n", name, host, port, backend->weight);
    return 0;
}

int load_balancer_remove_backend(const char *name) {
    if (!g_load_balancer.initialized || !name) {
        return -1;
    }
    
    int idx = -1;
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        if (strcmp(g_load_balancer.backends[i].name, name) == 0) {
            idx = i;
            break;
        }
    }
    
    if (idx < 0) return -1;
    
    // Сдвиг массивов
    for (int i = idx; i < g_load_balancer.backend_count - 1; i++) {
        g_load_balancer.backends[i] = g_load_balancer.backends[i + 1];
        g_load_balancer.backend_stats[i] = g_load_balancer.backend_stats[i + 1];
    }
    
    g_load_balancer.backend_count--;
    g_load_balancer.stats.total_backends--;
    g_load_balancer.stats.active_backends--;
    
    kprintf("[LB] Backend removed: %s\n", name);
    return 0;
}

int load_balancer_set_backend_enabled(const char *name, bool enabled) {
    load_balancer_backend_config_t *backend = find_backend(name);
    if (!backend) return -1;
    
    backend->enabled = enabled;
    
    if (!enabled) {
        g_load_balancer.stats.active_backends--;
    } else {
        g_load_balancer.stats.active_backends++;
    }
    
    kprintf("[LB] Backend %s %s\n", name, enabled ? "enabled" : "disabled");
    return 0;
}

int load_balancer_set_backend_weight(const char *name, double weight) {
    load_balancer_backend_config_t *backend = find_backend(name);
    if (!backend) return -1;
    
    backend->weight = weight > 0 ? weight : 1.0;
    kprintf("[LB] Backend %s weight set to %.1f\n", name, backend->weight);
    return 0;
}

const load_balancer_backend_config_t* load_balancer_get_backend_config(const char *name) {
    return find_backend(name);
}

const load_balancer_backend_stats_t* load_balancer_get_backend_stats(const char *name) {
    return find_backend_stats(name);
}

int load_balancer_get_all_backends(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return -1;
    
    int offset = 0;
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        if (i > 0) {
            offset += snprintf(buffer + offset, buffer_size - offset, ", ");
        }
        offset += snprintf(buffer + offset, buffer_size - offset, "%s",
                           g_load_balancer.backends[i].name);
    }
    
    return g_load_balancer.backend_count;
}

// ============================================================================
// Выбор алгоритма
// ============================================================================

int load_balancer_set_algorithm(load_balancer_algorithm_t algorithm) {
    if (!g_load_balancer.initialized) {
        return -1;
    }
    
    g_load_balancer.algorithm = algorithm;
    g_load_balancer.rr_index = 0;
    
    kprintf("[LB] Algorithm changed to: %s\n", load_balancer_algorithm_to_string(algorithm));
    return 0;
}

load_balancer_algorithm_t load_balancer_get_algorithm(void) {
    return g_load_balancer.algorithm;
}

// ============================================================================
// Выбор backend узла
// ============================================================================

const char* load_balancer_select_backend(const char *client_ip) {
    if (!g_load_balancer.initialized || g_load_balancer.backend_count == 0) {
        return NULL;
    }
    
    const char *selected = NULL;
    
    switch (g_load_balancer.algorithm) {
        case LOAD_BALANCER_ROUND_ROBIN:
            selected = select_round_robin();
            break;
        case LOAD_BALANCER_LEAST_CONNECTIONS:
            selected = select_least_connections();
            break;
        case LOAD_BALANCER_LEAST_LOAD:
            selected = select_least_load();
            break;
        case LOAD_BALANCER_WEIGHTED:
            selected = select_weighted();
            break;
        case LOAD_BALANCER_IP_HASH:
            selected = select_ip_hash(client_ip);
            break;
        case LOAD_BALANCER_RANDOM:
            selected = select_random();
            break;
        default:
            selected = select_round_robin();
            break;
    }
    
    if (selected) {
        g_load_balancer.stats.total_requests++;
        g_load_balancer.stats.successful_requests++;
    } else {
        g_load_balancer.stats.failed_requests++;
    }
    
    return selected;
}

const char* load_balancer_get_least_loaded_backend(void) {
    return select_least_load();
}

const char* load_balancer_get_least_connections_backend(void) {
    return select_least_connections();
}

int load_balancer_notify_new_connection(const char *backend_name) {
    if (!backend_name) return -1;
    
    load_balancer_backend_stats_t *stats = find_backend_stats(backend_name);
    if (!stats) return -1;
    
    stats->active_connections++;
    stats->total_connections++;
    
    // Проверка на перегрузку
    load_balancer_backend_config_t *backend = find_backend(backend_name);
    if (backend && stats->active_connections > backend->max_connections) {
        kprintf("[LB] Backend %s overloaded: %d connections (max: %d)\n",
                backend_name, stats->active_connections, backend->max_connections);
        
        if (g_load_balancer.auto_rebalance) {
            load_balancer_redistribute_from_node(backend_name, 
                stats->active_connections - backend->max_connections);
        }
    }
    
    return 0;
}

int load_balancer_notify_connection_closed(const char *backend_name) {
    if (!backend_name) return -1;
    
    load_balancer_backend_stats_t *stats = find_backend_stats(backend_name);
    if (!stats) return -1;
    
    if (stats->active_connections > 0) {
        stats->active_connections--;
    }
    
    return 0;
}

// ============================================================================
// Перераспределение нагрузки
// ============================================================================

int load_balancer_rebalance(void) {
    if (!g_load_balancer.initialized) {
        return -1;
    }
    
    kprintf("[LB] Rebalancing load\n");
    
    double avg_load = load_balancer_get_avg_load_percent();
    g_load_balancer.stats.avg_load_percent = avg_load;
    
    // Поиск перегруженных и недогруженных узлов
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        load_balancer_backend_stats_t *stats = &g_load_balancer.backend_stats[i];
        
        if (stats->load_percent > g_load_balancer.rebalance_threshold_percent) {
            int to_redistribute = (int)(stats->active_connections * 0.2);  // 20% подключений
            if (to_redistribute > 0) {
                load_balancer_redistribute_from_node(g_load_balancer.backends[i].name, to_redistribute);
            }
        }
    }
    
    g_load_balancer.stats.rebalances++;
    g_load_balancer.stats.last_rebalance = get_current_time_ms();
    
    return 0;
}

int load_balancer_redistribute_from_node(const char *source_node, int connections_count) {
    if (!source_node || connections_count <= 0) {
        return -1;
    }
    
    kprintf("[LB] Redistributing %d connections from %s\n", connections_count, source_node);
    
    // Поиск наименее загруженных узлов
    const char *target = load_balancer_get_least_loaded_backend();
    if (!target || strcmp(target, source_node) == 0) {
        kprintf("[LB] No suitable target for redistribution\n");
        return -1;
    }
    
    // В реальной реализации здесь была бы логика перераспределения
    // через Cluster Manager
    
    kprintf("[LB] Would move %d connections from %s to %s\n", 
            connections_count, source_node, target);
    
    return 0;
}

void load_balancer_set_rebalance_threshold(int percent) {
    g_load_balancer.rebalance_threshold_percent = percent > 0 ? percent : 80;
    kprintf("[LB] Rebalance threshold set to %d%%\n", g_load_balancer.rebalance_threshold_percent);
}

void load_balancer_set_auto_rebalance(bool enabled) {
    g_load_balancer.auto_rebalance = enabled;
    kprintf("[LB] Auto rebalance %s\n", enabled ? "enabled" : "disabled");
}

// ============================================================================
// Health Check
// ============================================================================

#ifdef _WIN32
static DWORD WINAPI health_check_thread_func(LPVOID arg)
#else
static void* health_check_thread_func(void *arg)
#endif
{
    kprintf("[LB] Health check thread started\n");
    
    while (g_load_balancer.health_check_running) {
#ifdef _WIN32
        Sleep(5000);  // 5 секунд
#else
        struct timespec ts;
        ts.tv_sec = 5;
        nanosleep(&ts, NULL);
#endif
        
        if (g_load_balancer.health_check_running) {
            load_balancer_run_health_check();
        }
    }
    
    kprintf("[LB] Health check thread stopped\n");
    return 0;
}

int load_balancer_run_health_check(void) {
    if (!g_load_balancer.initialized) {
        return 0;
    }
    
    int healthy_count = 0;
    int64_t now = get_current_time_ms();
    
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        load_balancer_backend_config_t *backend = &g_load_balancer.backends[i];
        load_balancer_backend_stats_t *stats = &g_load_balancer.backend_stats[i];
        
        if (!backend->enabled) {
            if (stats->status != LOAD_BALANCER_BACKEND_OFFLINE) {
                update_backend_status(backend->name, LOAD_BALANCER_BACKEND_OFFLINE);
            }
            continue;
        }
        
        int result = -1;
        
        // Использование callback или встроенная проверка
        if (g_load_balancer.health_check_callback) {
            result = g_load_balancer.health_check_callback(backend->name, backend->host, backend->port);
        } else {
            result = tcp_health_check(backend->host, backend->port, 3000);
        }
        
        stats->last_health_check = now;
        
        if (result == 0) {
            healthy_count++;
            stats->consecutive_failures = 0;
            
            if (stats->status == LOAD_BALANCER_BACKEND_OFFLINE ||
                stats->status == LOAD_BALANCER_BACKEND_FAILED) {
                update_backend_status(backend->name, LOAD_BALANCER_BACKEND_ONLINE);
            } else if (stats->status == LOAD_BALANCER_BACKEND_DEGRADED) {
                update_backend_status(backend->name, LOAD_BALANCER_BACKEND_ONLINE);
            }
        } else {
            stats->consecutive_failures++;
            
            if (stats->consecutive_failures >= 3) {
                if (stats->status != LOAD_BALANCER_BACKEND_FAILED) {
                    update_backend_status(backend->name, LOAD_BALANCER_BACKEND_FAILED);
                }
            } else if (stats->status == LOAD_BALANCER_BACKEND_ONLINE) {
                update_backend_status(backend->name, LOAD_BALANCER_BACKEND_DEGRADED);
            }
        }
    }
    
    g_load_balancer.stats.active_backends = healthy_count;
    g_load_balancer.stats.offline_backends = g_load_balancer.backend_count - healthy_count;
    
    return healthy_count;
}

int load_balancer_start_health_checks(int interval_ms) {
    if (!g_load_balancer.initialized) {
        return -1;
    }
    
    if (g_load_balancer.health_check_running) {
        return 0;
    }
    
    g_load_balancer.health_check_running = true;
    
#ifdef _WIN32
    g_load_balancer.health_check_thread = CreateThread(NULL, 0, health_check_thread_func, NULL, 0, NULL);
    if (!g_load_balancer.health_check_thread) {
        g_load_balancer.health_check_running = false;
        return -1;
    }
#else
    if (pthread_create(&g_load_balancer.health_check_thread, NULL, health_check_thread_func, NULL) != 0) {
        g_load_balancer.health_check_running = false;
        return -1;
    }
#endif
    
    kprintf("[LB] Health checks started\n");
    return 0;
}

void load_balancer_stop_health_checks(void) {
    if (!g_load_balancer.health_check_running) {
        return;
    }
    
    g_load_balancer.health_check_running = false;
    
#ifdef _WIN32
    if (g_load_balancer.health_check_thread) {
        WaitForSingleObject(g_load_balancer.health_check_thread, INFINITE);
        CloseHandle(g_load_balancer.health_check_thread);
    }
#else
    pthread_join(g_load_balancer.health_check_thread, NULL);
#endif
    
    kprintf("[LB] Health checks stopped\n");
}

void load_balancer_set_health_check_callback(load_balancer_health_check_callback_t callback) {
    g_load_balancer.health_check_callback = callback;
}

// ============================================================================
// Статистика
// ============================================================================

int load_balancer_get_stats(load_balancer_stats_t *stats) {
    if (!g_load_balancer.initialized || !stats) {
        return -1;
    }
    
    memcpy(stats, &g_load_balancer.stats, sizeof(load_balancer_stats_t));
    stats->algorithm = g_load_balancer.algorithm;
    stats->avg_load_percent = load_balancer_get_avg_load_percent();
    
    return 0;
}

int load_balancer_get_stats_string(char *buffer, size_t buffer_size) {
    if (!g_load_balancer.initialized || !buffer || buffer_size < 256) {
        return -1;
    }
    
    load_balancer_stats_t stats;
    load_balancer_get_stats(&stats);
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "Load Balancer Statistics:\n");
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Algorithm: %s\n", load_balancer_algorithm_to_string(stats.algorithm));
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Backends: %d (Active: %d, Offline: %d)\n",
                       stats.total_backends, stats.active_backends, stats.offline_backends);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Requests: Total %llu, Success %llu, Failed %llu\n",
                       (unsigned long long)stats.total_requests,
                       (unsigned long long)stats.successful_requests,
                       (unsigned long long)stats.failed_requests);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Avg Load: %.1f%%, Rebalances: %llu\n",
                       stats.avg_load_percent,
                       (unsigned long long)stats.rebalances);
    
    return 0;
}

void load_balancer_reset_stats(void) {
    if (!g_load_balancer.initialized) return;
    
    memset(&g_load_balancer.stats, 0, sizeof(g_load_balancer.stats));
    g_load_balancer.stats.total_backends = g_load_balancer.backend_count;
    g_load_balancer.stats.active_backends = g_load_balancer.backend_count;
    kprintf("[LB] Statistics reset\n");
}

double load_balancer_get_avg_load_percent(void) {
    if (g_load_balancer.backend_count == 0) {
        return 0.0;
    }
    
    double total_load = 0.0;
    int online_count = 0;
    
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        if (!g_load_balancer.backends[i].enabled) continue;
        if (g_load_balancer.backend_stats[i].status != LOAD_BALANCER_BACKEND_ONLINE) continue;
        
        total_load += g_load_balancer.backend_stats[i].load_percent;
        online_count++;
    }
    
    if (online_count == 0) {
        return 0.0;
    }
    
    return total_load / online_count;
}

int load_balancer_get_total_active_connections(void) {
    int total = 0;
    
    for (int i = 0; i < g_load_balancer.backend_count; i++) {
        total += g_load_balancer.backend_stats[i].active_connections;
    }
    
    return total;
}

double load_balancer_get_requests_per_second(void) {
    int64_t now = get_current_time_ms();
    
    if ((now - g_load_balancer.last_second_time) >= 1000) {
        g_load_balancer.last_second_requests = g_load_balancer.stats.successful_requests;
        g_load_balancer.last_second_time = now;
    }
    
    return (double)g_load_balancer.last_second_requests;
}

// ============================================================================
// Callback функции
// ============================================================================

void load_balancer_set_backend_change_callback(load_balancer_backend_change_callback_t callback) {
    g_load_balancer.backend_change_callback = callback;
}

// ============================================================================
// Утилиты
// ============================================================================

const char* load_balancer_algorithm_to_string(load_balancer_algorithm_t algo) {
    switch (algo) {
        case LOAD_BALANCER_ROUND_ROBIN: return "Round Robin";
        case LOAD_BALANCER_LEAST_CONNECTIONS: return "Least Connections";
        case LOAD_BALANCER_LEAST_LOAD: return "Least Load";
        case LOAD_BALANCER_WEIGHTED: return "Weighted";
        case LOAD_BALANCER_IP_HASH: return "IP Hash";
        case LOAD_BALANCER_RANDOM: return "Random";
        default: return "Unknown";
    }
}

const char* load_balancer_backend_status_to_string(load_balancer_backend_status_t status) {
    switch (status) {
        case LOAD_BALANCER_BACKEND_OFFLINE: return "Offline";
        case LOAD_BALANCER_BACKEND_ONLINE: return "Online";
        case LOAD_BALANCER_BACKEND_DEGRADED: return "Degraded";
        case LOAD_BALANCER_BACKEND_FAILED: return "Failed";
        default: return "Unknown";
    }
}

int load_balancer_ip_hash(const char *client_ip, int backend_count) {
    if (!client_ip || backend_count <= 0) return 0;
    
    // DJB2 hash
    unsigned long hash = 5381;
    int c;
    
    while ((c = *client_ip++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash % backend_count;
}
