/**
 * @file test_performance_monitor.c
 * @brief Тесты для системы мониторинга производительности
 *
 * @version 1.0.33
 * @date 1 апреля 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#include "../net/performance-monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <time.h>
#endif

/* ============================================
 * Утилиты тестирования
 * ============================================ */

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { \
        tests_passed++; \
        printf("  ✓ %s\n", msg); \
    } else { \
        tests_failed++; \
        printf("  ✗ FAILED: %s\n", msg); \
    } \
} while(0)

/* ============================================
 * Тесты инициализации
 * ============================================ */

static void test_perf_monitor_init_cleanup(void) {
    printf("\n=== Test: Init/Cleanup ===\n");
    
    perf_monitor_ctx_t ctx;
    int ret;
    
    /* Тест 1: Инициализация с NULL config */
    ret = perf_monitor_init(&ctx, NULL);
    TEST_ASSERT(ret == 0, "Init with NULL config");
    TEST_ASSERT(ctx.initialized == 1, "Context marked as initialized");
    TEST_ASSERT(ctx.config.sample_interval_ms == 1000, "Default sample interval");
    TEST_ASSERT(ctx.config.enable_ml_detection == 1, "ML detection enabled by default");
    perf_monitor_cleanup(&ctx);
    
    /* Тест 2: Инициализация с кастомной конфигурацией */
    perf_monitor_config_t config = {
        .sample_interval_ms = 500,
        .enable_ml_detection = 0,
        .cpu_threshold = 90.0,
        .memory_threshold = 85.0,
        .latency_threshold_ms = 500
    };
    
    ret = perf_monitor_init(&ctx, &config);
    TEST_ASSERT(ret == 0, "Init with custom config");
    TEST_ASSERT(ctx.config.sample_interval_ms == 500, "Custom sample interval");
    TEST_ASSERT(ctx.config.enable_ml_detection == 0, "ML detection disabled");
    TEST_ASSERT(ctx.config.cpu_threshold == 90.0, "Custom CPU threshold");
    perf_monitor_cleanup(&ctx);
    
    /* Тест 3: Инициализация с NULL контекстом */
    ret = perf_monitor_init(NULL, NULL);
    TEST_ASSERT(ret == -1, "Init with NULL context returns -1");
    
    /* Тест 4: Очистка NULL контекста */
    perf_monitor_cleanup(NULL);  /* Должно быть no-op */
    TEST_ASSERT(1, "Cleanup with NULL is safe");
    
    /* Тест 5: Двойная очистка */
    ret = perf_monitor_init(&ctx, NULL);
    TEST_ASSERT(ret == 0, "Init for double cleanup test");
    perf_monitor_cleanup(&ctx);
    perf_monitor_cleanup(&ctx);  /* Вторая очистка */
    TEST_ASSERT(1, "Double cleanup is safe");
}

/* ============================================
 * Тесты записи метрик
 * ============================================ */

