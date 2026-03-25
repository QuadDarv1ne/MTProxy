/*
 * Audit Logging System for MTProxy
 * Система детального логирования событий безопасности и аудита
 * 
 * Особенности:
 * - Детальное логирование всех событий безопасности
 * - Поддержка различных уровней аудита
 * - JSON формат для удобного парсинга
 * - Асинхронная запись для минимизации влияния на производительность
 * - Поддержка rotation логов
 * - Correlation ID для трассировки событий
 */

#ifndef AUDIT_LOG_H
#define AUDIT_LOG_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Константы и перечисления
 * ============================================================================ */

#define AUDIT_LOG_VERSION "1.0.0"
#define AUDIT_LOG_MAX_PATH_LEN 512
#define AUDIT_LOG_MAX_MESSAGE_LEN 2048
#define AUDIT_LOG_MAX_CONTEXT_LEN 1024
#define AUDIT_LOG_BUFFER_SIZE 8192
#define AUDIT_LOG_MAX_CORRELATION_ID_LEN 64
#define AUDIT_LOG_MAX_USER_ID_LEN 128
#define AUDIT_LOG_MAX_IP_ADDR_LEN 64

#define AUDIT_LOG_DEFAULT_ROTATION_SIZE_MB 100
#define AUDIT_LOG_DEFAULT_ROTATION_COUNT 10
#define AUDIT_LOG_DEFAULT_QUEUE_SIZE 4096

/* ============================================================================
 * Уровни аудита
 * ============================================================================ */

typedef enum {
    AUDIT_LEVEL_NONE = 0,       // Отключено
    AUDIT_LEVEL_CRITICAL = 1,   // Только критические события
    AUDIT_LEVEL_SECURITY = 2,   // События безопасности
    AUDIT_LEVEL_ACCESS = 3,     // Доступ + безопасность
    AUDIT_LEVEL_ALL = 4         // Все события
} audit_level_t;

/* ============================================================================
 * Категории событий аудита
 * ============================================================================ */

typedef enum {
    AUDIT_CATEGORY_NONE = 0,
    
    // Аутентификация и авторизация
    AUDIT_CATEGORY_AUTHENTICATION,    // Аутентификация
    AUDIT_CATEGORY_AUTHORIZATION,     // Авторизация
    AUDIT_CATEGORY_ACCESS_CONTROL,    // Контроль доступа
    
    // Безопасность
    AUDIT_CATEGORY_SECURITY_VIOLATION, // Нарушения безопасности
    AUDIT_CATEGORY_RATE_LIMITING,      // Rate limiting
    AUDIT_CATEGORY_DDOS_PROTECTION,    // DDoS защита
    AUDIT_CATEGORY_ENCRYPTION,         // Шифрование
    
    // Конфигурация
    AUDIT_CATEGORY_CONFIG_CHANGE,     // Изменения конфигурации
    AUDIT_CATEGORY_ADMIN_ACTION,      // Действия администратора
    
    // Соединения
    AUDIT_CATEGORY_CONNECTION,        // Подключения
    AUDIT_CATEGORY_DISCONNECTION,     // Отключения
    AUDIT_CATEGORY_TRAFFIC,           // Трафик
    
    // Система
    AUDIT_CATEGORY_SYSTEM,            // Системные события
    AUDIT_CATEGORY_ERROR,             // Ошибки
    AUDIT_CATEGORY_PERFORMANCE        // Производительность
    
} audit_category_t;

/* ============================================================================
 * Типы событий аудита
 * ============================================================================ */

