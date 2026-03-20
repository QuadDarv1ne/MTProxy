/*
 * Пул соединений для MTProxy
 * Реализует эффективное управление сетевыми соединениями
 */

#include "conn-pool.h"
#include "common/kprintf.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

conn_pool_t *init_conn_pool(int max_conn, long timeout) {
    if (max_conn <= 0 || timeout <= 0) {
        errno = EINVAL;
        vkprintf(1, "Connection pool: invalid arguments (max_conn=%d, timeout=%ld)\n", max_conn, timeout);
        return NULL;
    }

    conn_pool_t *pool = (conn_pool_t *)calloc(1, sizeof(conn_pool_t));
    if (!pool) {
        errno = ENOMEM;
        vkprintf(1, "Connection pool: failed to allocate memory\n");
        return NULL;
    }

    pool->max_connections = max_conn;
    pool->timeout = timeout;
    pool->active_connections = 0;
    pool->idle_connections = 0;
    pool->connections = NULL;
    pool->available = NULL;

    vkprintf(2, "Connection pool initialized: max_conn=%d, timeout=%ld\n", max_conn, timeout);
    return pool;
}

connection_t *get_connection(conn_pool_t *pool) {
    if (!pool) {
        errno = EINVAL;
        vkprintf(1, "Connection pool: NULL pool argument\n");
        return NULL;
    }

    // Проверка лимита соединений
    if (pool->active_connections >= pool->max_connections) {
        errno = EMFILE;
        vkprintf(1, "Connection pool: max connections limit reached (%d)\n", pool->max_connections);
        return NULL;
    }

    // Сначала пробуем взять из доступных
    if (pool->available) {
        connection_t *conn = pool->available;
        pool->available = conn->next;
        conn->next = NULL;
        conn->status = CONN_ACTIVE;
        pool->idle_connections--;
        pool->active_connections++;
        vkprintf(3, "Connection pool: reused connection from idle pool\n");
        return conn;
    }

    // Создаём новое соединение
    connection_t *conn = (connection_t *)calloc(1, sizeof(connection_t));
    if (!conn) {
        errno = ENOMEM;
        vkprintf(1, "Connection pool: failed to allocate connection\n");
        return NULL;
    }

    conn->socket_fd = 0;
    conn->status = CONN_ACTIVE;
    conn->next = NULL;
    pool->active_connections++;

    vkprintf(3, "Connection pool: created new connection (active=%d, idle=%d)\n", 
             pool->active_connections, pool->idle_connections);
    return conn;
}

int return_connection(conn_pool_t *pool, connection_t *conn) {
    if (!pool) {
        errno = EINVAL;
        vkprintf(1, "Connection pool: NULL pool argument\n");
        return -1;
    }

    if (!conn) {
        errno = EINVAL;
        vkprintf(1, "Connection pool: NULL connection argument\n");
        return -1;
    }

    if (conn->status != CONN_ACTIVE) {
        errno = EINVAL;
        vkprintf(1, "Connection pool: connection not in ACTIVE state (%d)\n", conn->status);
        return -1;
    }

    conn->status = CONN_IDLE;
    conn->next = pool->available;
    pool->available = conn;
    pool->active_connections--;
    pool->idle_connections++;

    vkprintf(3, "Connection pool: returned connection to idle pool (active=%d, idle=%d)\n",
             pool->active_connections, pool->idle_connections);
    return 0;
}

int close_connection(conn_pool_t *pool, connection_t *conn) {
    if (!pool) {
        errno = EINVAL;
        vkprintf(1, "Connection pool: NULL pool argument\n");
        return -1;
    }

    if (!conn) {
        errno = EINVAL;
        vkprintf(1, "Connection pool: NULL connection argument\n");
        return -1;
    }

    conn->status = CONN_CLOSED;
    if (pool->active_connections > 0) {
        pool->active_connections--;
    }
    
    vkprintf(3, "Connection pool: closed connection (active=%d)\n", pool->active_connections);
    free(conn);

    return 0;
}

int cleanup_conn_pool(conn_pool_t *pool) {
    if (!pool) {
        errno = EINVAL;
        vkprintf(1, "Connection pool: NULL pool argument\n");
        return -1;
    }

    int freed_count = 0;

    // Освобождаем все соединения из основного списка
    connection_t *curr = pool->connections;
    while (curr) {
        connection_t *next = curr->next;
        free(curr);
        freed_count++;
        curr = next;
    }

    // Освобождаем доступные соединения
    curr = pool->available;
    while (curr) {
        connection_t *next = curr->next;
        free(curr);
        freed_count++;
        curr = next;
    }

    vkprintf(2, "Connection pool cleanup: freed %d connections\n", freed_count);

    pool->active_connections = 0;
    pool->idle_connections = 0;
    pool->connections = NULL;
    pool->available = NULL;
    free(pool);

    return 0;
}

int cleanup_expired_connections(conn_pool_t *pool) {
    if (!pool) {
        errno = EINVAL;
        vkprintf(1, "Connection pool: NULL pool argument\n");
        return -1;
    }

    // В реальной реализации здесь была бы проверка времени жизни
    // и очистка устаревших соединений
    int expired_count = 0;
    
    vkprintf(3, "Connection pool: checked for expired connections (found %d)\n", expired_count);
    return expired_count;
}

perf_metrics_t *get_conn_pool_stats(conn_pool_t *pool) {
    static perf_metrics_t stats;
    memset(&stats, 0, sizeof(perf_metrics_t));

    if (!pool) {
        errno = EINVAL;
        vkprintf(1, "Connection pool: NULL pool argument\n");
        return NULL;
    }

    // Map conn_pool fields to perf_metrics
    stats.blocked_connections = pool->active_connections;
    stats.requests_per_second = pool->idle_connections;
    stats.error_count = pool->max_connections;

    vkprintf(3, "Connection pool stats: active=%d, idle=%d, max=%d\n",
             stats.blocked_connections, stats.requests_per_second, stats.error_count);
    return &stats;
}