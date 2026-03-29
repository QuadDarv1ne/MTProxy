/**
 * @file test_ml_integration.c
 * @brief Тесты для системы интеграции ML с Alert Manager
 * 
 * @version 1.0.32
 * @date 29 марта 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#include "../system/ml/ml-integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/* ============================================
 * Тестовые утилиты
 * ============================================ */

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("  ✓ %s\n", message); \
        } else { \
            tests_failed++; \
            printf("  ✗ FAILED: %s\n", message); \
        } \
    } while(0)

/* ============================================
 * Callback функции для тестов
 * ============================================ */

static int alert_callback_count = 0;
static int last_alert_level = 0;
static char last_alert_type[64] = {0};
static char last_alert_message[256] = {0};

static void test_alert_callback(int level, const char* type, const char* message, void* user_data) {
    alert_callback_count++;
    last_alert_level = level;
    strncpy(last_alert_type, type, sizeof(last_alert_type) - 1);
    strncpy(last_alert_message, message, sizeof(last_alert_message) - 1);
    (void)user_data;
}

static int log_callback_count = 0;
static char last_log_message[256] = {0};

static void test_log_callback(int level, const char* message, void* user_data) {
    log_callback_count++;
    strncpy(last_log_message, message, sizeof(last_log_message) - 1);
    (void)user_data;
}

/* ============================================
 * Тесты инициализации и очистки
 * ============================================ */

static void test_ml_integration_init_cleanup(void) {
    printf("\n=== Test: ML Integration Init/Cleanup ===\n");
    
    ml_integration_t ml;
    
    /* Тест 1: Инициализация */
    int ret = ml_integration_init(&ml, 10);
    TEST_ASSERT(ret == 0, "Init returns 0");
    TEST_ASSERT(ml.n_monitors == 0, "No monitors after init");
    TEST_ASSERT(ml.max_monitors == 10, "Max monitors set correctly");
    
    ml_integration_cleanup(&ml);
    
    /* Тест 2: Очистка NULL */
    ml_integration_cleanup(NULL);
    TEST_ASSERT(1, "Cleanup with NULL doesn't crash");
    
    /* Тест 3: Инициализация с большим количеством мониторов */
    ret = ml_integration_init(&ml, 100);
    TEST_ASSERT(ret == 0, "Init with 100 monitors returns 0");
    TEST_ASSERT(ml.max_monitors == 100, "Max monitors is 100");
    
    ml_integration_cleanup(&ml);
}

/* ============================================
 * Тесты добавления/удаления мониторов
 * ============================================ */

static void test_ml_integration_add_remove_monitor(void) {
    printf("\n=== Test: ML Integration Add/Remove Monitor ===\n");
    
    ml_integration_t ml;
    ml_integration_init(&ml, 10);
    
    /* Тест 1: Добавление монитора аномалий */
    ml_monitor_config_t config = {0};
    strncpy(config.name, "TestAnomaly", sizeof(config.name) - 1);
    config.type = ML_MONITOR_ANOMALY;
    config.anomaly_algorithm = ANOMALY_ALGO_ZSCORE;
    config.anomaly_threshold = 0.6f;
    config.enable_alerts = false;
    
    int monitor_id;
    int ret = ml_integration_add_monitor(&ml, &config, &monitor_id);
    TEST_ASSERT(ret == 0, "Add anomaly monitor returns 0");
    TEST_ASSERT(monitor_id == 0, "Monitor ID is 0");
    TEST_ASSERT(ml.n_monitors == 1, "Monitors count is 1");
    
    /* Тест 2: Добавление монитора прогнозов */
    memset(&config, 0, sizeof(config));
    strncpy(config.name, "TestForecast", sizeof(config.name) - 1);
    config.type = ML_MONITOR_FORECAST;
    config.forecast_algorithm = PREDICT_ALGO_LINEAR_REGRESSION;
    config.forecast_horizon = 10;
    config.warning_threshold = 75.0f;
    config.critical_threshold = 90.0f;
    
    ret = ml_integration_add_monitor(&ml, &config, &monitor_id);
    TEST_ASSERT(ret == 0, "Add forecast monitor returns 0");
    TEST_ASSERT(monitor_id == 1, "Monitor ID is 1");
    TEST_ASSERT(ml.n_monitors == 2, "Monitors count is 2");
    
    /* Тест 3: Удаление монитора */
    ret = ml_integration_remove_monitor(&ml, 0);
    TEST_ASSERT(ret == 0, "Remove monitor returns 0");
    TEST_ASSERT(ml.n_monitors == 1, "Monitors count is 1 after removal");
    
    /* Тест 4: Удаление несуществующего монитора */
    ret = ml_integration_remove_monitor(&ml, 100);
    TEST_ASSERT(ret == -1, "Remove non-existent monitor returns -1");
    
    ml_integration_cleanup(&ml);
}

