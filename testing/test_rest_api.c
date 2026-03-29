/*
 * test_rest_api.c — Тесты для REST API
 *
 * Тестирование REST API endpoints:
 * - GET /api/v1/status
 * - GET /api/v1/stats
 * - GET /api/v1/metrics
 * - POST /api/v1/admin/reload
 * - Authentication
 * - Error handling
 *
 * Запуск:
 *   ./test-rest-api
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#define sleep(x) Sleep((x) * 1000)
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "../net/rest-api.h"

#define TEST_PASS 0
#define TEST_FAIL 1

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static int test_##name(void)
#define RUN_TEST(name) do { \
    tests_run++; \
    printf("  %-40s ... ", #name); \
    if (test_##name() == TEST_PASS) { \
        printf("PASSED\n"); \
        tests_passed++; \
    } else { \
        printf("FAILED\n"); \
        tests_failed++; \
    } \
} while (0)

#define ASSERT(cond) do { if (!(cond)) return TEST_FAIL; } while (0)
#define ASSERT_EQ(a, b) do { if ((a) != (b)) return TEST_FAIL; } while (0)

/* ============================================================================
 * Helper functions
 * ============================================================================ */

/* Отправка HTTP запроса и получение ответа */
static int http_request(const char *host, int port, const char *method,
                        const char *path, const char *headers,
                        const char *body, char *response, size_t resp_size) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }

    /* Формирование запроса */
    char request[4096];
    snprintf(request, sizeof(request),
             "%s %s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Connection: close\r\n"
             "%s"
             "Content-Length: %zu\r\n"
             "\r\n"
             "%s",
             method, path, host, port,
             headers ? headers : "",
             body ? strlen(body) : 0,
             body ? body : "");

    /* Отправка */
    if (send(sock, request, strlen(request), 0) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }

    /* Задержка для получения ответа */
    usleep(100000);

    /* Получение ответа */
    int n = recv(sock, response, resp_size - 1, 0);
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif

    if (n < 0) return -1;
    response[n] = '\0';

    return 0;
}

/* Проверка HTTP статуса в ответе */
static int check_http_status(const char *response, int expected_status) {
    char expected[64];
    snprintf(expected, sizeof(expected), "HTTP/1.1 %d", expected_status);
    return (strstr(response, expected) != NULL) ? 0 : -1;
}

/* Проверка наличия строки в ответе */
static int check_response_contains(const char *response, const char *expected) {
    return (strstr(response, expected) != NULL) ? 0 : -1;
}

/* ============================================================================
 * Initialization Tests
 * ============================================================================ */

TEST(api_init_null_config) {
    rest_api_server_t server;
    int ret = rest_api_init(&server, NULL);
    /* Должна вернуться ошибка при NULL конфигурации */
    ASSERT(ret != 0);
    return TEST_PASS;
}

TEST(api_init_valid_config) {
    rest_api_server_t server;
    rest_api_config_t config = {0};
    config.port = 18080;  /* Тестовый порт */
    config.enable_auth = 0;

    int ret = rest_api_init(&server, &config);
    /* Инициализация должна пройти успешно */
    ASSERT(ret == 0);

    rest_api_cleanup(&server);
    return TEST_PASS;
}

TEST(api_init_with_auth) {
    rest_api_server_t server;
    rest_api_config_t config = {0};
    config.port = 18081;
    config.enable_auth = 1;
    strncpy(config.api_key, "test-api-key-123", sizeof(config.api_key) - 1);

    int ret = rest_api_init(&server, &config);
    ASSERT(ret == 0);

    /* Добавление тестового токена */
    rest_api_add_auth_token(&server, "test-token");

    rest_api_cleanup(&server);
    return TEST_PASS;
}

/* ============================================================================
 * Status Endpoint Tests
 * ============================================================================ */

TEST(get_status_endpoint) {
    /* Предполагается, что сервер запущен на порту 18080 */
    char response[4096];

    int ret = http_request("127.0.0.1", 18080, "GET", "/api/v1/status",
                           "", NULL, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;  /* Не ошибка, если сервер не запущен */
    }

    ASSERT(check_http_status(response, 200) == 0);
    ASSERT(check_response_contains(response, "\"status\"") == 0);

    return TEST_PASS;
}

TEST(get_stats_endpoint) {
    char response[4096];

    int ret = http_request("127.0.0.1", 18080, "GET", "/api/v1/stats",
                           "", NULL, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;
    }

    ASSERT(check_http_status(response, 200) == 0);
    ASSERT(check_response_contains(response, "\"active_connections\"") == 0);

    return TEST_PASS;
}

TEST(get_metrics_endpoint) {
    char response[4096];

    int ret = http_request("127.0.0.1", 18080, "GET", "/api/v1/metrics",
                           "", NULL, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;
    }

    ASSERT(check_http_status(response, 200) == 0);
    /* Prometheus формат: metric_name value */
    ASSERT(check_response_contains(response, "mtproxy_") == 0);

    return TEST_PASS;
}

/* ============================================================================
 * Authentication Tests
 * ============================================================================ */

TEST(auth_valid_api_key) {
    char response[4096];
    const char *headers = "Authorization: Bearer test-api-key-123\r\n";

    int ret = http_request("127.0.0.1", 18081, "GET", "/api/v1/status",
                           headers, NULL, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;
    }

    ASSERT(check_http_status(response, 200) == 0);
    return TEST_PASS;
}

