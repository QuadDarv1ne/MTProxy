/*
    MTProxy Alert Manager
    Система уведомлений и алертинга
    
    Поддерживаемые каналы:
    - Telegram Bot
    - Email (SMTP)
    - Slack Webhook
    
    Примеры использования:
    alert_manager_init();
    alert_manager_add_recipient(ALERT_CHANNEL_TELEGRAM, "bot_token", "chat_id");
    alert_manager_add_recipient(ALERT_CHANNEL_EMAIL, "smtp@example.com", "user@example.com");
    alert_manager_add_recipient(ALERT_CHANNEL_SLACK, "webhook_url", NULL);
    alert_manager_send_alert(ALERT_LEVEL_CRITICAL, "ServerDown", "MTProxy server is down!");
    alert_manager_cleanup();
*/

#ifndef ALERT_MANAGER_H
#define ALERT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Версия Alert Manager
#define ALERT_MANAGER_VERSION "1.0.0"

// Максимальные размеры
#define ALERT_MAX_CHANNELS 8
#define ALERT_MAX_RECIPIENTS_PER_CHANNEL 16
#define ALERT_MAX_TOKEN_LEN 256
#define ALERT_MAX_MESSAGE_LEN 2048
#define ALERT_MAX_TITLE_LEN 256
#define ALERT_MAX_HOSTNAME_LEN 256

// Уровни алертов
typedef enum {
    ALERT_LEVEL_DEBUG = 0,
    ALERT_LEVEL_INFO = 1,
    ALERT_LEVEL_WARNING = 2,
    ALERT_LEVEL_ERROR = 3,
    ALERT_LEVEL_CRITICAL = 4
} alert_level_t;

// Типы каналов
typedef enum {
    ALERT_CHANNEL_TELEGRAM = 0,
    ALERT_CHANNEL_EMAIL = 1,
    ALERT_CHANNEL_SLACK = 2,
    ALERT_CHANNEL_WEBHOOK = 3,
    ALERT_CHANNEL_CUSTOM = 4
} alert_channel_t;

// Типы алертов
typedef enum {
    ALERT_TYPE_SERVER_DOWN = 0,
    ALERT_TYPE_SERVER_UP = 1,
    ALERT_TYPE_HIGH_CPU = 2,
    ALERT_TYPE_HIGH_MEMORY = 3,
    ALERT_TYPE_HIGH_CONNECTIONS = 4,
    ALERT_TYPE_RATE_LIMIT = 5,
    ALERT_TYPE_SECURITY = 6,
    ALERT_TYPE_CONFIG_CHANGED = 7,
    ALERT_TYPE_CUSTOM = 99
} alert_type_t;

// Статус алерта
typedef enum {
    ALERT_STATUS_PENDING = 0,
    ALERT_STATUS_SENT = 1,
    ALERT_STATUS_FAILED = 2,
    ALERT_STATUS_ACKNOWLEDGED = 3
} alert_status_t;

// Конфигурация получателя
typedef struct {
    char token[ALERT_MAX_TOKEN_LEN];      // Token/API key
    char recipient[ALERT_MAX_TOKEN_LEN];  // Chat ID, Email, Webhook URL
    bool enabled;
    int64_t created_at;
    int alerts_sent;
    int alerts_failed;
} alert_recipient_t;

// Конфигурация канала
typedef struct {
    alert_channel_t type;
    char name[64];
    alert_recipient_t recipients[ALERT_MAX_RECIPIENTS_PER_CHANNEL];
    int recipient_count;
    bool enabled;
    
    // Настройки
    int min_level;           // Минимальный уровень для отправки
    int rate_limit_sec;      // Rate limit между алертами (сек)
    bool aggregate;          // Агрегировать алерты
    int aggregate_window_sec; // Окно агрегации (сек)
} alert_channel_config_t;

// Структура алерта
typedef struct {
    uint64_t id;
    alert_level_t level;
    alert_type_t type;
    char title[ALERT_MAX_TITLE_LEN];
    char message[ALERT_MAX_MESSAGE_LEN];
    char source[ALERT_MAX_HOSTNAME_LEN];
    int64_t timestamp;
    alert_status_t status;
    int channels_sent;
    char metadata[512];  // JSON метаданные
} alert_t;

// Статистика Alert Manager
typedef struct {
    uint64_t total_alerts;
    uint64_t alerts_sent;
    uint64_t alerts_failed;
    uint64_t alerts_by_level[5];
    uint64_t alerts_by_type[10];
    int64_t last_alert_time;
    int active_channels;
    int active_recipients;
} alert_manager_stats_t;

// Callback для кастомных каналов
typedef int (*alert_send_callback_t)(alert_channel_t type, const char *token, 
                                      const char *recipient, const alert_t *alert);

// ============================================================================
// Инициализация и очистка
// ============================================================================

/**
 * Инициализация Alert Manager
 * @return 0 при успехе, -1 при ошибке
 */
int alert_manager_init(void);

/**
 * Очистка Alert Manager
 */
void alert_manager_cleanup(void);

/**
 * Проверка инициализации
 * @return true если инициализирован
 */
bool alert_manager_is_initialized(void);

// ============================================================================
// Управление каналами
// ============================================================================

/**
 * Добавить канал уведомлений
 * @param type Тип канала
 * @param name Имя канала
 * @param min_level Минимальный уровень алертов
 * @return 0 при успехе, -1 при ошибке
 */
int alert_manager_add_channel(alert_channel_t type, const char *name, int min_level);

/**
 * Удалить канал
 * @param type Тип канала
 * @return 0 при успехе, -1 при ошибке
 */