/* ============================================
 * Тесты обновления мониторов
 * ============================================ */

static void test_ml_integration_update_monitor(void) {
    printf("\n=== Test: ML Integration Update Monitor ===\n");
    
    ml_integration_t ml;
    ml_integration_init(&ml, 10);
    
    /* Добавление монитора аномалий */
    ml_monitor_config_t config = {0};
    strncpy(config.name, "UpdateTest", sizeof(config.name) - 1);
    config.type = ML_MONITOR_ANOMALY;
    config.anomaly_algorithm = ANOMALY_ALGO_ZSCORE;
    config.anomaly_threshold = 0.6f;
    config.enable_alerts = false;
    
    int monitor_id;
    ml_integration_add_monitor(&ml, &config, &monitor_id);
    
    /* Тест 1: Обновление нормальными данными */
    for (int i = 0; i < 50; i++) {
        double value = 100.0 + 5.0 * sin((double)i / 5.0);
        ml_integration_update_monitor(&ml, monitor_id, value, 0);
    }
    
    ml_monitor_stats_t stats;
    ml_integration_get_monitor_stats(&ml, monitor_id, &stats);
    TEST_ASSERT(stats.total_checks == 50, "Total checks is 50");
    TEST_ASSERT(stats.anomalies_detected == 0, "No anomalies in normal data");
    
    /* Тест 2: Обновление аномальными данными */
    double anomalous_value = 200.0;
    ml_integration_update_monitor(&ml, monitor_id, anomalous_value, 0);
    
    ml_integration_get_monitor_stats(&ml, monitor_id, &stats);
    TEST_ASSERT(stats.total_checks == 51, "Total checks is 51");
    TEST_ASSERT(stats.anomalies_detected >= 1, "Anomaly detected");
    
    /* Тест 3: Обновление несуществующего монитора */
    int ret = ml_integration_update_monitor(&ml, 100, 50.0, 0);
    TEST_ASSERT(ret == -1, "Update non-existent monitor returns -1");
    
    ml_integration_cleanup(&ml);
}

/* ============================================
 * Тесты Alert callback
 * ============================================ */

static void test_ml_integration_alert_callback(void) {
    printf("\n=== Test: ML Integration Alert Callback ===\n");
    
    ml_integration_t ml;
    ml_integration_init(&ml, 10);
    
    /* Установка callback */
    alert_callback_count = 0;
    ml_integration_set_alert_callback(&ml, test_alert_callback, NULL);
    
    /* Добавление монитора с включенными алертами */
    ml_monitor_config_t config = {0};
    strncpy(config.name, "AlertTest", sizeof(config.name) - 1);
    config.type = ML_MONITOR_ANOMALY;
    config.anomaly_algorithm = ANOMALY_ALGO_ZSCORE;
    config.anomaly_threshold = 0.5f; /* Низкий порог */
    config.enable_alerts = true;
    
    int monitor_id;
    ml_integration_add_monitor(&ml, &config, &monitor_id);
    
    /* Обучение на нормальных данных */
    for (int i = 0; i < 30; i++) {
        ml_integration_update_monitor(&ml, monitor_id, 50.0, 0);
    }
    
    /* Тест 1: Алерт при аномалии */
    alert_callback_count = 0;
    ml_integration_update_monitor(&ml, monitor_id, 150.0, 0);
    
    TEST_ASSERT(alert_callback_count >= 1, "Alert callback triggered");
    TEST_ASSERT(last_alert_level >= 3, "Alert level is ERROR or higher");
    TEST_ASSERT(strstr(last_alert_type, "Anomaly") != NULL, "Alert type contains 'Anomaly'");
    
    ml_integration_cleanup(&ml);
}

