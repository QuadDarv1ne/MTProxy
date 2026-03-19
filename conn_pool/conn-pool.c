/*
 * Пул соединений для MTProxy
 * Реализует эффективное управление сетевыми соединениями
 */

#include "conn-pool.h"
#include <stdlib.h>

conn_pool_t *init_conn_pool(int max_conn, long timeout) {
    if (max_conn <= 0 || timeout <= 0) {
        return 0;
    }
    
    conn_pool_t *pool = (conn_pool_t *)calloc(1, sizeof(conn_pool_t));
    if (!pool) {
        return 0;
    }
    
    pool->max_connections = max_conn;
    pool->timeout = timeout;
    pool->active_connections = 0;
    pool->idle_connections = 0;
    pool->connections = 0;
    pool->available = 0;
    
    return pool;
}

connection_t *get_connection(conn_pool_t *pool) {
    if (!pool) {
        return 0;
    }
    
    // Сначала пробуем взять из доступных
    if (pool->available) {
        connection_t *conn = pool->available;
        pool->available = conn->next;
        conn->next = 0;
        conn->status = CONN_ACTIVE;
        pool->idle_connections--;
        pool->active_connections++;
        return conn;
    }
    
    // Создаём новое соединение
    connection_t *conn = (connection_t *)calloc(1, sizeof(connection_t));
    if (!conn) {
        return 0;
    }
    
    conn->socket_fd = 0;
    conn->status = CONN_ACTIVE;
    conn->next = 0;
    pool->active_connections++;
    
    return conn;
}

int return_connection(conn_pool_t *pool, connection_t *conn) {
    if (!pool || !conn) {
        return -1;
    }
    
    conn->status = CONN_IDLE;
    conn->next = pool->available;
    pool->available = conn;
    pool->active_connections--;
    pool->idle_connections++;
    
    return 0;
}

int close_connection(conn_pool_t *pool, connection_t *conn) {
    if (!pool || !conn) {
        return -1;
    }
    
    conn->status = CONN_CLOSED;
    if (pool->active_connections > 0) {
        pool->active_connections--;
    }
    free(conn);
    
    return 0;
}

int cleanup_conn_pool(conn_pool_t *pool) {
    if (!pool) {
        return -1;
    }
    
    // Освобождаем все соединения
    connection_t *curr = pool->connections;
    while (curr) {
        connection_t *next = curr->next;
        free(curr);
        curr = next;
    }
    
    // Освобождаем доступные
    curr = pool->available;
    while (curr) {
        connection_t *next = curr->next;
        free(curr);
        curr = next;
    }
    
    pool->active_connections = 0;
    pool->idle_connections = 0;
    pool->connections = 0;
    pool->available = 0;
    free(pool);
    
    return 0;
}

int cleanup_expired_connections(conn_pool_t *pool) {
    if (!pool) {
        return 0;
    }
    
    // В реальной реализации здесь была бы проверка времени жизни
    return 0;
}

perf_metrics_t *get_conn_pool_stats(conn_pool_t *pool) {
    static perf_metrics_t stats;

    if (!pool) {
        return 0;
    }

    // Map conn_pool fields to perf_metrics
    stats.blocked_connections = pool->active_connections;
    stats.requests_per_second = pool->idle_connections;
    stats.error_count = pool->max_connections;

    return &stats;
}