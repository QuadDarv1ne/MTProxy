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

#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include "net/network-profiler.h"
#include "common/kprintf.h"
#include "common/common-stats.h"
#include "common/precise-time.h"

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

static struct network_profiler_stats profiler_stats = {0};

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

// Network anomaly detection
struct anomaly_detector {
    double baseline_latency_ms;
    double baseline_throughput_mbps;
    double baseline_packet_loss_rate;
    double current_latency_ms;
    double current_throughput_mbps;
    double current_packet_loss_rate;
    int anomaly_threshold;
    time_t last_update;
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

static struct profiler_config global_profiler_config = {
    .enable_latency_profiling = 1,
    .enable_throughput_profiling = 1,
    .enable_anomaly_detection = 1,
    .sampling_rate = 100, // Sample every 100th packet
    .alert_threshold_ms = 1000,
    .performance_window_seconds = 60,
    .max_profile_entries = 10000
};

// Profile storage
#define MAX_CONNECTION_PROFILES 10000
static struct connection_profile connection_profiles[MAX_CONNECTION_PROFILES];
static int profile_count = 0;
static pthread_mutex_t profile_mutex = PTHREAD_MUTEX_INITIALIZER;

// Latency measurement structure
struct latency_sample {
    unsigned long long timestamp_us;
    unsigned long long latency_us;
    int connection_id;
    enum packet_type type;
};

#define LATENCY_HISTORY_SIZE 10000
static struct latency_sample latency_history[LATENCY_HISTORY_SIZE];
static int latency_history_index = 0;
static pthread_mutex_t latency_mutex = PTHREAD_MUTEX_INITIALIZER;

// Performance thresholds
static const struct performance_thresholds {
    double max_acceptable_latency_ms;
    double min_acceptable_throughput_mbps;
    double max_acceptable_packet_loss_rate;
    double anomaly_detection_sensitivity;
} default_thresholds = {
    .max_acceptable_latency_ms = 50.0,
    .min_acceptable_throughput_mbps = 10.0,
    .max_acceptable_packet_loss_rate = 0.01,
    .anomaly_detection_sensitivity = 2.0
};

// Инициализация network profiler
int network_profiler_init(void) {
    pthread_mutex_init(&profile_mutex, NULL);
    pthread_mutex_init(&latency_mutex, NULL);
    
    // Initialize connection profiles
    memset(connection_profiles, 0, sizeof(connection_profiles));
    profile_count = 0;
    
    // Initialize latency history
    memset(latency_history, 0, sizeof(latency_history));
    latency_history_index = 0;
    
    vkprintf(1, "Network profiler initialized with config: "
             "sampling_rate=%d, alert_threshold=%dms\n",
             global_profiler_config.sampling_rate,
             global_profiler_config.alert_threshold_ms);
    
    return 0;
}

// Создание профиля соединения
int network_profiler_create_connection_profile(
    int connection_id,
    unsigned int client_ip, unsigned short client_port,
    unsigned int server_ip, unsigned short server_port) {
    
    pthread_mutex_lock(&profile_mutex);
    
    if (profile_count >= MAX_CONNECTION_PROFILES) {
        pthread_mutex_unlock(&profile_mutex);
        return -1;
    }
    
    struct connection_profile *profile = &connection_profiles[profile_count];
    
    profile->connection_id = connection_id;
    profile->client_ip = client_ip;
    profile->client_port = client_port;
    profile->server_ip = server_ip;
    profile->server_port = server_port;
    profile->connect_time = time(NULL);
    profile->last_activity = profile->connect_time;
    profile->bytes_sent = 0;
    profile->bytes_received = 0;
    profile->packets_sent = 0;
    profile->packets_received = 0;
    profile->latency_sum_us = 0;
    profile->latency_samples = 0;
    profile->max_latency_us = 0;
    profile->min_latency_us = UINT64_MAX;
    profile->state = CONNECTION_STATE_CONNECTING;
    profile->is_anomalous = 0;
    profile->performance_score = 100;
    
    profile_count++;
    profiler_stats.total_connections++;
    profiler_stats.active_connections++;
    
    pthread_mutex_unlock(&profile_mutex);
    
    vkprintf(3, "Created connection profile %d: %u:%u -> %u:%u\n",
             connection_id, client_ip, client_port, server_ip, server_port);
    
    return 0;
}

// Обновление статистики соединения
int network_profiler_update_connection_stats(
    int connection_id,
    unsigned long long bytes_sent,
    unsigned long long bytes_received,
    unsigned long long packets_sent,
    unsigned long long packets_received) {
    
    pthread_mutex_lock(&profile_mutex);
    
    struct connection_profile *profile = NULL;
    for (int i = 0; i < profile_count; i++) {
        if (connection_profiles[i].connection_id == connection_id) {
            profile = &connection_profiles[i];
            break;
        }
    }
    
    if (!profile) {
        pthread_mutex_unlock(&profile_mutex);
        return -1;
    }
    
    // Update profile statistics
    profile->bytes_sent += bytes_sent;
    profile->bytes_received += bytes_received;
    profile->packets_sent += packets_sent;
    profile->packets_received += packets_received;
    profile->last_activity = time(NULL);
    
    // Update global statistics
    profiler_stats.total_bytes_sent += bytes_sent;
    profiler_stats.total_bytes_received += bytes_received;
    profiler_stats.packets_sent += packets_sent;
    profiler_stats.packets_received += packets_received;
    
    pthread_mutex_unlock(&profile_mutex);
    return 0;
}

// Измерение latency
int network_profiler_measure_latency(
    int connection_id,
    unsigned long long send_time_us,
    unsigned long long receive_time_us,
    enum packet_type type) {
    
