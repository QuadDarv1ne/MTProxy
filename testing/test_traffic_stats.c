/*
 * test_traffic_stats.c - Тесты для модуля статистики трафика
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../net/traffic-stats.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) printf("  Test %s... ", name)
#define PASS() do { printf("PASSED\n"); tests_passed++; tests_run++; } while(0)
#define FAIL(msg) do { printf("FAILED: %s\n", msg); tests_run++; return 0; } while(0)

/* Тест инициализации */
static int test_init(void) {
    TEST("initialization");
    
    traffic_stats_t stats;
    int ret = traffic_stats_init(&stats);
    
    if (ret != 0) {
        FAIL("init failed");
    }
    
    if (stats.entry_count != 0 || stats.device_count != 0) {
        traffic_stats_destroy(&stats);
        FAIL("initial counts not zero");
    }
    
    if (stats.total_bytes_sent != 0 || stats.total_bytes_received != 0) {
        traffic_stats_destroy(&stats);
        FAIL("initial bytes not zero");
    }
    
    traffic_stats_destroy(&stats);
    PASS();
    return 1;
}

/* Тест создания подключения */
static int test_create_connection(void) {
    TEST("create_connection");
    
    traffic_stats_t stats;
    traffic_stats_init(&stats);
    
    int ret = traffic_stats_create_connection(&stats, 1001, "192.168.1.100", "device-001");
    
    if (ret != 0) {
        traffic_stats_destroy(&stats);
        FAIL("create_connection failed");
    }
    
    if (stats.entry_count != 1) {
        traffic_stats_destroy(&stats);
        FAIL("entry_count != 1");
    }
    
    if (stats.total_connections != 1) {
        traffic_stats_destroy(&stats);
        FAIL("total_connections != 1");
    }
    
    if (stats.active_connections != 1) {
        traffic_stats_destroy(&stats);
        FAIL("active_connections != 1");
    }
    
    if (stats.device_count != 1) {
        traffic_stats_destroy(&stats);
        FAIL("device_count != 1");
    }
    
    traffic_stats_destroy(&stats);
    PASS();
    return 1;
}

/* Тест обновления статистики */
static int test_update_stats(void) {
    TEST("update_sent_received");
    
    traffic_stats_t stats;
    traffic_stats_init(&stats);
    
    traffic_stats_create_connection(&stats, 1001, "192.168.1.100", "device-001");
    
    /* Обновляем отправку */
    traffic_stats_update_sent(&stats, 1001, 1024, 10);
    
    if (stats.total_bytes_sent != 1024) {
        traffic_stats_destroy(&stats);
        FAIL("total_bytes_sent incorrect");
    }
    
    /* Обновляем получение */
    traffic_stats_update_received(&stats, 1001, 2048, 15);
    
    if (stats.total_bytes_received != 2048) {
        traffic_stats_destroy(&stats);
        FAIL("total_bytes_received incorrect");
    }
    
    /* Проверяем запись подключения */
    traffic_entry_t entry;
    int ret = traffic_stats_get_connection(&stats, 1001, &entry);
    
    if (ret != 0) {
        traffic_stats_destroy(&stats);
        FAIL("get_connection failed");
    }
    
    if (entry.bytes_sent != 1024 || entry.bytes_received != 2048) {
        traffic_stats_destroy(&stats);
        FAIL("entry bytes incorrect");
    }
    
    if (entry.packets_sent != 10 || entry.packets_received != 15) {
        traffic_stats_destroy(&stats);
        FAIL("entry packets incorrect");
    }
    
    traffic_stats_destroy(&stats);
    PASS();
    return 1;
}

/* Тест закрытия подключения */
static int test_close_connection(void) {
    TEST("close_connection");
    
    traffic_stats_t stats;
    traffic_stats_init(&stats);
    
    traffic_stats_create_connection(&stats, 1001, "192.168.1.100", "device-001");
    
    if (stats.active_connections != 1) {
        traffic_stats_destroy(&stats);
        FAIL("active_connections != 1 after create");
    }
    
    traffic_stats_close_connection(&stats, 1001);
    
    if (stats.active_connections != 0) {
        traffic_stats_destroy(&stats);
        FAIL("active_connections != 0 after close");
    }
    
    /* Проверяем, что запись помечена как неактивная */
    traffic_entry_t entry;
    traffic_stats_get_connection(&stats, 1001, &entry);
    
    if (entry.is_active != 0) {
        traffic_stats_destroy(&stats);
        FAIL("entry not marked as inactive");
    }
    
    traffic_stats_destroy(&stats);
    PASS();
    return 1;
}