/* ============================================
 * Тесты Log callback
 * ============================================ */

static void test_ml_integration_log_callback(void) {
    printf("\n=== Test: ML Integration Log Callback ===\n");
    
    ml_integration_t ml;
    ml_integration_init(&ml, 10);
    
    /* Установка callback */
    log_callback_count = 0;
    ml_integration_set_log_callback(&ml, test_log_callback, NULL);
    
    /* Добавление монитора */
    ml_monitor_config_t config = {0};
    strncpy(config.name, "LogTest", sizeof(config.name) - 1);
    config.type = ML_MONITOR_ANOMALY;
    config.enable_logging = true;
    
    int monitor_id;
    ml_integration_add_monitor(&ml, &config, &monitor_id);
    
    /* Тест 1: Логирование при добавлении */
    TEST_ASSERT(log_callback_count >= 1, "Log callback triggered on add");
    
    ml_integration_cleanup(&ml);
}

/* ============================================
 * Тесты паузы/возобновления
 * ============================================ */

static void test_ml_integration_pause_resume(void) {
    printf("\n=== Test: ML Integration Pause/Resume ===\n");
    
    ml_integration_t ml;
    ml_integration_init(&ml, 10);
    
    /* Добавление монитора */
    ml_monitor_config_t config = {0};
    strncpy(config.name, "PauseTest", sizeof(config.name) - 1);
    config.type = ML_MONITOR_ANOMALY;
    
    int monitor_id;
    ml_integration_add_monitor(&ml, &config, &monitor_id);
    
    /* Тест 1: Приостановка */
    int ret = ml_integration_pause_monitor(&ml, monitor_id);
    TEST_ASSERT(ret == 0, "Pause monitor returns 0");
    
    ml_monitor_stats_t stats;
    ml_integration_get_monitor_stats(&ml, monitor_id, &stats);
    TEST_ASSERT(stats.status == ML_MONITOR_STATUS_PAUSED, "Status is PAUSED");
    
    /* Тест 2: Возобновление */
    ret = ml_integration_resume_monitor(&ml, monitor_id);
    TEST_ASSERT(ret == 0, "Resume monitor returns 0");
    
    ml_integration_get_monitor_stats(&ml, monitor_id, &stats);
    TEST_ASSERT(stats.status == ML_MONITOR_STATUS_ACTIVE, "Status is ACTIVE");
    
    /* Тест 3: Приостановка несуществующего монитора */
    ret = ml_integration_pause_monitor(&ml, 100);
    TEST_ASSERT(ret == -1, "Pause non-existent returns -1");
    
    ml_integration_cleanup(&ml);
}

/* ============================================
 * Тесты статистики
 * ============================================ */

