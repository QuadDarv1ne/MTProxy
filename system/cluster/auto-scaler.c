/*
    MTProxy Auto-Scaler
    Реализация системы автоматического масштабирования
*/

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

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
    #include <pthread.h>
    #include <sys/resource.h>
#endif

#include "system/cluster/auto-scaler.h"
#include "common/utils.h"

// ============================================================================
// Глобальные переменные
// ============================================================================

static struct {
    bool initialized;
    bool running;
    auto_scaler_config_t config;
    auto_scaler_stats_t stats;
    
    auto_scaler_metric_config_t metrics[AUTO_SCALER_MAX_METRICS];
    auto_scaler_metric_status_t metric_status[AUTO_SCALER_MAX_METRICS];
    int metric_count;
    
    auto_scaler_scale_up_callback_t scale_up_callback;
    auto_scaler_scale_down_callback_t scale_down_callback;
    auto_scaler_metric_reader_t metric_reader;
    
    int64_t last_scale_up_time;
    int64_t last_scale_down_time;
    
#ifdef _WIN32
    HANDLE monitor_thread;
#else
    pthread_t monitor_thread;
#endif
} g_auto_scaler = {0};

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

static auto_scaler_metric_config_t* find_metric(auto_scaler_metric_type_t type) {
    for (int i = 0; i < g_auto_scaler.metric_count; i++) {
        if (g_auto_scaler.metrics[i].type == type) {
            return &g_auto_scaler.metrics[i];
        }
    }
    return NULL;
}

static double read_system_cpu_usage(void) {
#ifdef _WIN32
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user)) {
        return 0.0;
    }
    
    ULARGE_INTEGER i, k, u;
    i.LowPart = idle.dwLowDateTime;
    i.HighPart = idle.dwHighDateTime;
    k.LowPart = kernel.dwLowDateTime;
    k.HighPart = kernel.dwHighDateTime;
    u.LowPart = user.dwLowDateTime;
    u.HighPart = user.dwHighDateTime;
    
    uint64_t total = (k.QuadPart - i.QuadPart) + u.QuadPart;
    if (total == 0) return 0.0;
    
    double usage = (double)(k.QuadPart - i.QuadPart) / total * 100.0;
    return usage;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0.0;
    }
    
    // Упрощённая оценка
    return (double)usage.ru_utime.tv_sec % 100;
#endif
}

static double read_system_memory_usage(void) {
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status)) {
        return 0.0;
    }
    
    uint64_t used = status.ullTotalPhys - status.ullAvailPhys;
    return (double)used / status.ullTotalPhys * 100.0;
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long avail = sysconf(_SC_AVPHYS_PAGES);
    if (pages <= 0 || avail <= 0) return 0.0;
    
    return (double)(pages - avail) / pages * 100.0;
#endif
}

static void update_metric_status(auto_scaler_metric_config_t *metric, double value) {
    auto_scaler_metric_status_t *status = NULL;
    
    for (int i = 0; i < g_auto_scaler.metric_count; i++) {
        if (g_auto_scaler.metric_status[i].type == metric->type) {
            status = &g_auto_scaler.metric_status[i];
            break;
        }
    }
    
    if (!status) {
        status = &g_auto_scaler.metric_status[g_auto_scaler.metric_count++];
        status->type = metric->type;
        snprintf(status->name, sizeof(status->name), "%s", metric->name);
        status->min_value = value;
        status->max_value = value;
    }
    
    status->current_value = value;
    status->avg_value = (status->avg_value * 0.9) + (value * 0.1);
    if (value > status->max_value) status->max_value = value;
    if (value < status->min_value) status->min_value = value;
    status->last_updated = get_current_time_ms();
    
    // Проверка порогов
    status->triggered = (value >= metric->scale_up_threshold);
}