typedef enum {
    AUDIT_EVENT_NONE = 0,
    
    // Аутентификация
    AUDIT_EVENT_LOGIN_SUCCESS,
    AUDIT_EVENT_LOGIN_FAILURE,
    AUDIT_EVENT_LOGOUT,
    AUDIT_EVENT_SESSION_CREATED,
    AUDIT_EVENT_SESSION_DESTROYED,
    AUDIT_EVENT_TOKEN_ISSUED,
    AUDIT_EVENT_TOKEN_REVOKED,
    
    // Авторизация
    AUDIT_EVENT_PERMISSION_GRANTED,
    AUDIT_EVENT_PERMISSION_DENIED,
    AUDIT_EVENT_ROLE_ASSIGNED,
    AUDIT_EVENT_ROLE_REVOKED,
    
    // Доступ
    AUDIT_EVENT_RESOURCE_ACCESSED,
    AUDIT_EVENT_ACCESS_DENIED,
    AUDIT_EVENT_WHITELIST_ADDED,
    AUDIT_EVENT_BLACKLIST_ADDED,
    
    // Безопасность
    AUDIT_EVENT_SECURITY_VIOLATION,
    AUDIT_EVENT_RATE_LIMIT_EXCEEDED,
    AUDIT_EVENT_DDOS_ATTACK_DETECTED,
    AUDIT_EVENT_INVALID_SIGNATURE,
    AUDIT_EVENT_CERTIFICATE_ERROR,
    AUDIT_EVENT_TLS_HANDSHAKE,
    
    // Конфигурация
    AUDIT_EVENT_CONFIG_MODIFIED,
    AUDIT_EVENT_CONFIG_RELOADED,
    AUDIT_EVENT_SECRET_ADDED,
    AUDIT_EVENT_SECRET_REMOVED,
    
    // Администрирование
    AUDIT_EVENT_ADMIN_COMMAND_EXECUTED,
    AUDIT_EVENT_USER_CREATED,
    AUDIT_EVENT_USER_DELETED,
    AUDIT_EVENT_API_KEY_CREATED,
    AUDIT_EVENT_API_KEY_REVOKED,
    
    // Соединения
    AUDIT_EVENT_CLIENT_CONNECTED,
    AUDIT_EVENT_CLIENT_DISCONNECTED,
    AUDIT_EVENT_CONNECTION_TIMEOUT,
    AUDIT_EVENT_CONNECTION_LIMIT_REACHED,
    
    // Трафик
    AUDIT_EVENT_TRAFFIC_THRESHOLD,
    AUDIT_EVENT_BANDWIDTH_LIMIT,
    
    // Система
    AUDIT_EVENT_SYSTEM_START,
    AUDIT_EVENT_SYSTEM_STOP,
    AUDIT_EVENT_SYSTEM_RESTART,
    AUDIT_EVENT_HEALTH_CHECK,
    
    // Ошибки
    AUDIT_EVENT_CRITICAL_ERROR,
    AUDIT_EVENT_RECOVERY_ACTION,
    
    // Производительность
    AUDIT_EVENT_PERFORMANCE_THRESHOLD,
    AUDIT_EVENT_RESOURCE_EXHAUSTION
    
} audit_event_type_t;

/* ============================================================================
 * Структуры данных
 * ============================================================================ */

/* Контекст события аудита */
typedef struct {
    char correlation_id[AUDIT_LOG_MAX_CORRELATION_ID_LEN];  // ID для трассировки
    char user_id[AUDIT_LOG_MAX_USER_ID_LEN];                // ID пользователя
    char ip_address[AUDIT_LOG_MAX_IP_ADDR_LEN];             // IP адрес
    uint16_t port;                                          // Порт
    char session_id[AUDIT_LOG_MAX_CORRELATION_ID_LEN];      // ID сессии
    uint64_t bytes_sent;                                    // Отправлено байт
    uint64_t bytes_received;                                // Получено байт
    uint32_t duration_ms;                                   // Длительность (мс)
    int result_code;                                        // Код результата
    char custom_data[AUDIT_LOG_MAX_CONTEXT_LEN];            // Дополнительные данные
} audit_context_t;

/* Событие аудита */
typedef struct {
    uint64_t event_id;                    // Уникальный ID события
    audit_event_type_t event_type;        // Тип события
    audit_category_t category;            // Категория
    audit_level_t level;                  // Уровень
    uint64_t timestamp;                   // Временная метка (epoch ms)
    char timestamp_iso[32];               // Временная метка (ISO 8601)
    char hostname[256];                   // Имя хоста
    char process_name[64];                // Имя процесса
    pid_t process_id;                     // PID процесса
    char thread_name[64];                 // Имя потока
    pthread_t thread_id;                  // ID потока
    char message[AUDIT_LOG_MAX_MESSAGE_LEN];  // Сообщение
    audit_context_t context;              // Контекст
    char signature[64];                   // Подпись (опционально)
} audit_event_t;

/* Конфигурация системы аудита */
typedef struct {
    bool enabled;                                 // Включено
    audit_level_t level;                          // Уровень аудита
    char log_path[AUDIT_LOG_MAX_PATH_LEN];        // Путь к файлу логов
    uint64_t rotation_size_bytes;                 // Размер для rotation
    uint32_t rotation_count;                      // Количество файлов
    bool async_mode;                              // Асинхронный режим
    uint32_t queue_size;                          // Размер очереди
    bool json_format;                             // JSON формат
    bool include_timestamps;                      // Включать временные метки
    bool include_hostname;                        // Включать имя хоста
    bool include_process_info;                    // Включать info о процессе
    bool compress_rotated;                        // Сжимать ротированные логи
    bool sync_to_disk;                            // Синхронная запись
    uint32_t flush_interval_ms;                   // Интервал сброса (мс)
    audit_category_t enabled_categories;          // Включенные категории
} audit_config_t;