static void test_ml_integration_statistics(void) {
    printf("\n=== Test: ML Integration Statistics ===\n");
    
    ml_integration_t ml;
    ml_integration_init(&ml, 10);
    
    /* Добавление мониторов */
    ml_monitor_config_t config = {0};
    strncpy(config.name, "StatsTest1", sizeof(config.name) - 1);
    config.type = ML_MONITOR_ANOMALY;
    
    int monitor_id1;
    ml_integration_add_monitor(&ml, &config, &monitor_id1);
    
    strncpy(config.name, "StatsTest2", sizeof(config.name) - 1);
    int monitor_id2;
    ml_integration_add_monitor(&ml, &config, &monitor_id2);
    
    /* Обновление данных */
    for (int i = 0; i < 20; i++) {
        ml_integration_update_monitor(&ml, monitor_id1, 50.0 + i, 0);
        ml_integration_update_monitor(&ml, monitor_id2, 100.0 - i * 2, 0);
    }
    
    /* Тест 1: Получение статистики монитора */
    ml_monitor_stats_t stats;
    int ret = ml_integration_get_monitor_stats(&ml, monitor_id1, &stats);
    TEST_ASSERT(ret == 0, "Get monitor stats returns 0");
    TEST_ASSERT(stats.total_checks == 20, "Total checks is 20");
    
    /* Тест 2: Получение общей статистики */
    uint64_t total_anomalies, total_forecasts, total_alerts;
    ml_integration_get_stats(&ml, &total_anomalies, &total_forecasts, &total_alerts);
    TEST_ASSERT(total_anomalies >= 0, "Total anomalies is non-negative");
    
    /* Тест 3: Получение статистики несуществующего монитора */
    ret = ml_integration_get_monitor_stats(&ml, 100, &stats);
    TEST_ASSERT(ret == -1, "Get stats for non-existent returns -1");
    
    ml_integration_cleanup(&ml);
}

/* ============================================
 * Тесты Forecast монитора
 * ============================================ */

static void test_ml_integration_forecast_monitor(void) {
    printf("\n=== Test: ML Integration Forecast Monitor ===\n");
    
    ml_integration_t ml;
    ml_integration_init(&ml, 10);
    
    /* Добавление монитора прогнозов */
    ml_monitor_config_t config = {0};
    strncpy(config.name, "ForecastTest", sizeof(config.name) - 1);
    config.type = ML_MONITOR_FORECAST;
    config.forecast_algorithm = PREDICT_ALGO_LINEAR_REGRESSION;
    config.forecast_horizon = 5;
    config.warning_threshold = 150.0f;
    config.critical_threshold = 200.0f;
    config.enable_alerts = false;
    
    int monitor_id;
    ml_integration_add_monitor(&ml, &config, &monitor_id);
    
    /* Тест 1: Обновление линейными данными */
    for (int i = 0; i < 50; i++) {
        double value = 50.0 + i * 0.5;
        ml_integration_update_monitor(&ml, monitor_id, value, 0);
    }
    
    ml_monitor_stats_t stats;
    ml_integration_get_monitor_stats(&ml, monitor_id, &stats);
    TEST_ASSERT(stats.total_checks == 50, "Total checks is 50");
    TEST_ASSERT(stats.last_forecast_value > 0, "Last forecast value is set");
    
    ml_integration_cleanup(&ml);
}

/* ============================================
 * Тесты JSON экспорта
 * ============================================ */

static void test_ml_integration_json_export(void) {
    printf("\n=== Test: ML Integration JSON Export ===\n");
    
    ml_integration_t ml;
    ml_integration_init(&ml, 10);
    
    /* Добавление мониторов */
    ml_monitor_config_t config = {0};
    strncpy(config.name, "JSONTest1", sizeof(config.name) - 1);
    config.type = ML_MONITOR_ANOMALY;
    int monitor_id1;
    ml_integration_add_monitor(&ml, &config, &monitor_id1);
    
    strncpy(config.name, "JSONTest2", sizeof(config.name) - 1);
    config.type = ML_MONITOR_FORECAST;
    int monitor_id2;
    ml_integration_add_monitor(&ml, &config, &monitor_id2);
    
    /* Тест 1: Экспорт в JSON */
    char buffer[2048];
    int written = ml_integration_export_json(&ml, buffer, sizeof(buffer));
    TEST_ASSERT(written > 0, "JSON export returns positive bytes");
    TEST_ASSERT(written < (int)sizeof(buffer), "JSON fits in buffer");
    TEST_ASSERT(strstr(buffer, "\"monitors\"") != NULL, "JSON contains monitors");
    TEST_ASSERT(strstr(buffer, "\"total_anomalies\"") != NULL, "JSON contains total_anomalies");
    TEST_ASSERT(strstr(buffer, "JSONTest1") != NULL, "JSON contains monitor name");
    
    printf("    JSON output (%d bytes):\n", written);
    printf("    %.*s\n", (written < 300) ? written : 300, buffer);
    
    /* Тест 2: Экспорт с NULL буфером */
    written = ml_integration_export_json(&ml, NULL, 0);
    TEST_ASSERT(written == -1, "Export with NULL buffer returns -1");
    
    ml_integration_cleanup(&ml);
}

