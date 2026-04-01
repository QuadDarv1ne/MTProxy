/**
 * @file performance-monitor.c
 * @brief Система мониторинга производительности с ML-аналитикой
 *
 * Функции:
 * - Сбор метрик производительности в реальном времени
 * - Интеграция с ML системой для детекции аномалий
 * - Автоматическое выявление узких мест
 * - Генерация алертов при деградации
 *
 * @version 1.0.33
 * @date 1 апреля 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#include "performance-monitor.h"
#include "../system/ml/anomaly-detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif

/* ============================================
 * Внутренние функции
 * ============================================ */

/**
 * @brief Получить текущее время в миллисекундах
 */
static uint64_t get_time_ms(void) {
#ifdef _WIN32
    return GetTickCount64();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

/**
 * @brief Получить использование CPU
 */
static double get_cpu_usage(void) {
#ifdef _WIN32
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return 0.0;
    }
    
    ULARGE_INTEGER idle, kernel, user;
    idle.LowPart = idleTime.dwLowDateTime;
    idle.HighPart = idleTime.dwHighDateTime;
    kernel.LowPart = kernelTime.dwLowDateTime;
    kernel.HighPart = kernelTime.dwHighDateTime;
    user.LowPart = userTime.dwLowDateTime;
    user.HighPart = userTime.dwHighDateTime;
    
    ULONGLONG total = (kernel.QuadPart - idle.QuadPart) + user.QuadPart;
    if (total == 0) return 0.0;
    
    return 100.0 * (double)(kernel.QuadPart - idle.QuadPart) / total;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0.0;
    }
    
    double utime = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    double stime = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
    
    return (utime + stime) * 100.0 / (double)sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

/**
 * @brief Получить использование памяти
 */
static uint64_t get_memory_usage(void) {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return 0;
    }
    return pmc.WorkingSetSize;
#else
    FILE *file = fopen("/proc/self/statm", "r");
    if (!file) return 0;
    
    unsigned long size, resident;
    if (fscanf(file, "%lu %lu", &size, &resident) != 2) {
        fclose(file);
        return 0;
    }
    fclose(file);
    
    return resident * sysconf(_SC_PAGESIZE);
#endif
}

/* ============================================
 * Основные функции
 * ============================================ */

int perf_monitor_init(perf_monitor_ctx_t *ctx, perf_monitor_config_t *config) {
    if (!ctx) return -1;
    
    memset(ctx, 0, sizeof(perf_monitor_ctx_t));
    
    /* Копирование конфигурации */
    if (config) {
        ctx->config = *config;
    } else {
        /* Конфигурация по умолчанию */
        ctx->config.sample_interval_ms = 1000;
        ctx->config.enable_ml_detection = 1;
        ctx->config.cpu_threshold = 80.0;
        ctx->config.memory_threshold = 80.0;
        ctx->config.latency_threshold_ms = 1000;
    }
    
    /* Инициализация мьютексов */
#ifdef _WIN32
    InitializeCriticalSection(&ctx->lock);
#else
    pthread_mutex_init(&ctx->lock, NULL);
    pthread_rwlock_init(&ctx->rwlock, NULL);
#endif
    
    /* Инициализация ML детектора если включено */
    if (ctx->config.enable_ml_detection) {
        int ret = anomaly_detector_init(&ctx->ml_detector, 
                                         ANOMALY_ALGO_ZSCORE,
                                         PERF_NUM_METRICS,
                                         100,
                                         0.7f);
        if (ret != 0) {
            ctx->config.enable_ml_detection = 0;
        }
    }
    
    ctx->start_time = get_time_ms();
    ctx->initialized = 1;
    
    return 0;
}

