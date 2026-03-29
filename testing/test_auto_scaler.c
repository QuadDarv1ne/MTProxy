/*
    MTProxy Auto-Scaler Tests
    Тесты для системы автоматического масштабирования
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "system/cluster/auto-scaler.h"
#include "testing/test_common.h"

// ============================================================================
// Тесты инициализации
// ============================================================================

static int test_auto_scaler_init(void) {
    TEST_START();
    
    int result = auto_scaler_init("test-cluster");
    
    ASSERT(result == 0, "Инициализация успешна");
    ASSERT(auto_scaler_is_initialized() == true, "Auto-scaler инициализирован");
    
    // Повторная инициализация
    result = auto_scaler_init("test-cluster");
    ASSERT(result == 0, "Повторная инициализация успешна");
    
    TEST_END();
}

static int test_auto_scaler_init_null(void) {
    TEST_START();
    
    int result = auto_scaler_init(NULL);
    ASSERT(result == -1, "NULL cluster_name отклонён");
    
    TEST_END();
}

static int test_auto_scaler_cleanup(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    auto_scaler_cleanup();
    
    ASSERT(auto_scaler_is_initialized() == false, "Auto-scaler очищен");
    
    TEST_END();
}

// ============================================================================
// Тесты конфигурации
// ============================================================================

static int test_auto_scaler_set_policy(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    
    int result = auto_scaler_set_policy(AUTO_SCALER_POLICY_AGGRESSIVE);
    ASSERT(result == 0, "Политика установлена");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

static int test_auto_scaler_set_limits(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    
    int result = auto_scaler_set_limits(2, 8);
    ASSERT(result == 0, "Лимиты установлены");
    
    // Невалидные лимиты
    result = auto_scaler_set_limits(10, 5);
    ASSERT(result == -1, "Невалидные лимиты отклонены");
    
    result = auto_scaler_set_limits(0, 5);
    ASSERT(result == -1, "min_nodes = 0 отклонён");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

static int test_auto_scaler_add_metric(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    
    int result = auto_scaler_add_metric(AUTO_SCALER_METRIC_CPU, 80.0, 30.0);
    ASSERT(result == 0, "Метрика CPU добавлена");
    
    result = auto_scaler_add_metric(AUTO_SCALER_METRIC_MEMORY, 85.0, 40.0);
    ASSERT(result == 0, "Метрика Memory добавлена");
    
    // Дублирующаяся метрика
    result = auto_scaler_add_metric(AUTO_SCALER_METRIC_CPU, 90.0, 20.0);
    ASSERT(result == -1, "Дублирующаяся метрика отклонена");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

static int test_auto_scaler_configure_metric(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    auto_scaler_add_metric(AUTO_SCALER_METRIC_CPU, 80.0, 30.0);
    
    auto_scaler_metric_config_t config = {0};
    config.type = AUTO_SCALER_METRIC_CPU;
    config.scale_up_threshold = 90.0;
    config.scale_down_threshold = 20.0;
    config.weight = 5;
    config.enabled = true;
    
    int result = auto_scaler_configure_metric(AUTO_SCALER_METRIC_CPU, &config);
    ASSERT(result == 0, "Метрика настроена");
    
    // Несуществующая метрика
    result = auto_scaler_configure_metric(AUTO_SCALER_METRIC_CONNECTIONS, &config);
    ASSERT(result == -1, "Несуществующая метрика отклонена");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты запуска и остановки
// ============================================================================

static int test_auto_scaler_start_stop(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    
    int result = auto_scaler_start();
    ASSERT(result == 0, "Auto-scaler запущен");
    ASSERT(auto_scaler_is_running() == true, "Auto-scaler работает");
    
    auto_scaler_stop();
    ASSERT(auto_scaler_is_running() == false, "Auto-scaler остановлен");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты мониторинга
// ============================================================================

static int test_auto_scaler_get_metric_value(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    auto_scaler_add_metric(AUTO_SCALER_METRIC_CPU, 80.0, 30.0);
    
    double value = auto_scaler_get_metric_value(AUTO_SCALER_METRIC_CPU);
    // Значение зависит от системы
    
    value = auto_scaler_get_metric_value(AUTO_SCALER_METRIC_CONNECTIONS);
    ASSERT(value == 0.0, "Метрика CONNECTIONS = 0 (не добавлена)");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

static int test_auto_scaler_get_metric_status(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    auto_scaler_add_metric(AUTO_SCALER_METRIC_CPU, 80.0, 30.0);
    
    const auto_scaler_metric_status_t *status = 
        auto_scaler_get_metric_status(AUTO_SCALER_METRIC_CPU);
    ASSERT(status != NULL, "Статус метрики получен");
    ASSERT(status->type == AUTO_SCALER_METRIC_CPU, "Тип метрики совпадает");
    
    status = auto_scaler_get_metric_status(AUTO_SCALER_METRIC_MEMORY);
    ASSERT(status == NULL, "Статус несуществующей метрики = NULL");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

static int test_auto_scaler_get_total_load_percent(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    
    double load = auto_scaler_get_total_load_percent();
    ASSERT(load == 0.0, "Нагрузка по умолчанию 0%");
    
    auto_scaler_add_metric(AUTO_SCALER_METRIC_CPU, 80.0, 30.0);
    
    load = auto_scaler_get_total_load_percent();
    ASSERT(load >= 0.0 && load <= 100.0, "Нагрузка в диапазоне 0-100%");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

static int test_auto_scaler_needs_scaling(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    
    bool needs = auto_scaler_needs_scaling();
    ASSERT(needs == false, "Масштабирование не требуется");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты управления узлами
// ============================================================================

static int test_auto_scaler_set_current_nodes(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    
    int result = auto_scaler_set_current_nodes(3);
    ASSERT(result == 0, "Количество узлов установлено");
    
    int nodes = auto_scaler_get_current_nodes();
    ASSERT(nodes == 3, "Количество узлов = 3");
    
    result = auto_scaler_set_current_nodes(-1);
    ASSERT(result == -1, "Отрицательное количество отклонено");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

static int test_auto_scaler_get_desired_nodes(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    auto_scaler_set_current_nodes(3);
    
    int desired = auto_scaler_get_desired_nodes();
    ASSERT(desired == 3, "Желаемое количество = текущему (нет необходимости)");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты статистики
// ============================================================================

static int test_auto_scaler_get_stats(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    auto_scaler_set_current_nodes(3);
    
    auto_scaler_stats_t stats;
    int result = auto_scaler_get_stats(&stats);
    
    ASSERT(result == 0, "Статистика получена");
    ASSERT(stats.current_nodes == 3, "Текущее количество узлов = 3");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

static int test_auto_scaler_get_stats_string(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    
    char buffer[512] = {0};
    int result = auto_scaler_get_stats_string(buffer, sizeof(buffer));
    
    ASSERT(result == 0, "Строка статистики получена");
    ASSERT(strlen(buffer) > 0, "Строка не пустая");
    ASSERT(strstr(buffer, "Auto-Scaler") != NULL, "Содержит заголовок");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

static int test_auto_scaler_reset_stats(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    
    auto_scaler_reset_stats();
    
    auto_scaler_stats_t stats;
    auto_scaler_get_stats(&stats);
    
    ASSERT(stats.total_scale_up_events == 0, "scale_up_events сброшен");
    ASSERT(stats.total_scale_down_events == 0, "scale_down_events сброшен");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

static int test_auto_scaler_get_scale_events_count(void) {
    TEST_START();
    
    auto_scaler_init("test-cluster");
    
    int count = auto_scaler_get_scale_events_count();
    ASSERT(count == 0, "Событий масштабирования: 0");
    
    auto_scaler_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты утилит
// ============================================================================

static int test_auto_scaler_policy_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(auto_scaler_policy_to_string(AUTO_SCALER_POLICY_CONSERVATIVE), "Conservative") == 0, "Conservative");
    ASSERT(strcmp(auto_scaler_policy_to_string(AUTO_SCALER_POLICY_MODERATE), "Moderate") == 0, "Moderate");
    ASSERT(strcmp(auto_scaler_policy_to_string(AUTO_SCALER_POLICY_AGGRESSIVE), "Aggressive") == 0, "Aggressive");
    
    TEST_END();
}

static int test_auto_scaler_metric_type_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(auto_scaler_metric_type_to_string(AUTO_SCALER_METRIC_CPU), "CPU") == 0, "CPU");
    ASSERT(strcmp(auto_scaler_metric_type_to_string(AUTO_SCALER_METRIC_MEMORY), "Memory") == 0, "Memory");
    ASSERT(strcmp(auto_scaler_metric_type_to_string(AUTO_SCALER_METRIC_CONNECTIONS), "Connections") == 0, "Connections");
    
    TEST_END();
}

static int test_auto_scaler_scale_direction_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(auto_scaler_scale_direction_to_string(AUTO_SCALER_SCALE_NONE), "None") == 0, "None");
    ASSERT(strcmp(auto_scaler_scale_direction_to_string(AUTO_SCALER_SCALE_UP), "Scale Up") == 0, "Scale Up");
    ASSERT(strcmp(auto_scaler_scale_direction_to_string(AUTO_SCALER_SCALE_DOWN), "Scale Down") == 0, "Scale Down");
    
    TEST_END();
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    test_init("MTProxy Auto-Scaler Tests");
    
    // Инициализация
    test_run(test_auto_scaler_init);
    test_run(test_auto_scaler_init_null);
    test_run(test_auto_scaler_cleanup);
    
    // Конфигурация
    test_run(test_auto_scaler_set_policy);
    test_run(test_auto_scaler_set_limits);
    test_run(test_auto_scaler_add_metric);
    test_run(test_auto_scaler_configure_metric);
    
    // Запуск/остановка
    test_run(test_auto_scaler_start_stop);
    
    // Мониторинг
    test_run(test_auto_scaler_get_metric_value);
    test_run(test_auto_scaler_get_metric_status);
    test_run(test_auto_scaler_get_total_load_percent);
    test_run(test_auto_scaler_needs_scaling);
    
    // Управление узлами
    test_run(test_auto_scaler_set_current_nodes);
    test_run(test_auto_scaler_get_desired_nodes);
    
    // Статистика
    test_run(test_auto_scaler_get_stats);
    test_run(test_auto_scaler_get_stats_string);
    test_run(test_auto_scaler_reset_stats);
    test_run(test_auto_scaler_get_scale_events_count);
    
    // Утилиты
    test_run(test_auto_scaler_policy_to_string);
    test_run(test_auto_scaler_metric_type_to_string);
    test_run(test_auto_scaler_scale_direction_to_string);
    
    return test_finish();
}
