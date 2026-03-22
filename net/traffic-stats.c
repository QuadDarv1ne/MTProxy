/*
 * traffic-stats.c - Модуль статистики трафика
 * Учёт трафика по подключениям, IP-адресам и устройствам
 */

#include "traffic-stats.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* Инициализация модуля */
int traffic_stats_init(traffic_stats_t *stats) {
    if (!stats) {
        return -1;
    }
    
    memset(stats, 0, sizeof(traffic_stats_t));
    
    if (pthread_mutex_init(&stats->lock, NULL) != 0) {
        return -1;
    }
    
    stats->start_time = time(NULL);
    stats->entry_count = 0;
    stats->device_count = 0;
    stats->history_index = 0;
    
    return 0;
}

/* Освобождение ресурсов */
void traffic_stats_destroy(traffic_stats_t *stats) {
    if (!stats) {
        return;
    }
    
    pthread_mutex_destroy(&stats->lock);
}

/* Поиск подключения по ID */
static int find_connection(traffic_stats_t *stats, uint64_t connection_id) {
    for (int i = 0; i < stats->entry_count; i++) {
        if (stats->entries[i].connection_id == connection_id) {
            return i;
        }
    }
    return -1;
}

/* Поиск устройства по ID */
static int find_device(traffic_stats_t *stats, const char *device_id) {
    for (int i = 0; i < stats->device_count; i++) {
        if (strcmp(stats->devices[i].device_id, device_id) == 0) {
            return i;
        }
    }
    return -1;
}

/* Создание новой записи о подключении */
int traffic_stats_create_connection(traffic_stats_t *stats,
                                     uint64_t connection_id,
                                     const char *client_ip,
                                     const char *device_id) {
    if (!stats || !client_ip || !device_id) {
        return -1;
    }
    
    pthread_mutex_lock(&stats->lock);
    
    /* Проверяем, нет ли уже такого подключения */
    int existing = find_connection(stats, connection_id);
    if (existing >= 0) {
        pthread_mutex_unlock(&stats->lock);
        return -1; /* Уже существует */
    }
    
    /* Проверяем место */
    if (stats->entry_count >= MAX_TRAFFIC_ENTRIES) {
        /* Удаляем старые неактивные записи */
        traffic_stats_cleanup_inactive(stats, 3600); /* 1 час */
        
        if (stats->entry_count >= MAX_TRAFFIC_ENTRIES) {
            pthread_mutex_unlock(&stats->lock);
            return -1;
        }
    }
    
    /* Создаём новую запись */
    traffic_entry_t *entry = &stats->entries[stats->entry_count];
    entry->connection_id = connection_id;
    strncpy(entry->client_ip, client_ip, sizeof(entry->client_ip) - 1);
    entry->client_ip[sizeof(entry->client_ip) - 1] = '\0';
    strncpy(entry->device_id, device_id, sizeof(entry->device_id) - 1);
    entry->device_id[sizeof(entry->device_id) - 1] = '\0';
    entry->bytes_sent = 0;
    entry->bytes_received = 0;
    entry->packets_sent = 0;
    entry->packets_received = 0;
    entry->connection_start = time(NULL);
    entry->last_activity = entry->connection_start;
    entry->is_active = 1;
    
    stats->entry_count++;
    stats->total_connections++;
    stats->active_connections++;
    
    /* Обновляем или создаём запись устройства */
    int dev_idx = find_device(stats, device_id);
    if (dev_idx < 0) {
        if (stats->device_count < MAX_TRAFFIC_ENTRIES) {
            dev_idx = stats->device_count++;
            device_traffic_t *dev = &stats->devices[dev_idx];
            memset(dev, 0, sizeof(device_traffic_t));
            strncpy(dev->device_id, device_id, sizeof(dev->device_id) - 1);
            strncpy(dev->client_ip, client_ip, sizeof(dev->client_ip) - 1);
            dev->first_seen = entry->connection_start;
        }
    }
    
    if (dev_idx >= 0) {
        device_traffic_t *dev = &stats->devices[dev_idx];
        dev->connection_count++;
        dev->last_seen = entry->connection_start;
    }
    
    pthread_mutex_unlock(&stats->lock);

    return 0;
}

