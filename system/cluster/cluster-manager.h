/*
    MTProxy Cluster Manager
    Система управления кластером MTProxy
    
    Поддерживаемые функции:
    - Регистрация узлов в кластере
    - Синхронизация конфигурации
    - Распределение нагрузки
    - Обнаружение сбоев
    - Автоматическое восстановление
    
    Примеры использования:
    cluster_init("cluster-main", "node1");
    cluster_add_node("node2", "192.168.1.102", 8888);
    cluster_add_node("node3", "192.168.1.103", 8888);
    cluster_start();
    cluster_sync_config();
    cluster_get_status();
    cluster_cleanup();
*/

#ifndef CLUSTER_MANAGER_H
#define CLUSTER_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Версия Cluster Manager
#define CLUSTER_MANAGER_VERSION "1.0.0"

// Максимальные размеры
#define CLUSTER_MAX_NODES 16
#define CLUSTER_MAX_NAME_LEN 64
#define CLUSTER_MAX_HOST_LEN 256
#define CLUSTER_MAX_TOKEN_LEN 128
#define CLUSTER_MAX_MESSAGE_LEN 1024

// Роли узлов
typedef enum {
    CLUSTER_ROLE_LEADER = 0,
    CLUSTER_ROLE_FOLLOWER = 1,
    CLUSTER_ROLE_CANDIDATE = 2,
    CLUSTER_ROLE_STANDALONE = 3
} cluster_role_t;

// Статусы узлов
typedef enum {
    CLUSTER_NODE_OFFLINE = 0,
    CLUSTER_NODE_STARTING = 1,
    CLUSTER_NODE_ONLINE = 2,
    CLUSTER_NODE_DEGRADED = 3,
    CLUSTER_NODE_FAILED = 4
} cluster_node_state_t;

// Типы сообщений кластера
typedef enum {
    CLUSTER_MSG_HEARTBEAT = 0,
    CLUSTER_MSG_ELECTION_REQUEST = 1,
    CLUSTER_MSG_ELECTION_RESPONSE = 2,
    CLUSTER_MSG_CONFIG_SYNC = 3,
    CLUSTER_MSG_CONFIG_ACK = 4,
    CLUSTER_MSG_STATUS_REQUEST = 5,
    CLUSTER_MSG_STATUS_RESPONSE = 6,
    CLUSTER_MSG_LOAD_BALANCE = 7,
    CLUSTER_MSG_FAILOVER = 8
} cluster_message_type_t;

// Конфигурация узла
typedef struct {
    char name[CLUSTER_MAX_NAME_LEN];
    char host[CLUSTER_MAX_HOST_LEN];
    int port;
    int management_port;
    cluster_role_t role;
    int priority;  // Приоритет для выборов лидера
    char token[CLUSTER_MAX_TOKEN_LEN];  // Token для аутентификации
    bool enabled;
} cluster_node_config_t;

// Статус узла
typedef struct {
    char name[CLUSTER_MAX_NAME_LEN];
    cluster_node_state_t status;
    cluster_role_t role;
    uint64_t uptime_ms;
    int active_connections;
    int max_connections;
    double cpu_usage;
    uint64_t memory_usage;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    int64_t last_heartbeat;
    int64_t last_seen;
    char error_message[256];
} cluster_node_info_t;

// Сообщение кластера
typedef struct {
    cluster_message_type_t type;
    char source[CLUSTER_MAX_NAME_LEN];
    char destination[CLUSTER_MAX_NAME_LEN];
    uint64_t message_id;
    int64_t timestamp;
    int payload_size;
    char payload[CLUSTER_MAX_MESSAGE_LEN];
} cluster_message_t;

// Статистика кластера
typedef struct {
    int total_nodes;
    int online_nodes;
    int offline_nodes;
    int degraded_nodes;
    cluster_role_t local_role;
    char leader_name[CLUSTER_MAX_NAME_LEN];
    int64_t cluster_uptime_ms;
    uint64_t total_messages_sent;
    uint64_t total_messages_received;
    uint64_t config_sync_count;
    uint64_t failover_count;
    double avg_load_percent;
} cluster_stats_t;

// Callback функции
typedef void (*cluster_node_change_callback_t)(const char *node_name, 
                                                cluster_node_status_t old_status,
                                                cluster_node_status_t new_status);
typedef void (*cluster_leader_change_callback_t)(const char *old_leader,
                                                  const char *new_leader);
typedef int (*cluster_message_handler_t)(const cluster_message_t *msg);

// ============================================================================
// Инициализация и очистка
// ============================================================================

/**
 * Инициализация Cluster Manager
 * @param cluster_name Имя кластера
 * @param local_node_name Имя локального узла
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_init(const char *cluster_name, const char *local_node_name);

/**
 * Очистка Cluster Manager
 */
void cluster_cleanup(void);

/**
 * Проверка инициализации
 * @return true если инициализирован
 */
bool cluster_is_initialized(void);

// ============================================================================
// Управление узлами
// ============================================================================

/**
 * Добавить узел в кластер
 * @param name Имя узла
 * @param host Хост узла
 * @param port Порт для подключения
 * @param management_port Порт управления
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_add_node(const char *name, const char *host, int port, int management_port);

/**
 * Удалить узел из кластера
 * @param name Имя узла
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_remove_node(const char *name);

/**
 * Получить конфигурацию узла
 * @param name Имя узла
 * @return Конфигурация узла или NULL
 */
const cluster_node_config_t* cluster_get_node_config(const char *name);