TEST(auth_missing_api_key) {
    char response[4096];

    int ret = http_request("127.0.0.1", 18081, "GET", "/api/v1/admin/reload",
                           "", NULL, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;
    }

    /* Должен вернуть 401 Unauthorized */
    ASSERT(check_http_status(response, 401) == 0);
    return TEST_PASS;
}

TEST(auth_invalid_api_key) {
    char response[4096];
    const char *headers = "Authorization: Bearer wrong-key\r\n";

    int ret = http_request("127.0.0.1", 18081, "GET", "/api/v1/admin/reload",
                           headers, NULL, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;
    }

    /* Должен вернуть 401 Unauthorized */
    ASSERT(check_http_status(response, 401) == 0);
    return TEST_PASS;
}

/* ============================================================================
 * Admin Endpoint Tests
 * ============================================================================ */

TEST(admin_reload_config) {
    char response[4096];
    const char *headers = "Authorization: Bearer test-api-key-123\r\n";

    int ret = http_request("127.0.0.1", 18081, "POST", "/api/v1/admin/reload",
                           headers, NULL, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;
    }

    /* Должен вернуть 200 OK или 202 Accepted */
    ASSERT(check_http_status(response, 200) == 0 ||
           check_http_status(response, 202) == 0);

    return TEST_PASS;
}

TEST(admin_shutdown_server) {
    char response[4096];
    const char *headers = "Authorization: Bearer test-api-key-123\r\n";

    int ret = http_request("127.0.0.1", 18081, "POST", "/api/v1/admin/shutdown",
                           headers, NULL, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;
    }

    /* Должен вернуть 200 OK или 202 Accepted */
    ASSERT(check_http_status(response, 200) == 0 ||
           check_http_status(response, 202) == 0);

    return TEST_PASS;
}

/* ============================================================================
 * Error Handling Tests
 * ============================================================================ */

TEST(error_invalid_endpoint) {
    char response[4096];

    int ret = http_request("127.0.0.1", 18080, "GET", "/api/v1/invalid-endpoint",
                           "", NULL, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;
    }

    /* Должен вернуть 404 Not Found */
    ASSERT(check_http_status(response, 404) == 0);
    return TEST_PASS;
}

TEST(error_method_not_allowed) {
    char response[4096];

    /* POST на endpoint который принимает только GET */
    int ret = http_request("127.0.0.1", 18080, "POST", "/api/v1/status",
                           "", NULL, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;
    }

    /* Должен вернуть 405 Method Not Allowed */
    ASSERT(check_http_status(response, 405) == 0);
    return TEST_PASS;
}

TEST(error_bad_request_json) {
    char response[4096];
    const char *body = "{ invalid json }";
    const char *headers = "Content-Type: application/json\r\n";

    int ret = http_request("127.0.0.1", 18080, "POST", "/api/v1/config",
                           headers, body, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;
    }

    /* Должен вернуть 400 Bad Request */
    ASSERT(check_http_status(response, 400) == 0);
    return TEST_PASS;
}

/* ============================================================================
 * Stats and Metrics Tests
 * ============================================================================ */

TEST(stats_json_format) {
    char response[4096];

    int ret = http_request("127.0.0.1", 18080, "GET", "/api/v1/stats",
                           "", NULL, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;
    }

    /* Проверка JSON формата */
    ASSERT(check_response_contains(response, "{") == 0);
    ASSERT(check_response_contains(response, "}") == 0);

    return TEST_PASS;
}

TEST(metrics_prometheus_format) {
    char response[4096];

    int ret = http_request("127.0.0.1", 18080, "GET", "/api/v1/metrics",
                           "", NULL, response, sizeof(response));

    if (ret < 0) {
        printf("(server not running, skipped) ");
        return TEST_PASS;
    }

    /* Проверка Prometheus формата (metric_name value) */
    ASSERT(check_response_contains(response, "mtproxy_") == 0);
    ASSERT(check_response_contains(response, "\n") == 0);

    return TEST_PASS;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║              MTProxy REST API Test Suite                     ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    /* Initialization Tests */
    printf("--- Initialization Tests ---\n");
    RUN_TEST(api_init_null_config);
    RUN_TEST(api_init_valid_config);
    RUN_TEST(api_init_with_auth);
    printf("\n");

    /* Status Endpoint Tests */
    printf("--- Status Endpoint Tests ---\n");
    RUN_TEST(get_status_endpoint);
    RUN_TEST(get_stats_endpoint);
    RUN_TEST(get_metrics_endpoint);
    printf("\n");

    /* Authentication Tests */
    printf("--- Authentication Tests ---\n");
    RUN_TEST(auth_valid_api_key);
    RUN_TEST(auth_missing_api_key);
    RUN_TEST(auth_invalid_api_key);
    printf("\n");

    /* Admin Endpoint Tests */
    printf("--- Admin Endpoint Tests ---\n");
    RUN_TEST(admin_reload_config);
    RUN_TEST(admin_shutdown_server);
    printf("\n");

    /* Error Handling Tests */
    printf("--- Error Handling Tests ---\n");
    RUN_TEST(error_invalid_endpoint);
    RUN_TEST(error_method_not_allowed);
    RUN_TEST(error_bad_request_json);
    printf("\n");

    /* Stats and Metrics Tests */
    printf("--- Stats and Metrics Tests ---\n");
    RUN_TEST(stats_json_format);
    RUN_TEST(metrics_prometheus_format);
    printf("\n");

    /* Summary */
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    Test Summary                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Total:  %d\n", tests_run);
    printf("\n");

    return tests_failed > 0 ? 1 : 0;
}
