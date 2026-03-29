/*
    MTProxy Production Integration Tests
    Интеграционные тесты для production-кластера
    
    Тестируемые компоненты:
    - Cluster Manager + Load Balancer
    - Auto-Scaler + Health Check
    - Distributed Tracing + Alert Manager
    - End-to-End сценарии
    
    Запуск:
    ./bin/test-production-integration --verbose
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <pthread.h>
#endif

#include "system/cluster/cluster-manager.h"
#include "system/cluster/load-balancer.h"
#include "system/cluster/auto-scaler.h"
#include "system/monitoring/health-check.h"
#include "system/monitoring/alert-manager.h"
#include "system/monitoring/distributed-tracing.h"
#include "testing/test_common.h"

// ============================================================================
// Глобальные переменные
// ============================================================================

static struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    bool verbose;
} g_production_tests = {0};

// ============================================================================
// Вспомогательные функции
// ============================================================================

static void log_test(const char *test_name, bool passed) {
    printf("[%s] %s: %s\n", 
           passed ? "PASS" : "FAIL",
           test_name,
           passed ? "OK" : "FAILED");
    
    if (passed) {
        g_production_tests.passed_tests++;
    } else {
        g_production_tests.failed_tests++;
    }
    g_production_tests.total_tests++;
}

static void simulate_load(int requests_count, int delay_ms) {
    for (int i = 0; i < requests_count; i++) {
#ifdef _WIN32
        Sleep(delay_ms);
#else
        usleep(delay_ms * 1000);
#endif
    }
}

// ============================================================================
// Тест 1: Cluster + Load Balancer Integration
// ============================================================================

static int test_cluster_load_balancer_integration(void) {
    const char *test_name = "Cluster + Load Balancer Integration";
    bool passed = true;
    
    printf("\n=== Test: %s ===\n", test_name);
    
    // Инициализация кластера
    int result = cluster_init("test-cluster", "node1");
    if (result != 0) {
        log_test(test_name, false);
        return 1;
    }
    
    // Добавление узлов
    cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    cluster_add_node("node3", "192.168.1.103", 8888, 9000);
    
    // Инициализация Load Balancer
    result = load_balancer_init(LOAD_BALANCER_LEAST_CONNECTIONS);
    if (result != 0) {
        cluster_cleanup();
        log_test(test_name, false);
        return 1;
    }
    
    // Добавление backend узлов
    load_balancer_add_backend("node1", "127.0.0.1", 8888, 1.0);
    load_balancer_add_backend("node2", "127.0.0.1", 8889, 1.0);
    load_balancer_add_backend("node3", "127.0.0.1", 8890, 1.0);
    
    // Запуск
    cluster_start(9000);
    load_balancer_start_health_checks(2000);
    
    // Симуация запросов
    for (int i = 0; i < 10; i++) {
        const char *backend = load_balancer_select_backend("192.168.1.100");
        if (backend) {
            load_balancer_notify_new_connection(backend);
        }
    }
    
    // Проверка статистики
    load_balancer_stats_t lb_stats;
    load_balancer_get_stats(&lb_stats);
    
    if (lb_stats.total_requests != 10) {
        passed = false;
    }
    
    // Остановка
    load_balancer_stop_health_checks();
    cluster_stop();
    
    // Очистка
    load_balancer_cleanup();
    cluster_cleanup();
    
    log_test(test_name, passed);
    return passed ? 0 : 1;
}

// ============================================================================
// Тест 2: Auto-Scaler + Health Check Integration
// ============================================================================

static int test_auto_scaler_health_check_integration(void) {
    const char *test_name = "Auto-Scaler + Health Check Integration";
    bool passed = true;
    
    printf("\n=== Test: %s ===\n", test_name);
    
    // Инициализация Health Check
    health_check_init();
    health_check_add_http_node("web1", "http://localhost:8080/health", 3000);
    health_check_add_tcp_node("db1", "localhost", 5432, 2000);
    
    // Инициализация Auto-Scaler
    auto_scaler_init("test-cluster");
    auto_scaler_set_limits(1, 5);
    auto_scaler_set_policy(AUTO_SCALER_POLICY_MODERATE);
    
    // Запуск
    health_check_start(3000);
    auto_scaler_start();
    
    // Ожидание проверки
    simulate_load(5, 100);
    
    // Проверка статистики
    auto_scaler_stats_t as_stats;
    auto_scaler_get_stats(&as_stats);
    
    health_check_stats_t hc_stats;
    health_check_get_stats(&hc_stats);
    
    if (hc_stats.total_nodes != 2) {
        passed = false;
    }
    
    // Остановка
    auto_scaler_stop();
    health_check_stop();
    
    // Очистка
    auto_scaler_cleanup();
    health_check_cleanup();
    
    log_test(test_name, passed);
    return passed ? 0 : 1;
}

// ============================================================================
// Тест 3: Distributed Tracing + Alert Manager Integration
// ============================================================================

static int test_tracing_alert_integration(void) {
    const char *test_name = "Distributed Tracing + Alert Manager Integration";
    bool passed = true;
    
    printf("\n=== Test: %s ===\n", test_name);
    
    // Инициализация Alert Manager
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_WARNING);
    
    // Инициализация Tracing
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    // Создание trace с алертами
    trace_context_t *trace = tracing_start_trace("request-flow");
    tracing_add_span_attribute(
        tracing_start_span(trace, "process-request", TRACING_SPAN_SERVER),
        "client_ip", "192.168.1.100"
    );
    
    // Симуация ошибки
    bool error_occurred = true;
    if (error_occurred) {
        alert_manager_send_alert(
            ALERT_LEVEL_ERROR,
            ALERT_TYPE_SECURITY,
            "Request Processing Error",
            "Failed to process request from 192.168.1.100"
        );
    }
    
    // Проверка статистики
    alert_manager_stats_t alert_stats;
    alert_manager_get_stats(&alert_stats);
    
    tracing_stats_t trace_stats;
    tracing_get_stats(&trace_stats);
    
    if (trace_stats.total_traces != 1) {
        passed = false;
    }
    
    // Очистка
    tracing_end_trace(trace);
    tracing_cleanup();
    alert_manager_cleanup();
    
    log_test(test_name, passed);
    return passed ? 0 : 1;
}

// ============================================================================
// Тест 4: Full Stack Integration (All Components)
// ============================================================================

static int test_full_stack_integration(void) {
    const char *test_name = "Full Stack Integration (All Components)";
    bool passed = true;
    
    printf("\n=== Test: %s ===\n", test_name);
    
    // Инициализация всех компонентов
    cluster_init("production-cluster", "node1");
    load_balancer_init(LOAD_BALANCER_ROUND_ROBIN);
    auto_scaler_init("production-cluster");
    health_check_init();
    alert_manager_init();
    tracing_init("production-service");
    
    // Настройка
    cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    load_balancer_add_backend("node1", "127.0.0.1", 8888, 1.0);
    auto_scaler_set_limits(1, 10);
    health_check_add_tcp_node("node1", "localhost", 8888, 1000);
    alert_manager_add_channel(ALERT_CHANNEL_SLACK, "Slack", ALERT_LEVEL_ERROR);
    tracing_set_sampling_rate(0.1);
    
    // Запуск
    cluster_start(9000);
    load_balancer_start_health_checks(2000);
    health_check_start(3000);
    auto_scaler_start();
    
    // Симуация рабочей нагрузки
    printf("Simulating production load...\n");
    for (int i = 0; i < 50; i++) {
        // Load Balancer
        const char *backend = load_balancer_select_backend("192.168.1.100");
        if (backend) {
            load_balancer_notify_new_connection(backend);
        }
        
        // Tracing
        trace_context_t *trace = tracing_start_trace("request");
        span_t *span = tracing_start_span(trace, "process", TRACING_SPAN_SERVER);
        tracing_add_span_attribute(span, "request_id", "12345");
        simulate_load(1, 10);
        tracing_end_span(span);
        tracing_end_trace(trace);
        
        // Health Check
        health_check_run_health_check();
        
        // Auto-Scaler
        auto_scaler_check_and_scale();
    }
    
    // Сбор статистики
    cluster_stats_t cluster_stats;
    load_balancer_stats_t lb_stats;
    auto_scaler_stats_t as_stats;
    health_check_stats_t hc_stats;
    alert_manager_stats_t alert_stats;
    tracing_stats_t trace_stats;
    
    cluster_get_stats(&cluster_stats);
    load_balancer_get_stats(&lb_stats);
    auto_scaler_get_stats(&as_stats);
    health_check_get_stats(&hc_stats);
    alert_manager_get_stats(&alert_stats);
    tracing_get_stats(&trace_stats);
    
    // Валидация
    if (lb_stats.total_requests != 50) {
        printf("Expected 50 LB requests, got %llu\n", 
               (unsigned long long)lb_stats.total_requests);
        passed = false;
    }
    
    if (trace_stats.total_traces != 50) {
        printf("Expected 50 traces, got %llu\n", 
               (unsigned long long)trace_stats.total_traces);
        passed = false;
    }
    
    printf("Cluster nodes: %d\n", cluster_stats.total_nodes);
    printf("LB requests: %llu\n", (unsigned long long)lb_stats.total_requests);
    printf("Auto-scaler nodes: %d\n", as_stats.current_nodes);
    printf("Health checks: %d nodes\n", hc_stats.total_nodes);
    printf("Alerts sent: %llu\n", (unsigned long long)alert_stats.alerts_sent);
    printf("Traces: %llu total, %llu sampled\n", 
           (unsigned long long)trace_stats.total_traces,
           (unsigned long long)trace_stats.sampled_traces);
    
    // Остановка
    auto_scaler_stop();
    health_check_stop();
    load_balancer_stop_health_checks();
    cluster_stop();
    
    // Очистка
    tracing_cleanup();
    alert_manager_cleanup();
    health_check_cleanup();
    auto_scaler_cleanup();
    load_balancer_cleanup();
    cluster_cleanup();
    
    log_test(test_name, passed);
    return passed ? 0 : 1;
}

// ============================================================================
// Тест 5: Failover Scenario
// ============================================================================

static int test_failover_scenario(void) {
    const char *test_name = "Failover Scenario";
    bool passed = true;
    
    printf("\n=== Test: %s ===\n", test_name);
    
    // Инициализация
    cluster_init("failover-cluster", "node1");
    cluster_add_node("node2", "192.168.1.102", 8888, 9000);
    cluster_add_node("node3", "192.168.1.103", 8888, 9000);
    
    load_balancer_init(LOAD_BALANCER_LEAST_CONNECTIONS);
    load_balancer_add_backend("node1", "127.0.0.1", 8888, 1.0);
    load_balancer_add_backend("node2", "127.0.0.1", 8889, 1.0);
    load_balancer_add_backend("node3", "127.0.0.1", 8890, 1.0);
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_CRITICAL);
    
    // Запуск
    cluster_start(9000);
    load_balancer_start_health_checks(1000);
    
    // Симуация работы
    for (int i = 0; i < 20; i++) {
        const char *backend = load_balancer_select_backend("192.168.1.100");
        if (backend) {
            load_balancer_notify_new_connection(backend);
        }
    }
    
    // Симуация отказа узла
    printf("Simulating node2 failure...\n");
    cluster_handle_node_failure("node2");
    load_balancer_set_backend_enabled("node2", false);
    
    // Продолжение работы с оставшимися узлами
    for (int i = 0; i < 10; i++) {
        const char *backend = load_balancer_select_backend("192.168.1.100");
        if (backend) {
            load_balancer_notify_new_connection(backend);
        }
    }
    
    // Проверка что запросы обрабатываются
    load_balancer_stats_t stats;
    load_balancer_get_stats(&stats);
    
    if (stats.total_requests != 30) {
        printf("Expected 30 requests, got %llu\n", 
               (unsigned long long)stats.total_requests);
        passed = false;
    }
    
    // Остановка
    load_balancer_stop_health_checks();
    cluster_stop();
    
    // Очистка
    alert_manager_cleanup();
    load_balancer_cleanup();
    cluster_cleanup();
    
    log_test(test_name, passed);
    return passed ? 0 : 1;
}

// ============================================================================
// Тест 6: High Load Scenario
// ============================================================================

static int test_high_load_scenario(void) {
    const char *test_name = "High Load Scenario";
    bool passed = true;
    
    printf("\n=== Test: %s ===\n", test_name);
    
    // Инициализация
    load_balancer_init(LOAD_BALANCER_LEAST_LOAD);
    load_balancer_add_backend("node1", "127.0.0.1", 8888, 1.0);
    load_balancer_add_backend("node2", "127.0.0.1", 8889, 1.0);
    load_balancer_add_backend("node3", "127.0.0.1", 8890, 1.0);
    
    tracing_init("high-load-test");
    tracing_set_sampling_rate(0.01);  // 1% sampling
    
    // Запуск
    load_balancer_start_health_checks(5000);
    
    // Высокая нагрузка: 1000 запросов
    printf("Processing 1000 requests...\n");
    for (int i = 0; i < 1000; i++) {
        const char *backend = load_balancer_select_backend("192.168.1.100");
        if (backend) {
            load_balancer_notify_new_connection(backend);
            load_balancer_notify_connection_closed(backend);
        }
        
        // Tracing для каждого 10-го запроса
        if (i % 10 == 0) {
            trace_context_t *trace = tracing_start_trace("high-load-request");
            span_t *span = tracing_start_span(trace, "request", TRACING_SPAN_SERVER);
            tracing_end_span(span);
            tracing_end_trace(trace);
        }
    }
    
    // Проверка статистики
    load_balancer_stats_t lb_stats;
    load_balancer_get_stats(&lb_stats);
    
    tracing_stats_t trace_stats;
    tracing_get_stats(&trace_stats);
    
    printf("LB requests: %llu\n", (unsigned long long)lb_stats.total_requests);
    printf("Traces: %llu\n", (unsigned long long)trace_stats.total_traces);
    
    if (lb_stats.total_requests != 1000) {
        passed = false;
    }
    
    // Остановка
    load_balancer_stop_health_checks();
    
    // Очистка
    tracing_cleanup();
    load_balancer_cleanup();
    
    log_test(test_name, passed);
    return passed ? 0 : 1;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    printf("===========================================\n");
    printf("MTProxy Production Integration Tests\n");
    printf("===========================================\n\n");
    
    // Парсинг аргументов
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            g_production_tests.verbose = true;
        }
    }
    
    // Запуск тестов
    test_cluster_load_balancer_integration();
    test_auto_scaler_health_check_integration();
    test_tracing_alert_integration();
    test_full_stack_integration();
    test_failover_scenario();
    test_high_load_scenario();
    
    // Итоги
    printf("\n===========================================\n");
    printf("Production Integration Tests Summary\n");
    printf("===========================================\n");
    printf("Total:   %d\n", g_production_tests.total_tests);
    printf("Passed:  %d\n", g_production_tests.passed_tests);
    printf("Failed:  %d\n", g_production_tests.failed_tests);
    printf("Success: %.1f%%\n", 
           g_production_tests.total_tests > 0 ? 
           (double)g_production_tests.passed_tests / g_production_tests.total_tests * 100.0 : 0.0);
    printf("===========================================\n");
    
    return g_production_tests.failed_tests > 0 ? 1 : 0;
}