/* Обновление статистики отправки */
int traffic_stats_update_sent(traffic_stats_t *stats,
                               uint64_t connection_id,
                               uint64_t bytes,
                               uint64_t packets) {
    if (!stats) {
        return -1;
    }
    
    pthread_mutex_lock(&stats->lock);
    
    int idx = find_connection(stats, connection_id);
    if (idx < 0) {
        pthread_mutex_unlock(&stats->lock);
        return -1; /* Подключение не найдено */
    }
    
    traffic_entry_t *entry = &stats->entries[idx];
    entry->bytes_sent += bytes;
    entry->packets_sent += packets;
    entry->last_activity = time(NULL);
    
    /* Обновляем глобальную статистику */
    stats->total_bytes_sent += bytes;
    
    /* Обновляем статистику устройства */
    int dev_idx = find_device(stats, entry->device_id);
    if (dev_idx >= 0) {
        stats->devices[dev_idx].total_bytes_sent += bytes;
        stats->devices[dev_idx].total_packets_sent += packets;
    }
    
    pthread_mutex_unlock(&stats->lock);
    
    return 0;
}

/* Обновление статистики получения */
int traffic_stats_update_received(traffic_stats_t *stats,
                                   uint64_t connection_id,
                                   uint64_t bytes,
                                   uint64_t packets) {
    if (!stats) {
        return -1;
    }
    
    pthread_mutex_lock(&stats->lock);
    
    int idx = find_connection(stats, connection_id);
    if (idx < 0) {
        pthread_mutex_unlock(&stats->lock);
        return -1; /* Подключение не найдено */
    }
    
    traffic_entry_t *entry = &stats->entries[idx];
    entry->bytes_received += bytes;
    entry->packets_received += packets;
    entry->last_activity = time(NULL);
    
    /* Обновляем глобальную статистику */
    stats->total_bytes_received += bytes;
    
    /* Обновляем статистику устройства */
    int dev_idx = find_device(stats, entry->device_id);
    if (dev_idx >= 0) {
        stats->devices[dev_idx].total_bytes_received += bytes;
        stats->devices[dev_idx].total_packets_received += packets;
    }
    
    pthread_mutex_unlock(&stats->lock);
    
    return 0;
}

/* Закрытие подключения */
int traffic_stats_close_connection(traffic_stats_t *stats,
                                    uint64_t connection_id) {
    if (!stats) {
        return -1;
    }
    
    pthread_mutex_lock(&stats->lock);
    
    int idx = find_connection(stats, connection_id);
    if (idx < 0) {
        pthread_mutex_unlock(&stats->lock);
        return -1; /* Подключение не найдено */
    }
    
    stats->entries[idx].is_active = 0;
    if (stats->active_connections > 0) {
        stats->active_connections--;
    }
    
    pthread_mutex_unlock(&stats->lock);

    return 0;
}

/* Получение статистики по подключению */
int traffic_stats_get_connection(traffic_stats_t *stats,
                                  uint64_t connection_id,
                                  traffic_entry_t *entry) {
    if (!stats || !entry) {
        return -1;
    }
    
    pthread_mutex_lock(&stats->lock);
    
    int idx = find_connection(stats, connection_id);
    if (idx < 0) {
        pthread_mutex_unlock(&stats->lock);
        return -1;
    }
    
    memcpy(entry, &stats->entries[idx], sizeof(traffic_entry_t));
    
    pthread_mutex_unlock(&stats->lock);
    
    return 0;
}

/* Получение статистики по устройству */
int traffic_stats_get_device(traffic_stats_t *stats,
                              const char *device_id,
                              device_traffic_t *device) {
    if (!stats || !device_id || !device) {
        return -1;
    }
    
    pthread_mutex_lock(&stats->lock);
    
    int idx = find_device(stats, device_id);
    if (idx < 0) {
        pthread_mutex_unlock(&stats->lock);
        return -1;
    }
    
    memcpy(device, &stats->devices[idx], sizeof(device_traffic_t));
    
    pthread_mutex_unlock(&stats->lock);
    
    return 0;
}

/* Получение глобальной статистики */
void traffic_stats_get_global(traffic_stats_t *stats,
                               uint64_t *total_sent,
                               uint64_t *total_received,
                               uint64_t *active_conn,
                               uint64_t *total_conn) {
    if (!stats) {
        return;
    }
    
    pthread_mutex_lock(&stats->lock);
    
    if (total_sent) *total_sent = stats->total_bytes_sent;
    if (total_received) *total_received = stats->total_bytes_received;
    if (active_conn) *active_conn = stats->active_connections;
    if (total_conn) *total_conn = stats->total_connections;
    
    pthread_mutex_unlock(&stats->lock);
}

/* Добавление записи в историю */
static void add_history_sample(traffic_stats_t *stats) {
    traffic_sample_t *sample = &stats->history[stats->history_index];
    sample->timestamp = (uint64_t)time(NULL);
    sample->bytes_sent = stats->total_bytes_sent;
    sample->bytes_received = stats->total_bytes_received;
    
    stats->history_index = (stats->history_index + 1) % TRAFFIC_HISTORY_SIZE;
}

