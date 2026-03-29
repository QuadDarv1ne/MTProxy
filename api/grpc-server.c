/*
 * gRPC Server Implementation for MTProxy
 * Реализация gRPC сервера для управления MTProxy
 */

#include "grpc-server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __linux__
#include <sys/utsname.h>
#endif

/* ============================================================================
 * Вспомогательные функции
 * ============================================================================ */

const char *grpc_server_state_to_string(grpc_server_state_t state) {
    switch (state) {
        case GRPC_SERVER_STATE_UNKNOWN:   return "unknown";
        case GRPC_SERVER_STATE_STARTING:  return "starting";
        case GRPC_SERVER_STATE_RUNNING:   return "running";
        case GRPC_SERVER_STATE_STOPPING:  return "stopping";
        case GRPC_SERVER_STATE_STOPPED:   return "stopped";
        case GRPC_SERVER_STATE_ERROR:     return "error";
        default:                          return "unknown";
    }
}

grpc_server_state_t grpc_server_state_from_string(const char *str) {
    if (!str) return GRPC_SERVER_STATE_UNKNOWN;
    
    if (strcmp(str, "unknown") == 0)   return GRPC_SERVER_STATE_UNKNOWN;
    if (strcmp(str, "starting") == 0)  return GRPC_SERVER_STATE_STARTING;
    if (strcmp(str, "running") == 0)   return GRPC_SERVER_STATE_RUNNING;
    if (strcmp(str, "stopping") == 0)  return GRPC_SERVER_STATE_STOPPING;
    if (strcmp(str, "stopped") == 0)   return GRPC_SERVER_STATE_STOPPED;
    if (strcmp(str, "error") == 0)     return GRPC_SERVER_STATE_ERROR;
    
    return GRPC_SERVER_STATE_UNKNOWN;
}

static void get_platform_info(grpc_platform_info_t *platform) {
    memset(platform, 0, sizeof(grpc_platform_info_t));
    
#ifdef _WIN32
    strncpy(platform->os, "windows", sizeof(platform->os) - 1);
#ifdef _M_X64
    strncpy(platform->arch, "x86_64", sizeof(platform->arch) - 1);
#elif defined(_M_ARM64)
    strncpy(platform->arch, "arm64", sizeof(platform->arch) - 1);
#elif defined(_M_IX86)
    strncpy(platform->arch, "x86", sizeof(platform->arch) - 1);
#elif defined(_M_ARM)
    strncpy(platform->arch, "arm", sizeof(platform->arch) - 1);
#endif
    platform->cpu_cores = (int32_t)getenv("NUMBER_OF_PROCESSORS");
    if (platform->cpu_cores <= 0) platform->cpu_cores = 1;
    
    // Получение информации о памяти (Windows)
    // Упрощённая реализация
    platform->total_memory_bytes = 8LL * 1024 * 1024 * 1024; // 8GB default
    
#elif defined(__linux__)
    struct utsname uts;
    if (uname(&uts) == 0) {
        snprintf(platform->os, sizeof(platform->os), "%s %s", uts.sysname, uts.release);
        
        if (strstr(uts.machine, "x86_64") || strstr(uts.machine, "amd64")) {
            strncpy(platform->arch, "x86_64", sizeof(platform->arch) - 1);
        } else if (strstr(uts.machine, "aarch64") || strstr(uts.machine, "arm64")) {
            strncpy(platform->arch, "arm64", sizeof(platform->arch) - 1);
        } else if (strstr(uts.machine, "arm")) {
            strncpy(platform->arch, "arm", sizeof(platform->arch) - 1);
        } else if (strstr(uts.machine, "i386") || strstr(uts.machine, "i686")) {
            strncpy(platform->arch, "x86", sizeof(platform->arch) - 1);
        }
    }
    
    // Получение количества CPU
    platform->cpu_cores = (int32_t)sysconf(_SC_NPROCESSORS_ONLN);
    if (platform->cpu_cores <= 0) platform->cpu_cores = 1;
    
    // Получение информации о памяти
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGESIZE);
    if (pages > 0 && page_size > 0) {
        platform->total_memory_bytes = (int64_t)pages * page_size;
    } else {
        platform->total_memory_bytes = 8LL * 1024 * 1024 * 1024;
    }
    