/**
 * Получить статус узла
 * @param name Имя узла
 * @return Статус узла или NULL
 */
const cluster_node_status_t* cluster_get_node_status(const char *name);

/**
 * Получить список всех узлов
 * @param buffer Буфер для имён
 * @param buffer_size Размер буфера
 * @return Количество узлов
 */
int cluster_get_all_nodes(char *buffer, size_t buffer_size);

// ============================================================================
// Запуск и остановка
// ============================================================================

/**
 * Запустить Cluster Manager
 * @param listen_port Порт для входящих соединений кластера
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_start(int listen_port);

/**
 * Остановить Cluster Manager
 */
void cluster_stop(void);

/**
 * Проверить, запущен ли кластер
 * @return true если запущен
 */
bool cluster_is_running(void);

// ============================================================================
// Лидерство и выборы
// ============================================================================

/**
 * Получить текущего лидера
 * @return Имя лидера или NULL
 */
const char* cluster_get_leader(void);

/**
 * Получить локальную роль
 * @return Локальная роль
 */
cluster_role_t cluster_get_local_role(void);

/**
 * Инициировать выборы лидера
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_start_election(void);

/**
 * Стать лидером (принудительно)
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_become_leader(void);

/**
 * Сдаться с роли лидера
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_step_down(void);

// ============================================================================
// Синхронизация конфигурации
// ============================================================================

/**
 * Синхронизировать конфигурацию со всеми узлами
 * @param config_data JSON конфигурация
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_sync_config(const char *config_data);

/**
 * Получить последнюю синхронизированную конфигурацию
 * @param buffer Буфер для конфигурации
 * @param buffer_size Размер буфера
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_get_config(char *buffer, size_t buffer_size);

/**
 * Запросить конфигурацию у лидера
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_request_config(void);

// ============================================================================
// Балансировка нагрузки
// ============================================================================

/**
 * Получить наименее загруженный узел
 * @return Имя узла или NULL
 */
const char* cluster_get_least_loaded_node(void);

/**
 * Распределить подключения
 * @param source_node Исходный узел
 * @param connections_count Количество подключений для перераспределения
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_rebalance_connections(const char *source_node, int connections_count);

/**
 * Получить средний процент загрузки кластера
 * @return Процент загрузки (0-100)
 */
double cluster_get_avg_load_percent(void);

// ============================================================================
// Failover
// ============================================================================

/**
 * Обработать отказ узла
 * @param failed_node Имя отказавшего узла
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_handle_node_failure(const char *failed_node);

/**
 * Восстановить узел после отказа
 * @param node_name Имя узла
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_recovery_node(const char *node_name);

/**
 * Включить автоматический failover
 * @param enabled Статус
 */
void cluster_set_auto_failover(bool enabled);

// ============================================================================
// Отправка сообщений
// ============================================================================

/**
 * Отправить сообщение узлу
 * @param destination Имя узла получателя
 * @param type Тип сообщения
 * @param payload Данные сообщения
 * @param payload_size Размер данных
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_send_message(const char *destination, cluster_message_type_t type,
                         const char *payload, int payload_size);

/**
 * Отправить сообщение всем узлам
 * @param type Тип сообщения
 * @param payload Данные сообщения
 * @param payload_size Размер данных
 * @return Количество отправленных сообщений
 */
int cluster_broadcast_message(cluster_message_type_t type,
                              const char *payload, int payload_size);

/**
 * Установить обработчик сообщений
 * @param type Тип сообщения
 * @param handler Функция обработчика
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_set_message_handler(cluster_message_type_t type, 
                                 cluster_message_handler_t handler);

// ============================================================================
// Callback функции
// ============================================================================

/**
 * Установить callback изменения статуса узла
 * @param callback Функция обратного вызова
 */
void cluster_set_node_change_callback(cluster_node_change_callback_t callback);

/**
 * Установить callback смены лидера
 * @param callback Функция обратного вызова
 */
void cluster_set_leader_change_callback(cluster_leader_change_callback_t callback);

// ============================================================================
// Статистика и мониторинг
// ============================================================================

/**
 * Получить статистику кластера
 * @param stats Структура статистики
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_get_stats(cluster_stats_t *stats);

/**
 * Получить строку статистики
 * @param buffer Буфер для вывода
 * @param buffer_size Размер буфера
 * @return 0 при успехе, -1 при ошибке
 */
int cluster_get_stats_string(char *buffer, size_t buffer_size);

/**
 * Сбросить статистику
 */
void cluster_reset_stats(void);

/**
 * Получить uptime кластера (мс)
 * @return Uptime в миллисекундах
 */
int64_t cluster_get_uptime(void);

/**
 * Получить количество онлайн узлов
 * @return Количество онлайн узлов
 */
int cluster_get_online_count(void);

/**
 * Проверить кворум
 * @return true если есть кворум
 */
bool cluster_has_quorum(void);

// ============================================================================
// Утилиты
// ============================================================================

/**
 * Конвертировать роль в строку
 * @param role Роль
 * @return Строка роли
 */
const char* cluster_role_to_string(cluster_role_t role);

/**
 * Конвертировать статус узла в строку
 * @param status Статус
 * @return Строка статуса
 */
const char* cluster_node_status_to_string(cluster_node_state_t status);

/**
 * Конвертировать тип сообщения в строку
 * @param type Тип сообщения
 * @return Строка типа
 */
const char* cluster_message_type_to_string(cluster_message_type_t type);

#ifdef __cplusplus
}
#endif

#endif // CLUSTER_MANAGER_H
