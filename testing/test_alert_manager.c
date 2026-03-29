/*
    MTProxy Alert Manager Tests
    Тесты для системы уведомлений и алертинга
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "system/monitoring/alert-manager.h"
#include "testing/test_common.h"

// ============================================================================
// Тесты инициализации
// ============================================================================

static int test_alert_manager_init(void) {
    TEST_START();
    
    int result = alert_manager_init();
    
    ASSERT(result == 0, "Инициализация успешна");
    ASSERT(alert_manager_is_initialized() == true, "Менеджер инициализирован");
    
    // Повторная инициализация должна возвращать 0
    result = alert_manager_init();
    ASSERT(result == 0, "Повторная инициализация успешна");
    
    TEST_END();
}

static int test_alert_manager_cleanup(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_cleanup();
    
    ASSERT(alert_manager_is_initialized() == false, "Менеджер очищен");
    
    TEST_END();
}

// ============================================================================
// Тесты управления каналами
// ============================================================================

static int test_alert_manager_add_channel(void) {
    TEST_START();
    
    alert_manager_init();
    
    int result = alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_WARNING);
    ASSERT(result == 0, "Канал Telegram добавлен");
    
    result = alert_manager_add_channel(ALERT_CHANNEL_EMAIL, "Email", ALERT_LEVEL_ERROR);
    ASSERT(result == 0, "Канал Email добавлен");
    
    result = alert_manager_add_channel(ALERT_CHANNEL_SLACK, "Slack", ALERT_LEVEL_INFO);
    ASSERT(result == 0, "Канал Slack добавлен");
    
    // Попытка добавить тот же канал повторно
    result = alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram2", ALERT_LEVEL_INFO);
    ASSERT(result == -1, "Повторное добавление канала невозможно");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_manager_remove_channel(void) {
    TEST_START();
    
    alert_manager_init();
    
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_WARNING);
    
    int result = alert_manager_remove_channel(ALERT_CHANNEL_TELEGRAM);
    ASSERT(result == 0, "Канал удалён");
    
    result = alert_manager_remove_channel(ALERT_CHANNEL_TELEGRAM);
    ASSERT(result == -1, "Повторное удаление невозможно");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_manager_set_channel_enabled(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_WARNING);
    
    int result = alert_manager_set_channel_enabled(ALERT_CHANNEL_TELEGRAM, false);
    ASSERT(result == 0, "Канал выключен");
    
    result = alert_manager_set_channel_enabled(ALERT_CHANNEL_TELEGRAM, true);
    ASSERT(result == 0, "Канал включён");
    
    result = alert_manager_set_channel_enabled(ALERT_CHANNEL_EMAIL, false);
    ASSERT(result == -1, "Выключение несуществующего канала невозможно");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_manager_set_channel_rate_limit(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_WARNING);
    
    int result = alert_manager_set_channel_rate_limit(ALERT_CHANNEL_TELEGRAM, 10);
    ASSERT(result == 0, "Rate limit установлен");
    
    result = alert_manager_set_channel_rate_limit(ALERT_CHANNEL_TELEGRAM, 0);
    ASSERT(result == 0, "Rate limit отключён");
    
    alert_manager_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты управления получателями
// ============================================================================

static int test_alert_manager_add_recipient(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_WARNING);
    
    int result = alert_manager_add_recipient(ALERT_CHANNEL_TELEGRAM, "bot_token", "chat_id_123");
    ASSERT(result == 0, "Получатель добавлен");
    
    result = alert_manager_add_recipient(ALERT_CHANNEL_TELEGRAM, "bot_token2", "chat_id_456");
    ASSERT(result == 0, "Второй получатель добавлен");
    
    result = alert_manager_add_recipient(ALERT_CHANNEL_EMAIL, "smtp", "test@example.com");
    ASSERT(result == -1, "Добавление в несуществующий канал невозможно");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_manager_remove_recipient(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_WARNING);
    alert_manager_add_recipient(ALERT_CHANNEL_TELEGRAM, "bot_token", "chat_id_123");
    
    int result = alert_manager_remove_recipient(ALERT_CHANNEL_TELEGRAM, "chat_id_123");
    ASSERT(result == 0, "Получатель удалён");
    
    result = alert_manager_remove_recipient(ALERT_CHANNEL_TELEGRAM, "chat_id_123");
    ASSERT(result == -1, "Повторное удаление невозможно");
    
    alert_manager_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты отправки алертов
// ============================================================================

static int test_alert_manager_send_alert(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_WARNING);
    alert_manager_add_recipient(ALERT_CHANNEL_TELEGRAM, "bot_token", "chat_id_123");
    
    uint64_t alert_id = alert_manager_send_alert(
        ALERT_LEVEL_CRITICAL,
        ALERT_TYPE_SERVER_DOWN,
        "Test Alert",
        "This is a test alert message"
    );
    
    ASSERT(alert_id > 0, "Алерт отправлен");
    ASSERT(alert_id == 1, "ID первого алерта = 1");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_manager_send_alert_multiple(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_INFO);
    alert_manager_add_recipient(ALERT_CHANNEL_TELEGRAM, "bot_token", "chat_id_123");
    
    uint64_t id1 = alert_manager_send_alert(ALERT_LEVEL_INFO, ALERT_TYPE_CUSTOM, "Alert 1", "Message 1");
    uint64_t id2 = alert_manager_send_alert(ALERT_LEVEL_WARNING, ALERT_TYPE_CUSTOM, "Alert 2", "Message 2");
    uint64_t id3 = alert_manager_send_alert(ALERT_LEVEL_CRITICAL, ALERT_TYPE_CUSTOM, "Alert 3", "Message 3");
    
    ASSERT(id1 == 1, "ID первого алерта = 1");
    ASSERT(id2 == 2, "ID второго алерта = 2");
    ASSERT(id3 == 3, "ID третьего алерта = 3");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_manager_send_alert_min_level(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_ERROR);
    alert_manager_add_recipient(ALERT_CHANNEL_TELEGRAM, "bot_token", "chat_id_123");
    
    // Алерт уровнем ниже минимального не должен отправляться
    uint64_t id1 = alert_manager_send_alert(ALERT_LEVEL_WARNING, ALERT_TYPE_CUSTOM, "Low Level", "Message");
    
    // Алерт уровнем выше минимального должен отправляться
    uint64_t id2 = alert_manager_send_alert(ALERT_LEVEL_CRITICAL, ALERT_TYPE_CUSTOM, "High Level", "Message");
    
    ASSERT(id1 > 0, "Алерт создан (но не отправлен в канал)");
    ASSERT(id2 > 0, "Алерт отправлен");
    
    alert_manager_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты встроенных алертов
// ============================================================================

static int test_alert_server_down(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_INFO);
    alert_manager_add_recipient(ALERT_CHANNEL_TELEGRAM, "bot_token", "chat_id_123");
    
    alert_server_down("test-server");
    
    const alert_t* last = alert_manager_get_last_alert();
    ASSERT(last != NULL, "Последний алерт существует");
    ASSERT(last->level == ALERT_LEVEL_CRITICAL, "Уровень CRITICAL");
    ASSERT(last->type == ALERT_TYPE_SERVER_DOWN, "Тип ServerDown");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_server_up(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_INFO);
    
    alert_server_up("test-server");
    
    const alert_t* last = alert_manager_get_last_alert();
    ASSERT(last != NULL, "Последний алерт существует");
    ASSERT(last->level == ALERT_LEVEL_INFO, "Уровень INFO");
    ASSERT(last->type == ALERT_TYPE_SERVER_UP, "Тип ServerUp");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_high_cpu(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_INFO);
    
    alert_high_cpu(95.5, 90.0);
    
    const alert_t* last = alert_manager_get_last_alert();
    ASSERT(last != NULL, "Последний алерт существует");
    ASSERT(last->level == ALERT_LEVEL_WARNING, "Уровень WARNING");
    ASSERT(last->type == ALERT_TYPE_HIGH_CPU, "Тип HighCPU");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_high_memory(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_INFO);
    
    alert_high_memory(7000000000ULL, 8000000000ULL, 80.0);
    
    const alert_t* last = alert_manager_get_last_alert();
    ASSERT(last != NULL, "Последний алерт существует");
    ASSERT(last->level == ALERT_LEVEL_WARNING, "Уровень WARNING");
    ASSERT(last->type == ALERT_TYPE_HIGH_MEMORY, "Тип HighMemory");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_high_connections(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_INFO);
    
    alert_high_connections(900, 1000);
    
    const alert_t* last = alert_manager_get_last_alert();
    ASSERT(last != NULL, "Последний алерт существует");
    ASSERT(last->level == ALERT_LEVEL_WARNING, "Уровень WARNING");
    ASSERT(last->type == ALERT_TYPE_HIGH_CONNECTIONS, "Тип HighConnections");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_rate_limit_exceeded(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_INFO);
    
    alert_rate_limit_exceeded("192.168.1.100", 150, 100);
    
    const alert_t* last = alert_manager_get_last_alert();
    ASSERT(last != NULL, "Последний алерт существует");
    ASSERT(last->level == ALERT_LEVEL_WARNING, "Уровень WARNING");
    ASSERT(last->type == ALERT_TYPE_RATE_LIMIT, "Тип RateLimit");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_security_event(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_INFO);
    
    alert_security_event("Unauthorized access attempt");
    
    const alert_t* last = alert_manager_get_last_alert();
    ASSERT(last != NULL, "Последний алерт существует");
    ASSERT(last->level == ALERT_LEVEL_ERROR, "Уровень ERROR");
    ASSERT(last->type == ALERT_TYPE_SECURITY, "Тип Security");
    
    alert_manager_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты статистики
// ============================================================================

static int test_alert_manager_get_stats(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_INFO);
    alert_manager_add_recipient(ALERT_CHANNEL_TELEGRAM, "bot_token", "chat_id_123");
    
    alert_manager_send_alert(ALERT_LEVEL_INFO, ALERT_TYPE_CUSTOM, "Test", "Message");
    
    alert_manager_stats_t stats;
    int result = alert_manager_get_stats(&stats);
    
    ASSERT(result == 0, "Статистика получена");
    ASSERT(stats.total_alerts == 1, "Всего алертов: 1");
    ASSERT(stats.alerts_sent > 0, "Отправленные алерты > 0");
    ASSERT(stats.active_channels == 1, "Активных каналов: 1");
    ASSERT(stats.active_recipients == 1, "Активных получателей: 1");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_manager_get_stats_string(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_INFO);
    
    alert_manager_send_alert(ALERT_LEVEL_INFO, ALERT_TYPE_CUSTOM, "Test", "Message");
    
    char buffer[512] = {0};
    int result = alert_manager_get_stats_string(buffer, sizeof(buffer));
    
    ASSERT(result == 0, "Строка статистики получена");
    ASSERT(strlen(buffer) > 0, "Строка не пустая");
    ASSERT(strstr(buffer, "Alert Manager Statistics") != NULL, "Содержит заголовок");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_manager_reset_stats(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_INFO);
    
    alert_manager_send_alert(ALERT_LEVEL_INFO, ALERT_TYPE_CUSTOM, "Test", "Message");
    
    alert_manager_reset_stats();
    
    alert_manager_stats_t stats;
    alert_manager_get_stats(&stats);
    
    ASSERT(stats.total_alerts == 0, "Статистика сброшена");
    ASSERT(stats.alerts_sent == 0, "Отправленные сброшены");
    
    alert_manager_cleanup();
    
    TEST_END();
}

// ============================================================================
// Тесты утилит
// ============================================================================

static int test_alert_level_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(alert_level_to_string(ALERT_LEVEL_DEBUG), "DEBUG") == 0, "DEBUG");
    ASSERT(strcmp(alert_level_to_string(ALERT_LEVEL_INFO), "INFO") == 0, "INFO");
    ASSERT(strcmp(alert_level_to_string(ALERT_LEVEL_WARNING), "WARNING") == 0, "WARNING");
    ASSERT(strcmp(alert_level_to_string(ALERT_LEVEL_ERROR), "ERROR") == 0, "ERROR");
    ASSERT(strcmp(alert_level_to_string(ALERT_LEVEL_CRITICAL), "CRITICAL") == 0, "CRITICAL");
    
    TEST_END();
}

static int test_alert_type_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(alert_type_to_string(ALERT_TYPE_SERVER_DOWN), "ServerDown") == 0, "ServerDown");
    ASSERT(strcmp(alert_type_to_string(ALERT_TYPE_SERVER_UP), "ServerUp") == 0, "ServerUp");
    ASSERT(strcmp(alert_type_to_string(ALERT_TYPE_HIGH_CPU), "HighCPU") == 0, "HighCPU");
    ASSERT(strcmp(alert_type_to_string(ALERT_TYPE_SECURITY), "Security") == 0, "Security");
    
    TEST_END();
}

static int test_alert_status_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(alert_status_to_string(ALERT_STATUS_PENDING), "Pending") == 0, "Pending");
    ASSERT(strcmp(alert_status_to_string(ALERT_STATUS_SENT), "Sent") == 0, "Sent");
    ASSERT(strcmp(alert_status_to_string(ALERT_STATUS_FAILED), "Failed") == 0, "Failed");
    
    TEST_END();
}

static int test_alert_level_from_string(void) {
    TEST_START();
    
    ASSERT(alert_level_from_string("DEBUG") == ALERT_LEVEL_DEBUG, "DEBUG");
    ASSERT(alert_level_from_string("INFO") == ALERT_LEVEL_INFO, "INFO");
    ASSERT(alert_level_from_string("WARNING") == ALERT_LEVEL_WARNING, "WARNING");
    ASSERT(alert_level_from_string("ERROR") == ALERT_LEVEL_ERROR, "ERROR");
    ASSERT(alert_level_from_string("CRITICAL") == ALERT_LEVEL_CRITICAL, "CRITICAL");
    
    TEST_END();
}

// ============================================================================
// Тесты получения алертов
// ============================================================================

static int test_alert_manager_get_last_alert(void) {
    TEST_START();
    
    alert_manager_init();
    
    const alert_t* last = alert_manager_get_last_alert();
    ASSERT(last == NULL, "Нет алертов после инициализации");
    
    alert_manager_send_alert(ALERT_LEVEL_INFO, ALERT_TYPE_CUSTOM, "Test", "Message");
    
    last = alert_manager_get_last_alert();
    ASSERT(last != NULL, "Последний алерт существует");
    ASSERT(last->level == ALERT_LEVEL_INFO, "Уровень INFO");
    
    alert_manager_cleanup();
    
    TEST_END();
}

static int test_alert_manager_get_alert_by_id(void) {
    TEST_START();
    
    alert_manager_init();
    alert_manager_add_channel(ALERT_CHANNEL_TELEGRAM, "Telegram", ALERT_LEVEL_INFO);
    
    uint64_t id1 = alert_manager_send_alert(ALERT_LEVEL_INFO, ALERT_TYPE_CUSTOM, "Test 1", "Message 1");
    uint64_t id2 = alert_manager_send_alert(ALERT_LEVEL_WARNING, ALERT_TYPE_CUSTOM, "Test 2", "Message 2");
    
    const alert_t* alert = alert_manager_get_alert_by_id(id1);
    ASSERT(alert != NULL, "Алерт по ID найден");
    ASSERT(alert->id == id1, "ID совпадает");
    
    alert = alert_manager_get_alert_by_id(id2);
    ASSERT(alert != NULL, "Второй алерт по ID найден");
    ASSERT(alert->id == id2, "ID совпадает");
    
    alert = alert_manager_get_alert_by_id(999);
    ASSERT(alert == NULL, "Несуществующий ID не найден");
    
    alert_manager_cleanup();
    
    TEST_END();
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    test_init("MTProxy Alert Manager Tests");
    
    // Инициализация
    test_run(test_alert_manager_init);
    test_run(test_alert_manager_cleanup);
    
    // Каналы
    test_run(test_alert_manager_add_channel);
    test_run(test_alert_manager_remove_channel);
    test_run(test_alert_manager_set_channel_enabled);
    test_run(test_alert_manager_set_channel_rate_limit);
    
    // Получатели
    test_run(test_alert_manager_add_recipient);
    test_run(test_alert_manager_remove_recipient);
    
    // Отправка алертов
    test_run(test_alert_manager_send_alert);
    test_run(test_alert_manager_send_alert_multiple);
    test_run(test_alert_manager_send_alert_min_level);
    
    // Встроенные алерты
    test_run(test_alert_server_down);
    test_run(test_alert_server_up);
    test_run(test_alert_high_cpu);
    test_run(test_alert_high_memory);
    test_run(test_alert_high_connections);
    test_run(test_alert_rate_limit_exceeded);
    test_run(test_alert_security_event);
    
    // Статистика
    test_run(test_alert_manager_get_stats);
    test_run(test_alert_manager_get_stats_string);
    test_run(test_alert_manager_reset_stats);
    
    // Утилиты
    test_run(test_alert_level_to_string);
    test_run(test_alert_type_to_string);
    test_run(test_alert_status_to_string);
    test_run(test_alert_level_from_string);
    
    // Получение алертов
    test_run(test_alert_manager_get_last_alert);
    test_run(test_alert_manager_get_alert_by_id);
    
    return test_finish();
}