#elif defined(__APPLE__)
    strncpy(platform->os, "darwin", sizeof(platform->os) - 1);
    // macOS CPU и память можно получить через sysctl
    platform->cpu_cores = (int32_t)sysconf(_SC_NPROCESSORS_ONLN);
    if (platform->cpu_cores <= 0) platform->cpu_cores = 1;
    platform->total_memory_bytes = 8LL * 1024 * 1024 * 1024;
#else
    strncpy(platform->os, "unknown", sizeof(platform->os) - 1);
    strncpy(platform->arch, "unknown", sizeof(platform->arch) - 1);
    platform->cpu_cores = 1;
    platform->total_memory_bytes = 8LL * 1024 * 1024 * 1024;
#endif
}

static int64_t get_current_time_unix(void) {
    struct timespec ts;
#ifdef _WIN32
    // Windows реализация
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    // Конвертация из 100-наносекундных интервалов в секунды
    return (int64_t)((uli.QuadPart - 116444736000000000ULL) / 10000000);
#else
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return (int64_t)ts.tv_sec;
    }
    return (int64_t)time(NULL);
#endif
}

static int64_t get_uptime_seconds(void) {
#ifdef _WIN32
    return (int64_t)(GetTickCount64() / 1000);
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (int64_t)ts.tv_sec;
    }
    return 0;
#endif
}

/* ============================================================================
 * Callback функции по умолчанию
 * ============================================================================ */

static int default_get_status_callback(grpc_server_status_t *status) {
    if (!status) return -1;
    
    memset(status, 0, sizeof(grpc_server_status_t));
    status->state = GRPC_SERVER_STATE_RUNNING;
    strncpy(status->version, "1.0.29", sizeof(status->version) - 1);
    strncpy(status->commit_hash, "unknown", sizeof(status->commit_hash) - 1);
    status->uptime_seconds = get_uptime_seconds();
    status->start_time_unix = get_current_time_unix() - status->uptime_seconds;
    get_platform_info(&status->platform);
    
    return 0;
}

static int default_get_config_callback(grpc_proxy_config_t *config) {
    if (!config) return -1;
    
    memset(config, 0, sizeof(grpc_proxy_config_t));
    config->port = 8888;
    config->ipv6_enabled = false;
    config->max_connections = 10000;
    config->backlog = 128;
    config->workers = 1;
    config->use_aes_ni = true;
    strncpy(config->log_level, "INFO", sizeof(config->log_level) - 1);
    
    return 0;
}

static int default_get_statistics_callback(grpc_proxy_statistics_t *stats) {
    if (!stats) return -1;
    
    memset(stats, 0, sizeof(grpc_proxy_statistics_t));
    stats->active_connections = 0;
    stats->total_connections = 0;
    stats->timestamp_unix = get_current_time_unix();
    stats->uptime_seconds = get_uptime_seconds();
    
    return 0;
}

/* ============================================================================
 * Основные функции gRPC сервера
 * ============================================================================ */

int grpc_server_init(grpc_server_context_t *ctx, const grpc_server_config_t *config) {
    if (!ctx || !config) {
        return -1;
    }
    
    memset(ctx, 0, sizeof(grpc_server_context_t));
    
    // Копирование конфигурации
    memcpy(&ctx->config, config, sizeof(grpc_server_config_t));
    
    // Инициализация mutex
    pthread_mutex_init(&ctx->mutex, NULL);
    
    // Установка callback функций по умолчанию
    ctx->get_status_callback = default_get_status_callback;
    ctx->get_config_callback = default_get_config_callback;
    ctx->get_statistics_callback = default_get_statistics_callback;
    
    // Инициализация статуса
    ctx->status.state = GRPC_SERVER_STATE_STOPPED;
    strncpy(ctx->status.version, "1.0.29", sizeof(ctx->status.version) - 1);
    get_platform_info(&ctx->status.platform);
    
    ctx->initialized = true;
    ctx->running = false;
    
    // Примечание: реальная инициализация gRPC сервера требует
    // подключения gRPC C library и генерации кода из .proto файла
    // Здесь реализована заглушка для интеграции
    
    printf("[gRPC] Server initialized on port %d\n", config->port);
    
    return 0;
}

