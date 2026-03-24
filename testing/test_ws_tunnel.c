/*
 * test_ws_tunnel.c - Тесты для WebSocket туннеля (tg-ws-proxy интеграция)
 * Тестирование ws-tunnel.c модуля
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include "net/ws-tunnel.h"

// Статистика тестов
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Макросы для тестирования
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

#define TEST_START(name) \
    printf("\n=== Test: %s ===\n", name)

#define TEST_END() \
    printf("Completed: %d run, %d passed, %d failed\n", \
           tests_run, tests_passed, tests_failed)

/* ============================================
   Тест 1: Инициализация контекста
   ============================================ */

void test_ws_tunnel_init() {
    TEST_START("ws_tunnel_init");

    ws_tunnel_ctx_t ctx;
    ws_tunnel_config_t config;

    // Тест 1: Инициализация с config
    memset(&config, 0, sizeof(config));
    config.use_websocket = 1;
    config.auto_fallback = 1;
    config.dc_auto_select = 1;
    config.connection_timeout = 5000;
    snprintf(config.ws_server_prefix, sizeof(config.ws_server_prefix), "kws");

    int result = ws_tunnel_init(&ctx, &config);
    TEST_ASSERT(result == 0, "ws_tunnel_init с config успешен");
    TEST_ASSERT(ctx.config.use_websocket == 1, "use_websocket установлен");
    TEST_ASSERT(ctx.config.auto_fallback == 1, "auto_fallback установлен");
    TEST_ASSERT(ctx.config.dc_auto_select == 1, "dc_auto_select установлен");
    TEST_ASSERT(ctx.config.connection_timeout == 5000, "connection_timeout установлен");

    // Тест 2: Инициализация без config (NULL)
    ws_tunnel_ctx_t ctx2;
    result = ws_tunnel_init(&ctx2, NULL);
    TEST_ASSERT(result == 0, "ws_tunnel_init без config успешен");
    TEST_ASSERT(ctx2.config.use_websocket == 1, "use_websocket по умолчанию = 1");
    TEST_ASSERT(ctx2.config.auto_fallback == 1, "auto_fallback по умолчанию = 1");

    // Тест 3: Инициализация с NULL ctx
    result = ws_tunnel_init(NULL, &config);
    TEST_ASSERT(result != 0, "ws_tunnel_init с NULL ctx возвращает ошибку");

    TEST_END();
}

/* ============================================
   Тест 2: Конфигурация DC
   ============================================ */

void test_dc_configuration() {
    TEST_START("dc_configuration");

    ws_tunnel_ctx_t ctx;
    ws_tunnel_config_t config;

    memset(&config, 0, sizeof(config));
    config.use_websocket = 1;
    config.dc_auto_select = 0;  // Ручной выбор DC

    // Настройка DC
    telegram_dc_t test_dcs[] = {
        {1, "127.0.0.1", 8080, 0, 0, 1},
        {2, "127.0.0.1", 8081, 0, 0, 1},
        {3, "127.0.0.1", 8082, 0, 0, 1}
    };

    memcpy(config.dcs, test_dcs, sizeof(test_dcs));
    config.dc_count = 3;
    config.current_dc_id = 2;

    int result = ws_tunnel_init(&ctx, &config);
    TEST_ASSERT(result == 0, "ws_tunnel_init с DC config успешен");
    TEST_ASSERT(ctx.config.dc_count == 3, "dc_count = 3");
    TEST_ASSERT(ctx.config.current_dc_id == 2, "current_dc_id = 2");

    // Проверка данных DC
    TEST_ASSERT(strcmp(ctx.config.dcs[0].ip, "127.0.0.1") == 0, "DC 1 IP установлен");
    TEST_ASSERT(ctx.config.dcs[0].port == 8080, "DC 1 порт установлен");
    TEST_ASSERT(ctx.config.dcs[1].port == 8081, "DC 2 порт установлен");
    TEST_ASSERT(ctx.config.dcs[2].port == 8082, "DC 3 порт установлен");

    TEST_END();
}

/* ============================================
   Тест 3: Конфигурация WebSocket
   ============================================ */