static void test_perf_monitor_record(void) {
    printf("\n=== Test: Record Metrics ===\n");
    
    perf_monitor_ctx_t ctx;
    perf_metrics_t metrics;
    int ret;
    
    perf_monitor_init(&ctx, NULL);
    
    /* Тест 1: Запись валидных метрик */
    memset(&metrics, 0, sizeof(metrics));
    metrics.cpu_percent = 45.5;
    metrics.memory_bytes = 100 * 1024 * 1024;  /* 100 MB */
    metrics.avg_latency_ms = 25.0;
    metrics.connections_active = 100;
    
    ret = perf_monitor_record(&ctx, &metrics);
    TEST_ASSERT(ret == 0, "Record valid metrics");
    TEST_ASSERT(ctx.metrics_count == 1, "Metrics count incremented");
    TEST_ASSERT(ctx.total_samples == 1, "Total samples incremented");
    TEST_ASSERT(ctx.stats.last_cpu_usage == 45.5, "Last CPU usage updated");
    
    /* Тест 2: Запись нескольких метрик */
    for (int i = 0; i < 10; i++) {
        metrics.cpu_percent = 30.0 + i * 2;
        ret = perf_monitor_record(&ctx, &metrics);
        TEST_ASSERT(ret == 0, "Record metric %d", i + 1);
    }
    TEST_ASSERT(ctx.metrics_count == 11, "Multiple metrics recorded");
    
    /* Тест 3: Запись с NULL контекстом */
    ret = perf_monitor_record(NULL, &metrics);
    TEST_ASSERT(ret == -1, "Record with NULL context returns -1");
    
    /* Тест 4: Запись с NULL метриками */
    ret = perf_monitor_record(&ctx, NULL);
    TEST_ASSERT(ret == -1, "Record with NULL metrics returns -1");
    
    /* Тест 5: Переполнение истории */
    perf_monitor_cleanup(&ctx);
    perf_monitor_init(&ctx, NULL);
    
    for (int i = 0; i < PERF_MAX_HISTORY + 50; i++) {
        metrics.cpu_percent = (double)(i % 100);
        ret = perf_monitor_record(&ctx, &metrics);
        TEST_ASSERT(ret == 0, "Record overflow metric %d", i);
    }
    TEST_ASSERT(ctx.metrics_count == PERF_MAX_HISTORY, "History capped at max");
    
    perf_monitor_cleanup(&ctx);
}

/* ============================================
 * Тесты сэмплирования
 * ============================================ */

static void test_perf_monitor_sample(void) {
    printf("\n=== Test: Sampling ===\n");
    
    perf_monitor_ctx_t ctx;
    int ret;
    
    /* Тест 1: Сэмплирование с NULL контекстом */
    ret = perf_monitor_sample(NULL);
    TEST_ASSERT(ret == -1, "Sample with NULL context returns -1");
    
    /* Тест 2: Сэмплирование валидного контекста */
    ret = perf_monitor_init(&ctx, NULL);
    TEST_ASSERT(ret == 0, "Init for sampling");
    
    ret = perf_monitor_sample(&ctx);
    TEST_ASSERT(ret == 0, "Sample system metrics");
    TEST_ASSERT(ctx.metrics_count == 1, "Sample recorded");
    TEST_ASSERT(ctx.stats.last_cpu_usage >= 0.0, "CPU usage captured");
    TEST_ASSERT(ctx.stats.last_memory_usage > 0, "Memory usage captured");
    
    /* Тест 3: Множественные сэмплы */
    for (int i = 0; i < 5; i++) {
        ret = perf_monitor_sample(&ctx);
        TEST_ASSERT(ret == 0, "Sample %d", i + 1);
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
    }
    TEST_ASSERT(ctx.metrics_count == 6, "Multiple samples recorded");
    
    perf_monitor_cleanup(&ctx);
}

/* ============================================
 * Тесты статистики
 * ============================================ */

