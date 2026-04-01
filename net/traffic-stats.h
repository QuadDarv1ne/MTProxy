/*
 * traffic-stats.h - Модуль статистики трафика
 * Учёт трафика по подключениям, IP-адресам и устройствам
 */

#ifndef __TRAFFIC_STATS_H__
#define __TRAFFIC_STATS_H__

#include <stdint.h>
#include <pthread.h>

#define MAX_TRAFFIC_ENTRIES 1024
#define TRAFFIC_HISTORY_SIZE 256

/* Структура статистики для одного подключения */
typedef struct {
    uint64_t connection_id;
    char client_ip[64];
    char device_id[64];
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t packets_sent;
    uint64_t packets_received;
    time_t connection_start;
    time_t last_activity;
    int is_active;
} traffic_entry_t;

/* История трафика для одного подключения */
typedef struct {
    uint64_t timestamp;
    uint64_t bytes_sent;
    uint64_t bytes_received;
} traffic_sample_t;

/* Агрегированная статистика по устройству */
typedef struct {
    char device_id[64];
    char client_ip[64];
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    uint64_t total_packets_sent;
    uint64_t total_packets_received;
    uint64_t connection_count;
    time_t first_seen;
    time_t last_seen;
} device_traffic_t;

/* Основная структура статистики */
typedef struct {
    traffic_entry_t entries[MAX_TRAFFIC_ENTRIES];
    int entry_count;
    pthread_mutex_t lock;
    
    /* Агрегированная статистика */
    device_traffic_t devices[MAX_TRAFFIC_ENTRIES];
    int device_count;
    
    /* Глобальная статистика */
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    uint64_t total_connections;
    uint64_t active_connections;
    time_t start_time;
    
    /* История для графиков */
    traffic_sample_t history[TRAFFIC_HISTORY_SIZE];
    int history_index;
} traffic_stats_t;

/* Инициализация модуля */
int traffic_stats_init(traffic_stats_t *stats);

/* Освобождение ресурсов */
void traffic_stats_destroy(traffic_stats_t *stats);

/* Создание новой записи о подключении */
int traffic_stats_create_connection(traffic_stats_t *stats, 
                                     uint64_t connection_id,
                                     const char *client_ip,
                                     const char *device_id);

/* Обновление статистики отправки */
int traffic_stats_update_sent(traffic_stats_t *stats,
                               uint64_t connection_id,
                               uint64_t bytes,
                               uint64_t packets);

/* Обновление статистики получения */
int traffic_stats_update_received(traffic_stats_t *stats,
                                   uint64_t connection_id,
                                   uint64_t bytes,
                                   uint64_t packets);

/* Закрытие подключения */
int traffic_stats_close_connection(traffic_stats_t *stats,
                                    uint64_t connection_id);

/* Получение статистики по подключению */
int traffic_stats_get_connection(traffic_stats_t *stats,
                                  uint64_t connection_id,
                                  traffic_entry_t *entry);

/* Получение статистики по устройству */
int traffic_stats_get_device(traffic_stats_t *stats,
                              const char *device_id,
                              device_traffic_t *device);

/* Получение глобальной статистики */
void traffic_stats_get_global(traffic_stats_t *stats,
                               uint64_t *total_sent,
                               uint64_t *total_received,
                               uint64_t *active_conn,
                               uint64_t *total_conn);

/* Получение истории трафика */
int traffic_stats_get_history(traffic_stats_t *stats,
                               traffic_sample_t *samples,
                               int max_samples);

/* Экспорт статистики в JSON */
int traffic_stats_export_json(traffic_stats_t *stats,
                               char *buffer,
                               size_t buffer_size);

/* Сброс статистики */
void traffic_stats_reset(traffic_stats_t *stats);

/* Удаление старых неактивных записей */
int traffic_stats_cleanup_inactive(traffic_stats_t *stats,
                                    time_t inactive_threshold);

#endif /* __TRAFFIC_STATS_H__ */