static auto_scaler_scale_direction_t evaluate_scaling(void) {
    if (g_auto_scaler.metric_count == 0) {
        return AUTO_SCALER_SCALE_NONE;
    }
    
    int scale_up_votes = 0;
    int scale_down_votes = 0;
    int total_weight = 0;
    
    for (int i = 0; i < g_auto_scaler.metric_count; i++) {
        auto_scaler_metric_config_t *metric = &g_auto_scaler.metrics[i];
        auto_scaler_metric_status_t *status = &g_auto_scaler.metric_status[i];
        
        if (!metric->enabled) continue;
        
        int weight = metric->weight > 0 ? metric->weight : 1;
        total_weight += weight;
        
        if (status->current_value >= metric->scale_up_threshold) {
            scale_up_votes += weight;
        } else if (status->current_value <= metric->scale_down_threshold) {
            scale_down_votes += weight;
        }
    }
    
    if (total_weight == 0) return AUTO_SCALER_SCALE_NONE;
    
    // Определение направления на основе голосов
    double scale_up_ratio = (double)scale_up_votes / total_weight;
    double scale_down_ratio = (double)scale_down_votes / total_weight;
    
    // Применение политики
    double threshold = 0.5;
    switch (g_auto_scaler.config.policy) {
        case AUTO_SCALER_POLICY_CONSERVATIVE:
            threshold = 0.8;  // Требуется 80% голосов
            break;
        case AUTO_SCALER_POLICY_MODERATE:
            threshold = 0.6;  // Требуется 60% голосов
            break;
        case AUTO_SCALER_POLICY_AGGRESSIVE:
            threshold = 0.4;  // Достаточно 40% голосов
            break;
        default:
            break;
    }
    
    if (scale_up_ratio >= threshold) {
        return AUTO_SCALER_SCALE_UP;
    } else if (scale_down_ratio >= threshold) {
        return AUTO_SCALER_SCALE_DOWN;
    }
    
    return AUTO_SCALER_SCALE_NONE;
}

static int perform_scale_up(void) {
    int64_t now = get_current_time_ms() / 1000;
    
    // Проверка cooldown
    if (g_auto_scaler.last_scale_up_time > 0) {
        if ((now - g_auto_scaler.last_scale_up_time) < g_auto_scaler.config.scale_up_cooldown_sec) {
            kprintf("[AUTOSCALE] Scale up on cooldown\n");
            return -1;
        }
    }
    
    // Проверка лимита
    if (g_auto_scaler.config.current_nodes >= g_auto_scaler.config.max_nodes) {
        kprintf("[AUTOSCALE] Already at max nodes: %d\n", g_auto_scaler.config.max_nodes);
        return -1;
    }
    
    int desired = g_auto_scaler.config.current_nodes + 1;
    kprintf("[AUTOSCALE] Scaling up: %d -> %d nodes\n", 
            g_auto_scaler.config.current_nodes, desired);
    
    // Вызов callback
    if (g_auto_scaler.scale_up_callback) {
        int result = g_auto_scaler.scale_up_callback(g_auto_scaler.config.current_nodes, desired);
        if (result == 0) {
            g_auto_scaler.config.current_nodes = desired;
            g_auto_scaler.stats.total_scale_up_events++;
            g_auto_scaler.stats.last_scale_time = now;
            g_auto_scaler.stats.last_scale_direction = AUTO_SCALER_SCALE_UP;
            g_auto_scaler.last_scale_up_time = now;
            
            if (desired > g_auto_scaler.stats.peak_nodes) {
                g_auto_scaler.stats.peak_nodes = desired;
            }
            
            return 0;
        }
        g_auto_scaler.stats.failed_scale_events++;
        return -1;
    }
    
    return -1;
}

static int perform_scale_down(void) {
    int64_t now = get_current_time_ms() / 1000;
    
    // Проверка cooldown
    if (g_auto_scaler.last_scale_down_time > 0) {
        if ((now - g_auto_scaler.last_scale_down_time) < g_auto_scaler.config.scale_down_cooldown_sec) {
            kprintf("[AUTOSCALE] Scale down on cooldown\n");
            return -1;
        }
    }
    
    // Проверка лимита
    if (g_auto_scaler.config.current_nodes <= g_auto_scaler.config.min_nodes) {
        kprintf("[AUTOSCALE] Already at min nodes: %d\n", g_auto_scaler.config.min_nodes);
        return -1;
    }
    
    int desired = g_auto_scaler.config.current_nodes - 1;
    kprintf("[AUTOSCALE] Scaling down: %d -> %d nodes\n", 
            g_auto_scaler.config.current_nodes, desired);
    
    // Вызов callback
    if (g_auto_scaler.scale_down_callback) {
        int result = g_auto_scaler.scale_down_callback(g_auto_scaler.config.current_nodes, desired);
        if (result == 0) {
            g_auto_scaler.config.current_nodes = desired;
            g_auto_scaler.stats.total_scale_down_events++;
            g_auto_scaler.stats.last_scale_time = now;
            g_auto_scaler.stats.last_scale_direction = AUTO_SCALER_SCALE_DOWN;
            g_auto_scaler.last_scale_down_time = now;
            return 0;
        }
        g_auto_scaler.stats.failed_scale_events++;
        return -1;
    }
    
    return -1;
}

