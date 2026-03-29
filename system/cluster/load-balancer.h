/*
    MTProxy Load Balancer
    Система балансировки нагрузки между узлами кластера
    
    Алгоритмы балансировки:
    - Round Robin
    - Least Connections
    - Least Load (CPU/Memory)
    - Weighted
    - IP Hash
    
    Примеры использования:
    load_balancer_init();
    load_balancer_set_algorithm(LOAD_BALANCER_LEAST_CONNECTIONS);
    load_balancer_add_backend("node1", "192.168.1.101", 8888, 1.0);
    load_balancer_add_backend("node2", "192.168.1.102", 8888, 1.0);
    load_balancer_start();
    const char* node = load_balancer_select_backend(client_ip);
    load_balancer_cleanup();
*/

#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Версия Load Balancer
#define LOAD_BALANCER_VERSION "1.0.0"

// Максимальные размеры
#define LOAD_BALANCER_MAX_BACKENDS 32
#define LOAD_BALANCER_MAX_NAME_LEN 64
#define LOAD_BALANCER_MAX_HOST_LEN 256
#define LOAD_BALANCER_MAX_IP_LEN 64

// Алгоритмы балансировки
typedef enum {
    LOAD_BALANCER_ROUND_ROBIN = 0,
    LOAD_BALANCER_LEAST_CONNECTIONS = 1,
    LOAD_BALANCER_LEAST_LOAD = 2,
    LOAD_BALANCER_WEIGHTED = 3,
    LOAD_BALANCER_IP_HASH = 4,
    LOAD_BALANCER_RANDOM = 5
} load_balancer_algorithm_t;

// Статус backend узла
typedef enum {
    LOAD_BALANCER_BACKEND_OFFLINE = 0,
    LOAD_BALANCER_BACKEND_ONLINE = 1,
    LOAD_BALANCER_BACKEND_DEGRADED = 2,
    LOAD_BALANCER_BACKEND_FAILED = 3
} load_balancer_backend_status_t;

// Конфигурация backend узла
typedef struct {
    char name[LOAD_BALANCER_MAX_NAME_LEN];
    char host[LOAD_BALANCER_MAX_HOST_LEN];
    int port;
    double weight;  // Вес для weighted алгоритма (0.0 - 10.0)
    int max_connections;  // Максимальное количество подключений
    int max_load_percent;  // Максимальный процент нагрузки
    bool enabled;
    char metadata[256];
} load_balancer_backend_config_t;

// Статистика backend узла
typedef struct {
    char name[LOAD_BALANCER_MAX_NAME_LEN];
    load_balancer_backend_status_t status;
    int active_connections;
    int total_connections;
    int64_t bytes_sent;
    int64_t bytes_received;
    double cpu_usage;
    uint64_t memory_usage;
    double load_percent;
    int64_t last_health_check;
    int consecutive_failures;
    int requests_handled;
} load_balancer_backend_stats_t;

// Статистика Load Balancer
typedef struct {
    int total_backends;
    int active_backends;
    int offline_backends;
    load_balancer_algorithm_t algorithm;
    uint64_t total_requests;
    uint64_t successful_requests;
    uint64_t failed_requests;
    uint64_t rebalances;
    int64_t last_rebalance;
    double avg_response_time_ms;
    double avg_load_percent;
} load_balancer_stats_t;

// Callback функции
typedef void (*load_balancer_backend_change_callback_t)(const char *backend_name,
                                                         load_balancer_backend_status_t old_status,
                                                         load_balancer_backend_status_t new_status);
typedef int (*load_balancer_health_check_callback_t)(const char *backend_name, 
                                                      const char *host, int port);

// ============================================================================
// Инициализация и очистка
// ============================================================================

/**
 * Инициализация Load Balancer
 * @param algorithm Алгоритм балансировки
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_init(load_balancer_algorithm_t algorithm);

/**
 * Очистка Load Balancer
 */
void load_balancer_cleanup(void);

/**
 * Проверка инициализации
 * @return true если инициализирован
 */
bool load_balancer_is_initialized(void);

// ============================================================================
// Управление backend узлами
// ============================================================================

/**
 * Добавить backend узел
 * @param name Имя узла
 * @param host Хост узла
 * @param port Порт узла
 * @param weight Вес узла (для weighted алгоритма)
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_add_backend(const char *name, const char *host, int port, double weight);

/**
 * Удалить backend узел
 * @param name Имя узла
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_remove_backend(const char *name);

/**
 * Включить/выключить backend узел
 * @param name Имя узла
 * @param enabled Статус
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_set_backend_enabled(const char *name, bool enabled);

/**
 * Обновить вес backend узла
 * @param name Имя узла
 * @param weight Новый вес
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_set_backend_weight(const char *name, double weight);

/**
 * Получить конфигурацию backend узла
 * @param name Имя узла
 * @return Конфигурация или NULL
 */
const load_balancer_backend_config_t* load_balancer_get_backend_config(const char *name);