/* Статистика системы аудита */
typedef struct {
    uint64_t total_events;           // Всего событий
    uint64_t events_logged;          // Записано событий
    uint64_t events_dropped;         // Отброшено событий
    uint64_t events_failed;          // Ошибок записи
    uint64_t bytes_written;          // Записано байт
    uint64_t rotations_performed;    // Выполнено ротаций
    uint64_t queue_size_current;     // Текущий размер очереди
    uint64_t queue_size_max;         // Макс размер очереди
    double avg_latency_us;           // Средняя задержка (мкс)
    double max_latency_us;           // Макс задержка (мкс)
    uint64_t events_by_category[16]; // События по категориям
    uint64_t events_by_level[5];     // События по уровням
} audit_stats_t;

/* Callback функция для обработки событий */
typedef void (*audit_event_callback_t)(const audit_event_t *event, void *user_data);

/* Основной контекст системы аудита */
typedef struct {
    audit_config_t config;              // Конфигурация
    audit_stats_t stats;                // Статистика
    pthread_mutex_t mutex;              // Мьютекс
    pthread_cond_t cond;                // Условная переменная
    pthread_t writer_thread;            // Поток записи
    bool running;                       // Запущено
    bool initialized;                   // Инициализировано
    
    // Асинхронная очередь
    audit_event_t *queue;               // Очередь событий
    uint32_t queue_head;                // Голова очереди
    uint32_t queue_tail;                // Хвост очереди
    uint32_t queue_count;               // Количество в очереди
    
    // Файлы
    FILE *current_log;                  // Текущий файл
    char current_path[AUDIT_LOG_MAX_PATH_LEN];  // Текущий путь
    uint64_t current_size;              // Текущий размер
    
    // Счетчики
    uint64_t event_counter;             // Счетчик событий
    
    // Callback'и
    audit_event_callback_t on_event;    // Callback на событие
    void *callback_user_data;           // Данные callback'а
    
    // Буфер для форматирования
    char format_buffer[AUDIT_LOG_BUFFER_SIZE];  // Буфер форматирования
} audit_logger_t;

/* ============================================================================
 * API системы аудита
 * ============================================================================ */

/**
 * @brief Инициализация системы аудита
 * @param logger Указатель на логгер
 * @param config Конфигурация
 * @return 0 при успехе, -1 при ошибке
 */
int audit_logger_init(audit_logger_t *logger, const audit_config_t *config);

/**
 * @brief Запуск системы аудита
 * @param logger Логгер
 * @return 0 при успехе, -1 при ошибке
 */
int audit_logger_start(audit_logger_t *logger);

/**
 * @brief Остановка системы аудита
 * @param logger Логгер
 */
void audit_logger_stop(audit_logger_t *logger);

/**
 * @brief Очистка системы аудита
 * @param logger Логгер
 */
void audit_logger_cleanup(audit_logger_t *logger);

/**
 * @brief Логирование события
 * @param logger Логгер
 * @param event_type Тип события
 * @param category Категория
 * @param level Уровень
 * @param message Сообщение
 * @param context Контекст (может быть NULL)
 * @return 0 при успехе, -1 при ошибке
 */
int audit_log_event(audit_logger_t *logger,
                   audit_event_type_t event_type,
                   audit_category_t category,
                   audit_level_t level,
                   const char *message,
                   const audit_context_t *context);

/**
 * @brief Логирование события с форматированием
 * @param logger Логгер
 * @param event_type Тип события
 * @param category Категория
 * @param level Уровень
 * @param format Формат (printf-style)
 * @param ... Аргументы
 * @return 0 при успехе, -1 при ошибке
 */
int audit_log_event_fmt(audit_logger_t *logger,
                       audit_event_type_t event_type,
                       audit_category_t category,
                       audit_level_t level,
                       const char *format, ...);

/**
 * @brief Быстрое логирование (для критических событий)
 * @param logger Логгер
 * @param event_type Тип события
 * @param message Сообщение
 * @return 0 при успехе, -1 при ошибке
 */
int audit_log_critical(audit_logger_t *logger,
                      audit_event_type_t event_type,
                      const char *message);

/**
 * @brief Логирование события безопасности
 * @param logger Логгер
 * @param event_type Тип события
 * @param ipAddress IP адрес
 * @param userId ID пользователя
 * @param message Сообщение
 * @return 0 при успехе, -1 при ошибке
 */
int audit_log_security(audit_logger_t *logger,
                      audit_event_type_t event_type,
                      const char *ip_address,
                      const char *user_id,
                      const char *message);