#ifdef _WIN32
static DWORD WINAPI monitor_thread_func(LPVOID arg)
#else
static void* monitor_thread_func(void *arg)
#endif
{
    kprintf("[AUTOSCALE] Monitor thread started (interval: %d sec)\n",
            g_auto_scaler.config.check_interval_sec);
    
    while (g_auto_scaler.running) {
#ifdef _WIN32
        Sleep(g_auto_scaler.config.check_interval_sec * 1000);
#else
        sleep(g_auto_scaler.config.check_interval_sec);
#endif
        
        if (g_auto_scaler.running) {
            auto_scaler_check_and_scale();
        }
    }
    
    kprintf("[AUTOSCALE] Monitor thread stopped\n");
    return 0;
}

// ============================================================================
// Инициализация и очистка
// ============================================================================

int auto_scaler_init(const char *cluster_name) {
    if (!cluster_name) {
        return -1;
    }
    
    if (g_auto_scaler.initialized) {
        return 0;
    }
    
    memset(&g_auto_scaler, 0, sizeof(g_auto_scaler));
    
    snprintf(g_auto_scaler.config.cluster_name, 
             sizeof(g_auto_scaler.config.cluster_name), "%s", cluster_name);
    
    g_auto_scaler.config.policy = AUTO_SCALER_POLICY_MODERATE;
    g_auto_scaler.config.min_nodes = 1;
    g_auto_scaler.config.max_nodes = 10;
    g_auto_scaler.config.current_nodes = 1;
    g_auto_scaler.config.scale_up_cooldown_sec = 300;  // 5 минут
    g_auto_scaler.config.scale_down_cooldown_sec = 600;  // 10 минут
    g_auto_scaler.config.check_interval_sec = 30;  // 30 секунд
    g_auto_scaler.config.enabled = true;
    
    g_auto_scaler.stats.start_time = get_current_time_ms();
    
    // Добавление метрик по умолчанию
    auto_scaler_add_metric(AUTO_SCALER_METRIC_CPU, 80.0, 30.0);
    auto_scaler_add_metric(AUTO_SCALER_METRIC_MEMORY, 85.0, 40.0);
    auto_scaler_add_metric(AUTO_SCALER_METRIC_CONNECTIONS, 8000, 2000);
    
    g_auto_scaler.initialized = true;
    
    kprintf("[AUTOSCALE] Initialized for cluster: %s\n", cluster_name);
    return 0;
}

void auto_scaler_cleanup(void) {
    if (!g_auto_scaler.initialized) {
        return;
    }
    
    auto_scaler_stop();
    
    memset(&g_auto_scaler, 0, sizeof(g_auto_scaler));
    kprintf("[AUTOSCALE] Cleaned up\n");
}

bool auto_scaler_is_initialized(void) {
    return g_auto_scaler.initialized;
}

// ============================================================================
// Конфигурация
// ============================================================================

int auto_scaler_set_policy(auto_scaler_policy_t policy) {
    if (!g_auto_scaler.initialized) {
        return -1;
    }
    
    g_auto_scaler.config.policy = policy;
    kprintf("[AUTOSCALE] Policy set to: %s\n", 
            auto_scaler_policy_to_string(policy));
    return 0;
}

int auto_scaler_set_limits(int min_nodes, int max_nodes) {
    if (!g_auto_scaler.initialized || min_nodes <= 0 || max_nodes < min_nodes) {
        return -1;
    }
    
    g_auto_scaler.config.min_nodes = min_nodes;
    g_auto_scaler.config.max_nodes = max_nodes;
    
    // Корректировка текущего количества
    if (g_auto_scaler.config.current_nodes < min_nodes) {
        g_auto_scaler.config.current_nodes = min_nodes;
    } else if (g_auto_scaler.config.current_nodes > max_nodes) {
        g_auto_scaler.config.current_nodes = max_nodes;
    }
    
    kprintf("[AUTOSCALE] Limits set: min=%d, max=%d\n", min_nodes, max_nodes);
    return 0;
}