int alert_manager_remove_channel(alert_channel_t type);

/**
 * Включить/выключить канал
 * @param type Тип канала
 * @param enabled Статус
 * @return 0 при успехе, -1 при ошибке
 */
int alert_manager_set_channel_enabled(alert_channel_t type, bool enabled);

/**
 * Настроить rate limit для канала
 * @param type Тип канала
 * @param rate_limit_sec Rate limit в секундах
 * @return 0 при успехе, -1 при ошибке
 */
int alert_manager_set_channel_rate_limit(alert_channel_t type, int rate_limit_sec);

// ============================================================================
// Управление получателями
// ============================================================================

/**
 * Добавить получателя в канал
 * @param type Тип канала
 * @param token Token/API key
 * @param recipient Получатель (chat_id, email, webhook_url)
 * @return 0 при успехе, -1 при ошибке
 */
int alert_manager_add_recipient(alert_channel_t type, const char *token, 
                                 const char *recipient);

/**
 * Удалить получателя из канала
 * @param type Тип канала
 * @param recipient Получатель
 * @return 0 при успехе, -1 при ошибке
 */
int alert_manager_remove_recipient(alert_channel_t type, const char *recipient);

/**
 * Установить callback для кастомного канала
 * @param callback Функция обратного вызова
 * @return 0 при успехе, -1 при ошибке
 */
int alert_manager_set_custom_callback(alert_send_callback_t callback);

// ============================================================================
// Отправка алертов
// ============================================================================

/**
 * Отправить алерт
 * @param level Уровень алерта
 * @param type Тип алерта
 * @param title Заголовок
 * @param message Сообщение
 * @return ID алерта или -1 при ошибке
 */
uint64_t alert_manager_send_alert(alert_level_t level, alert_type_t type, 
                                   const char *title, const char *message);

/**
 * Отправить алерт с метаданными
 * @param level Уровень алерта
 * @param type Тип алерта
 * @param title Заголовок
 * @param message Сообщение
 * @param metadata JSON метаданные
 * @return ID алерта или -1 при ошибке
 */
uint64_t alert_manager_send_alert_with_metadata(alert_level_t level, alert_type_t type,
                                                 const char *title, const char *message,
                                                 const char *metadata);

/**
 * Отправить алерт в конкретный канал
 * @param channel Тип канала
 * @param alert Алерт
 * @return 0 при успехе, -1 при ошибке
 */
int alert_manager_send_to_channel(alert_channel_t channel, const alert_t *alert);

// ============================================================================
// Встроенные алерты
// ============================================================================

/**
 * Алерт: Сервер упал
 * @param server_name Имя сервера
 */
void alert_server_down(const char *server_name);

/**
 * Алерт: Сервер запустился
 * @param server_name Имя сервера
 */
void alert_server_up(const char *server_name);

/**
 * Алерт: Высокий CPU
 * @param cpu_usage Процент использования CPU
 * @param threshold Порог
 */
void alert_high_cpu(double cpu_usage, double threshold);

/**
 * Алерт: Высокая память
 * @param memory_usage Использовано памяти (байт)
 * @param totalMemory Всего памяти (байт)
 * @param threshold Порог в процентах
 */
void alert_high_memory(uint64_t memory_usage, uint64_t total_memory, double threshold);

/**
 * Алерт: Много подключений
 * @param connections Количество подключений
 * @param max_connections Максимум
 */
void alert_high_connections(uint64_t connections, uint64_t max_connections);

/**
 * Алерт: Rate limit превышен
 * @param ip IP адрес
 * @param requests Количество запросов
 * @param limit Лимит
 */
void alert_rate_limit_exceeded(const char *ip, int requests, int limit);

/**
 * Алерт: Проблема безопасности
 * @param description Описание
 */
void alert_security_event(const char *description);

// ============================================================================
// Статистика и мониторинг
// ============================================================================

/**
 * Получить статистику Alert Manager
 * @param stats Структура статистики
 * @return 0 при успехе, -1 при ошибке
 */
int alert_manager_get_stats(alert_manager_stats_t *stats);

/**
 * Получить строку статистики
 * @param buffer Буфер для вывода
 * @param buffer_size Размер буфера
 * @return 0 при успехе, -1 при ошибке
 */
int alert_manager_get_stats_string(char *buffer, size_t buffer_size);

/**
 * Сбросить статистику
 */
void alert_manager_reset_stats(void);

/**
 * Получить последний алерт
 * @return Указатель на последний алерт или NULL
 */
const alert_t* alert_manager_get_last_alert(void);

/**
 * Получить алерт по ID
 * @param alert_id ID алерта
 * @return Указатель на алерт или NULL
 */
const alert_t* alert_manager_get_alert_by_id(uint64_t alert_id);

// ============================================================================
// Утилиты
// ============================================================================

/**
 * Конвертировать уровень алерта в строку
 * @param level Уровень
 * @return Строка уровня
 */
const char* alert_level_to_string(alert_level_t level);

/**
 * Конвертировать тип алерта в строку
 * @param type Тип
 * @return Строка типа
 */
const char* alert_type_to_string(alert_type_t type);

/**
 * Конвертировать статус алерта в строку
 * @param status Статус
 * @return Строка статуса
 */
const char* alert_status_to_string(alert_status_t status);

/**
 * Конвертировать строку в уровень алерта
 * @param str Строка
 * @return Уровень
 */
alert_level_t alert_level_from_string(const char *str);

#ifdef __cplusplus
}
#endif

#endif // ALERT_MANAGER_H