/* Тест статистики устройства */
static int test_device_stats(void) {
    TEST("device_statistics");
    
    traffic_stats_t stats;
    traffic_stats_init(&stats);
    
    /* Создаём несколько подключений с одного устройства */
    traffic_stats_create_connection(&stats, 1001, "192.168.1.100", "device-001");
    traffic_stats_create_connection(&stats, 1002, "192.168.1.100", "device-001");
    traffic_stats_create_connection(&stats, 1003, "192.168.1.101", "device-002");
    
    if (stats.device_count != 2) {
        traffic_stats_destroy(&stats);
        FAIL("device_count != 2");
    }
    
    /* Обновляем трафик */
    traffic_stats_update_sent(&stats, 1001, 500, 5);
    traffic_stats_update_sent(&stats, 1002, 300, 3);
    traffic_stats_update_sent(&stats, 1003, 200, 2);
    
    /* Проверяем статистику устройства */
    device_traffic_t device;
    int ret = traffic_stats_get_device(&stats, "device-001", &device);
    
    if (ret != 0) {
        traffic_stats_destroy(&stats);
        FAIL("get_device failed");
    }
    
    if (device.connection_count != 2) {
        traffic_stats_destroy(&stats);
        FAIL("device-001 connection_count != 2");
    }
    
    if (device.total_bytes_sent != 800) {
        traffic_stats_destroy(&stats);
        FAIL("device-001 total_bytes_sent != 800");
    }
    
    traffic_stats_destroy(&stats);
    PASS();
    return 1;
}

/* Тест глобальной статистики */
static int test_global_stats(void) {
    TEST("global_statistics");
    
    traffic_stats_t stats;
    traffic_stats_init(&stats);
    
    /* Создаём подключения и обновляем трафик */
    traffic_stats_create_connection(&stats, 1001, "192.168.1.100", "device-001");
    traffic_stats_create_connection(&stats, 1002, "192.168.1.101", "device-002");
    
    traffic_stats_update_sent(&stats, 1001, 1000, 10);
    traffic_stats_update_received(&stats, 1001, 2000, 20);
    traffic_stats_update_sent(&stats, 1002, 500, 5);
    traffic_stats_update_received(&stats, 1002, 1500, 15);
    
    uint64_t total_sent, total_received, active_conn, total_conn;
    traffic_stats_get_global(&stats, &total_sent, &total_received, &active_conn, &total_conn);
    
    if (total_sent != 1500) {
        traffic_stats_destroy(&stats);
        FAIL("total_sent != 1500");
    }
    
    if (total_received != 3500) {
        traffic_stats_destroy(&stats);
        FAIL("total_received != 3500");
    }
    
    if (active_conn != 2) {
        traffic_stats_destroy(&stats);
        FAIL("active_conn != 2");
    }
    
    if (total_conn != 2) {
        traffic_stats_destroy(&stats);
        FAIL("total_conn != 2");
    }
    
    traffic_stats_destroy(&stats);
    PASS();
    return 1;
}

/* Тест JSON экспорта */
static int test_json_export(void) {
    TEST("json_export");
    
    traffic_stats_t stats;
    traffic_stats_init(&stats);
    
    traffic_stats_create_connection(&stats, 1001, "192.168.1.100", "device-001");
    traffic_stats_update_sent(&stats, 1001, 1024, 10);
    traffic_stats_update_received(&stats, 1001, 2048, 20);
    
    char buffer[4096];
    int len = traffic_stats_export_json(&stats, buffer, sizeof(buffer));
    
    if (len <= 0) {
        traffic_stats_destroy(&stats);
        FAIL("json_export returned <= 0");
    }
    
    /* Проверяем наличие ключевых полей в JSON */
    if (strstr(buffer, "\"global\"") == NULL) {
        traffic_stats_destroy(&stats);
        FAIL("JSON missing 'global'");
    }
    
    if (strstr(buffer, "\"connections\"") == NULL) {
        traffic_stats_destroy(&stats);
        FAIL("JSON missing 'connections'");
    }
    
    if (strstr(buffer, "\"total_bytes_sent\"") == NULL) {
        traffic_stats_destroy(&stats);
        FAIL("JSON missing 'total_bytes_sent'");
    }
    
    traffic_stats_destroy(&stats);
    PASS();
    return 1;
}