void auto_scaler_set_check_interval(int interval_sec) {
    g_auto_scaler.config.check_interval_sec = interval_sec > 0 ? interval_sec : 30;
    kprintf("[AUTOSCALE] Check interval set to: %d sec\n", 
            g_auto_scaler.config.check_interval_sec);
}

int auto_scaler_add_metric(auto_scaler_metric_type_t type,
                            double scale_up_threshold,
                            double scale_down_threshold) {
    if (!g_auto_scaler.initialized || g_auto_scaler.metric_count >= AUTO_SCALER_MAX_METRICS) {
        return -1;
    }
    
    if (find_metric(type) != NULL) {
        kprintf("[AUTOSCALE] Metric already exists: %d\n", type);
        return -1;
    }
    
    auto_scaler_metric_config_t *metric = &g_auto_scaler.metrics[g_auto_scaler.metric_count];
    memset(metric, 0, sizeof(auto_scaler_metric_config_t));
    
    metric->type = type;
    snprintf(metric->name, sizeof(metric->name), "%s", 
             auto_scaler_metric_type_to_string(type));
    metric->scale_up_threshold = scale_up_threshold;
    metric->scale_down_threshold = scale_down_threshold;
    metric->cooldown_seconds = 300;
    metric->weight = 1;
    metric->enabled = true;
    
    g_auto_scaler.metric_count++;
    
    kprintf("[AUTOSCALE] Metric added: %s (up=%.1f, down=%.1f)\n",
            metric->name, scale_up_threshold, scale_down_threshold);
    return 0;
}

int auto_scaler_configure_metric(auto_scaler_metric_type_t type,
                                  const auto_scaler_metric_config_t *config) {
    if (!g_auto_scaler.initialized || !config) {
        return -1;
    }
    
    auto_scaler_metric_config_t *metric = find_metric(type);
    if (!metric) {
        return -1;
    }
    
    memcpy(metric, config, sizeof(auto_scaler_metric_config_t));
    kprintf("[AUTOSCALE] Metric configured: %s\n", metric->name);
    return 0;
}

// ============================================================================
// Запуск и остановка
// ============================================================================

int auto_scaler_start(void) {
    if (!g_auto_scaler.initialized) {
        return -1;
    }
    
    if (g_auto_scaler.running) {
        kprintf("[AUTOSCALE] Already running\n");
        return 0;
    }
    
    g_auto_scaler.running = true;
    
#ifdef _WIN32
    g_auto_scaler.monitor_thread = CreateThread(NULL, 0, monitor_thread_func, NULL, 0, NULL);
    if (!g_auto_scaler.monitor_thread) {
        g_auto_scaler.running = false;
        return -1;
    }
#else
    if (pthread_create(&g_auto_scaler.monitor_thread, NULL, monitor_thread_func, NULL) != 0) {
        g_auto_scaler.running = false;
        return -1;
    }
#endif
    
    kprintf("[AUTOSCALE] Started\n");
    return 0;
}

void auto_scaler_stop(void) {
    if (!g_auto_scaler.running) {
        return;
    }
    
    g_auto_scaler.running = false;
    
#ifdef _WIN32
    if (g_auto_scaler.monitor_thread) {
        WaitForSingleObject(g_auto_scaler.monitor_thread, INFINITE);
        CloseHandle(g_auto_scaler.monitor_thread);
    }
#else
    pthread_join(g_auto_scaler.monitor_thread, NULL);
#endif
    
    kprintf("[AUTOSCALE] Stopped\n");
}

bool auto_scaler_is_running(void) {
    return g_auto_scaler.running;
}

