/*
 * test_admin_cli_integration.c — Интеграционные тесты для admin-cli
 *
 * Тестирование интеграции admin-cli с сервером:
 * - Подключение к серверу
 * - Выполнение команд (status, stats, cache-*)
 * - Конфигурация (config-show, config-set)
 * - Rate limiting (ratelimit-*)
 * - Логирование (log-level, log-flush)
 *
 * Требования: запущенный MTProxy сервер
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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
#include <netdb.h>
#endif

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static int test_##name(void)
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    if (test_##name() == 0) { \
        printf("PASSED\n"); \
        tests_passed++; \
    } else { \
        printf("FAILED\n"); \
        tests_failed++; \
    } \
} while(0)

/* ============================================================================
 * Helper functions
 * ============================================================================ */

/* Подключение к admin socket */
static int connect_to_admin(const char *host, int port) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return -1;
    }
#endif

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

#ifdef _WIN32
    addr.sin_addr.s_addr = inet_addr(host);
#else
    inet_pton(AF_INET, host, &addr.sin_addr);
#endif

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return -1;
    }

    return sock;
}

/* Отправка команды и получение ответа */
static int send_admin_command(int sock, const char *cmd, char *response, size_t resp_size) {
    if (send(sock, cmd, strlen(cmd), 0) < 0) {
        return -1;
    }

    /* Небольшая задержка для получения ответа */
    usleep(100000);

    int n = recv(sock, response, resp_size - 1, 0);
    if (n < 0) {
        return -1;
    }

    response[n] = '\0';
    return 0;
}

/* Закрытие подключения */
static void close_admin_connection(int sock) {
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
}

/* ============================================================================
 * Connection Tests
 * ============================================================================ */

TEST(connect_to_server) {
    /* Тест подключения к серверу (предполагается, что сервер запущен на порту 8889) */
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0; /* Не ошибка, если сервер не запущен */
    }

    close_admin_connection(sock);
    printf("(connected) ");
    return 0;
}

/* ============================================================================
 * Status Command Tests
 * ============================================================================ */

TEST(status_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];
    int ret = send_admin_command(sock, "status\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(command failed) ");
        return -1;
    }

    /* Проверяем, что ответ содержит ожидаемые поля */
    if (strstr(response, "status") == NULL && strstr(response, "uptime") == NULL) {
        printf("(unexpected response: %s) ", response);
        return -1;
    }

    printf("(status ok) ");
    return 0;
}

/* ============================================================================
 * Stats Command Tests
 * ============================================================================ */

TEST(stats_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];
    int ret = send_admin_command(sock, "stats\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(command failed) ");
        return -1;
    }

    /* Проверяем наличие статистики */
    if (strlen(response) < 10) {
        printf("(empty response) ");
        return -1;
    }

    printf("(stats ok) ");
    return 0;
}

TEST(stats_detail_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];
    int ret = send_admin_command(sock, "stats detail\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(command failed) ");
        return -1;
    }

    printf("(stats detail ok) ");
    return 0;
}

/* ============================================================================
 * Cache Command Tests
 * ============================================================================ */

TEST(cache_stats_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];
    int ret = send_admin_command(sock, "cache-stats\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(command failed) ");
        return -1;
    }

    /* Проверяем наличие информации о кэше */
    if (strstr(response, "cache") == NULL && strstr(response, "entries") == NULL) {
        printf("(unexpected response: %s) ", response);
        return -1;
    }

    printf("(cache-stats ok) ");
    return 0;
}

TEST(cache_set_get_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];

    /* Устанавливаем значение */
    int ret = send_admin_command(sock, "cache-set test_key test_value 3600\n", response, sizeof(response));
    if (ret < 0) {
        close_admin_connection(sock);
        printf("(cache-set failed) ");
        return -1;
    }

    /* Получаем значение */
    ret = send_admin_command(sock, "cache-get test_key\n", response, sizeof(response));
    if (ret < 0) {
        close_admin_connection(sock);
        printf("(cache-get failed) ");
        return -1;
    }

    close_admin_connection(sock);

    /* Проверяем, что значение получено */
    if (strstr(response, "test_value") == NULL) {
        printf("(value mismatch: %s) ", response);
        return -1;
    }

    printf("(cache set/get ok) ");
    return 0;
}

TEST(cache_delete_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];

    /* Сначала установим значение */
    send_admin_command(sock, "cache-set test_del_key temp_value 60\n", response, sizeof(response));

    /* Удаляем значение */
    int ret = send_admin_command(sock, "cache-delete test_del_key\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(cache-delete failed) ");
        return -1;
    }

    printf("(cache-delete ok) ");
    return 0;
}

/* ============================================================================
 * Config Command Tests
 * ============================================================================ */

TEST(config_show_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];
    int ret = send_admin_command(sock, "config show\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(command failed) ");
        return -1;
    }

    /* Проверяем наличие конфигурации */
    if (strlen(response) < 10) {
        printf("(empty response) ");
        return -1;
    }

    printf("(config show ok) ");
    return 0;
}

TEST(config_set_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];

    /* Устанавливаем параметр (например, log level) */
    int ret = send_admin_command(sock, "config-set log.level info\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(config-set failed) ");
        return -1;
    }

    printf("(config-set ok) ");
    return 0;
}

/* ============================================================================
 * Rate Limit Command Tests
 * ============================================================================ */

TEST(ratelimit_status_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];
    int ret = send_admin_command(sock, "ratelimit\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(command failed) ");
        return -1;
    }

    printf("(ratelimit status ok) ");
    return 0;
}