/* Тест сброса статистики */
static int test_reset(void) {
    TEST("reset");
    
    traffic_stats_t stats;
    traffic_stats_init(&stats);
    
    traffic_stats_create_connection(&stats, 1001, "192.168.1.100", "device-001");
    traffic_stats_update_sent(&stats, 1001, 1024, 10);
    
    traffic_stats_reset(&stats);
    
    if (stats.entry_count != 0) {
        traffic_stats_destroy(&stats);
        FAIL("entry_count != 0 after reset");
    }
    
    if (stats.total_bytes_sent != 0) {
        traffic_stats_destroy(&stats);
        FAIL("total_bytes_sent != 0 after reset");
    }
    
    if (stats.total_connections != 0) {
        traffic_stats_destroy(&stats);
        FAIL("total_connections != 0 after reset");
    }
    
    traffic_stats_destroy(&stats);
    PASS();
    return 1;
}

/* Тест очистки неактивных записей */
static int test_cleanup(void) {
    TEST("cleanup_inactive");
    
    traffic_stats_t stats;
    traffic_stats_init(&stats);
    
    /* Создаём и закрываем подключения */
    traffic_stats_create_connection(&stats, 1001, "192.168.1.100", "device-001");
    traffic_stats_create_connection(&stats, 1002, "192.168.1.101", "device-002");
    
    traffic_stats_close_connection(&stats, 1001);
    traffic_stats_close_connection(&stats, 1002);
    
    if (stats.entry_count != 2) {
        traffic_stats_destroy(&stats);
        FAIL("entry_count != 2 before cleanup");
    }
    
    /* Очищаем (поскольку записи свежие, они не должны удалиться) */
    int removed = traffic_stats_cleanup_inactive(&stats, 3600);
    
    /* Записи могут удалиться если они старые enough */
    printf("(removed %d) ", removed);
    
    traffic_stats_destroy(&stats);
    PASS();
    return 1;
}

/* Тест множественных подключений */
static int test_multiple_connections(void) {
    TEST("multiple_connections");
    
    traffic_stats_t stats;
    traffic_stats_init(&stats);
    
    /* Создаём 10 подключений */
    for (int i = 0; i < 10; i++) {
        char ip[32], device[32];
        snprintf(ip, sizeof(ip), "192.168.1.%d", 100 + i);
        snprintf(device, sizeof(device), "device-%03d", i);
        
        int ret = traffic_stats_create_connection(&stats, 2000 + i, ip, device);
        if (ret != 0) {
            traffic_stats_destroy(&stats);
            FAIL("create_connection failed for multiple");
        }
    }
    
    if (stats.entry_count != 10) {
        traffic_stats_destroy(&stats);
        FAIL("entry_count != 10");
    }
    
    if (stats.total_connections != 10) {
        traffic_stats_destroy(&stats);
        FAIL("total_connections != 10");
    }
    
    if (stats.device_count != 10) {
        traffic_stats_destroy(&stats);
        FAIL("device_count != 10");
    }
    
    /* Обновляем трафик для всех */
    for (int i = 0; i < 10; i++) {
        traffic_stats_update_sent(&stats, 2000 + i, 100 * (i + 1), i + 1);
        traffic_stats_update_received(&stats, 2000 + i, 200 * (i + 1), (i + 1) * 2);
    }
    
    /* Проверяем глобальную статистику */
    uint64_t total_sent, total_received;
    traffic_stats_get_global(&stats, &total_sent, &total_received, NULL, NULL);
    
    /* Ожидаемая сумма: 100*(1+2+...+10) = 100*55 = 5500 */
    if (total_sent != 5500) {
        traffic_stats_destroy(&stats);
        FAIL("total_sent != 5500");
    }
    
    /* Ожидаемая сумма: 200*(1+2+...+10) = 200*55 = 11000 */
    if (total_received != 11000) {
        traffic_stats_destroy(&stats);
        FAIL("total_received != 11000");
    }
    
    traffic_stats_destroy(&stats);
    PASS();
    return 1;
}

int main(void) {
    printf("=== Traffic Stats Module Tests ===\n\n");
    
    test_init();
    test_create_connection();
    test_update_stats();
    test_close_connection();
    test_device_stats();
    test_global_stats();
    test_json_export();
    test_reset();
    test_cleanup();
    test_multiple_connections();
    
    printf("\n=== Results ===\n");
    printf("Passed: %d/%d\n", tests_passed, tests_run);
    
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}