auto_scaler_scale_direction_t auto_scaler_check_and_scale(void) {
    if (!g_auto_scaler.initialized || !g_auto_scaler.config.enabled) {
        return AUTO_SCALER_SCALE_NONE;
    }
    
    // Чтение метрик
    for (int i = 0; i < g_auto_scaler.metric_count; i++) {
        auto_scaler_metric_config_t *metric = &g_auto_scaler.metrics[i];
        double value = 0.0;
        
        if (g_auto_scaler.metric_reader) {
            value = g_auto_scaler.metric_reader(metric->type);
        } else {
            // Встроенные ридеры
            switch (metric->type) {
                case AUTO_SCALER_METRIC_CPU:
                    value = read_system_cpu_usage();
                    break;
                case AUTO_SCALER_METRIC_MEMORY:
                    value = read_system_memory_usage();
                    break;
                default:
                    value = 0.0;
                    break;
            }
        }
        
        update_metric_status(metric, value);
        
        // Обновление статистики
        if (metric->type == AUTO_SCALER_METRIC_CPU) {
            g_auto_scaler.stats.avg_cpu_usage = 
                (g_auto_scaler.stats.avg_cpu_usage * 0.9) + (value * 0.1);
        } else if (metric->type == AUTO_SCALER_METRIC_MEMORY) {
            g_auto_scaler.stats.avg_memory_usage = 
                (g_auto_scaler.stats.avg_memory_usage * 0.9) + (value * 0.1);
        }
    }
    
    // Оценка необходимости масштабирования
    auto_scaler_scale_direction_t direction = evaluate_scaling();
    
    if (direction == AUTO_SCALER_SCALE_UP) {
        perform_scale_up();
    } else if (direction == AUTO_SCALER_SCALE_DOWN) {
        perform_scale_down();
    }
    
    return direction;
}

// ============================================================================
// Callback функции
// ============================================================================

void auto_scaler_set_scale_up_callback(auto_scaler_scale_up_callback_t callback) {
    g_auto_scaler.scale_up_callback = callback;
    kprintf("[AUTOSCALE] Scale up callback set\n");
}

void auto_scaler_set_scale_down_callback(auto_scaler_scale_down_callback_t callback) {
    g_auto_scaler.scale_down_callback = callback;
    kprintf("[AUTOSCALE] Scale down callback set\n");
}

void auto_scaler_set_metric_reader(auto_scaler_metric_reader_t callback) {
    g_auto_scaler.metric_reader = callback;
    kprintf("[AUTOSCALE] Metric reader callback set\n");
}

// ============================================================================
// Мониторинг
// ============================================================================

double auto_scaler_get_metric_value(auto_scaler_metric_type_t type) {
    auto_scaler_metric_config_t *metric = find_metric(type);
    if (!metric) return 0.0;
    
    auto_scaler_metric_status_t *status = NULL;
    for (int i = 0; i < g_auto_scaler.metric_count; i++) {
        if (g_auto_scaler.metric_status[i].type == type) {
            status = &g_auto_scaler.metric_status[i];
            break;
        }
    }
    
    return status ? status->current_value : 0.0;
}

const auto_scaler_metric_status_t* auto_scaler_get_metric_status(auto_scaler_metric_type_t type) {
    for (int i = 0; i < g_auto_scaler.metric_count; i++) {
        if (g_auto_scaler.metric_status[i].type == type) {
            return &g_auto_scaler.metric_status[i];
        }
    }
    return NULL;
}

double auto_scaler_get_total_load_percent(void) {
    if (g_auto_scaler.metric_count == 0) return 0.0;
    
    double total = 0.0;
    int count = 0;
    
    for (int i = 0; i < g_auto_scaler.metric_count; i++) {
        auto_scaler_metric_status_t *status = &g_auto_scaler.metric_status[i];
        auto_scaler_metric_config_t *metric = &g_auto_scaler.metrics[i];
        
        if (!metric->enabled) continue;
        
        // Нормализация к проценту
        double normalized = (status->current_value / metric->scale_up_threshold) * 100.0;
        if (normalized > 100.0) normalized = 100.0;
        
        total += normalized;
        count++;
    }
    
    return count > 0 ? total / count : 0.0;
}

bool auto_scaler_needs_scaling(void) {
    auto_scaler_scale_direction_t direction = evaluate_scaling();
    return direction != AUTO_SCALER_SCALE_NONE;
}

// ============================================================================
// Статистика
// ============================================================================

int auto_scaler_get_stats(auto_scaler_stats_t *stats) {
    if (!g_auto_scaler.initialized || !stats) {
        return -1;
    }
    
    memcpy(stats, &g_auto_scaler.stats, sizeof(auto_scaler_stats_t));
    stats->current_nodes = g_auto_scaler.config.current_nodes;
    return 0;
}