int perf_monitor_record(perf_monitor_ctx_t *ctx, perf_metrics_t *metrics) {
    if (!ctx || !ctx->initialized || !metrics) return -1;
    
#ifdef _WIN32
    EnterCriticalSection(&ctx->lock);
#else
    pthread_mutex_lock(&ctx->lock);
#endif
    
    /* Добавление метрик в историю */
    if (ctx->metrics_count < PERF_MAX_HISTORY) {
        ctx->metrics_history[ctx->metrics_count] = *metrics;
        ctx->metrics_timestamps[ctx->metrics_count] = get_time_ms();
        ctx->metrics_count++;
    } else {
        /* Сдвиг истории */
        memmove(&ctx->metrics_history[0], &ctx->metrics_history[1],
                sizeof(perf_metrics_t) * (PERF_MAX_HISTORY - 1));
        memmove(&ctx->metrics_timestamps[0], &ctx->metrics_timestamps[1],
                sizeof(uint64_t) * (PERF_MAX_HISTORY - 1));
        
        ctx->metrics_history[ctx->metrics_count - 1] = *metrics;
        ctx->metrics_timestamps[ctx->metrics_count - 1] = get_time_ms();
    }
    
    ctx->total_samples++;
    
    /* Обновление статистики */
    ctx->stats.total_samples++;
    ctx->stats.last_cpu_usage = metrics->cpu_percent;
    ctx->stats.last_memory_usage = metrics->memory_bytes;
    ctx->stats.last_latency_ms = metrics->avg_latency_ms;
    
    /* ML детекция аномалий */
    if (ctx->config.enable_ml_detection && ctx->total_samples >= 10) {
        double features[PERF_NUM_METRICS] = {
            (double)metrics->cpu_percent,
            (double)metrics->memory_bytes / (1024.0 * 1024.0),
            (double)metrics->avg_latency_ms,
            (double)metrics->connections_active,
            (double)metrics->requests_per_second
        };
        
        float anomaly_score;
        int ret = anomaly_detector_predict(&ctx->ml_detector, features, &anomaly_score);
        
        if (ret == 0 && anomaly_score > ctx->config.cpu_threshold / 100.0f) {
            ctx->stats.anomalies_detected++;
            ctx->last_anomaly_time = get_time_ms();
            ctx->last_anomaly_score = anomaly_score;
        }
    }
    
    /* Проверка порогов */
    if (metrics->cpu_percent > ctx->config.cpu_threshold) {
        ctx->stats.cpu_threshold_exceeded++;
    }
    if (metrics->memory_bytes > ctx->config.memory_threshold * 1024 * 1024) {
        ctx->stats.memory_threshold_exceeded++;
    }
    if (metrics->avg_latency_ms > ctx->config.latency_threshold_ms) {
        ctx->stats.latency_threshold_exceeded++;
    }
    
#ifdef _WIN32
    LeaveCriticalSection(&ctx->lock);
#else
    pthread_mutex_unlock(&ctx->lock);
#endif
    
    return 0;
}

int perf_monitor_sample(perf_monitor_ctx_t *ctx) {
    if (!ctx || !ctx->initialized) return -1;
    
    perf_metrics_t metrics;
    memset(&metrics, 0, sizeof(perf_metrics_t));
    
    /* Сбор базовых метрик */
    metrics.cpu_percent = get_cpu_usage();
    metrics.memory_bytes = get_memory_usage();
    metrics.timestamp = get_time_ms();
    
    /* Запись метрик */
    return perf_monitor_record(ctx, &metrics);
}

int perf_monitor_get_stats(perf_monitor_ctx_t *ctx, perf_stats_t *stats) {
    if (!ctx || !ctx->initialized || !stats) return -1;
    
#ifdef _WIN32
    EnterCriticalSection(&ctx->lock);
#else
    pthread_mutex_lock(&ctx->lock);
#endif
    
    *stats = ctx->stats;
    
    /* Вычисление средних значений */
    if (ctx->metrics_count > 0) {
        double total_cpu = 0, total_mem = 0, total_latency = 0;
        
        for (int i = 0; i < ctx->metrics_count; i++) {
            total_cpu += ctx->metrics_history[i].cpu_percent;
            total_mem += ctx->metrics_history[i].memory_bytes;
            total_latency += ctx->metrics_history[i].avg_latency_ms;
        }
        
        stats->avg_cpu_usage = total_cpu / ctx->metrics_count;
        stats->avg_memory_usage = total_mem / ctx->metrics_count;
        stats->avg_latency_ms = total_latency / ctx->metrics_count;
    }
    
#ifdef _WIN32
    LeaveCriticalSection(&ctx->lock);
#else
    pthread_mutex_unlock(&ctx->lock);
#endif
    
    return 0;
}