    unsigned long long latency_us = receive_time_us - send_time_us;
    
    // Update connection profile
    pthread_mutex_lock(&profile_mutex);
    
    struct connection_profile *profile = NULL;
    for (int i = 0; i < profile_count; i++) {
        if (connection_profiles[i].connection_id == connection_id) {
            profile = &connection_profiles[i];
            break;
        }
    }
    
    if (profile) {
        profile->latency_sum_us += latency_us;
        profile->latency_samples++;
        profile->last_activity = time(NULL);
        
        if (latency_us > profile->max_latency_us) {
            profile->max_latency_us = latency_us;
        }
        if (latency_us < profile->min_latency_us) {
            profile->min_latency_us = latency_us;
        }
    }
    
    pthread_mutex_unlock(&profile_mutex);
    
    // Store latency sample
    pthread_mutex_lock(&latency_mutex);
    
    struct latency_sample *sample = &latency_history[latency_history_index];
    sample->timestamp_us = receive_time_us;
    sample->latency_us = latency_us;
    sample->connection_id = connection_id;
    sample->type = type;
    
    latency_history_index = (latency_history_index + 1) % LATENCY_HISTORY_SIZE;
    
    pthread_mutex_unlock(&latency_mutex);
    
    // Update global statistics
    profiler_stats.latency_measurements++;
    profiler_stats.total_latency_us += latency_us;
    
    if (latency_us > profiler_stats.max_latency_us) {
        profiler_stats.max_latency_us = latency_us;
    }
    if (latency_us < profiler_stats.min_latency_us || profiler_stats.min_latency_us == 0) {
        profiler_stats.min_latency_us = latency_us;
    }
    
    // Check for anomalies
    if (latency_us > global_profiler_config.alert_threshold_ms * 1000) {
        profiler_stats.anomaly_detections++;
        vkprintf(2, "High latency detected: %llu us for connection %d\n", 
                 latency_us, connection_id);
    }
    
    profiler_stats.profiling_samples++;
    return 0;
}

// Обнаружение аномалий
static int detect_connection_anomalies(struct connection_profile *profile) {
    if (profile->latency_samples == 0) {
        return 0;
    }
    
    double avg_latency_ms = (double)profile->latency_sum_us / 
                           (double)profile->latency_samples / 1000.0;
    
    // Check latency anomaly
    if (avg_latency_ms > default_thresholds.max_acceptable_latency_ms) {
        profile->is_anomalous = 1;
        profile->performance_score -= 20;
        return 1;
    }
    
    // Check throughput anomaly
    time_t duration = time(NULL) - profile->connect_time;
    if (duration > 0) {
        double throughput_mbps = (double)(profile->bytes_received + profile->bytes_sent) / 
                                (double)duration / 125000.0; // Convert to Mbps
        
        if (throughput_mbps < default_thresholds.min_acceptable_throughput_mbps) {
            profile->is_anomalous = 1;
            profile->performance_score -= 15;
            return 1;
        }
    }
    
    // Check packet loss
    if (profile->packets_sent > 0) {
        double packet_loss_rate = (double)(profile->packets_sent - profile->packets_received) / 
                                 (double)profile->packets_sent;
        
        if (packet_loss_rate > default_thresholds.max_acceptable_packet_loss_rate) {
            profile->is_anomalous = 1;
            profile->performance_score -= 25;
            return 1;
        }
    }
    
    return 0;
}

// Обновление состояния соединения
int network_profiler_update_connection_state(
    int connection_id,
    enum connection_state new_state) {
    
    pthread_mutex_lock(&profile_mutex);
    
    struct connection_profile *profile = NULL;
    for (int i = 0; i < profile_count; i++) {
        if (connection_profiles[i].connection_id == connection_id) {
            profile = &connection_profiles[i];
            break;
        }
    }
    
    if (!profile) {
        pthread_mutex_unlock(&profile_mutex);
        return -1;
    }
    
    // Update state
    enum connection_state old_state = profile->state;
    profile->state = new_state;
    profile->last_activity = time(NULL);
    
