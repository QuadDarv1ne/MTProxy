/*
    This file is part of Mtproto-proxy Library.

    Mtproto-proxy Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Mtproto-proxy Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Mtproto-proxy Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2024-2026 MTProto Proxy Enhanced Project
              2024-2026 Dupley Maxim Igorevich (Maestro7IT)
*/

#ifndef __NETWORK_PROFILER_H__
#define __NETWORK_PROFILER_H__

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Connection states
enum connection_state {
    CONNECTION_STATE_UNKNOWN = 0,
    CONNECTION_STATE_CONNECTING,
    CONNECTION_STATE_ESTABLISHED,
    CONNECTION_STATE_CLOSING,
    CONNECTION_STATE_CLOSED,
    CONNECTION_STATE_ERROR,
    CONNECTION_STATE_TIMEOUT
};

// Packet types for profiling
enum packet_type {
    PACKET_TYPE_UNKNOWN = 0,
    PACKET_TYPE_DATA,
    PACKET_TYPE_CONTROL,
    PACKET_TYPE_HANDSHAKE,
    PACKET_TYPE_KEEPALIVE
};

// Network profiling statistics
struct network_profiler_stats {
    // Connection metrics
    long long total_connections;
    long long active_connections;
    long long connection_attempts;
    long long failed_connections;
    long long connection_timeouts;
    
    // Data transfer metrics
    long long total_bytes_sent;
    long long total_bytes_received;
    long long packets_sent;
    long long packets_received;
    long long packet_loss;
    
    // Performance metrics
    long long latency_measurements;
    long long total_latency_us;
    long long max_latency_us;
    long long min_latency_us;
    
    // Error metrics
    long long protocol_errors;
    long long network_errors;
    long long timeout_errors;
    long long buffer_overflows;
    
    // Profiling counters
    long long profiling_samples;
    long long anomaly_detections;
    long long performance_degradations;
};

// Connection profiling data
struct connection_profile {
    int connection_id;
    unsigned int client_ip;
    unsigned short client_port;
    unsigned int server_ip;
    unsigned short server_port;
    time_t connect_time;
    time_t last_activity;
    unsigned long long bytes_sent;
    unsigned long long bytes_received;
    unsigned long long packets_sent;
    unsigned long long packets_received;
    unsigned long long latency_sum_us;
    unsigned long long latency_samples;
    unsigned long long max_latency_us;
    unsigned long long min_latency_us;
    enum connection_state state;
    int is_anomalous;
    int performance_score; // 0-100
};

// Profiling configuration
struct profiler_config {
    int enable_latency_profiling;
    int enable_throughput_profiling;
    int enable_anomaly_detection;
    int sampling_rate; // 1/N packets sampled
    int alert_threshold_ms;
    int performance_window_seconds;
    unsigned long long max_profile_entries;
};

// Инициализация network profiler
int network_profiler_init(void);

// Создание профиля соединения
int network_profiler_create_connection_profile(
    int connection_id,
    unsigned int client_ip, unsigned short client_port,
    unsigned int server_ip, unsigned short server_port);

// Обновление статистики соединения
int network_profiler_update_connection_stats(
    int connection_id,
    unsigned long long bytes_sent,
    unsigned long long bytes_received,
    unsigned long long packets_sent,
    unsigned long long packets_received);

// Измерение latency
int network_profiler_measure_latency(
    int connection_id,
    unsigned long long send_time_us,
    unsigned long long receive_time_us,
    enum packet_type type);

// Обновление состояния соединения
int network_profiler_update_connection_state(
    int connection_id,
    enum connection_state new_state);

// Получение профиля соединения
struct connection_profile *network_profiler_get_connection_profile(int connection_id);

// Получение статистики profiler
void network_profiler_get_stats(struct network_profiler_stats *stats);

// Вывод статистики
void network_profiler_print_stats(void);

// Очистка profiler
void network_profiler_cleanup(void);

// Получение топ аномальных соединений
int network_profiler_get_anomalous_connections(
    struct connection_profile *anomalous_connections,
    int max_count);

// Сброс статистики
void network_profiler_reset_stats(void);

// Настройка profiler конфигурации
int network_profiler_set_config(const struct profiler_config *config);
int network_profiler_get_config(struct profiler_config *config);

// Расширенные функции профилирования
int network_profiler_enable_tracing(int connection_id);
int network_profiler_disable_tracing(int connection_id);
int network_profiler_get_connection_history(int connection_id, 
                                          struct latency_sample *samples, 
                                          int max_samples);

#ifdef __cplusplus
}
#endif

#endif // __NETWORK_PROFILER_H__