/**
 * @brief Логирование события доступа
 * @param logger Логгер
 * @param event_type Тип события
 * @param resource Ресурс
 * @param allowed Разрешено (1) или нет (0)
 * @param reason Причина
 * @return 0 при успехе, -1 при ошибке
 */
int audit_log_access(audit_logger_t *logger,
                    audit_event_type_t event_type,
                    const char *resource,
                    int allowed,
                    const char *reason);

/**
 * @brief Логирование изменения конфигурации
 * @param logger Логгер
 * @param param Параметр
 * @param old_value Старое значение
 * @param new_value Новое значение
 * @param admin_id ID администратора
 * @return 0 при успехе, -1 при ошибке
 */
int audit_log_config_change(audit_logger_t *logger,
                           const char *param,
                           const char *old_value,
                           const char *new_value,
                           const char *admin_id);

/**
 * @brief Логирование подключения клиента
 * @param logger Логгер
 * @param client_fd FD клиента
 * @param ip_address IP адрес
 * @param port Порт
 * @param secret_id ID секрета
 * @return 0 при успехе, -1 при ошибке
 */
int audit_log_client_connect(audit_logger_t *logger,
                            int client_fd,
                            const char *ip_address,
                            uint16_t port,
                            const char *secret_id);

/**
 * @brief Логирование отключения клиента
 * @param logger Логгер
 * @param client_fd FD клиента
 * @param ip_address IP адрес
 * @param reason Причина
 * @param duration_ms Длительность (мс)
 * @param bytes_sent Отправлено байт
 * @param bytes_received Получено байт
 * @return 0 при успехе, -1 при ошибке
 */
int audit_log_client_disconnect(audit_logger_t *logger,
                               int client_fd,
                               const char *ip_address,
                               const char *reason,
                               uint32_t duration_ms,
                               uint64_t bytes_sent,
                               uint64_t bytes_received);

/**
 * @brief Получение статистики
 * @param logger Логгер
 * @param stats Статистика
 */
void audit_logger_get_stats(audit_logger_t *logger, audit_stats_t *stats);

/**
 * @brief Сброс статистики
 * @param logger Логгер
 */
void audit_logger_reset_stats(audit_logger_t *logger);

/**
 * @brief Принудительный сброс на диск
 * @param logger Логгер
 * @return 0 при успехе, -1 при ошибке
 */
int audit_logger_flush(audit_logger_t *logger);

/**
 * @brief Ротация логов
 * @param logger Логгер
 * @return 0 при успехе, -1 при ошибке
 */
int audit_logger_rotate(audit_logger_t *logger);

/**
 * @brief Установка callback функции
 * @param logger Логгер
 * @param callback Callback функция
 * @param user_data Данные пользователя
 */
void audit_logger_set_callback(audit_logger_t *logger,
                              audit_event_callback_t callback,
                              void *user_data);

/**
 * @brief Проверка включенности категории
 * @param logger Логгер
 * @param category Категория
 * @return 1 если включено, 0 если нет
 */
int audit_is_category_enabled(audit_logger_t *logger, audit_category_t category);

/**
 * @brief Проверка включенности уровня
 * @param logger Логгер
 * @param level Уровень
 * @return 1 если включено, 0 если нет
 */
int audit_is_level_enabled(audit_logger_t *logger, audit_level_t level);

/**
 * @brief Генерация correlation ID
 * @param buffer Буфер
 * @param size Размер
 * @return Указатель на буфер
 */
char *audit_generate_correlation_id(char *buffer, size_t size);

/**
 * @brief Преобразование типа события в строку
 * @param event_type Тип события
 * @return Строковое представление
 */
const char *audit_event_type_to_string(audit_event_type_t event_type);

/**
 * @brief Преобразование категории в строку
 * @param category Категория
 * @return Строковое представление
 */
const char *audit_category_to_string(audit_category_t category);

/**
 * @brief Преобразование уровня в строку
 * @param level Уровень
 * @return Строковое представление
 */
const char *audit_level_to_string(audit_level_t level);

/**
 * @brief Конвертация события в JSON
 * @param event Событие
 * @param buffer Буфер
 * @param size Размер
 * @return Длина JSON строки
 */
size_t audit_event_to_json(const audit_event_t *event, char *buffer, size_t size);

/**
 * @brief Конвертация статистики в JSON
 * @param stats Статистика
 * @param buffer Буфер
 * @param size Размер
 * @return Длина JSON строки
 */
size_t audit_stats_to_json(const audit_stats_t *stats, char *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* AUDIT_LOG_H */