    // Handle state transitions
    switch (new_state) {
        case CONNECTION_STATE_ESTABLISHED:
            if (old_state == CONNECTION_STATE_CONNECTING) {
                profiler_stats.connection_attempts++;
            }
            break;
            
        case CONNECTION_STATE_CLOSED:
            if (profile->state != CONNECTION_STATE_ERROR) {
                profiler_stats.active_connections--;
            }
            break;
            
        case CONNECTION_STATE_ERROR:
            profiler_stats.failed_connections++;
            profiler_stats.active_connections--;
            profile->performance_score = 0;
            break;
            
        case CONNECTION_STATE_TIMEOUT:
            profiler_stats.connection_timeouts++;
            profiler_stats.active_connections--;
            profile->performance_score -= 50;
            break;
            
        default:
            break;
    }
    
    // Detect anomalies for established connections
    if (new_state == CONNECTION_STATE_ESTABLISHED) {
        detect_connection_anomalies(profile);
    }
    
    pthread_mutex_unlock(&profile_mutex);
    
    vkprintf(3, "Connection %d state changed: %d -> %d\n", 
             connection_id, old_state, new_state);
    
    return 0;
}

// Получение профиля соединения
struct connection_profile *network_profiler_get_connection_profile(int connection_id) {
    pthread_mutex_lock(&profile_mutex);
    
    for (int i = 0; i < profile_count; i++) {
        if (connection_profiles[i].connection_id == connection_id) {
            pthread_mutex_unlock(&profile_mutex);
            return &connection_profiles[i];
        }
    }
    
    pthread_mutex_unlock(&profile_mutex);
    return NULL;
}

// Получение статистики profiler
void network_profiler_get_stats(struct network_profiler_stats *stats) {
    if (stats) {
        memcpy(stats, &profiler_stats, sizeof(struct network_profiler_stats));
    }
}

// Вывод статистики
void network_profiler_print_stats(void) {
    vkprintf(1, "Network Profiler Statistics:\n");
    vkprintf(1, "  Total Connections: %lld\n", profiler_stats.total_connections);
    vkprintf(1, "  Active Connections: %lld\n", profiler_stats.active_connections);
    vkprintf(1, "  Connection Attempts: %lld\n", profiler_stats.connection_attempts);
    vkprintf(1, "  Failed Connections: %lld\n", profiler_stats.failed_connections);
    vkprintf(1, "  Connection Timeouts: %lld\n", profiler_stats.connection_timeouts);
    vkprintf(1, "  Bytes Sent: %lld\n", profiler_stats.total_bytes_sent);
    vkprintf(1, "  Bytes Received: %lld\n", profiler_stats.total_bytes_received);
    vkprintf(1, "  Packets Sent: %lld\n", profiler_stats.packets_sent);
    vkprintf(1, "  Packets Received: %lld\n", profiler_stats.packets_received);
    vkprintf(1, "  Packet Loss: %lld\n", profiler_stats.packet_loss);
    vkprintf(1, "  Latency Measurements: %lld\n", profiler_stats.latency_measurements);
    
    if (profiler_stats.latency_measurements > 0) {
        double avg_latency_ms = (double)profiler_stats.total_latency_us / 
                               (double)profiler_stats.latency_measurements / 1000.0;
        vkprintf(1, "  Average Latency: %.2f ms\n", avg_latency_ms);
        vkprintf(1, "  Max Latency: %.2f ms\n", (double)profiler_stats.max_latency_us / 1000.0);
        vkprintf(1, "  Min Latency: %.2f ms\n", (double)profiler_stats.min_latency_us / 1000.0);
    }
    
    vkprintf(1, "  Protocol Errors: %lld\n", profiler_stats.protocol_errors);
    vkprintf(1, "  Network Errors: %lld\n", profiler_stats.network_errors);
    vkprintf(1, "  Timeout Errors: %lld\n", profiler_stats.timeout_errors);
    vkprintf(1, "  Buffer Overflows: %lld\n", profiler_stats.buffer_overflows);
    vkprintf(1, "  Anomaly Detections: %lld\n", profiler_stats.anomaly_detections);
    vkprintf(1, "  Performance Degradations: %lld\n", profiler_stats.performance_degradations);
}

// Очистка profiler
void network_profiler_cleanup(void) {
    pthread_mutex_destroy(&profile_mutex);
    pthread_mutex_destroy(&latency_mutex);
    
    memset(&profiler_stats, 0, sizeof(profiler_stats));
    memset(connection_profiles, 0, sizeof(connection_profiles));
    profile_count = 0;
    
    vkprintf(1, "Network profiler cleaned up\n");
}

// Получение топ аномальных соединений
int network_profiler_get_anomalous_connections(
    struct connection_profile *anomalous_connections,
    int max_count) {
    
    if (!anomalous_connections || max_count <= 0) {
        return -1;
    }
    
    pthread_mutex_lock(&profile_mutex);
    
    int count = 0;
    for (int i = 0; i < profile_count && count < max_count; i++) {
        if (connection_profiles[i].is_anomalous) {
            memcpy(&anomalous_connections[count], &connection_profiles[i], 
                   sizeof(struct connection_profile));
            count++;
        }
    }
    
    pthread_mutex_unlock(&profile_mutex);
    return count;
}

// Сброс статистики
void network_profiler_reset_stats(void) {
    memset(&profiler_stats, 0, sizeof(profiler_stats));
    vkprintf(1, "Network profiler statistics reset\n");
}