// Поток gRPC сервера
static void *grpc_server_thread_func(void *arg) {
    grpc_server_context_t *ctx = (grpc_server_context_t *)arg;
    
    printf("[gRPC] Server thread started\n");
    
    // Основной цикл обработки gRPC запросов
    // В реальной реализации здесь будет event loop gRPC
    while (ctx->running) {
        // Обработка запросов из completion queue
        // gRPC реализация требует подключения gRPC C library
        
        // Заглушка: просто спим
        struct timespec ts = {1, 0};
        nanosleep(&ts, NULL);
    }
    
    printf("[gRPC] Server thread stopped\n");
    return NULL;
}

int grpc_server_start(grpc_server_context_t *ctx) {
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    if (ctx->running) {
        return 0;  // Уже запущен
    }
    
    pthread_mutex_lock(&ctx->mutex);
    
    ctx->running = true;
    ctx->status.state = GRPC_SERVER_STATE_STARTING;
    
    // Запуск потока сервера
    if (pthread_create(&ctx->server_thread, NULL, grpc_server_thread_func, ctx) != 0) {
        ctx->running = false;
        ctx->status.state = GRPC_SERVER_STATE_ERROR;
        strncpy(ctx->status.error_message, "Failed to create server thread", 
                sizeof(ctx->status.error_message) - 1);
        pthread_mutex_unlock(&ctx->mutex);
        return -1;
    }
    
    // Небольшая задержка для запуска
    struct timespec ts = {0, 100000000}; // 100ms
    nanosleep(&ts, NULL);
    
    ctx->status.state = GRPC_SERVER_STATE_RUNNING;
    ctx->status.start_time_unix = get_current_time_unix();
    
    pthread_mutex_unlock(&ctx->mutex);
    
    printf("[gRPC] Server started on %s:%d\n", 
           ctx->config.bind_address[0] ? ctx->config.bind_address : "0.0.0.0",
           ctx->config.port);
    
    return 0;
}

void grpc_server_stop(grpc_server_context_t *ctx) {
    if (!ctx || !ctx->initialized || !ctx->running) {
        return;
    }
    
    pthread_mutex_lock(&ctx->mutex);
    
    ctx->running = false;
    ctx->status.state = GRPC_SERVER_STATE_STOPPING;
    
    pthread_mutex_unlock(&ctx->mutex);
    
    // Ожидание завершения потока
    if (ctx->server_thread) {
        pthread_join(ctx->server_thread, NULL);
    }
    
    pthread_mutex_lock(&ctx->mutex);
    ctx->status.state = GRPC_SERVER_STATE_STOPPED;
    pthread_mutex_unlock(&ctx->mutex);
    
    printf("[gRPC] Server stopped\n");
}

void grpc_server_cleanup(grpc_server_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    // Остановка если запущен
    if (ctx->running) {
        grpc_server_stop(ctx);
    }
    
    // Очистка mutex
    pthread_mutex_destroy(&ctx->mutex);
    
    ctx->initialized = false;
    
    printf("[gRPC] Server cleaned up\n");
}

/* ============================================================================
 * Регистрация callback функций
 * ============================================================================ */

void grpc_server_set_status_callback(grpc_server_context_t *ctx,
                                    int (*callback)(grpc_server_status_t *status)) {
    if (ctx) {
        ctx->get_status_callback = callback;
    }
}

void grpc_server_set_start_callback(grpc_server_context_t *ctx,
                                   int (*callback)(bool force)) {
    if (ctx) {
        ctx->start_server_callback = callback;
    }
}

void grpc_server_set_stop_callback(grpc_server_context_t *ctx,
                                  int (*callback)(bool graceful, int32_t timeout_sec)) {
    if (ctx) {
        ctx->stop_server_callback = callback;
    }
}

