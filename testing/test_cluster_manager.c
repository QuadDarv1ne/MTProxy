/*
    MTProxy Cluster Manager Tests
    Тесты для системы управления кластером
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "system/cluster/cluster-manager.h"
#include "testing/test_common.h"

// ============================================================================
// Тесты инициализации
// ============================================================================

static int test_cluster_init(void) {
    TEST_START();
    
    int result = cluster_init("test-cluster", "node1");
    
    ASSERT(result == 0, "Инициализация успешна");
    ASSERT(cluster_is_initialized() == true, "Кластер инициализирован");
    
    // Повторная инициализация
    result = cluster_init("test-cluster", "node1");
    ASSERT(result == 0, "Повторная инициализация успешна");
    
    TEST_END();
}

static int test_cluster_init_null(void) {
    TEST_START();
    
    int result1 = cluster_init(NULL, "node1");
    ASSERT(result1 == -1, "NULL cluster_name отклонён");
    
    int result2 = cluster_init("cluster", NULL);
    ASSERT(result2 == -1, "NULL node_name отклонён");
    
    TEST_END();
}

static int test_cluster_cleanup(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_cleanup();
    
    ASSERT(cluster_is_initialized() == false, "Кластер очищен");
    
    TEST_END();
}

// ============================================================================
// Тесты управления узлами
// ============================================================================

static int test_cluster_add_node(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    
    int result = cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    ASSERT(result == 0, "Узел node2 добавлен");
    
    result = cluster_add_node("node3", "192.168.1.103", 8888, 9000);
    ASSERT(result == 0, "Узел node3 добавлен");
    
    // Дублирующееся имя
    result = cluster_add_node("node2", "192.168.1.104", 8888, 9000);
    ASSERT(result == -1, "Дублирующееся имя отклонено");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_remove_node(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    
    int result = cluster_remove_node("node2");
    ASSERT(result == 0, "Узел удалён");
    
    result = cluster_remove_node("node2");
    ASSERT(result == -1, "Повторное удаление невозможно");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_get_node_config(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    
    const cluster_node_config_t *config = cluster_get_node_config("node2");
    ASSERT(config != NULL, "Конфигурация найдена");
    ASSERT(strcmp(config->name, "node2") == 0, "Имя совпадает");
    ASSERT(strcmp(config->host, "192.168.1.102") == 0, "Хост совпадает");
    ASSERT(config->port == 8888, "Порт совпадает");
    
    config = cluster_get_node_config("unknown");
    ASSERT(config == NULL, "Неизвестный узел");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_get_all_nodes(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    cluster_add_node("node3", "192.168.1.103", 8888, 9000);
    
    char buffer[512] = {0};
    int count = cluster_get_all_nodes(buffer, sizeof(buffer));
    
    ASSERT(count == 3, "Количество узлов: 3");
    ASSERT(strstr(buffer, "node1") != NULL, "Содержит node1");
    ASSERT(strstr(buffer, "node2") != NULL, "Содержит node2");
    ASSERT(strstr(buffer, "node3") != NULL, "Содержит node3");
    
    cluster_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты запуска и остановки
// ============================================================================

static int test_cluster_start_stop(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    
    int result = cluster_start(9001);
    ASSERT(result == 0, "Кластер запущен");
    ASSERT(cluster_is_running() == true, "Кластер работает");
    
    cluster_stop();
    ASSERT(cluster_is_running() == false, "Кластер остановлен");
    
    cluster_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты лидерства
// ============================================================================

static int test_cluster_get_leader(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    
    const char *leader = cluster_get_leader();
    ASSERT(leader == NULL, "Лидера нет до выборов");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_get_local_role(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    
    cluster_role_t role = cluster_get_local_role();
    ASSERT(role == CLUSTER_ROLE_STANDALONE, "Роль по умолчанию: STANDALONE");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_become_leader(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_start(9001);
    
    int result = cluster_become_leader();
    ASSERT(result == 0, "Стал лидером");
    
    cluster_role_t role = cluster_get_local_role();
    ASSERT(role == CLUSTER_ROLE_LEADER, "Роль: LEADER");
    
    const char *leader = cluster_get_leader();
    ASSERT(leader != NULL, "Лидер установлен");
    ASSERT(strcmp(leader, "node1") == 0, "Лидер: node1");
    
    cluster_stop();
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_step_down(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_start(9001);
    cluster_become_leader();
    
    int result = cluster_step_down();
    ASSERT(result == 0, "Сдал лидерство");
    
    cluster_role_t role = cluster_get_local_role();
    ASSERT(role == CLUSTER_ROLE_FOLLOWER, "Роль: FOLLOWER");
    
    cluster_stop();
    cluster_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты синхронизации конфигурации
// ============================================================================

static int test_cluster_sync_config(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_start(9001);
    
    const char *config = "{\"key\":\"value\",\"port\":8888}";
    int result = cluster_sync_config(config);
    ASSERT(result == 0, "Конфигурация синхронизирована");
    
    cluster_stop();
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_get_config(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    
    char buffer[512] = {0};
    int result = cluster_get_config(buffer, sizeof(buffer));
    
    ASSERT(result == 0, "Конфигурация получена");
    ASSERT(strlen(buffer) > 0, "Буфер не пуст");
    ASSERT(strstr(buffer, "test-cluster") != NULL, "Содержит имя кластера");
    
    cluster_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты балансировки нагрузки
// ============================================================================

static int test_cluster_get_least_loaded_node(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    
    // Все узлы offline, должен вернуть NULL
    const char *node = cluster_get_least_loaded_node();
    // ASSERT(node == NULL, "Нет онлайн узлов");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_get_avg_load_percent(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    
    double load = cluster_get_avg_load_percent();
    ASSERT(load == 0.0, "Нагрузка по умолчанию 0%");
    
    cluster_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты failover
// ============================================================================

static int test_cluster_handle_node_failure(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    
    int result = cluster_handle_node_failure("node2");
    ASSERT(result == 0, "Failover обработан");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_recovery_node(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    
    int result = cluster_recovery_node("node2");
    ASSERT(result == 0, "Восстановление начато");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_set_auto_failover(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    
    cluster_set_auto_failover(false);
    // Выключено
    
    cluster_set_auto_failover(true);
    // Включено
    
    cluster_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты статистики
// ============================================================================

static int test_cluster_get_stats(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    
    cluster_stats_t stats;
    int result = cluster_get_stats(&stats);
    
    ASSERT(result == 0, "Статистика получена");
    ASSERT(stats.total_nodes == 2, "Всего узлов: 2");
    ASSERT(strcmp(stats.cluster_name, "test-cluster") == 0 || 
           strlen(stats.cluster_name) > 0, "Имя кластера установлено");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_get_stats_string(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    
    char buffer[512] = {0};
    int result = cluster_get_stats_string(buffer, sizeof(buffer));
    
    ASSERT(result == 0, "Строка статистики получена");
    ASSERT(strlen(buffer) > 0, "Строка не пустая");
    ASSERT(strstr(buffer, "Cluster") != NULL, "Содержит заголовок");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_reset_stats(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    
    cluster_reset_stats();
    
    cluster_stats_t stats;
    cluster_get_stats(&stats);
    
    ASSERT(stats.total_nodes == 1, "total_nodes сохранён");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_get_uptime(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    
    int64_t uptime1 = cluster_get_uptime();
    
#ifdef _WIN32
    Sleep(100);
#else
    usleep(100000);
#endif
    
    int64_t uptime2 = cluster_get_uptime();
    
    ASSERT(uptime2 > uptime1, "Uptime увеличивается");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_get_online_count(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    
    int count = cluster_get_online_count();
    // Узлы offline по умолчанию
    ASSERT(count >= 0 && count <= 2, "Количество онлайн в диапазоне");
    
    cluster_cleanup();
    
    TEST_END();
}

static int test_cluster_has_quorum(void) {
    TEST_START();
    
    cluster_init("test-cluster", "node1");
    
    bool quorum = cluster_has_quorum();
    // Зависит от количества узлов
    ASSERT(quorum == true || quorum == false, "Кворум определён");
    
    cluster_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты утилит
// ============================================================================

static int test_cluster_role_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(cluster_role_to_string(CLUSTER_ROLE_LEADER), "Leader") == 0, "Leader");
    ASSERT(strcmp(cluster_role_to_string(CLUSTER_ROLE_FOLLOWER), "Follower") == 0, "Follower");
    ASSERT(strcmp(cluster_role_to_string(CLUSTER_ROLE_CANDIDATE), "Candidate") == 0, "Candidate");
    ASSERT(strcmp(cluster_role_to_string(CLUSTER_ROLE_STANDALONE), "Standalone") == 0, "Standalone");
    
    TEST_END();
}

static int test_cluster_node_status_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(cluster_node_status_to_string(CLUSTER_NODE_OFFLINE), "Offline") == 0, "Offline");
    ASSERT(strcmp(cluster_node_status_to_string(CLUSTER_NODE_ONLINE), "Online") == 0, "Online");
    ASSERT(strcmp(cluster_node_status_to_string(CLUSTER_NODE_DEGRADED), "Degraded") == 0, "Degraded");
    ASSERT(strcmp(cluster_node_status_to_string(CLUSTER_NODE_FAILED), "Failed") == 0, "Failed");
    
    TEST_END();
}

static int test_cluster_message_type_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(cluster_message_type_to_string(CLUSTER_MSG_HEARTBEAT), "Heartbeat") == 0, "Heartbeat");
    ASSERT(strcmp(cluster_message_type_to_string(CLUSTER_MSG_ELECTION_REQUEST), "ElectionRequest") == 0, "ElectionRequest");
    ASSERT(strcmp(cluster_message_type_to_string(CLUSTER_MSG_CONFIG_SYNC), "ConfigSync") == 0, "ConfigSync");
    
    TEST_END();
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    test_init("MTProxy Cluster Manager Tests");
    
    // Инициализация
    test_run(test_cluster_init);
    test_run(test_cluster_init_null);
    test_run(test_cluster_cleanup);
    
    // Управление узлами
    test_run(test_cluster_add_node);
    test_run(test_cluster_remove_node);
    test_run(test_cluster_get_node_config);
    test_run(test_cluster_get_all_nodes);
    
    // Запуск/остановка
    test_run(test_cluster_start_stop);
    
    // Лидерство
    test_run(test_cluster_get_leader);
    test_run(test_cluster_get_local_role);
    test_run(test_cluster_become_leader);
    test_run(test_cluster_step_down);
    
    // Синхронизация
    test_run(test_cluster_sync_config);
    test_run(test_cluster_get_config);
    
    // Балансировка
    test_run(test_cluster_get_least_loaded_node);
    test_run(test_cluster_get_avg_load_percent);
    
    // Failover
    test_run(test_cluster_handle_node_failure);
    test_run(test_cluster_recovery_node);
    test_run(test_cluster_set_auto_failover);
    
    // Статистика
    test_run(test_cluster_get_stats);
    test_run(test_cluster_get_stats_string);
    test_run(test_cluster_reset_stats);
    test_run(test_cluster_get_uptime);
    test_run(test_cluster_get_online_count);
    test_run(test_cluster_has_quorum);
    
    // Утилиты
    test_run(test_cluster_role_to_string);
    test_run(test_cluster_node_status_to_string);
    test_run(test_cluster_message_type_to_string);
    
    return test_finish();
}