/**
 * Получить статистику backend узла
 * @param name Имя узла
 * @return Статистика или NULL
 */
const load_balancer_backend_stats_t* load_balancer_get_backend_stats(const char *name);

/**
 * Получить список всех backend узлов
 * @param buffer Буфер для имён
 * @param buffer_size Размер буфера
 * @return Количество узлов
 */
int load_balancer_get_all_backends(char *buffer, size_t buffer_size);

// ============================================================================
// Выбор алгоритма
// ============================================================================

/**
 * Установить алгоритм балансировки
 * @param algorithm Алгоритм
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_set_algorithm(load_balancer_algorithm_t algorithm);

/**
 * Получить текущий алгоритм
 * @return Текущий алгоритм
 */
load_balancer_algorithm_t load_balancer_get_algorithm(void);

// ============================================================================
// Выбор backend узла
// ============================================================================

/**
 * Выбрать backend узел для подключения
 * @param client_ip IP клиента (для IP hash)
 * @return Имя выбранного узла или NULL
 */
const char* load_balancer_select_backend(const char *client_ip);

/**
 * Выбрать backend узел с наименьшей нагрузкой
 * @return Имя узла или NULL
 */
const char* load_balancer_get_least_loaded_backend(void);

/**
 * Выбрать backend узел с наименьшим количеством подключений
 * @return Имя узла или NULL
 */
const char* load_balancer_get_least_connections_backend(void);

/**
 * Уведомить о новом подключении
 * @param backend_name Имя узла
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_notify_new_connection(const char *backend_name);

/**
 * Уведомить о закрытии подключения
 * @param backend_name Имя узла
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_notify_connection_closed(const char *backend_name);

// ============================================================================
// Перераспределение нагрузки
// ============================================================================

/**
 * Выполнить перераспределение нагрузки
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_rebalance(void);

/**
 * Перераспределить подключения с перегруженного узла
 * @param source_node Имя перегруженного узла
 * @param connections_count Количество подключений для перераспределения
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_redistribute_from_node(const char *source_node, int connections_count);

/**
 * Установить порог нагрузки для перераспределения
 * @param percent Процент нагрузки (0-100)
 */
void load_balancer_set_rebalance_threshold(int percent);

/**
 * Включить/выключить автоматическое перераспределение
 * @param enabled Статус
 */
void load_balancer_set_auto_rebalance(bool enabled);

// ============================================================================
// Health Check
// ============================================================================

/**
 * Установить callback для проверки здоровья узлов
 * @param callback Функция проверки
 */
void load_balancer_set_health_check_callback(load_balancer_health_check_callback_t callback);

/**
 * Выполнить проверку здоровья всех узлов
 * @return Количество здоровых узлов
 */
int load_balancer_run_health_check(void);

/**
 * Запустить фоновые проверки здоровья
 * @param interval_ms Интервал между проверками (мс)
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_start_health_checks(int interval_ms);

/**
 * Остановить фоновые проверки
 */
void load_balancer_stop_health_checks(void);

// ============================================================================
// Статистика и мониторинг
// ============================================================================

/**
 * Получить статистику Load Balancer
 * @param stats Структура статистики
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_get_stats(load_balancer_stats_t *stats);

/**
 * Получить строку статистики
 * @param buffer Буфер для вывода
 * @param buffer_size Размер буфера
 * @return 0 при успехе, -1 при ошибке
 */
int load_balancer_get_stats_string(char *buffer, size_t buffer_size);

/**
 * Сбросить статистику
 */
void load_balancer_reset_stats(void);

/**
 * Получить средний процент нагрузки
 * @return Процент нагрузки (0-100)
 */
double load_balancer_get_avg_load_percent(void);

/**
 * Получить общее количество активных подключений
 * @return Количество подключений
 */
int load_balancer_get_total_active_connections(void);

/**
 * Получить количество успешных запросов в секунду
 * @return Запросов в секунду
 */
double load_balancer_get_requests_per_second(void);

// ============================================================================
// Callback функции
// ============================================================================

/**
 * Установить callback изменения статуса backend
 * @param callback Функция обратного вызова
 */
void load_balancer_set_backend_change_callback(load_balancer_backend_change_callback_t callback);

// ============================================================================
// Утилиты
// ============================================================================

/**
 * Конвертировать алгоритм в строку
 * @param algo Алгоритм
 * @return Строка алгоритма
 */
const char* load_balancer_algorithm_to_string(load_balancer_algorithm_t algo);

/**
 * Конвертировать статус backend в строку
 * @param status Статус
 * @return Строка статуса
 */
const char* load_balancer_backend_status_to_string(load_balancer_backend_status_t status);

/**
 * Получить IP hash для клиента
 * @param client_ip IP клиента
 * @param backend_count Количество backend узлов
 * @return Индекс backend узла
 */
int load_balancer_ip_hash(const char *client_ip, int backend_count);

#ifdef __cplusplus
}
#endif

#endif // LOAD_BALANCER_H