/* Получение истории трафика */
int traffic_stats_get_history(traffic_stats_t *stats,
                               traffic_sample_t *samples,
                               int max_samples) {
    if (!stats || !samples || max_samples <= 0) {
        return -1;
    }
    
    pthread_mutex_lock(&stats->lock);
    
    /* Добавляем текущую запись */
    add_history_sample(stats);
    
    int count = (max_samples < TRAFFIC_HISTORY_SIZE) ? max_samples : TRAFFIC_HISTORY_SIZE;
    
    /* Копируем историю в правильном порядке */
    for (int i = 0; i < count; i++) {
        int idx = (stats->history_index + i) % TRAFFIC_HISTORY_SIZE;
        samples[i] = stats->history[idx];
    }
    
    pthread_mutex_unlock(&stats->lock);
    
    return count;
}

/* Экспорт статистики в JSON */
int traffic_stats_export_json(traffic_stats_t *stats,
                               char *buffer,
                               size_t buffer_size) {
    if (!stats || !buffer || buffer_size < 1024) {
        return -1;
    }
    
    pthread_mutex_lock(&stats->lock);
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "{\n");
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "  \"global\": {\n");
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "    \"total_bytes_sent\": %llu,\n",
                       (unsigned long long)stats->total_bytes_sent);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "    \"total_bytes_received\": %llu,\n",
                       (unsigned long long)stats->total_bytes_received);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "    \"active_connections\": %llu,\n",
                       (unsigned long long)stats->active_connections);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "    \"total_connections\": %llu\n",
                       (unsigned long long)stats->total_connections);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  },\n");
    
    /* Активные подключения */
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  \"connections\": [\n");
    
    int first = 1;
    for (int i = 0; i < stats->entry_count && offset < buffer_size - 200; i++) {
        traffic_entry_t *e = &stats->entries[i];
        if (!e->is_active) continue;
        
        if (!first) {
            offset += snprintf(buffer + offset, buffer_size - offset, ",\n");
        }
        first = 0;
        
        offset += snprintf(buffer + offset, buffer_size - offset,
                           "    {\n"
                           "      \"id\": %llu,\n"
                           "      \"client_ip\": \"%s\",\n"
                           "      \"device_id\": \"%s\",\n"
                           "      \"bytes_sent\": %llu,\n"
                           "      \"bytes_received\": %llu,\n"
                           "      \"packets_sent\": %llu,\n"
                           "      \"packets_received\": %llu\n"
                           "    }",
                           (unsigned long long)e->connection_id,
                           e->client_ip,
                           e->device_id,
                           (unsigned long long)e->bytes_sent,
                           (unsigned long long)e->bytes_received,
                           (unsigned long long)e->packets_sent,
                           (unsigned long long)e->packets_received);
    }
    
    offset += snprintf(buffer + offset, buffer_size - offset, "\n  ]\n");
    offset += snprintf(buffer + offset, buffer_size - offset, "}\n");
    
    pthread_mutex_unlock(&stats->lock);
    
    return offset;
}

/* Сброс статистики */
void traffic_stats_reset(traffic_stats_t *stats) {
    if (!stats) {
        return;
    }
    
    pthread_mutex_lock(&stats->lock);
    
    stats->entry_count = 0;
    stats->device_count = 0;
    stats->total_bytes_sent = 0;
    stats->total_bytes_received = 0;
    stats->total_connections = 0;
    stats->active_connections = 0;
    stats->start_time = time(NULL);
    stats->history_index = 0;
    
    memset(stats->entries, 0, sizeof(stats->entries));
    memset(stats->devices, 0, sizeof(stats->devices));
    memset(stats->history, 0, sizeof(stats->history));
    
    pthread_mutex_unlock(&stats->lock);
}

/* Удаление старых неактивных записей */
int traffic_stats_cleanup_inactive(traffic_stats_t *stats,
                                    time_t inactive_threshold) {
    if (!stats || inactive_threshold <= 0) {
        return -1;
    }
    
    pthread_mutex_lock(&stats->lock);
    
    time_t now = time(NULL);
    int removed = 0;
    
    for (int i = 0; i < stats->entry_count; i++) {
        traffic_entry_t *e = &stats->entries[i];
        
        if (e->is_active) continue; /* Пропускаем активные */
        
        if (now - e->last_activity > inactive_threshold) {
            /* Сдвигаем массив */
            for (int j = i; j < stats->entry_count - 1; j++) {
                stats->entries[j] = stats->entries[j + 1];
            }
            stats->entry_count--;
            removed++;
            i--; /* Проверяем тот же индекс снова */
        }
    }
    
    pthread_mutex_unlock(&stats->lock);

    return removed;
}
