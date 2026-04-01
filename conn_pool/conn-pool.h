#ifndef CONN_POOL_H
#define CONN_POOL_H

#include <stdint.h>
#include "../perf_monitor/perf-monitor.h"

/*
 * Статус соединения
 */
typedef enum {
    CONN_IDLE,      // Не используется
    CONN_ACTIVE,    // Активное использование
    CONN_CLOSED,    // Закрыто
    CONN_ERROR      // Ошибка
} conn_status_t;

/*
 * Структура для представления соединения
 */
typedef struct connection {
    int socket_fd;              // Дескриптор сокета
    conn_status_t status;       // Статус соединения
    uint32_t client_ip;         // IP-адрес клиента
    uint16_t client_port;       // Порт клиента
    long created_at;            // Время создания
    long last_used;             // Время последнего использования
    void *user_data;           // Дополнительные данные
    struct connection *next;   // Следующее соединение в списке
} connection_t;

/*
 * Структура пула соединений
 */
typedef struct conn_pool {
    connection_t *connections;     // Массив соединений
    connection_t *available;       // Список доступных соединений
    int max_connections;          // Максимальное количество соединений
    int active_connections;       // Количество активных соединений
    int idle_connections;         // Количество неиспользуемых соединений
    long timeout;                 // Время жизни неиспользуемого соединения (в секундах)
    perf_metrics_t *metrics;      // Метрики производительности
} conn_pool_t;

/*
 * Инициализирует пул соединений
 * @param max_conn: максимальное количество соединений в пуле
 * @param timeout: время жизни неиспользуемого соединения (в секундах)
 * @return: указатель на пул соединений или NULL в случае ошибки
 */
conn_pool_t *init_conn_pool(int max_conn, long timeout);

/*
 * Получает соединение из пула
 * @param pool: указатель на пул соединений
 * @return: указатель на соединение или NULL в случае ошибки
 */
connection_t *get_connection(conn_pool_t *pool);

/*
 * Возвращает соединение обратно в пул
 * @param pool: указатель на пул соединений
 * @param conn: указатель на соединение для возврата
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int return_connection(conn_pool_t *pool, connection_t *conn);

/*
 * Закрывает и удаляет соединение
 * @param pool: указатель на пул соединений
 * @param conn: указатель на соединение для закрытия
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int close_connection(conn_pool_t *pool, connection_t *conn);

/*
 * Освобождает все соединения в пуле
 * @param pool: указатель на пул соединений
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int cleanup_conn_pool(conn_pool_t *pool);

/*
 * Проверяет и очищает просроченные соединения
 * @param pool: указатель на пул соединений
 * @return: количество очищенных соединений
 */
int cleanup_expired_connections(conn_pool_t *pool);

/*
 * Получает статистику пула соединений
 * @param pool: указатель на пул соединений
 * @return: структура с информацией о состоянии пула
 */
perf_metrics_t *get_conn_pool_stats(conn_pool_t *pool);

#endif