/* ============================================
 * Тесты строковых конвертаций
 * ============================================ */

static void test_ml_integration_string_conversion(void) {
    printf("\n=== Test: ML Integration String Conversion ===\n");
    
    /* Тест 1: Конвертация типа монитора */
    const char* type_str = ml_monitor_type_to_string(ML_MONITOR_ANOMALY);
    TEST_ASSERT(strcmp(type_str, "Anomaly") == 0, "Type to string: Anomaly");
    
    type_str = ml_monitor_type_to_string(ML_MONITOR_FORECAST);
    TEST_ASSERT(strcmp(type_str, "Forecast") == 0, "Type to string: Forecast");
    
    /* Тест 2: Конвертация статуса */
    const char* status_str = ml_monitor_status_to_string(ML_MONITOR_STATUS_ACTIVE);
    TEST_ASSERT(strcmp(status_str, "Active") == 0, "Status to string: Active");
    
    status_str = ml_monitor_status_to_string(ML_MONITOR_STATUS_PAUSED);
    TEST_ASSERT(strcmp(status_str, "Paused") == 0, "Status to string: Paused");
}

/* ============================================
 * Тесты производительности
 * ============================================ */

static void test_ml_integration_performance(void) {
    printf("\n=== Test: ML Integration Performance ===\n");
    
    ml_integration_t ml;
    ml_integration_init(&ml, 10);
    
    /* Добавление монитора */
    ml_monitor_config_t config = {0};
    strncpy(config.name, "PerfTest", sizeof(config.name) - 1);
    config.type = ML_MONITOR_ANOMALY;
    config.anomaly_algorithm = ANOMALY_ALGO_ZSCORE;
    
    int monitor_id;
    ml_integration_add_monitor(&ml, &config, &monitor_id);
    
    /* Тест 1: Производительность обновления */
    uint64_t start = GetTickCount64();
    
    for (int i = 0; i < 1000; i++) {
        double value = 100.0 + 10.0 * sin((double)i / 10.0);
        ml_integration_update_monitor(&ml, monitor_id, value, 0);
    }
    
    uint64_t elapsed = GetTickCount64() - start;
    double avg_time_ms = (double)elapsed / 1000.0;
    
    TEST_ASSERT(avg_time_ms < 1.0, "Average update time < 1ms");
    printf("    1000 updates in %llu ms (%.3f ms/update)\n", elapsed, avg_time_ms);
    
    ml_integration_cleanup(&ml);
}

/* ============================================
 * Главная функция
 * ============================================ */

int main(void) {
    printf("============================================\n");
    printf("  ML Integration Tests\n");
    printf("  Version: 1.0.32\n");
    printf("============================================\n");
    
    /* Запуск тестов */
    test_ml_integration_init_cleanup();
    test_ml_integration_add_remove_monitor();
    test_ml_integration_update_monitor();
    test_ml_integration_alert_callback();
    test_ml_integration_log_callback();
    test_ml_integration_pause_resume();
    test_ml_integration_statistics();
    test_ml_integration_forecast_monitor();
    test_ml_integration_json_export();
    test_ml_integration_string_conversion();
    test_ml_integration_performance();
    
    /* Итоговый отчет */
    printf("\n============================================\n");
    printf("  Test Summary\n");
    printf("============================================\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Success Rate: %.1f%%\n", 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    printf("============================================\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