static void test_perf_monitor_stats(void) {
    printf("\n=== Test: Statistics ===\n");
    
    perf_monitor_ctx_t ctx;
    perf_stats_t stats;
    perf_metrics_t metrics;
    int ret;
    
    perf_monitor_init(&ctx, NULL);
    
    /* Тест 1: Получение статистики с NULL контекстом */
    ret = perf_monitor_get_stats(NULL, &stats);
    TEST_ASSERT(ret == -1, "Get stats with NULL context returns -1");
    
    /* Тест 2: Получение статистики с NULL stats */
    ret = perf_monitor_get_stats(&ctx, NULL);
    TEST_ASSERT(ret == -1, "Get stats with NULL stats returns -1");
    
    /* Тест 3: Получение статистики без метрик */
    ret = perf_monitor_get_stats(&ctx, &stats);
    TEST_ASSERT(ret == 0, "Get stats with no metrics");
    TEST_ASSERT(stats.total_samples == 0, "Total samples is 0");
    
    /* Тест 4: Получение статистики с метриками */
    for (int i = 0; i < 5; i++) {
        memset(&metrics, 0, sizeof(metrics));
        metrics.cpu_percent = 40.0 + i * 10;  /* 40, 50, 60, 70, 80 */
        metrics.memory_bytes = 100 * 1024 * 1024;
        metrics.avg_latency_ms = 20.0 + i * 5;
        perf_monitor_record(&ctx, &metrics);
    }
    
    ret = perf_monitor_get_stats(&ctx, &stats);
    TEST_ASSERT(ret == 0, "Get stats with metrics");
    TEST_ASSERT(stats.total_samples == 5, "Total samples is 5");
    TEST_ASSERT(stats.avg_cpu_usage > 59.0 && stats.avg_cpu_usage < 61.0, 
                "Average CPU is correct (~60%%)");
    
    /* Тест 5: Проверка порогов */
    perf_monitor_cleanup(&ctx);
    perf_monitor_config_t config = {
        .cpu_threshold = 50.0,
        .memory_threshold = 50.0,
        .latency_threshold_ms = 30
    };
    perf_monitor_init(&ctx, &config);
    
    metrics.cpu_percent = 70.0;  /* Выше порога */
    metrics.memory_bytes = 100 * 1024 * 1024;
    metrics.avg_latency_ms = 50.0;  /* Выше порога */
    perf_monitor_record(&ctx, &metrics);
    
    ret = perf_monitor_get_stats(&ctx, &stats);
    TEST_ASSERT(ret == 0, "Get stats after threshold test");
    TEST_ASSERT(stats.cpu_threshold_exceeded == 1, "CPU threshold exceeded");
    TEST_ASSERT(stats.latency_threshold_exceeded == 1, "Latency threshold exceeded");
    
    perf_monitor_cleanup(&ctx);
}

/* ============================================
 * Тесты JSON экспорта
 * ============================================ */

static void test_perf_monitor_json_export(void) {
    printf("\n=== Test: JSON Export ===\n");
    
    perf_monitor_ctx_t ctx;
    char buffer[2048];
    int written;
    
    perf_monitor_init(&ctx, NULL);
    
    /* Тест 1: Экспорт с NULL буфером */
    written = perf_monitor_export_json(&ctx, NULL, sizeof(buffer));
    TEST_ASSERT(written == -1, "Export with NULL buffer returns -1");
    
    /* Тест 2: Экспорт с маленьким буфером */
    written = perf_monitor_export_json(&ctx, buffer, 10);
    TEST_ASSERT(written == -1, "Export with small buffer returns -1");
    
    /* Тест 3: Экспорт без метрик */
    written = perf_monitor_export_json(&ctx, buffer, sizeof(buffer));
    TEST_ASSERT(written > 0, "Export with no metrics");
    TEST_ASSERT(strstr(buffer, "\"total_samples\"") != NULL, "JSON contains total_samples");
    
    /* Тест 4: Экспорт с метриками */
    perf_metrics_t metrics;
    memset(&metrics, 0, sizeof(metrics));
    metrics.cpu_percent = 55.5;
    metrics.memory_bytes = 200 * 1024 * 1024;
    metrics.avg_latency_ms = 35.0;
    perf_monitor_record(&ctx, &metrics);
    
    written = perf_monitor_export_json(&ctx, buffer, sizeof(buffer));
    TEST_ASSERT(written > 0, "Export with metrics");
    TEST_ASSERT(strstr(buffer, "\"start_time\"") != NULL, "JSON contains start_time");
    TEST_ASSERT(strstr(buffer, "\"stats\"") != NULL, "JSON contains stats");
    
    printf("    JSON output (%d bytes):\n", written);
    printf("    %.*s\n", (written < 300) ? written : 300, buffer);
    
    perf_monitor_cleanup(&ctx);
}

/* ============================================
 * Тесты ML детекции аномалий
 * ============================================ */

