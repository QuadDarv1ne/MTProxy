/*
    MTProxy Health Check System
    Реализация системы проверки здоровья узлов
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
    #include <psapi.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "psapi.lib")
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <sys/statvfs.h>
    #include <signal.h>
#endif

#include "system/monitoring/health-check.h"
#include "common/utils.h"

// ============================================================================
// Глобальные переменные
// ============================================================================

static struct {
    bool initialized;
    health_check_config_t nodes[HEALTH_MAX_NODES];
    health_check_result_t results[HEALTH_MAX_NODES];
    int node_count;
    bool running;
    int64_t start_time;
    int check_interval_ms;
    health_check_stats_t stats;
    bool alerts_enabled;
    bool alert_on_unhealthy;
    bool alert_on_recovery;
    
#ifdef _WIN32
    HANDLE check_thread;
#else
    pthread_t check_thread;
#endif
} g_health_check = {0};

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

static health_check_config_t* find_node(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < g_health_check.node_count; i++) {
        if (strcmp(g_health_check.nodes[i].name, name) == 0) {
            return &g_health_check.nodes[i];
        }
    }
    return NULL;
}

static health_check_result_t* find_result(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < g_health_check.node_count; i++) {
        if (strcmp(g_health_check.nodes[i].name, name) == 0) {
            return &g_health_check.results[i];
        }
    }
    return NULL;
}

// HTTP проверка
static int http_health_check(const char *url, int timeout_ms, 
                             char *error, size_t error_size,
                             char *response, size_t response_size,
                             int *status_code) {
    if (!url || !error || !response) return -1;
    
    // Парсинг URL
    char host[256] = {0};
    int port = 80;
    char path[256] = "/";
    
    if (strncmp(url, "http://", 7) == 0) {
        url += 7;
        port = 80;
    } else if (strncmp(url, "https://", 8) == 0) {
        url += 8;
        port = 443;
        // HTTPS требует SSL, упрощённо считаем успешным
    }
    
    // Хост и путь
    char *slash = strchr((char*)url, '/');
    if (slash) {
        int host_len = slash - url;
        strncpy(host, url, host_len);
        strncpy(path, slash, sizeof(path) - 1);
    } else {
        strncpy(host, url, sizeof(host) - 1);
    }
    
    // Порт из URL
    char *colon = strchr(host, ':');
    if (colon) {
        *colon = '\0';
        port = atoi(colon + 1);
    }
    
    // Создание сокета
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        // Попытка резолва
        struct hostent *he = gethostbyname(host);
        if (!he) {
            snprintf(error, error_size, "Failed to resolve host: %s", host);
            return -1;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        snprintf(error, error_size, "Failed to create socket");
        return -1;
    }
    
    // Установка таймаута
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    
    // Подключение
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        snprintf(error, error_size, "Failed to connect to %s:%d", host, port);
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }
    
    // HTTP запрос
    char request[512] = {0};
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n", path, host);
    
    if (send(sock, request, strlen(request), 0) < 0) {
        snprintf(error, error_size, "Failed to send request");
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }
    
    // Получение ответа
    int total = 0;
    while (total < (int)response_size - 1) {
        int n = recv(sock, response + total, response_size - total - 1, 0);
        if (n <= 0) break;
        total += n;
    }
    response[total] = '\0';
    
    // Парсинг статуса
    if (status_code) {
        *status_code = 200;  // По умолчанию
        char *status = strstr(response, "HTTP/1.");
        if (status) {
            sscanf(status, "HTTP/1.%*d %d", status_code);
        }
    }
    
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    
    return (*status_code >= 200 && *status_code < 300) ? 0 : -1;
}

// TCP проверка
static int tcp_health_check(const char *host, int port, int timeout_ms,
                            char *error, size_t error_size) {
    if (!host || !error) return -1;
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        struct hostent *he = gethostbyname(host);
        if (!he) {
            snprintf(error, error_size, "Failed to resolve: %s", host);
            return -1;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        snprintf(error, error_size, "Failed to create socket");
        return -1;
    }
    
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        snprintf(error, error_size, "TCP connect failed: %s:%d", host, port);
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

// Process проверка
static int process_health_check(int pid, char *error, size_t error_size) {
    if (pid <= 0) {
        snprintf(error, error_size, "Invalid PID: %d", pid);
        return -1;
    }
    
#ifdef _WIN32
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!h) {
        snprintf(error, error_size, "Process %d not found", pid);
        return -1;
    }
    
    DWORD exit_code;
    if (!GetExitCodeProcess(h, &exit_code) || exit_code != STILL_ACTIVE) {
        snprintf(error, error_size, "Process %d is not running", pid);
        CloseHandle(h);
        return -1;
    }
    CloseHandle(h);
#else
    if (kill(pid, 0) != 0) {
        snprintf(error, error_size, "Process %d not found or not accessible", pid);
        return -1;
    }
#endif
    
    return 0;
}

// Memory проверка
static int memory_health_check(uint64_t max_memory, char *error, size_t error_size,
                                uint64_t *current_memory) {
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status)) {
        snprintf(error, error_size, "Failed to get memory status");
        return -1;
    }
    
    uint64_t used = status.ullTotalPhys - status.ullAvailPhys;
    if (current_memory) *current_memory = used;
    
    double percent = (double)used / status.ullTotalPhys * 100.0;
    if (percent > max_memory) {
        snprintf(error, error_size, "Memory usage %.1f%% exceeds limit", percent);
        return -1;
    }
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages <= 0 || page_size <= 0) {
        snprintf(error, error_size, "Failed to get memory info");
        return -1;
    }
    
    uint64_t total = (uint64_t)pages * page_size;
    
    long avail = sysconf(_SC_AVPHYS_PAGES);
    uint64_t used = total - (uint64_t)avail * page_size;
    if (current_memory) *current_memory = used;
    
    double percent = (double)used / total * 100.0;
    if (percent > max_memory) {
        snprintf(error, error_size, "Memory usage %.1f%% exceeds limit", percent);
        return -1;
    }
#endif
    
    return 0;
}

// Disk проверка
static int disk_health_check(const char *path, int max_percent, 
                             char *error, size_t error_size) {
    if (!path) {
        snprintf(error, error_size, "Invalid path");
        return -1;
    }
    
#ifdef _WIN32
    ULARGE_INTEGER free, total, total_free;
    if (!GetDiskFreeSpaceExA(path, &free, &total, &total_free)) {
        snprintf(error, error_size, "Failed to get disk info for %s", path);
        return -1;
    }
    
    uint64_t used = total.QuadPart - free.QuadPart;
    double percent = (double)used / total.QuadPart * 100.0;
#else
    struct statvfs buf;
    if (statvfs(path, &buf) != 0) {
        snprintf(error, error_size, "Failed to get disk info for %s", path);
        return -1;
    }
    
    uint64_t total = buf.f_blocks * buf.f_frsize;
    uint64_t free = buf.f_bfree * buf.f_frsize;
    uint64_t used = total - free;
    double percent = (double)used / total * 100.0;
#endif
    
    if ((int)percent > max_percent) {
        snprintf(error, error_size, "Disk usage %.1f%% exceeds limit %d%%", 
                 percent, max_percent);
        return -1;
    }
    
    return 0;
}

// ============================================================================
// Инициализация и очистка
// ============================================================================

int health_check_init(void) {
    if (g_health_check.initialized) {
        return 0;
    }
    
    memset(&g_health_check, 0, sizeof(g_health_check));
    g_health_check.initialized = true;
    g_health_check.start_time = get_current_time_ms();
    
    kprintf("[HEALTH] Health Check system initialized\n");
    return 0;
}

void health_check_cleanup(void) {
    if (!g_health_check.initialized) {
        return;
    }
    
    health_check_stop();
    
    memset(&g_health_check, 0, sizeof(g_health_check));
    kprintf("[HEALTH] Health Check system cleaned up\n");
}

bool health_check_is_initialized(void) {
    return g_health_check.initialized;
}

// ============================================================================
// Управление узлами
// ============================================================================

int health_check_add_http_node(const char *name, const char *url, int timeout_ms) {
    if (!g_health_check.initialized || !name || !url) {
        return -1;
    }
    
    if (g_health_check.node_count >= HEALTH_MAX_NODES) {
        kprintf("[HEALTH] Error: Maximum nodes reached\n");
        return -1;
    }
    
    if (find_node(name) != NULL) {
        kprintf("[HEALTH] Error: Node already exists: %s\n", name);
        return -1;
    }
    
    health_check_config_t *node = &g_health_check.nodes[g_health_check.node_count];
    memset(node, 0, sizeof(health_check_config_t));
    
    snprintf(node->name, sizeof(node->name), "%s", name);
    snprintf(node->url, sizeof(node->url), "%s", url);
    node->type = (strncmp(url, "https://", 8) == 0) ? HEALTH_CHECK_HTTPS : HEALTH_CHECK_HTTP;
    node->timeout_ms = timeout_ms > 0 ? timeout_ms : 5000;
    node->interval_ms = 10000;
    node->unhealthy_threshold = 3;
    node->healthy_threshold = 2;
    node->enabled = true;
    
    // Инициализация результата
    health_check_result_t *result = &g_health_check.results[g_health_check.node_count];
    memset(result, 0, sizeof(health_check_result_t));
    snprintf(result->name, sizeof(result->name), "%s", name);
    result->status = HEALTH_STATUS_UNKNOWN;
    
    g_health_check.node_count++;
    g_health_check.stats.total_nodes++;
    
    kprintf("[HEALTH] HTTP node added: %s -> %s\n", name, url);
    return 0;
}

int health_check_add_tcp_node(const char *name, const char *host, int port, int timeout_ms) {
    if (!g_health_check.initialized || !name || !host) {
        return -1;
    }
    
    if (g_health_check.node_count >= HEALTH_MAX_NODES || find_node(name)) {
        return -1;
    }
    
    health_check_config_t *node = &g_health_check.nodes[g_health_check.node_count];
    memset(node, 0, sizeof(health_check_config_t));
    
    snprintf(node->name, sizeof(node->name), "%s", name);
    snprintf(node->url, sizeof(node->url), "tcp://%s:%d", host, port);
    node->type = HEALTH_CHECK_TCP;
    node->timeout_ms = timeout_ms > 0 ? timeout_ms : 3000;
    node->interval_ms = 5000;
    node->unhealthy_threshold = 3;
    node->healthy_threshold = 2;
    node->enabled = true;
    
    health_check_result_t *result = &g_health_check.results[g_health_check.node_count];
    memset(result, 0, sizeof(health_check_result_t));
    snprintf(result->name, sizeof(result->name), "%s", name);
    result->status = HEALTH_STATUS_UNKNOWN;
    
    g_health_check.node_count++;
    g_health_check.stats.total_nodes++;
    
    kprintf("[HEALTH] TCP node added: %s -> %s:%d\n", name, host, port);
    return 0;
}

int health_check_add_process_node(const char *name, int pid) {
    if (!g_health_check.initialized || !name || pid <= 0) {
        return -1;
    }
    
    if (g_health_check.node_count >= HEALTH_MAX_NODES || find_node(name)) {
        return -1;
    }
    
    health_check_config_t *node = &g_health_check.nodes[g_health_check.node_count];
    memset(node, 0, sizeof(health_check_config_t));
    
    snprintf(node->name, sizeof(node->name), "%s", name);
    snprintf(node->metadata, sizeof(node->metadata), "%d", pid);
    node->type = HEALTH_CHECK_PROCESS;
    node->interval_ms = 5000;
    node->enabled = true;
    
    health_check_result_t *result = &g_health_check.results[g_health_check.node_count];
    memset(result, 0, sizeof(health_check_result_t));
    snprintf(result->name, sizeof(result->name), "%s", name);
    result->status = HEALTH_STATUS_UNKNOWN;
    
    g_health_check.node_count++;
    g_health_check.stats.total_nodes++;
    
    kprintf("[HEALTH] Process node added: %s (PID: %d)\n", name, pid);
    return 0;
}

int health_check_add_memory_node(const char *name, uint64_t max_memory_bytes) {
    if (!g_health_check.initialized || !name) {
        return -1;
    }
    
    if (g_health_check.node_count >= HEALTH_MAX_NODES || find_node(name)) {
        return -1;
    }
    
    health_check_config_t *node = &g_health_check.nodes[g_health_check.node_count];
    memset(node, 0, sizeof(health_check_config_t));
    
    snprintf(node->name, sizeof(node->name), "%s", name);
    snprintf(node->metadata, sizeof(node->metadata), "%llu", (unsigned long long)max_memory_bytes);
    node->type = HEALTH_CHECK_MEMORY;
    node->interval_ms = 10000;
    node->enabled = true;
    
    health_check_result_t *result = &g_health_check.results[g_health_check.node_count];
    memset(result, 0, sizeof(health_check_result_t));
    snprintf(result->name, sizeof(result->name), "%s", name);
    result->status = HEALTH_STATUS_UNKNOWN;
    
    g_health_check.node_count++;
    g_health_check.stats.total_nodes++;
    
    kprintf("[HEALTH] Memory node added: %s (max: %llu bytes)\n", 
            name, (unsigned long long)max_memory_bytes);
    return 0;
}

int health_check_add_disk_node(const char *name, const char *path, int max_percent) {
    if (!g_health_check.initialized || !name || !path) {
        return -1;
    }
    
    if (g_health_check.node_count >= HEALTH_MAX_NODES || find_node(name)) {
        return -1;
    }
    
    health_check_config_t *node = &g_health_check.nodes[g_health_check.node_count];
    memset(node, 0, sizeof(health_check_config_t));
    
    snprintf(node->name, sizeof(node->name), "%s", name);
    snprintf(node->url, sizeof(node->url), "%s", path);
    snprintf(node->metadata, sizeof(node->metadata), "%d", max_percent);
    node->type = HEALTH_CHECK_DISK;
    node->interval_ms = 60000;
    node->enabled = true;
    
    health_check_result_t *result = &g_health_check.results[g_health_check.node_count];
    memset(result, 0, sizeof(health_check_result_t));
    snprintf(result->name, sizeof(result->name), "%s", name);
    result->status = HEALTH_STATUS_UNKNOWN;
    
    g_health_check.node_count++;
    g_health_check.stats.total_nodes++;
    
    kprintf("[HEALTH] Disk node added: %s (path: %s, max: %d%%)\n", name, path, max_percent);
    return 0;
}

int health_check_add_custom_node(const char *name, const char *metadata, 
                                  health_check_callback_t callback) {
    if (!g_health_check.initialized || !name || !callback) {
        return -1;
    }
    
    if (g_health_check.node_count >= HEALTH_MAX_NODES || find_node(name)) {
        return -1;
    }
    
    health_check_config_t *node = &g_health_check.nodes[g_health_check.node_count];
    memset(node, 0, sizeof(health_check_config_t));
    
    snprintf(node->name, sizeof(node->name), "%s", name);
    snprintf(node->metadata, sizeof(node->metadata), "%s", metadata ? metadata : "");
    node->type = HEALTH_CHECK_CUSTOM;
    node->interval_ms = 10000;
    node->enabled = true;
    
    health_check_result_t *result = &g_health_check.results[g_health_check.node_count];
    memset(result, 0, sizeof(health_check_result_t));
    snprintf(result->name, sizeof(result->name), "%s", name);
    result->status = HEALTH_STATUS_UNKNOWN;
    
    g_health_check.node_count++;
    g_health_check.stats.total_nodes++;
    
    kprintf("[HEALTH] Custom node added: %s\n", name);
    return 0;
}

int health_check_remove_node(const char *name) {
    if (!g_health_check.initialized || !name) {
        return -1;
    }
    
    int idx = -1;
    for (int i = 0; i < g_health_check.node_count; i++) {
        if (strcmp(g_health_check.nodes[i].name, name) == 0) {
            idx = i;
            break;
        }
    }
    
    if (idx < 0) return -1;
    
    // Сдвиг массивов
    for (int i = idx; i < g_health_check.node_count - 1; i++) {
        g_health_check.nodes[i] = g_health_check.nodes[i + 1];
        g_health_check.results[i] = g_health_check.results[i + 1];
    }
    
    g_health_check.node_count--;
    g_health_check.stats.total_nodes--;
    
    kprintf("[HEALTH] Node removed: %s\n", name);
    return 0;
}

int health_check_set_node_enabled(const char *name, bool enabled) {
    health_check_config_t *node = find_node(name);
    if (!node) return -1;
    
    node->enabled = enabled;
    kprintf("[HEALTH] Node %s %s\n", name, enabled ? "enabled" : "disabled");
    return 0;
}

int health_check_update_node_config(const char *name, const health_check_config_t *config) {
    if (!g_health_check.initialized || !name || !config) {
        return -1;
    }
    
    health_check_config_t *node = find_node(name);
    if (!node) return -1;
    
    memcpy(node, config, sizeof(health_check_config_t));
    snprintf(node->name, sizeof(node->name), "%s", name);
    
    kprintf("[HEALTH] Node config updated: %s\n", name);
    return 0;
}

// ============================================================================
// Выполнение проверок
// ============================================================================

static int perform_health_check(health_check_config_t *node, health_check_result_t *result) {
    if (!node || !result) return -1;
    
    result->check_id++;
    result->last_check_time = get_current_time_ms();
    
    int check_result = -1;
    int64_t start = get_current_time_ms();
    
    switch (node->type) {
        case HEALTH_CHECK_HTTP:
        case HEALTH_CHECK_HTTPS:
            check_result = http_health_check(
                node->url, node->timeout_ms,
                result->error_message, sizeof(result->error_message),
                result->response_body, sizeof(result->response_body),
                &result->http_status_code
            );
            break;
            
        case HEALTH_CHECK_TCP: {
            char host[256] = {0};
            int port = 0;
            // Парсинг tcp://host:port
            if (sscanf(node->url, "tcp://%[^:]:%d", host, &port) == 2) {
                check_result = tcp_health_check(host, port, node->timeout_ms,
                                                result->error_message, 
                                                sizeof(result->error_message));
            }
            break;
        }
        
        case HEALTH_CHECK_PROCESS: {
            int pid = atoi(node->metadata);
            check_result = process_health_check(pid, result->error_message, 
                                                 sizeof(result->error_message));
            break;
        }
        
        case HEALTH_CHECK_MEMORY: {
            uint64_t max_mem = strtoull(node->metadata, NULL, 10);
            uint64_t current_mem = 0;
            check_result = memory_health_check(max_mem, result->error_message,
                                                sizeof(result->error_message), &current_mem);
            break;
        }
        
        case HEALTH_CHECK_DISK: {
            int max_percent = atoi(node->metadata);
            check_result = disk_health_check(node->url, max_percent,
                                              result->error_message,
                                              sizeof(result->error_message));
            break;
        }
        
        case HEALTH_CHECK_CUSTOM: {
            // Вызов callback функции
            // В реальной реализации нужен глобальный callback registry
            check_result = 0;  // Заглушка
            break;
        }
    }
    
    result->response_time_ms = (int)(get_current_time_ms() - start);
    
    // Обновление счётчиков
    if (check_result == 0) {
        result->consecutive_successes++;
        result->consecutive_failures = 0;
        result->last_success_time = result->last_check_time;
        
        if (result->status == HEALTH_STATUS_UNHEALTHY && 
            result->consecutive_successes >= node->healthy_threshold) {
            result->status = HEALTH_STATUS_HEALTHY;
            kprintf("[HEALTH] Node %s recovered\n", node->name);
        } else if (result->status == HEALTH_STATUS_UNKNOWN) {
            result->status = HEALTH_STATUS_HEALTHY;
        }
    } else {
        result->consecutive_failures++;
        result->consecutive_successes = 0;
        result->last_failure_time = result->last_check_time;
        
        if (result->consecutive_failures >= node->unhealthy_threshold) {
            if (result->status != HEALTH_STATUS_UNHEALTHY) {
                result->status = HEALTH_STATUS_UNHEALTHY;
                kprintf("[HEALTH] Node %s is unhealthy: %s\n", node->name, result->error_message);
            }
        } else {
            result->status = HEALTH_STATUS_DEGRADED;
        }
    }
    
    return check_result;
}

static void run_all_checks(void) {
    for (int i = 0; i < g_health_check.node_count; i++) {
        health_check_config_t *node = &g_health_check.nodes[i];
        health_check_result_t *result = &g_health_check.results[i];
        
        if (!node->enabled) continue;
        
        int check_result = perform_health_check(node, result);
        
        g_health_check.stats.total_checks++;
        if (check_result == 0) {
            g_health_check.stats.successful_checks++;
        } else {
            g_health_check.stats.failed_checks++;
        }
    }
    
    g_health_check.stats.last_check_time = get_current_time_ms();
    
    // Обновление сводной статистики
    g_health_check.stats.healthy_nodes = 0;
    g_health_check.stats.unhealthy_nodes = 0;
    g_health_check.stats.degraded_nodes = 0;
    
    int total_response_time = 0;
    for (int i = 0; i < g_health_check.node_count; i++) {
        health_check_result_t *result = &g_health_check.results[i];
        total_response_time += result->response_time_ms;
        
        switch (result->status) {
            case HEALTH_STATUS_HEALTHY:
                g_health_check.stats.healthy_nodes++;
                break;
            case HEALTH_STATUS_UNHEALTHY:
                g_health_check.stats.unhealthy_nodes++;
                break;
            case HEALTH_STATUS_DEGRADED:
                g_health_check.stats.degraded_nodes++;
                break;
            default:
                break;
        }
    }
    
    if (g_health_check.stats.total_checks > 0) {
        g_health_check.stats.avg_response_time_ms = 
            (double)total_response_time / g_health_check.node_count;
    }
}

#ifdef _WIN32
static DWORD WINAPI health_check_thread_func(LPVOID arg)
#else
static void* health_check_thread_func(void *arg)
#endif
{
    kprintf("[HEALTH] Health check thread started (interval: %d ms)\n", 
            g_health_check.check_interval_ms);
    
    while (g_health_check.running) {
#ifdef _WIN32
        Sleep(g_health_check.check_interval_ms);
#else
        struct timespec ts;
        ts.tv_sec = g_health_check.check_interval_ms / 1000;
        ts.tv_nsec = (g_health_check.check_interval_ms % 1000) * 1000000;
        nanosleep(&ts, NULL);
#endif
        
        if (g_health_check.running) {
            run_all_checks();
        }
    }
    
    kprintf("[HEALTH] Health check thread stopped\n");
    return 0;
}

// ============================================================================
// Запуск и остановка
// ============================================================================

int health_check_start(int interval_ms) {
    if (!g_health_check.initialized) {
        return -1;
    }
    
    if (g_health_check.running) {
        kprintf("[HEALTH] Health checks already running\n");
        return 0;
    }
    
    g_health_check.check_interval_ms = interval_ms > 0 ? interval_ms : 10000;
    g_health_check.running = true;
    
#ifdef _WIN32
    g_health_check.check_thread = CreateThread(NULL, 0, health_check_thread_func, NULL, 0, NULL);
    if (!g_health_check.check_thread) {
        g_health_check.running = false;
        return -1;
    }
#else
    if (pthread_create(&g_health_check.check_thread, NULL, 
                       health_check_thread_func, NULL) != 0) {
        g_health_check.running = false;
        return -1;
    }
#endif
    
    kprintf("[HEALTH] Health checks started\n");
    return 0;
}

void health_check_stop(void) {
    if (!g_health_check.running) {
        return;
    }
    
    g_health_check.running = false;
    
#ifdef _WIN32
    if (g_health_check.check_thread) {
        WaitForSingleObject(g_health_check.check_thread, INFINITE);
        CloseHandle(g_health_check.check_thread);
        g_health_check.check_thread = NULL;
    }
#else
    pthread_join(g_health_check.check_thread, NULL);
#endif
    
    kprintf("[HEALTH] Health checks stopped\n");
}

int health_check_run_once(void) {
    if (!g_health_check.initialized) {
        return -1;
    }
    
    kprintf("[HEALTH] Running one-time health check\n");
    run_all_checks();
    return 0;
}

const health_check_result_t* health_check_check_node(const char *name) {
    if (!g_health_check.initialized || !name) {
        return NULL;
    }
    
    health_check_config_t *node = find_node(name);
    health_check_result_t *result = find_result(name);
    
    if (!node || !result) {
        return NULL;
    }
    
    perform_health_check(node, result);
    return result;
}

// ============================================================================
// Получение статуса
// ============================================================================

health_status_t health_check_get_node_status(const char *name) {
    health_check_result_t *result = find_result(name);
    if (!result) return HEALTH_STATUS_UNKNOWN;
    return result->status;
}

const health_check_result_t* health_check_get_node_result(const char *name) {
    return find_result(name);
}

int health_check_get_healthy_count(void) {
    return g_health_check.stats.healthy_nodes;
}

int health_check_get_total_count(void) {
    return g_health_check.node_count;
}

bool health_check_all_healthy(void) {
    for (int i = 0; i < g_health_check.node_count; i++) {
        if (g_health_check.results[i].status != HEALTH_STATUS_HEALTHY) {
            return false;
        }
    }
    return g_health_check.node_count > 0;
}

int health_check_get_unhealthy_nodes(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return -1;
    
    int count = 0;
    int offset = 0;
    
    for (int i = 0; i < g_health_check.node_count; i++) {
        if (g_health_check.results[i].status != HEALTH_STATUS_HEALTHY) {
            if (count > 0) {
                offset += snprintf(buffer + offset, buffer_size - offset, ", ");
            }
            offset += snprintf(buffer + offset, buffer_size - offset, "%s", 
                               g_health_check.nodes[i].name);
            count++;
        }
    }
    
    return count;
}

// ============================================================================
// Статистика
// ============================================================================

int health_check_get_stats(health_check_stats_t *stats) {
    if (!g_health_check.initialized || !stats) {
        return -1;
    }
    
    memcpy(stats, &g_health_check.stats, sizeof(health_check_stats_t));
    return 0;
}

int health_check_get_stats_string(char *buffer, size_t buffer_size) {
    if (!g_health_check.initialized || !buffer || buffer_size < 256) {
        return -1;
    }
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "Health Check Statistics:\n");
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Total Nodes: %d (Healthy: %d, Unhealthy: %d, Degraded: %d)\n",
                       g_health_check.stats.total_nodes,
                       g_health_check.stats.healthy_nodes,
                       g_health_check.stats.unhealthy_nodes,
                       g_health_check.stats.degraded_nodes);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Total Checks: %llu (Success: %llu, Failed: %llu)\n",
                       (unsigned long long)g_health_check.stats.total_checks,
                       (unsigned long long)g_health_check.stats.successful_checks,
                       (unsigned long long)g_health_check.stats.failed_checks);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Avg Response Time: %.2f ms\n",
                       g_health_check.stats.avg_response_time_ms);
    
    return 0;
}

void health_check_reset_stats(void) {
    if (!g_health_check.initialized) return;
    
    memset(&g_health_check.stats, 0, sizeof(g_health_check.stats));
    g_health_check.stats.total_nodes = g_health_check.node_count;
    kprintf("[HEALTH] Statistics reset\n");
}

int64_t health_check_get_uptime(void) {
    if (!g_health_check.initialized) return 0;
    return get_current_time_ms() - g_health_check.start_time;
}

// ============================================================================
// Alert Manager интеграция
// ============================================================================

int health_check_enable_alerts(bool alert_on_unhealthy, bool alert_on_recovery) {
    if (!g_health_check.initialized) {
        return -1;
    }
    
    g_health_check.alerts_enabled = true;
    g_health_check.alert_on_unhealthy = alert_on_unhealthy;
    g_health_check.alert_on_recovery = alert_on_recovery;
    
    kprintf("[HEALTH] Alerts enabled (unhealthy: %s, recovery: %s)\n",
            alert_on_unhealthy ? "yes" : "no", alert_on_recovery ? "yes" : "no");
    
    // Здесь должна быть интеграция с alert_manager
    // Для простоты - заглушка
    
    return 0;
}

void health_check_disable_alerts(void) {
    g_health_check.alerts_enabled = false;
    kprintf("[HEALTH] Alerts disabled\n");
}

// ============================================================================
// Утилиты
// ============================================================================

const char* health_status_to_string(health_status_t status) {
    switch (status) {
        case HEALTH_STATUS_UNKNOWN: return "Unknown";
        case HEALTH_STATUS_HEALTHY: return "Healthy";
        case HEALTH_STATUS_UNHEALTHY: return "Unhealthy";
        case HEALTH_STATUS_DEGRADED: return "Degraded";
        case HEALTH_STATUS_TIMEOUT: return "Timeout";
        default: return "Unknown";
    }
}

const char* health_check_type_to_string(health_check_type_t type) {
    switch (type) {
        case HEALTH_CHECK_HTTP: return "HTTP";
        case HEALTH_CHECK_HTTPS: return "HTTPS";
        case HEALTH_CHECK_TCP: return "TCP";
        case HEALTH_CHECK_PROCESS: return "Process";
        case HEALTH_CHECK_MEMORY: return "Memory";
        case HEALTH_CHECK_DISK: return "Disk";
        case HEALTH_CHECK_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

int health_check_get_cluster_health_percent(void) {
    if (g_health_check.node_count == 0) return 0;
    
    return (g_health_check.stats.healthy_nodes * 100) / g_health_check.node_count;
}