int perf_monitor_export_json(perf_monitor_ctx_t *ctx, char *buffer, size_t buffer_size) {
    if (!ctx || !ctx->initialized || !buffer || buffer_size < 256) return -1;
    
    perf_stats_t stats;
    perf_monitor_get_stats(ctx, &stats);
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "{\n");
    offset += snprintf(buffer + offset, buffer_size - offset, "  \"start_time\": %lu,\n", 
                       (unsigned long)ctx->start_time);
    offset += snprintf(buffer + offset, buffer_size - offset, "  \"total_samples\": %lu,\n",
                       (unsigned long)ctx->total_samples);
    offset += snprintf(buffer + offset, buffer_size - offset, "  \"metrics_count\": %d,\n",
                       ctx->metrics_count);
    offset += snprintf(buffer + offset, buffer_size - offset, "  \"stats\": {\n");
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "    \"avg_cpu_usage\": %.2f,\n", stats.avg_cpu_usage);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "    \"avg_memory_usage\": %.0f,\n", stats.avg_memory_usage);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "    \"avg_latency_ms\": %.2f,\n", stats.avg_latency_ms);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "    \"anomalies_detected\": %lu\n", 
                       (unsigned long)stats.anomalies_detected);
    offset += snprintf(buffer + offset, buffer_size - offset, "  }\n");
    offset += snprintf(buffer + offset, buffer_size - offset, "}\n");
    
    return offset;
}

void perf_monitor_cleanup(perf_monitor_ctx_t *ctx) {
    if (!ctx || !ctx->initialized) return;
    
    ctx->initialized = 0;
    
#ifdef _WIN32
    DeleteCriticalSection(&ctx->lock);
#else
    pthread_mutex_destroy(&ctx->lock);
    pthread_rwlock_destroy(&ctx->rwlock);
#endif
    
    if (ctx->config.enable_ml_detection) {
        anomaly_detector_cleanup(&ctx->ml_detector);
    }
    
    memset(ctx, 0, sizeof(perf_monitor_ctx_t));
}

void perf_monitor_print_stats(perf_monitor_ctx_t *ctx) {
    if (!ctx || !ctx->initialized) return;
    
    perf_stats_t stats;
    perf_monitor_get_stats(ctx, &stats);
    
    printf("\n=== Performance Monitor Statistics ===\n\n");
    printf("Total Samples: %lu\n", (unsigned long)ctx->total_samples);
    printf("Metrics in History: %d\n", ctx->metrics_count);
    printf("\nAverages:\n");
    printf("  CPU Usage: %.2f%%\n", stats.avg_cpu_usage);
    printf("  Memory Usage: %.0f bytes\n", stats.avg_memory_usage);
    printf("  Latency: %.2f ms\n", stats.avg_latency_ms);
    printf("\nThreshold Exceeded:\n");
    printf("  CPU: %lu times\n", (unsigned long)stats.cpu_threshold_exceeded);
    printf("  Memory: %lu times\n", (unsigned long)stats.memory_threshold_exceeded);
    printf("  Latency: %lu times\n", (unsigned long)stats.latency_threshold_exceeded);
    printf("\nML Detection:\n");
    printf("  Anomalies Detected: %lu\n", (unsigned long)stats.anomalies_detected);
    if (stats.anomalies_detected > 0) {
        printf("  Last Anomaly Score: %.3f\n", ctx->last_anomaly_score);
    }
    printf("\n");
}