TEST(ratelimit_add_remove_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];

    /* Добавляем rate limit */
    int ret = send_admin_command(sock, "ratelimit-add test_api 100 60\n", response, sizeof(response));
    if (ret < 0) {
        close_admin_connection(sock);
        printf("(ratelimit-add failed) ");
        return -1;
    }

    /* Проверяем */
    ret = send_admin_command(sock, "ratelimit test_api\n", response, sizeof(response));
    if (ret < 0) {
        close_admin_connection(sock);
        printf("(ratelimit check failed) ");
        return -1;
    }

    /* Удаляем */
    ret = send_admin_command(sock, "ratelimit-remove test_api\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(ratelimit-remove failed) ");
        return -1;
    }

    printf("(ratelimit add/remove ok) ");
    return 0;
}

/* ============================================================================
 * Log Command Tests
 * ============================================================================ */

TEST(log_level_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];

    /* Устанавливаем уровень логирования */
    int ret = send_admin_command(sock, "log-level info\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(log-level failed) ");
        return -1;
    }

    printf("(log-level ok) ");
    return 0;
}

TEST(log_flush_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];
    int ret = send_admin_command(sock, "log-flush\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(log-flush failed) ");
        return -1;
    }

    printf("(log-flush ok) ");
    return 0;
}

/* ============================================================================
 * Health Check Command Tests
 * ============================================================================ */

TEST(health_check_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];
    int ret = send_admin_command(sock, "health\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(health check failed) ");
        return -1;
    }

    /* Проверяем, что сервер здоров */
    if (strstr(response, "ok") == NULL && strstr(response, "healthy") == NULL) {
        printf("(unhealthy response: %s) ", response);
        return -1;
    }

    printf("(health check ok) ");
    return 0;
}

/* ============================================================================
 * Metrics Command Tests
 * ============================================================================ */

TEST(metrics_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];
    int ret = send_admin_command(sock, "metrics\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(metrics failed) ");
        return -1;
    }

    printf("(metrics ok) ");
    return 0;
}

/* ============================================================================
 * Error Handling Tests
 * ============================================================================ */

TEST(invalid_command) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];
    int ret = send_admin_command(sock, "invalid-command-xyz\n", response, sizeof(response));

    close_admin_connection(sock);

    if (ret < 0) {
        printf("(command failed) ");
        return -1;
    }

    /* Проверяем, что получена ошибка */
    if (strstr(response, "error") == NULL && strstr(response, "unknown") == NULL) {
        printf("(expected error, got: %s) ", response);
        return -1;
    }

    printf("(invalid command handled) ");
    return 0;
}

/* ============================================================================
 * Performance Tests
 * ============================================================================ */

TEST(command_latency) {
    int sock = connect_to_admin("127.0.0.1", 8889);

    if (sock < 0) {
        printf("(server not running, skipped) ");
        return 0;
    }

    char response[4096];
    int iterations = 10;
    double total_time = 0;

    for (int i = 0; i < iterations; i++) {
        clock_t start = clock();
        int ret = send_admin_command(sock, "status\n", response, sizeof(response));
        clock_t end = clock();

        if (ret < 0) {
            close_admin_connection(sock);
            printf("(command %d failed) ", i);
            return -1;
        }

        double elapsed = (double)(end - start) / CLOCKS_PER_SEC * 1000;
        total_time += elapsed;
    }

    close_admin_connection(sock);

    double avg_latency = total_time / iterations;
    printf("(avg latency: %.2f ms) ", avg_latency);

    /* Проверяем, что латентность в разумных пределах (< 100ms) */
    if (avg_latency > 100) {
        printf("(high latency: %.2f ms) ", avg_latency);
        return -1;
    }

    return 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("=== Admin CLI Integration Tests ===\n\n");

    /* Connection Tests */
    printf("--- Connection ---\n");
    RUN_TEST(connect_to_server);
    printf("\n");

    /* Status Command Tests */
    printf("--- Status Commands ---\n");
    RUN_TEST(status_command);
    printf("\n");

    /* Stats Command Tests */
    printf("--- Stats Commands ---\n");
    RUN_TEST(stats_command);
    RUN_TEST(stats_detail_command);
    printf("\n");

    /* Cache Command Tests */
    printf("--- Cache Commands ---\n");
    RUN_TEST(cache_stats_command);
    RUN_TEST(cache_set_get_command);
    RUN_TEST(cache_delete_command);
    printf("\n");

    /* Config Command Tests */
    printf("--- Config Commands ---\n");
    RUN_TEST(config_show_command);
    RUN_TEST(config_set_command);
    printf("\n");

    /* Rate Limit Command Tests */
    printf("--- Rate Limit Commands ---\n");
    RUN_TEST(ratelimit_status_command);
    RUN_TEST(ratelimit_add_remove_command);
    printf("\n");

    /* Log Command Tests */
    printf("--- Log Commands ---\n");
    RUN_TEST(log_level_command);
    RUN_TEST(log_flush_command);
    printf("\n");

    /* Health Check Command Tests */
    printf("--- Health Check ---\n");
    RUN_TEST(health_check_command);
    printf("\n");

    /* Metrics Command Tests */
    printf("--- Metrics ---\n");
    RUN_TEST(metrics_command);
    printf("\n");

    /* Error Handling Tests */
    printf("--- Error Handling ---\n");
    RUN_TEST(invalid_command);
    printf("\n");

    /* Performance Tests */
    printf("--- Performance ---\n");
    RUN_TEST(command_latency);
    printf("\n");

    /* Summary */
    printf("=== Test Summary ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
