/**
 * @file mtproxy.h
 * @brief Публичный API для интеграции с MTProxy
 * 
 * Этот файл содержит функции для управления MTProxy из внешних приложений
 * через FFI (Foreign Function Interface)
 */

#ifndef MTPROXY_PUBLIC_API_H
#define MTPROXY_PUBLIC_API_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Экспорт символов для Windows
#ifdef _WIN32
    #ifdef MTPROXY_EXPORTS
        #define MTPROXY_API __declspec(dllexport)
    #else
        #define MTPROXY_API __declspec(dllimport)
    #endif
#else
    #define MTPROXY_API __attribute__((visibility("default")))
#endif

/**
 * @brief Структура статистики прокси
 */
typedef struct {
    uint32_t active_connections;    ///< Активные подключения
    uint32_t total_connections;     ///< Всего подключений
    uint64_t bytes_sent;            ///< Отправлено байт
    uint64_t bytes_received;        ///< Получено байт
    uint64_t start_time;            ///< Время запуска (unix timestamp)
    double cpu_usage;               ///< Использование CPU (%)
    uint32_t memory_usage;          ///< Использование памяти (KB)
} mtproxy_stats_t;

/**
 * @brief Структура конфигурации
 */
typedef struct {
    uint16_t port;                  ///< Порт для прослушивания
    const char* secret;             ///< Секретный ключ (hex)
    uint32_t max_connections;       ///< Макс. количество подключений
    bool enable_ipv6;               ///< Включить IPv6
    bool enable_stats;              ///< Включить статистику
} mtproxy_config_t;

// ============================================================================
// Основные функции управления
// ============================================================================

/**
 * @brief Инициализация MTProxy
 * @return 0 при успехе, код ошибки при неудаче
 */
MTPROXY_API int mtproxy_init(void);

/**
 * @brief Запуск прокси сервера
 * @return 0 при успехе, код ошибки при неудаче
 */
MTPROXY_API int mtproxy_start(void);

/**
 * @brief Остановка прокси сервера
 */
MTPROXY_API void mtproxy_stop(void);

/**
 * @brief Проверка, запущен ли прокси
 * @return true если запущен, false иначе
 */
MTPROXY_API bool mtproxy_is_running(void);

// ============================================================================
// Функции конфигурации
// ============================================================================

/**
 * @brief Установка порта
 * @param port Порт (1-65535)
 * @return 0 при успехе, код ошибки при неудаче
 */
MTPROXY_API int mtproxy_set_port(uint16_t port);

/**
 * @brief Добавление секретного ключа
 * @param secret Hex-строка (64 символа для 32 байт)
 * @return 0 при успехе, код ошибки при неудаче
 */
MTPROXY_API int mtproxy_add_secret(const char* secret);

/**
 * @brief Удаление секретного ключа
 * @param secret Hex-строка для удаления
 * @return 0 при успехе, код ошибки при неудаче
 */
MTPROXY_API int mtproxy_remove_secret(const char* secret);

/**
 * @brief Очистка всех секретных ключей
 */
MTPROXY_API void mtproxy_clear_secrets(void);

/**
 * @brief Установка максимального количества подключений
 * @param max_connections Макс. количество
 * @return 0 при успехе, код ошибки при неудаче
 */
MTPROXY_API int mtproxy_set_max_connections(uint32_t max_connections);

/**
 * @brief Включение/выключение IPv6
 * @param enable true для включения
 * @return 0 при успехе, код ошибки при неудаче
 */
MTPROXY_API int mtproxy_set_ipv6(bool enable);

/**
 * @brief Применение полной конфигурации
 * @param config Структура конфигурации
 * @return 0 при успехе, код ошибки при неудаче
 */
MTPROXY_API int mtproxy_apply_config(const mtproxy_config_t* config);

// ============================================================================
// Функции статистики
// ============================================================================

/**
 * @brief Получение текущей статистики
 * @return Указатель на структуру статистики
 */
MTPROXY_API mtproxy_stats_t* mtproxy_get_stats(void);

/**
 * @brief Получение количества активных подключений
 * @return Количество подключений
 */
MTPROXY_API uint32_t mtproxy_get_active_connections(void);

/**
 * @brief Получение общего количества подключений
 * @return Количество подключений
 */
MTPROXY_API uint32_t mtproxy_get_total_connections(void);

/**
 * @brief Получение отправленного трафика
 * @return Количество байт
 */
MTPROXY_API uint64_t mtproxy_get_bytes_sent(void);

/**
 * @brief Получение полученного трафика
 * @return Количество байт
 */
MTPROXY_API uint64_t mtproxy_get_bytes_received(void);

/**
 * @brief Получение времени работы
 * @return Время в секундах
 */
MTPROXY_API uint64_t mtproxy_get_uptime(void);

// ============================================================================
// Утилиты
// ============================================================================

/**
 * @brief Генерация случайного секретного ключа
 * @param buffer Буфер для ключа (минимум 65 байт)
 * @param buffer_size Размер буфера
 * @return 0 при успехе, код ошибки при неудаче
 */
MTPROXY_API int mtproxy_generate_secret(char* buffer, size_t buffer_size);

/**
 * @brief Валидация секретного ключа
 * @param secret Hex-строка для проверки
 * @return true если ключ валиден
 */
MTPROXY_API bool mtproxy_validate_secret(const char* secret);

/**
 * @brief Получение версии библиотеки
 * @return Строка версии (например, "1.0.0")
 */
MTPROXY_API const char* mtproxy_get_version(void);

/**
 * @brief Получение последнего сообщения об ошибке
 * @return Строка с описанием ошибки
 */
MTPROXY_API const char* mtproxy_get_last_error(void);

#ifdef __cplusplus
}
#endif

#endif // MTPROXY_PUBLIC_API_H
