/*
    MTProxy Health Check System
    Система проверки здоровья узлов кластера
    
    Поддерживаемые проверки:
    - HTTP/HTTPS endpoint
    - TCP port
    - Process status
    - Memory usage
    - Disk usage
    - Custom script
    
    Примеры использования:
    health_check_init();
    health_check_add_node("node1", "http://localhost:8080/health", 5000);
    health_check_add_node("node2", "tcp://localhost:8888", 3000);
    health_check_start();
    health_check_status_t status = health_check_get_status("node1");
    health_check_cleanup();
*/

#ifndef HEALTH_CHECK_H
#define HEALTH_CHECK_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Версия Health Check
#define HEALTH_CHECK_VERSION "1.0.0"

// Максимальные размеры
#define HEALTH_MAX_NODES 32
#define HEALTH_MAX_NAME_LEN 128
#define HEALTH_MAX_URL_LEN 512
#define HEALTH_MAX_METADATA_LEN 1024

// Типы проверок
typedef enum {
    HEALTH_CHECK_HTTP = 0,
    HEALTH_CHECK_HTTPS = 1,
    HEALTH_CHECK_TCP = 2,
    HEALTH_CHECK_PROCESS = 3,
    HEALTH_CHECK_MEMORY = 4,
    HEALTH_CHECK_DISK = 5,
    HEALTH_CHECK_CUSTOM = 6
} health_check_type_t;

// Статус здоровья
typedef enum {
    HEALTH_STATUS_UNKNOWN = 0,
    HEALTH_STATUS_HEALTHY = 1,
    HEALTH_STATUS_UNHEALTHY = 2,
    HEALTH_STATUS_DEGRADED = 3,
    HEALTH_STATUS_TIMEOUT = 4
} health_status_t;

// Конфигурация проверки
typedef struct {
    char name[HEALTH_MAX_NAME_LEN];
    char url[HEALTH_MAX_URL_LEN];
    health_check_type_t type;
    int timeout_ms;
    int interval_ms;
    int unhealthy_threshold;  // Количество неудачных проверок до пометки unhealthy
    int healthy_threshold;    // Количество удачных проверок до пометки healthy
    char metadata[HEALTH_MAX_METADATA_LEN];
    bool enabled;
} health_check_config_t;

// Результат проверки
typedef struct {
    uint64_t check_id;
    char name[HEALTH_MAX_NAME_LEN];
    health_status_t status;
    int response_time_ms;
    int consecutive_failures;
    int consecutive_successes;
    int64_t last_check_time;
    int64_t last_success_time;
    int64_t last_failure_time;
    char error_message[256];
    char response_body[512];
    int http_status_code;
} health_check_result_t;

// Статистика Health Check
typedef struct {
    int total_nodes;
    int healthy_nodes;
    int unhealthy_nodes;
    int degraded_nodes;
    uint64_t total_checks;
    uint64_t successful_checks;
    uint64_t failed_checks;
    int64_t last_check_time;
    double avg_response_time_ms;
} health_check_stats_t;

// Callback для кастомных проверок
typedef int (*health_check_callback_t)(const char *node_name, const char *metadata, 
                                        char *error_message, size_t error_size);

// ============================================================================
// Инициализация и очистка
// ============================================================================

/**
 * Инициализация системы Health Check
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_init(void);

/**
 * Очистка системы Health Check
 */
void health_check_cleanup(void);

/**
 * Проверка инициализации
 * @return true если инициализирована
 */
bool health_check_is_initialized(void);

// ============================================================================
// Управление узлами
// ============================================================================