int auto_scaler_get_stats_string(char *buffer, size_t buffer_size) {
    if (!g_auto_scaler.initialized || !buffer || buffer_size < 256) {
        return -1;
    }
    
    auto_scaler_stats_t stats;
    auto_scaler_get_stats(&stats);
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "Auto-Scaler Statistics:\n");
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Cluster: %s, Policy: %s\n",
                       g_auto_scaler.config.cluster_name,
                       auto_scaler_policy_to_string(g_auto_scaler.config.policy));
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Nodes: %d (min: %d, max: %d, peak: %d)\n",
                       stats.current_nodes,
                       g_auto_scaler.config.min_nodes,
                       g_auto_scaler.config.max_nodes,
                       stats.peak_nodes);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Scale Events: Up %d, Down %d, Failed %d\n",
                       stats.total_scale_up_events,
                       stats.total_scale_down_events,
                       stats.failed_scale_events);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Avg Load: CPU %.1f%%, Memory %.1f%%\n",
                       stats.avg_cpu_usage,
                       stats.avg_memory_usage);
    
    return 0;
}

void auto_scaler_reset_stats(void) {
    if (!g_auto_scaler.initialized) return;
    
    memset(&g_auto_scaler.stats, 0, sizeof(g_auto_scaler.stats));
    g_auto_scaler.stats.start_time = get_current_time_ms();
    g_auto_scaler.stats.current_nodes = g_auto_scaler.config.current_nodes;
    kprintf("[AUTOSCALE] Statistics reset\n");
}

int auto_scaler_get_scale_events_count(void) {
    return g_auto_scaler.stats.total_scale_up_events + 
           g_auto_scaler.stats.total_scale_down_events;
}

// ============================================================================
// Управление узлами
// ============================================================================

int auto_scaler_set_current_nodes(int nodes) {
    if (!g_auto_scaler.initialized || nodes < 0) {
        return -1;
    }
    
    g_auto_scaler.config.current_nodes = nodes;
    g_auto_scaler.stats.current_nodes = nodes;
    
    kprintf("[AUTOSCALE] Current nodes set to: %d\n", nodes);
    return 0;
}

int auto_scaler_get_current_nodes(void) {
    return g_auto_scaler.config.current_nodes;
}

int auto_scaler_get_desired_nodes(void) {
    auto_scaler_scale_direction_t direction = evaluate_scaling();
    
    if (direction == AUTO_SCALER_SCALE_UP) {
        return g_auto_scaler.config.current_nodes + 1;
    } else if (direction == AUTO_SCALER_SCALE_DOWN) {
        return g_auto_scaler.config.current_nodes - 1;
    }
    
    return g_auto_scaler.config.current_nodes;
}

// ============================================================================
// Утилиты
// ============================================================================

const char* auto_scaler_policy_to_string(auto_scaler_policy_t policy) {
    switch (policy) {
        case AUTO_SCALER_POLICY_CONSERVATIVE: return "Conservative";
        case AUTO_SCALER_POLICY_MODERATE: return "Moderate";
        case AUTO_SCALER_POLICY_AGGRESSIVE: return "Aggressive";
        case AUTO_SCALER_POLICY_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

const char* auto_scaler_metric_type_to_string(auto_scaler_metric_type_t type) {
    switch (type) {
        case AUTO_SCALER_METRIC_CPU: return "CPU";
        case AUTO_SCALER_METRIC_MEMORY: return "Memory";
        case AUTO_SCALER_METRIC_CONNECTIONS: return "Connections";
        case AUTO_SCALER_METRIC_REQUESTS_PER_SEC: return "Requests/sec";
        case AUTO_SCALER_METRIC_LATENCY: return "Latency";
        case AUTO_SCALER_METRIC_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

const char* auto_scaler_scale_direction_to_string(auto_scaler_scale_direction_t direction) {
    switch (direction) {
        case AUTO_SCALER_SCALE_NONE: return "None";
        case AUTO_SCALER_SCALE_UP: return "Scale Up";
        case AUTO_SCALER_SCALE_DOWN: return "Scale Down";
        default: return "Unknown";
    }
}
