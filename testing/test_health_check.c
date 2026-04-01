/*
    MTProxy Health Check Tests
    Тесты для системы проверки здоровья узлов
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "system/monitoring/health-check.h"
#include "testing/test_common.h"

// ============================================================================
// Тесты инициализации
// ============================================================================

static int test_health_check_init(void) {
    TEST_START();
    
    int result = health_check_init();
    
    ASSERT(result == 0, "Инициализация успешна");
    ASSERT(health_check_is_initialized() == true, "Система инициализирована");
    
    // Повторная инициализация
    result = health_check_init();
    ASSERT(result == 0, "Повторная инициализация успешна");
    
    TEST_END();
}

static int test_health_check_cleanup(void) {
    TEST_START();
    
    health_check_init();
    health_check_cleanup();
    
    ASSERT(health_check_is_initialized() == false, "Система очищена");
    
    TEST_END();
}

// ============================================================================
// Тесты добавления узлов
// ============================================================================

static int test_health_check_add_http_node(void) {
    TEST_START();
    
    health_check_init();
    
    int result = health_check_add_http_node("web1", "http://localhost:8080/health", 3000);
    ASSERT(result == 0, "HTTP узел добавлен");
    
    result = health_check_add_http_node("web2", "https://example.com/health", 5000);
    ASSERT(result == 0, "HTTPS узел добавлен");
    
    // Дублирующееся имя
    result = health_check_add_http_node("web1", "http://other:80/health", 3000);
    ASSERT(result == -1, "Дублирующееся имя отклонено");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_add_tcp_node(void) {
    TEST_START();
    
    health_check_init();
    
    int result = health_check_add_tcp_node("db1", "localhost", 5432, 2000);
    ASSERT(result == 0, "TCP узел добавлен");
    
    result = health_check_add_tcp_node("redis", "127.0.0.1", 6379, 1000);
    ASSERT(result == 0, "TCP узел (IP) добавлен");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_add_process_node(void) {
    TEST_START();
    
    health_check_init();
    
    // Добавляем текущий процесс
    int pid = getpid();
    int result = health_check_add_process_node("self", pid);
    ASSERT(result == 0, "Process узел добавлен");
    
    // Неверный PID
    result = health_check_add_process_node("invalid", -1);
    ASSERT(result == -1, "Неверный PID отклонён");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_add_memory_node(void) {
    TEST_START();
    
    health_check_init();
    
    int result = health_check_add_memory_node("memory", 8ULL * 1024 * 1024 * 1024);
    ASSERT(result == 0, "Memory узел добавлен");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_add_disk_node(void) {
    TEST_START();
    
    health_check_init();
    
    int result = health_check_add_disk_node("disk-root", "/", 90);
    ASSERT(result == 0, "Disk узел добавлен");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_remove_node(void) {
    TEST_START();
    
    health_check_init();
    
    health_check_add_http_node("web1", "http://localhost:8080/health", 3000);
    
    int result = health_check_remove_node("web1");
    ASSERT(result == 0, "Узел удалён");
    
    result = health_check_remove_node("web1");
    ASSERT(result == -1, "Повторное удаление невозможно");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_set_node_enabled(void) {
    TEST_START();
    
    health_check_init();
    health_check_add_http_node("web1", "http://localhost:8080/health", 3000);
    
    int result = health_check_set_node_enabled("web1", false);
    ASSERT(result == 0, "Узел выключен");
    
    result = health_check_set_node_enabled("web1", true);
    ASSERT(result == 0, "Узел включен");
    
    result = health_check_set_node_enabled("unknown", false);
    ASSERT(result == -1, "Неизвестный узел");
    
    health_check_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты выполнения проверок
// ============================================================================

static int test_health_check_run_once(void) {
    TEST_START();
    
    health_check_init();
    
    // Добавляем узлы
    health_check_add_http_node("web1", "http://localhost:8080/health", 1000);
    health_check_add_tcp_node("tcp1", "127.0.0.1", 9999, 500);  // Закрытый порт
    
    int result = health_check_run_once();
    ASSERT(result == 0, "Проверка выполнена");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_check_node(void) {
    TEST_START();
    
    health_check_init();
    health_check_add_http_node("web1", "http://localhost:8080/health", 1000);
    
    const health_check_result_t *result = health_check_check_node("web1");
    ASSERT(result != NULL, "Результат проверки получен");
    ASSERT(strcmp(result->name, "web1") == 0, "Имя узла совпадает");
    
    result = health_check_check_node("unknown");
    ASSERT(result == NULL, "Неизвестный узел");
    
    health_check_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты статуса
// ============================================================================

static int test_health_check_get_node_status(void) {
    TEST_START();
    
    health_check_init();
    health_check_add_http_node("web1", "http://localhost:8080/health", 1000);
    
    // До первой проверки статус unknown
    health_status_t status = health_check_get_node_status("web1");
    ASSERT(status == HEALTH_STATUS_UNKNOWN, "Статус до проверки: UNKNOWN");
    
    // Выполняем проверку
    health_check_check_node("web1");
    
    status = health_check_get_node_status("web1");
    ASSERT(status == HEALTH_STATUS_HEALTHY || status == HEALTH_STATUS_UNHEALTHY,
           "Статус после проверки: HEALTHY или UNHEALTHY");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_get_healthy_count(void) {
    TEST_START();
    
    health_check_init();
    
    int count = health_check_get_healthy_count();
    ASSERT(count == 0, "Нет узлов - 0 здоровых");
    
    health_check_add_http_node("web1", "http://localhost:8080/health", 1000);
    health_check_add_http_node("web2", "http://localhost:8081/health", 1000);
    
    health_check_run_once();
    
    count = health_check_get_healthy_count();
    ASSERT(count >= 0 && count <= 2, "Количество здоровых узлов в диапазоне");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_get_total_count(void) {
    TEST_START();
    
    health_check_init();
    
    int count = health_check_get_total_count();
    ASSERT(count == 0, "Нет узлов");
    
    health_check_add_http_node("web1", "http://localhost:8080/health", 1000);
    health_check_add_tcp_node("tcp1", "localhost", 5432, 1000);
    
    count = health_check_get_total_count();
    ASSERT(count == 2, "Два узла");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_all_healthy(void) {
    TEST_START();
    
    health_check_init();
    
    bool all = health_check_all_healthy();
    ASSERT(all == false, "Нет узлов - не все здоровы");
    
    health_check_add_http_node("web1", "http://localhost:8080/health", 1000);
    
    // До проверки
    all = health_check_all_healthy();
    ASSERT(all == false, "До проверки - не все здоровы");
    
    health_check_run_once();
    
    // После проверки (зависит от доступности сервиса)
    all = health_check_all_healthy();
    // ASSERT(all == true/false, "После проверки");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_get_unhealthy_nodes(void) {
    TEST_START();
    
    health_check_init();
    health_check_add_http_node("web1", "http://localhost:8080/health", 1000);
    health_check_add_tcp_node("tcp1", "127.0.0.1", 9999, 100);  // Закрытый порт
    
    health_check_run_once();
    
    char buffer[512] = {0};
    int count = health_check_get_unhealthy_nodes(buffer, sizeof(buffer));
    
    ASSERT(count >= 0 && count <= 2, "Количество нездоровых узлов");
    
    health_check_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты статистики
// ============================================================================

static int test_health_check_get_stats(void) {
    TEST_START();
    
    health_check_init();
    health_check_add_http_node("web1", "http://localhost:8080/health", 1000);
    
    health_check_run_once();
    
    health_check_stats_t stats;
    int result = health_check_get_stats(&stats);
    
    ASSERT(result == 0, "Статистика получена");
    ASSERT(stats.total_nodes == 1, "Всего узлов: 1");
    ASSERT(stats.total_checks >= 1, "Проверок выполнено: >= 1");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_get_stats_string(void) {
    TEST_START();
    
    health_check_init();
    health_check_add_http_node("web1", "http://localhost:8080/health", 1000);
    
    health_check_run_once();
    
    char buffer[512] = {0};
    int result = health_check_get_stats_string(buffer, sizeof(buffer));
    
    ASSERT(result == 0, "Строка статистики получена");
    ASSERT(strlen(buffer) > 0, "Строка не пустая");
    ASSERT(strstr(buffer, "Health Check") != NULL, "Содержит заголовок");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_reset_stats(void) {
    TEST_START();
    
    health_check_init();
    health_check_add_http_node("web1", "http://localhost:8080/health", 1000);
    
    health_check_run_once();
    
    health_check_reset_stats();
    
    health_check_stats_t stats;
    health_check_get_stats(&stats);
    
    ASSERT(stats.total_nodes == 1, "total_nodes сохранён");
    ASSERT(stats.total_checks == 0, "total_checks сброшен");
    ASSERT(stats.successful_checks == 0, "successful_checks сброшен");
    
    health_check_cleanup();
    
    TEST_END();
}

static int test_health_check_get_uptime(void) {
    TEST_START();
    
    health_check_init();
    
    int64_t uptime1 = health_check_get_uptime();
    
#ifdef _WIN32
    Sleep(100);
#else
    usleep(100000);
#endif
    
    int64_t uptime2 = health_check_get_uptime();
    
    ASSERT(uptime2 > uptime1, "Uptime увеличивается");
    
    health_check_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты утилит
// ============================================================================

static int test_health_status_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(health_status_to_string(HEALTH_STATUS_UNKNOWN), "Unknown") == 0, "Unknown");
    ASSERT(strcmp(health_status_to_string(HEALTH_STATUS_HEALTHY), "Healthy") == 0, "Healthy");
    ASSERT(strcmp(health_status_to_string(HEALTH_STATUS_UNHEALTHY), "Unhealthy") == 0, "Unhealthy");
    ASSERT(strcmp(health_status_to_string(HEALTH_STATUS_DEGRADED), "Degraded") == 0, "Degraded");
    
    TEST_END();
}

static int test_health_check_type_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(health_check_type_to_string(HEALTH_CHECK_HTTP), "HTTP") == 0, "HTTP");
    ASSERT(strcmp(health_check_type_to_string(HEALTH_CHECK_TCP), "TCP") == 0, "TCP");
    ASSERT(strcmp(health_check_type_to_string(HEALTH_CHECK_PROCESS), "Process") == 0, "Process");
    ASSERT(strcmp(health_check_type_to_string(HEALTH_CHECK_MEMORY), "Memory") == 0, "Memory");
    ASSERT(strcmp(health_check_type_to_string(HEALTH_CHECK_DISK), "Disk") == 0, "Disk");
    
    TEST_END();
}

static int test_health_check_get_cluster_health_percent(void) {
    TEST_START();
    
    health_check_init();
    
    int percent = health_check_get_cluster_health_percent();
    ASSERT(percent == 0, "Нет узлов - 0%");
    
    health_check_add_http_node("web1", "http://localhost:8080/health", 1000);
    health_check_add_http_node("web2", "http://localhost:8081/health", 1000);
    
    health_check_run_once();
    
    percent = health_check_get_cluster_health_percent();
    ASSERT(percent >= 0 && percent <= 100, "Процент в диапазоне 0-100");
    
    health_check_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты запуска/остановки фонового режима
// ============================================================================

static int test_health_check_start_stop(void) {
    TEST_START();
    
    health_check_init();
    health_check_add_http_node("web1", "http://localhost:8080/health", 1000);
    
    int result = health_check_start(2000);  // Интервал 2 секунды
    ASSERT(result == 0, "Фоновые проверки запущены");
    
    // Даём поработать
#ifdef _WIN32
    Sleep(100);
#else
    usleep(100000);
#endif
    
    health_check_stop();
    
    health_check_cleanup();
    
    TEST_END();
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    test_init("MTProxy Health Check Tests");
    
    // Инициализация
    test_run(test_health_check_init);
    test_run(test_health_check_cleanup);
    
    // Добавление узлов
    test_run(test_health_check_add_http_node);
    test_run(test_health_check_add_tcp_node);
    test_run(test_health_check_add_process_node);
    test_run(test_health_check_add_memory_node);
    test_run(test_health_check_add_disk_node);
    test_run(test_health_check_remove_node);
    test_run(test_health_check_set_node_enabled);
    
    // Выполнение проверок
    test_run(test_health_check_run_once);
    test_run(test_health_check_check_node);
    
    // Статус
    test_run(test_health_check_get_node_status);
    test_run(test_health_check_get_healthy_count);
    test_run(test_health_check_get_total_count);
    test_run(test_health_check_all_healthy);
    test_run(test_health_check_get_unhealthy_nodes);
    
    // Статистика
    test_run(test_health_check_get_stats);
    test_run(test_health_check_get_stats_string);
    test_run(test_health_check_reset_stats);
    test_run(test_health_check_get_uptime);
    
    // Утилиты
    test_run(test_health_status_to_string);
    test_run(test_health_check_type_to_string);
    test_run(test_health_check_get_cluster_health_percent);
    
    // Фоновый режим
    test_run(test_health_check_start_stop);
    
    return test_finish();
}