void grpc_server_set_restart_callback(grpc_server_context_t *ctx,
                                     int (*callback)(bool graceful, int32_t timeout_sec)) {
    if (ctx) {
        ctx->restart_server_callback = callback;
    }
}

void grpc_server_set_get_config_callback(grpc_server_context_t *ctx,
                                        int (*callback)(grpc_proxy_config_t *config)) {
    if (ctx) {
        ctx->get_config_callback = callback;
    }
}

void grpc_server_set_update_config_callback(grpc_server_context_t *ctx,
                                           int (*callback)(const grpc_proxy_config_t *config, bool reload)) {
    if (ctx) {
        ctx->update_config_callback = callback;
    }
}

void grpc_server_set_validate_config_callback(grpc_server_context_t *ctx,
                                             int (*callback)(const grpc_proxy_config_t *config, grpc_validation_result_t *result)) {
    if (ctx) {
        ctx->validate_config_callback = callback;
    }
}

void grpc_server_set_statistics_callback(grpc_server_context_t *ctx,
                                        int (*callback)(grpc_proxy_statistics_t *stats)) {
    if (ctx) {
        ctx->get_statistics_callback = callback;
    }
}

void grpc_server_set_connections_callback(grpc_server_context_t *ctx,
                                         int (*callback)(int32_t limit, int32_t offset, const char *filter_ip, grpc_connection_list_t *list)) {
    if (ctx) {
        ctx->get_connections_callback = callback;
    }
}

void grpc_server_set_add_secret_callback(grpc_server_context_t *ctx,
                                        int (*callback)(const char *secret, const char *description)) {
    if (ctx) {
        ctx->add_secret_callback = callback;
    }
}

void grpc_server_set_remove_secret_callback(grpc_server_context_t *ctx,
                                           int (*callback)(const char *secret)) {
    if (ctx) {
        ctx->remove_secret_callback = callback;
    }
}

void grpc_server_set_list_secrets_callback(grpc_server_context_t *ctx,
                                          int (*callback)(grpc_secret_list_t *list)) {
    if (ctx) {
        ctx->list_secrets_callback = callback;
    }
}

void grpc_server_set_rate_limits_callback(grpc_server_context_t *ctx,
                                         int (*callback)(grpc_rate_limit_config_t *config)) {
    if (ctx) {
        ctx->get_rate_limits_callback = callback;
    }
}

void grpc_server_set_update_rate_limit_callback(grpc_server_context_t *ctx,
                                               int (*callback)(const grpc_rate_limit_config_t *config)) {
    if (ctx) {
        ctx->update_rate_limit_callback = callback;
    }
}

void grpc_server_set_logs_callback(grpc_server_context_t *ctx,
                                  int (*callback)(int32_t limit, const char *level, int64_t start_time, int64_t end_time, grpc_log_entries_t *entries)) {
    if (ctx) {
        ctx->get_logs_callback = callback;
    }
}

/* ============================================================================
 * Вспомогательные функции
 * ============================================================================ */

void grpc_server_get_stats(grpc_server_context_t *ctx, grpc_server_stats_t *stats) {
    if (!ctx || !stats) {
        return;
    }
    
    pthread_mutex_lock(&ctx->mutex);
    memcpy(stats, &ctx->stats, sizeof(grpc_server_stats_t));
    pthread_mutex_unlock(&ctx->mutex);
}

void grpc_server_reset_stats(grpc_server_context_t *ctx) {
    if (!ctx) {
        return;
    }
    
    pthread_mutex_lock(&ctx->mutex);
    memset(&ctx->stats, 0, sizeof(grpc_server_stats_t));
    pthread_mutex_unlock(&ctx->mutex);
}

bool grpc_server_is_running(grpc_server_context_t *ctx) {
    if (!ctx) {
        return false;
    }
    
    pthread_mutex_lock(&ctx->mutex);
    bool running = ctx->running;
    pthread_mutex_unlock(&ctx->mutex);
    
    return running;
}
