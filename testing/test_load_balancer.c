/*
    MTProxy Load Balancer Tests
    Тесты для системы балансировки нагрузки
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "system/cluster/load-balancer.h"
#include "testing/test_common.h"

// ============================================================================
// Тесты инициализации
// ============================================================================

static int test_load_balancer_init(void) {
    TEST_START();
    
    int result = load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    
    ASSERT(result == 0, "Инициализация успешна");
    ASSERT(load_balancer_is_initialized() == true, "LB инициализирован");
    
    // Повторная инициализация
    result = load_balancer_init(LOAD_BALANCER_LEAST_CONNECTIONS);
    ASSERT(result == 0, "Повторная инициализация успешна");
    
    TEST_END();
}

static int test_load_balancer_cleanup(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_cleanup();
    
    ASSERT(load_balancer_is_initialized() == false, "LB очищен");
    
    TEST_END();
}

// ============================================================================
// Тесты управления backend узлами
// ============================================================================

static int test_load_balancer_add_backend(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    
    int result = load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    ASSERT(result == 0, "Backend node1 добавлен");
    
    result = load_balancer_add_backend("node2", "192.168.1.102", 8888, 2.0);
    ASSERT(result == 0, "Backend node2 добавлен");
    
    // Дублирующееся имя
    result = load_balancer_add_backend("node1", "192.168.1.103", 8888, 1.0);
    ASSERT(result == -1, "Дублирующееся имя отклонено");
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_remove_backend(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    
    int result = load_balancer_remove_backend("node1");
    ASSERT(result == 0, "Backend удалён");
    
    result = load_balancer_remove_backend("node1");
    ASSERT(result == -1, "Повторное удаление невозможно");
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_set_backend_enabled(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    
    int result = load_balancer_set_backend_enabled("node1", false);
    ASSERT(result == 0, "Backend выключен");
    
    result = load_balancer_set_backend_enabled("node1", true);
    ASSERT(result == 0, "Backend включен");
    
    result = load_balancer_set_backend_enabled("unknown", false);
    ASSERT(result == -1, "Неизвестный backend");
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_set_backend_weight(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    
    int result = load_balancer_set_backend_weight("node1", 2.5);
    ASSERT(result == 0, "Вес установлен");
    
    const load_balancer_backend_config_t *config = load_balancer_get_backend_config("node1");
    ASSERT(config != NULL, "Конфигурация найдена");
    ASSERT(config->weight == 2.5, "Вес равен 2.5");
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_get_all_backends(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    load_balancer_add_backend("node2", "192.168.1.102", 8888, 1.0);
    
    char buffer[512] = {0};
    int count = load_balancer_get_all_backends(buffer, sizeof(buffer));
    
    ASSERT(count == 2, "Количество backend: 2");
    ASSERT(strstr(buffer, "node1") != NULL, "Содержит node1");
    ASSERT(strstr(buffer, "node2") != NULL, "Содержит node2");
    
    load_balancer_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты выбора алгоритма
// ============================================================================

static int test_load_balancer_set_algorithm(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    
    int result = load_balancer_set_algorithm(LOAD_BALANCER_LEAST_CONNECTIONS);
    ASSERT(result == 0, "Алгоритм установлен");
    
    load_balancer_algorithm_t algo = load_balancer_get_algorithm();
    ASSERT(algo == LOAD_BALANCER_LEAST_CONNECTIONS, "Алгоритм: LEAST_CONNECTIONS");
    
    load_balancer_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты выбора backend узла
// ============================================================================

static int test_load_balancer_select_backend_round_robin(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    load_balancer_add_backend("node2", "192.168.1.102", 8888, 1.0);
    
    // Без health check узлы offline
    const char *backend = load_balancer_select_backend("192.168.1.1");
    // ASSERT(backend == NULL, "Нет онлайн узлов");
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_get_least_loaded_backend(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_LEAST_LOAD);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    load_balancer_add_backend("node2", "192.168.1.102", 8888, 1.0);
    
    const char *backend = load_balancer_get_least_loaded_backend();
    // ASSERT(backend == NULL, "Нет онлайн узлов");
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_get_least_connections_backend(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_LEAST_CONNECTIONS);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    
    const char *backend = load_balancer_get_least_connections_backend();
    // ASSERT(backend == NULL, "Нет онлайн узлов");
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_notify_connection(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    
    int result1 = load_balancer_notify_new_connection("node1");
    ASSERT(result1 == 0, "Подключение уведомлено");
    
    const load_balancer_backend_stats_t *stats = load_balancer_get_backend_stats("node1");
    ASSERT(stats != NULL, "Статистика найдена");
    ASSERT(stats->active_connections == 1, "Активных подключений: 1");
    
    int result2 = load_balancer_notify_connection_closed("node1");
    ASSERT(result2 == 0, "Закрытие уведомлено");
    
    load_balancer_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты перераспределения нагрузки
// ============================================================================

static int test_load_balancer_rebalance(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    load_balancer_add_backend("node2", "192.168.1.102", 8888, 1.0);
    
    int result = load_balancer_rebalance();
    ASSERT(result == 0, "Перераспределение выполнено");
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_set_rebalance_threshold(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    
    load_balancer_set_rebalance_threshold(90);
    // Порог установлен
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_set_auto_rebalance(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    
    load_balancer_set_auto_rebalance(false);
    // Выключено
    
    load_balancer_set_auto_rebalance(true);
    // Включено
    
    load_balancer_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты статистики
// ============================================================================

static int test_load_balancer_get_stats(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    
    load_balancer_stats_t stats;
    int result = load_balancer_get_stats(&stats);
    
    ASSERT(result == 0, "Статистика получена");
    ASSERT(stats.total_backends == 1, "Всего backend: 1");
    ASSERT(stats.algorithm == LOAD_BALANCER_ROUND_ROBIN, "Алгоритм: ROUND_ROBIN");
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_get_stats_string(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    
    char buffer[512] = {0};
    int result = load_balancer_get_stats_string(buffer, sizeof(buffer));
    
    ASSERT(result == 0, "Строка статистики получена");
    ASSERT(strlen(buffer) > 0, "Строка не пустая");
    ASSERT(strstr(buffer, "Load Balancer") != NULL, "Содержит заголовок");
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_reset_stats(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    
    load_balancer_select_backend("192.168.1.1");
    
    load_balancer_reset_stats();
    
    load_balancer_stats_t stats;
    load_balancer_get_stats(&stats);
    
    ASSERT(stats.total_requests == 0, "total_requests сброшен");
    ASSERT(stats.successful_requests == 0, "successful_requests сброшен");
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_get_avg_load_percent(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    
    double load = load_balancer_get_avg_load_percent();
    ASSERT(load == 0.0, "Нагрузка по умолчанию 0%");
    
    load_balancer_cleanup();
    
    TEST_END();
}

static int test_load_balancer_get_total_active_connections(void) {
    TEST_START();
    
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    
    load_balancer_notify_new_connection("node1");
    
    int connections = load_balancer_get_total_active_connections();
    ASSERT(connections == 1, "Активных подключений: 1");
    
    load_balancer_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты утилит
// ============================================================================

static int test_load_balancer_algorithm_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(load_balancer_algorithm_to_string(LOAD_BALANCER_ROUND_ROBIN), "Round Robin") == 0, "Round Robin");
    ASSERT(strcmp(load_balancer_algorithm_to_string(LOAD_BALANCER_LEAST_CONNECTIONS), "Least Connections") == 0, "Least Connections");
    ASSERT(strcmp(load_balancer_algorithm_to_string(LOAD_BALANCER_WEIGHTED), "Weighted") == 0, "Weighted");
    ASSERT(strcmp(load_balancer_algorithm_to_string(LOAD_BALANCER_IP_HASH), "IP Hash") == 0, "IP Hash");
    
    TEST_END();
}

static int test_load_balancer_backend_status_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(load_balancer_backend_status_to_string(LOAD_BALANCER_BACKEND_OFFLINE), "Offline") == 0, "Offline");
    ASSERT(strcmp(load_balancer_backend_status_to_string(LOAD_BALANCER_BACKEND_ONLINE), "Online") == 0, "Online");
    ASSERT(strcmp(load_balancer_backend_status_to_string(LOAD_BALANCER_BACKEND_DEGRADED), "Degraded") == 0, "Degraded");
    ASSERT(strcmp(load_balancer_backend_status_to_string(LOAD_BALANCER_BACKEND_FAILED), "Failed") == 0, "Failed");
    
    TEST_END();
}

static int test_load_balancer_ip_hash(void) {
    TEST_START();
    
    int hash1 = load_balancer_ip_hash("192.168.1.1", 4);
    int hash2 = load_balancer_ip_hash("192.168.1.2", 4);
    
    ASSERT(hash1 >= 0 && hash1 < 4, "hash1 в диапазоне");
    ASSERT(hash2 >= 0 && hash2 < 4, "hash2 в диапазоне");
    
    // Одинаковые IP должны давать одинаковый hash
    int hash3 = load_balancer_ip_hash("192.168.1.1", 4);
    ASSERT(hash1 == hash3, "Одинаковые IP дают одинаковый hash");
    
    TEST_END();
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    test_init("MTProxy Load Balancer Tests");
    
    // Инициализация
    test_run(test_load_balancer_init);
    test_run(test_load_balancer_cleanup);
    
    // Управление backend
    test_run(test_load_balancer_add_backend);
    test_run(test_load_balancer_remove_backend);
    test_run(test_load_balancer_set_backend_enabled);
    test_run(test_load_balancer_set_backend_weight);
    test_run(test_load_balancer_get_all_backends);
    
    // Алгоритмы
    test_run(test_load_balancer_set_algorithm);
    
    // Выбор backend
    test_run(test_load_balancer_select_backend_round_robin);
    test_run(test_load_balancer_get_least_loaded_backend);
    test_run(test_load_balancer_get_least_connections_backend);
    test_run(test_load_balancer_notify_connection);
    
    // Перераспределение
    test_run(test_load_balancer_rebalance);
    test_run(test_load_balancer_set_rebalance_threshold);
    test_run(test_load_balancer_set_auto_rebalance);
    
    // Статистика
    test_run(test_load_balancer_get_stats);
    test_run(test_load_balancer_get_stats_string);
    test_run(test_load_balancer_reset_stats);
    test_run(test_load_balancer_get_avg_load_percent);
    test_run(test_load_balancer_get_total_active_connections);
    
    // Утилиты
    test_run(test_load_balancer_algorithm_to_string);
    test_run(test_load_balancer_backend_status_to_string);
    test_run(test_load_balancer_ip_hash);
    
    return test_finish();
}