static void test_perf_monitor_ml_detection(void) {
    printf("\n=== Test: ML Anomaly Detection ===\n");
    
    perf_monitor_ctx_t ctx;
    perf_monitor_config_t config;
    perf_metrics_t metrics;
    int ret;
    
    /* Тест 1: ML детекция включена по умолчанию */
    ret = perf_monitor_init(&ctx, NULL);
    TEST_ASSERT(ret == 0, "Init with default config");
    TEST_ASSERT(ctx.config.enable_ml_detection == 1, "ML detection enabled by default");
    perf_monitor_cleanup(&ctx);
    
    /* Тест 2: Отключение ML детекции */
    memset(&config, 0, sizeof(config));
    config.enable_ml_detection = 0;
    ret = perf_monitor_init(&ctx, &config);
    TEST_ASSERT(ret == 0, "Init with ML disabled");
    TEST_ASSERT(ctx.config.enable_ml_detection == 0, "ML detection disabled");
    perf_monitor_cleanup(&ctx);
    
    /* Тест 3: ML детекция с нормальными данными */
    config.enable_ml_detection = 1;
    ret = perf_monitor_init(&ctx, &config);
    TEST_ASSERT(ret == 0, "Init with ML enabled");
    
    /* Обучение на нормальных данных */
    memset(&metrics, 0, sizeof(metrics));
    metrics.cpu_percent = 30.0;
    metrics.memory_bytes = 100 * 1024 * 1024;
    metrics.avg_latency_ms = 20.0;
    metrics.connections_active = 50;
    metrics.requests_per_second = 1000;
    
    for (int i = 0; i < 20; i++) {
        perf_monitor_record(&ctx, &metrics);
    }
    
    TEST_ASSERT(ctx.total_samples == 20, "Training samples recorded");
    
    /* Тест 4: Детекция аномалии */
    metrics.cpu_percent = 95.0;  /* Аномально высокое значение */
    metrics.memory_bytes = 900 * 1024 * 1024;
    metrics.avg_latency_ms = 500.0;
    
    perf_monitor_record(&ctx, &metrics);
    
    /* Проверяем что аномалия могла быть обнаружена */
    TEST_ASSERT(ctx.stats.total_samples == 21, "Anomaly sample recorded");
    
    perf_monitor_cleanup(&ctx);
}

/* ============================================
 * Тесты производительности
 * ============================================ */

static void test_perf_monitor_performance(void) {
    printf("\n=== Test: Performance ===\n");
    
    perf_monitor_ctx_t ctx;
    perf_metrics_t metrics;
    uint64_t start, end;
    int iterations = 1000;
    
    perf_monitor_init(&ctx, NULL);
    memset(&metrics, 0, sizeof(metrics));
    metrics.cpu_percent = 50.0;
    metrics.memory_bytes = 200 * 1024 * 1024;
    
    /* Тест: Время записи метрик */
#ifdef _WIN32
    start = GetTickCount64();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    start = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
    
    for (int i = 0; i < iterations; i++) {
        perf_monitor_record(&ctx, &metrics);
    }
    
#ifdef _WIN32
    end = GetTickCount64();
#else
    gettimeofday(&tv, NULL);
    end = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
    
    uint64_t total_time = end - start;
    double avg_time = (double)total_time / iterations;
    
    TEST_ASSERT(total_time < 1000, "1000 records in < 1 second");
    printf("    Avg record time: %.3f ms\n", avg_time);
    printf("    Total time for %d records: %lu ms\n", iterations, (unsigned long)total_time);
    
    perf_monitor_cleanup(&ctx);
}

/* ============================================
 * Главная функция тестирования
 * ============================================ */

int main(void) {
    printf("\n============================================\n");
    printf("  Performance Monitor Tests\n");
    printf("  Version: 1.0.33\n");
    printf("============================================\n");
    
    test_perf_monitor_init_cleanup();
    test_perf_monitor_record();
    test_perf_monitor_sample();
    test_perf_monitor_stats();
    test_perf_monitor_json_export();
    test_perf_monitor_ml_detection();
    test_perf_monitor_performance();
    
    /* Итоговый отчет */
    printf("\n============================================\n");
    printf("  Test Summary\n");
    printf("============================================\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Success Rate: %.1f%%\n",
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    printf("============================================\n\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