void test_websocket_configuration() {
    TEST_START("websocket_configuration");

    ws_tunnel_ctx_t ctx;
    ws_tunnel_config_t config;

    memset(&config, 0, sizeof(config));

    // Тест 1: WebSocket включён
    config.use_websocket = 1;
    snprintf(config.ws_server_prefix, sizeof(config.ws_server_prefix), "kws");
    config.ws_port = 443;

    int result = ws_tunnel_init(&ctx, &config);
    TEST_ASSERT(result == 0, "WebSocket config успешен");
    TEST_ASSERT(ctx.config.use_websocket == 1, "use_websocket = 1");
    TEST_ASSERT(strcmp(ctx.config.ws_server_prefix, "kws") == 0, "ws_server_prefix = kws");
    TEST_ASSERT(ctx.config.ws_port == 443, "ws_port = 443");

    // Тест 2: WebSocket выключен (TCP fallback)
    ws_tunnel_ctx_t ctx2;
    memset(&config, 0, sizeof(config));
    config.use_websocket = 0;
    config.auto_fallback = 0;

    result = ws_tunnel_init(&ctx2, &config);
    TEST_ASSERT(result == 0, "TCP fallback config успешен");
    TEST_ASSERT(ctx2.config.use_websocket == 0, "use_websocket = 0 (TCP only)");

    TEST_END();
}

/* ============================================
   Тест 4: Очистка контекста
   ============================================ */

void test_ws_tunnel_cleanup() {
    TEST_START("ws_tunnel_cleanup");

    ws_tunnel_ctx_t ctx;
    ws_tunnel_config_t config;

    memset(&config, 0, sizeof(config));
    config.use_websocket = 1;

    int result = ws_tunnel_init(&ctx, &config);
    TEST_ASSERT(result == 0, "ws_tunnel_init успешен");

    // Очистка
    ws_tunnel_cleanup(&ctx);
    TEST_ASSERT(1, "ws_tunnel_cleanup выполнен без ошибок");

    // Тест очистки NULL
    ws_tunnel_cleanup(NULL);
    TEST_ASSERT(1, "ws_tunnel_cleanup(NULL) не вызывает crash");

    TEST_END();
}

/* ============================================
   Тест 5: Конфигурация таймаутов
   ============================================ */

void test_timeout_configuration() {
    TEST_START("timeout_configuration");

    ws_tunnel_ctx_t ctx;
    ws_tunnel_config_t config;

    // Тест 1: Короткий таймаут
    memset(&config, 0, sizeof(config));
    config.use_websocket = 1;
    config.connection_timeout = 1000;  // 1 секунда

    int result = ws_tunnel_init(&ctx, &config);
    TEST_ASSERT(result == 0, "Короткий таймаут установлен");
    TEST_ASSERT(ctx.config.connection_timeout == 1000, "connection_timeout = 1000ms");

    // Тест 2: Длинный таймаут
    ws_tunnel_ctx_t ctx2;
    memset(&config, 0, sizeof(config));
    config.use_websocket = 1;
    config.connection_timeout = 30000;  // 30 секунд

    result = ws_tunnel_init(&ctx2, &config);
    TEST_ASSERT(result == 0, "Длинный таймаут установлен");
    TEST_ASSERT(ctx2.config.connection_timeout == 30000, "connection_timeout = 30000ms");

    // Тест 3: Таймаут по умолчанию
    ws_tunnel_ctx_t ctx3;
    result = ws_tunnel_init(&ctx3, NULL);
    TEST_ASSERT(result == 0, "Инициализация с NULL config");
    TEST_ASSERT(ctx3.config.connection_timeout == 5000, "connection_timeout по умолчанию = 5000ms");

    TEST_END();
}

/* ============================================
   Главная функция
   ============================================ */

int main() {
    printf("===========================================\n");
    printf("  WS Tunnel Tests (tg-ws-proxy integration)\n");
    printf("===========================================\n\n");

#ifdef _WIN32
    // Инициализация Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    test_ws_tunnel_init();
    test_dc_configuration();
    test_websocket_configuration();
    test_ws_tunnel_cleanup();
    test_timeout_configuration();

#ifdef _WIN32
    WSACleanup();
#endif

    printf("\n===========================================\n");
    printf("  Final Summary\n");
    printf("===========================================\n");
    printf("  Total tests: %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Success rate: %.2f%%\n",
           (tests_run > 0) ? (tests_passed * 100.0) / tests_run : 0);
    printf("===========================================\n");

    return (tests_failed > 0) ? 1 : 0;
}