/**
 * Добавить узел для проверки (HTTP/HTTPS)
 * @param name Имя узла
 * @param url URL для проверки (http://host:port/health)
 * @param timeout_ms Таймаут проверки (мс)
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_add_http_node(const char *name, const char *url, int timeout_ms);

/**
 * Добавить узел для проверки (TCP)
 * @param name Имя узла
 * @param host Хост
 * @param port Порт
 * @param timeout_ms Таймаут проверки (мс)
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_add_tcp_node(const char *name, const char *host, int port, int timeout_ms);

/**
 * Добавить узел для проверки (Process)
 * @param name Имя узла
 * @param pid PID процесса
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_add_process_node(const char *name, int pid);

/**
 * Добавить узел для проверки (Memory)
 * @param name Имя узла
 * @param max_memory_bytes Максимальный порог памяти (байт)
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_add_memory_node(const char *name, uint64_t max_memory_bytes);

/**
 * Добавить узел для проверки (Disk)
 * @param name Имя узла
 * @param path Путь к диску/разделу
 * @param max_usage_percent Максимальный порог использования (%)
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_add_disk_node(const char *name, const char *path, int max_usage_percent);

/**
 * Добавить узел для проверки (Custom)
 * @param name Имя узла
 * @param metadata Метаданные для callback
 * @param callback Функция проверки
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_add_custom_node(const char *name, const char *metadata, 
                                  health_check_callback_t callback);

/**
 * Удалить узел
 * @param name Имя узла
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_remove_node(const char *name);

/**
 * Включить/выключить узел
 * @param name Имя узла
 * @param enabled Статус
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_set_node_enabled(const char *name, bool enabled);

/**
 * Обновить конфигурацию узла
 * @param name Имя узла
 * @param config Новая конфигурация
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_update_node_config(const char *name, const health_check_config_t *config);

// ============================================================================
// Запуск и остановка проверок
// ============================================================================

/**
 * Запустить фоновые проверки
 * @param interval_ms Интервал между проверками (мс)
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_start(int interval_ms);

/**
 * Остановить фоновые проверки
 */
void health_check_stop(void);

/**
 * Выполнить разовую проверку всех узлов
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_run_once(void);

/**
 * Выполнить разовую проверку конкретного узла
 * @param name Имя узла
 * @return Результат проверки или NULL при ошибке
 */
const health_check_result_t* health_check_check_node(const char *name);

// ============================================================================
// Получение статуса
// ============================================================================

/**
 * Получить статус узла
 * @param name Имя узла
 * @return Статус проверки
 */
health_status_t health_check_get_node_status(const char *name);

/**
 * Получить результат последней проверки узла
 * @param name Имя узла
 * @return Результат проверки или NULL
 */
const health_check_result_t* health_check_get_node_result(const char *name);

/**
 * Получить количество здоровых узлов
 * @return Количество здоровых узлов
 */
int health_check_get_healthy_count(void);

/**
 * Получить общее количество узлов
 * @return Общее количество узлов
 */
int health_check_get_total_count(void);

/**
 * Проверить, все ли узлы здоровы
 * @return true если все узлы здоровы
 */
bool health_check_all_healthy(void);

/**
 * Получить список нездоровых узлов
 * @param buffer Буфер для имён узлов
 * @param buffer_size Размер буфера
 * @return Количество нездоровых узлов
 */
int health_check_get_unhealthy_nodes(char *buffer, size_t buffer_size);

// ============================================================================
// Статистика
// ============================================================================

/**
 * Получить общую статистику
 * @param stats Структура статистики
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_get_stats(health_check_stats_t *stats);

/**
 * Получить строку статистики
 * @param buffer Буфер для вывода
 * @param buffer_size Размер буфера
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_get_stats_string(char *buffer, size_t buffer_size);

/**
 * Сбросить статистику
 */
void health_check_reset_stats(void);

/**
 * Получить uptime системы Health Check (мс)
 * @return Uptime в миллисекундах
 */
int64_t health_check_get_uptime(void);

// ============================================================================
// Alert Manager интеграция
// ============================================================================

/**
 * Включить интеграцию с Alert Manager
 * @param alert_on_unhealthy Отправлять алерт при unhealthy
 * @param alert_on_recovery Отправлять алерт при восстановлении
 * @return 0 при успехе, -1 при ошибке
 */
int health_check_enable_alerts(bool alert_on_unhealthy, bool alert_on_recovery);

/**
 * Отключить интеграцию с Alert Manager
 */
void health_check_disable_alerts(void);

// ============================================================================
// Утилиты
// ============================================================================

/**
 * Конвертировать статус в строку
 * @param status Статус
 * @return Строка статуса
 */
const char* health_status_to_string(health_status_t status);

/**
 * Конвертировать тип проверки в строку
 * @param type Тип проверки
 * @return Строка типа
 */
const char* health_check_type_to_string(health_check_type_t type);

/**
 * Получить процент здоровья кластера
 * @return Процент здоровых узлов (0-100)
 */
int health_check_get_cluster_health_percent(void);

#ifdef __cplusplus
}
#endif

#endif // HEALTH_CHECK_H
