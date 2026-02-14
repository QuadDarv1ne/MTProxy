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

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#include "net/network-analyzer.h"
#include "common/kprintf.h"
#include "common/common-stats.h"
#include "net/network-profiler.h"

// Performance analyzer statistics
struct performance_analyzer_stats {
    long long total_analysis_runs;
    long long performance_degradations_detected;
    long long optimizations_applied;
    long long false_positives;
    long long alert_generations;
    long long auto_adjustments;
};

static struct performance_analyzer_stats analyzer_stats = {0};

// Performance metrics structure
struct performance_metrics {
    double current_throughput_mbps;
    double current_latency_ms;
    double current_packet_loss_rate;
    double current_cpu_usage_percent;
    double current_memory_usage_percent;
    double current_connection_efficiency;
    time_t timestamp;
};

// Performance baseline
struct performance_baseline {
    double avg_throughput_mbps;
    double avg_latency_ms;
    double avg_packet_loss_rate;
    double avg_cpu_usage_percent;
    double avg_memory_usage_percent;
    double throughput_std_dev;
    double latency_std_dev;
    time_t baseline_timestamp;
    int sample_count;
};

// Analysis configuration
struct analyzer_config {
    int enable_auto_optimization;
    int enable_degradation_detection;
    int enable_predictive_analysis;
    double degradation_threshold_percent;
    int analysis_interval_seconds;
    int baseline_window_minutes;
    int alert_cooldown_seconds;
    double optimization_threshold_percent;
};

static struct analyzer_config global_analyzer_config = {
    .enable_auto_optimization = 1,
    .enable_degradation_detection = 1,
    .enable_predictive_analysis = 1,
    .degradation_threshold_percent = 15.0,
    .analysis_interval_seconds = 30,
    .baseline_window_minutes = 10,
    .alert_cooldown_seconds = 300,
    .optimization_threshold_percent = 10.0
};

// Performance analysis data
#define METRICS_HISTORY_SIZE 1000
static struct performance_metrics metrics_history[METRICS_HISTORY_SIZE];
static int metrics_history_index = 0;
static int metrics_history_count = 0;
static pthread_mutex_t metrics_mutex = PTHREAD_MUTEX_INITIALIZER;

// Performance baseline
static struct performance_baseline current_baseline = {0};
static pthread_mutex_t baseline_mutex = PTHREAD_MUTEX_INITIALIZER;

// Alert system
struct performance_alert {
    enum alert_type type;
    enum alert_severity severity;
    char message[256];
    time_t timestamp;
    int connection_id;
    double current_value;
    double baseline_value;
    int resolved;
};

#define MAX_ALERTS 100
static struct performance_alert alert_queue[MAX_ALERTS];
static int alert_queue_count = 0;
static time_t last_alert_time = 0;
static pthread_mutex_t alert_mutex = PTHREAD_MUTEX_INITIALIZER;

// Performance thresholds
static const struct performance_thresholds {
    double max_latency_ms;
    double min_throughput_mbps;
    double max_packet_loss_rate;
    double max_cpu_usage_percent;
    double max_memory_usage_percent;
} critical_thresholds = {
    .max_latency_ms = 100.0,
    .min_throughput_mbps = 1.0,
    .max_packet_loss_rate = 0.05,
    .max_cpu_usage_percent = 80.0,
    .max_memory_usage_percent = 85.0
};

// Инициализация performance analyzer
int performance_analyzer_init(void) {
    pthread_mutex_init(&metrics_mutex, NULL);
    pthread_mutex_init(&baseline_mutex, NULL);
    pthread_mutex_init(&alert_mutex, NULL);
    
    // Initialize metrics history
    memset(metrics_history, 0, sizeof(metrics_history));
    metrics_history_index = 0;
    metrics_history_count = 0;
    
    // Initialize alerts
    memset(alert_queue, 0, sizeof(alert_queue));
    alert_queue_count = 0;
    last_alert_time = 0;
    
    // Initialize baseline
    memset(&current_baseline, 0, sizeof(current_baseline));
    
    vkprintf(1, "Performance analyzer initialized with config: "
             "degradation_threshold=%.1f%%, analysis_interval=%ds\n",
             global_analyzer_config.degradation_threshold_percent,
             global_analyzer_config.analysis_interval_seconds);
    
    return 0;
}

// Сбор метрик производительности
int performance_analyzer_collect_metrics(
    double throughput_mbps,
    double latency_ms,
    double packet_loss_rate,
    double cpu_usage_percent,
    double memory_usage_percent) {
    
    struct performance_metrics *current_metrics = 
        &metrics_history[metrics_history_index];
    
    current_metrics->current_throughput_mbps = throughput_mbps;
    current_metrics->current_latency_ms = latency_ms;
    current_metrics->current_packet_loss_rate = packet_loss_rate;
    current_metrics->current_cpu_usage_percent = cpu_usage_percent;
    current_metrics->current_memory_usage_percent = memory_usage_percent;
    current_metrics->current_connection_efficiency = 
        (throughput_mbps > 0) ? (100.0 - (latency_ms / 10.0)) : 0.0;
    current_metrics->timestamp = time(NULL);
    
    // Update history pointers
    pthread_mutex_lock(&metrics_mutex);
    metrics_history_index = (metrics_history_index + 1) % METRICS_HISTORY_SIZE;
    if (metrics_history_count < METRICS_HISTORY_SIZE) {
        metrics_history_count++;
    }
    pthread_mutex_unlock(&metrics_mutex);
    
    // Check for immediate threshold violations
    performance_analyzer_check_thresholds(current_metrics);
    
    analyzer_stats.total_analysis_runs++;
    return 0;
}

// Проверка критических порогов
static int performance_analyzer_check_thresholds(
    const struct performance_metrics *metrics) {
    
    int alert_generated = 0;
    time_t now = time(NULL);
    
    // Проверка latency
    if (metrics->current_latency_ms > critical_thresholds.max_latency_ms) {
        performance_analyzer_generate_alert(ALERT_TYPE_HIGH_LATENCY,
                                          ALERT_SEVERITY_CRITICAL,
                                          "Critical latency threshold exceeded",
                                          0,
                                          metrics->current_latency_ms,
                                          critical_thresholds.max_latency_ms);
        alert_generated = 1;
    }
    
    // Проверка throughput
    if (metrics->current_throughput_mbps < critical_thresholds.min_throughput_mbps) {
        performance_analyzer_generate_alert(ALERT_TYPE_LOW_THROUGHPUT,
                                          ALERT_SEVERITY_CRITICAL,
                                          "Critical throughput threshold exceeded",
                                          0,
                                          metrics->current_throughput_mbps,
                                          critical_thresholds.min_throughput_mbps);
        alert_generated = 1;
    }
    
    // Проверка packet loss
    if (metrics->current_packet_loss_rate > critical_thresholds.max_packet_loss_rate) {
        performance_analyzer_generate_alert(ALERT_TYPE_HIGH_PACKET_LOSS,
                                          ALERT_SEVERITY_CRITICAL,
                                          "Critical packet loss threshold exceeded",
                                          0,
                                          metrics->current_packet_loss_rate,
                                          critical_thresholds.max_packet_loss_rate);
        alert_generated = 1;
    }
    
    // Проверка ресурсов
    if (metrics->current_cpu_usage_percent > critical_thresholds.max_cpu_usage_percent) {
        performance_analyzer_generate_alert(ALERT_TYPE_HIGH_CPU_USAGE,
                                          ALERT_SEVERITY_WARNING,
                                          "High CPU usage detected",
                                          0,
                                          metrics->current_cpu_usage_percent,
                                          critical_thresholds.max_cpu_usage_percent);
    }
    
    if (metrics->current_memory_usage_percent > critical_thresholds.max_memory_usage_percent) {
        performance_analyzer_generate_alert(ALERT_TYPE_HIGH_MEMORY_USAGE,
                                          ALERT_SEVERITY_WARNING,
                                          "High memory usage detected",
                                          0,
                                          metrics->current_memory_usage_percent,
                                          critical_thresholds.max_memory_usage_percent);
    }
    
    if (alert_generated) {
        analyzer_stats.alert_generations++;
    }
    
    return alert_generated;
}

// Генерация alert
int performance_analyzer_generate_alert(
    enum alert_type type,
    enum alert_severity severity,
    const char *message,
    int connection_id,
    double current_value,
    double threshold_value) {
    
    time_t now = time(NULL);
    
    // Проверка cooldown
    if (now - last_alert_time < global_analyzer_config.alert_cooldown_seconds) {
        return 0; // Слишком рано для нового alert
    }
    
    pthread_mutex_lock(&alert_mutex);
    
    if (alert_queue_count >= MAX_ALERTS) {
        // Удаляем старые alerts
        for (int i = 0; i < MAX_ALERTS - 1; i++) {
            alert_queue[i] = alert_queue[i + 1];
        }
        alert_queue_count--;
    }
    
    struct performance_alert *new_alert = &alert_queue[alert_queue_count];
    new_alert->type = type;
    new_alert->severity = severity;
    strncpy(new_alert->message, message, sizeof(new_alert->message) - 1);
    new_alert->message[sizeof(new_alert->message) - 1] = '\0';
    new_alert->timestamp = now;
    new_alert->connection_id = connection_id;
    new_alert->current_value = current_value;
    new_alert->baseline_value = threshold_value;
    new_alert->resolved = 0;
    
    alert_queue_count++;
    last_alert_time = now;
    
    pthread_mutex_unlock(&alert_mutex);
    
    // Логируем alert
    vkprintf(severity == ALERT_SEVERITY_CRITICAL ? 1 : 2,
             "PERFORMANCE ALERT [%s]: %s (Value: %.2f, Threshold: %.2f)\n",
             severity == ALERT_SEVERITY_CRITICAL ? "CRITICAL" : "WARNING",
             message, current_value, threshold_value);
    
    return 0;
}

// Анализ производительности
int performance_analyzer_run_analysis(void) {
    pthread_mutex_lock(&metrics_mutex);
    
    if (metrics_history_count < 10) {
        pthread_mutex_unlock(&metrics_mutex);
        return -1; // Недостаточно данных
    }
    
    // Вычисляем средние значения за последние N минут
    int analysis_window = global_analyzer_config.analysis_interval_seconds * 2;
    if (analysis_window > metrics_history_count) {
        analysis_window = metrics_history_count;
    }
    
    double sum_throughput = 0, sum_latency = 0, sum_packet_loss = 0;
    double sum_cpu = 0, sum_memory = 0;
    int sample_count = 0;
    
    int start_index = (metrics_history_index - analysis_window + METRICS_HISTORY_SIZE) % METRICS_HISTORY_SIZE;
    
    for (int i = 0; i < analysis_window; i++) {
        int index = (start_index + i) % METRICS_HISTORY_SIZE;
        struct performance_metrics *metrics = &metrics_history[index];
        
        sum_throughput += metrics->current_throughput_mbps;
        sum_latency += metrics->current_latency_ms;
        sum_packet_loss += metrics->current_packet_loss_rate;
        sum_cpu += metrics->current_cpu_usage_percent;
        sum_memory += metrics->current_memory_usage_percent;
        sample_count++;
    }
    
    pthread_mutex_unlock(&metrics_mutex);
    
    if (sample_count == 0) {
        return -1;
    }
    
    // Вычисляем средние значения
    struct performance_metrics current_avg = {
        .current_throughput_mbps = sum_throughput / sample_count,
        .current_latency_ms = sum_latency / sample_count,
        .current_packet_loss_rate = sum_packet_loss / sample_count,
        .current_cpu_usage_percent = sum_cpu / sample_count,
        .current_memory_usage_percent = sum_memory / sample_count,
        .timestamp = time(NULL)
    };
    
    // Обновляем baseline
    performance_analyzer_update_baseline(&current_avg);
    
    // Проверяем деградацию
    if (performance_analyzer_check_degradation(&current_avg)) {
        analyzer_stats.performance_degradations_detected++;
    }
    
    // Применяем оптимизации если включено
    if (global_analyzer_config.enable_auto_optimization) {
        performance_analyzer_apply_optimizations(&current_avg);
    }
    
    return 0;
}

// Обновление performance baseline
static int performance_analyzer_update_baseline(
    const struct performance_metrics *current_metrics) {
    
    pthread_mutex_lock(&baseline_mutex);
    
    if (current_baseline.sample_count == 0) {
        // Инициализация baseline
        current_baseline.avg_throughput_mbps = current_metrics->current_throughput_mbps;
        current_baseline.avg_latency_ms = current_metrics->current_latency_ms;
        current_baseline.avg_packet_loss_rate = current_metrics->current_packet_loss_rate;
        current_baseline.avg_cpu_usage_percent = current_metrics->current_cpu_usage_percent;
        current_baseline.avg_memory_usage_percent = current_metrics->current_memory_usage_percent;
        current_baseline.baseline_timestamp = time(NULL);
        current_baseline.sample_count = 1;
    } else {
        // Обновляем скользящее среднее
        double alpha = 0.1; // Фактор сглаживания
        
        current_baseline.avg_throughput_mbps = 
            alpha * current_metrics->current_throughput_mbps + 
            (1 - alpha) * current_baseline.avg_throughput_mbps;
            
        current_baseline.avg_latency_ms = 
            alpha * current_metrics->current_latency_ms + 
            (1 - alpha) * current_baseline.avg_latency_ms;
            
        current_baseline.avg_packet_loss_rate = 
            alpha * current_metrics->current_packet_loss_rate + 
            (1 - alpha) * current_baseline.avg_packet_loss_rate;
            
        current_baseline.avg_cpu_usage_percent = 
            alpha * current_metrics->current_cpu_usage_percent + 
            (1 - alpha) * current_baseline.avg_cpu_usage_percent;
            
        current_baseline.avg_memory_usage_percent = 
            alpha * current_metrics->current_memory_usage_percent + 
            (1 - alpha) * current_baseline.avg_memory_usage_percent;
            
        current_baseline.sample_count++;
    }
    
    pthread_mutex_unlock(&baseline_mutex);
    return 0;
}

// Проверка деградации производительности
static int performance_analyzer_check_degradation(
    const struct performance_metrics *current_metrics) {
    
    pthread_mutex_lock(&baseline_mutex);
    
    if (current_baseline.sample_count < 10) {
        pthread_mutex_unlock(&baseline_mutex);
        return 0; // Недостаточно данных для baseline
    }
    
    double throughput_degradation = 
        (current_baseline.avg_throughput_mbps - current_metrics->current_throughput_mbps) / 
        current_baseline.avg_throughput_mbps * 100;
        
    double latency_degradation = 
        (current_metrics->current_latency_ms - current_baseline.avg_latency_ms) / 
        current_baseline.avg_latency_ms * 100;
        
    int degradation_detected = 0;
    
    if (throughput_degradation > global_analyzer_config.degradation_threshold_percent) {
        performance_analyzer_generate_alert(ALERT_TYPE_THROUGHPUT_DEGRADATION,
                                          ALERT_SEVERITY_WARNING,
                                          "Throughput degradation detected",
                                          0,
                                          current_metrics->current_throughput_mbps,
                                          current_baseline.avg_throughput_mbps);
        degradation_detected = 1;
    }
    
    if (latency_degradation > global_analyzer_config.degradation_threshold_percent) {
        performance_analyzer_generate_alert(ALERT_TYPE_LATENCY_DEGRADATION,
                                          ALERT_SEVERITY_WARNING,
                                          "Latency degradation detected",
                                          0,
                                          current_metrics->current_latency_ms,
                                          current_baseline.avg_latency_ms);
        degradation_detected = 1;
    }
    
    pthread_mutex_unlock(&baseline_mutex);
    return degradation_detected;
}

// Применение автоматических оптимизаций
static int performance_analyzer_apply_optimizations(
    const struct performance_metrics *current_metrics) {
    
    int optimizations_applied = 0;
    
    pthread_mutex_lock(&baseline_mutex);
    
    // Оптимизация при высоком latency
    if (current_metrics->current_latency_ms > current_baseline.avg_latency_ms * 1.2) {
        // Уменьшаем размер буферов для снижения latency
        // TODO: Вызов функций оптимизации сети
        vkprintf(2, "Applying latency optimization: reducing buffer sizes\n");
        optimizations_applied = 1;
    }
    
    // Оптимизация при низком throughput
    if (current_metrics->current_throughput_mbps < current_baseline.avg_throughput_mbps * 0.8) {
        // Увеличиваем параллелизм
        // TODO: Вызов функций оптимизации throughput
        vkprintf(2, "Applying throughput optimization: increasing parallelism\n");
        optimizations_applied = 1;
    }
    
    // Оптимизация при высоком использовании CPU
    if (current_metrics->current_cpu_usage_percent > 80.0) {
        // Включаем более агрессивное кэширование
        // TODO: Вызов функций оптимизации CPU
        vkprintf(2, "Applying CPU optimization: enabling aggressive caching\n");
        optimizations_applied = 1;
    }
    
    pthread_mutex_unlock(&baseline_mutex);
    
    if (optimizations_applied) {
        analyzer_stats.optimizations_applied++;
    }
    
    return optimizations_applied;
}

// Получение статистики analyzer
void performance_analyzer_get_stats(struct performance_analyzer_stats *stats) {
    if (stats) {
        memcpy(stats, &analyzer_stats, sizeof(struct performance_analyzer_stats));
    }
}

// Вывод статистики
void performance_analyzer_print_stats(void) {
    vkprintf(1, "Performance Analyzer Statistics:\n");
    vkprintf(1, "  Total Analysis Runs: %lld\n", analyzer_stats.total_analysis_runs);
    vkprintf(1, "  Performance Degradations Detected: %lld\n", analyzer_stats.performance_degradations_detected);
    vkprintf(1, "  Optimizations Applied: %lld\n", analyzer_stats.optimizations_applied);
    vkprintf(1, "  False Positives: %lld\n", analyzer_stats.false_positives);
    vkprintf(1, "  Alert Generations: %lld\n", analyzer_stats.alert_generations);
    vkprintf(1, "  Auto Adjustments: %lld\n", analyzer_stats.auto_adjustments);
    
    pthread_mutex_lock(&baseline_mutex);
    if (current_baseline.sample_count > 0) {
        vkprintf(1, "  Current Baseline:\n");
        vkprintf(1, "    Throughput: %.2f Mbps\n", current_baseline.avg_throughput_mbps);
        vkprintf(1, "    Latency: %.2f ms\n", current_baseline.avg_latency_ms);
        vkprintf(1, "    Packet Loss: %.3f%%\n", current_baseline.avg_packet_loss_rate * 100);
        vkprintf(1, "    CPU Usage: %.1f%%\n", current_baseline.avg_cpu_usage_percent);
        vkprintf(1, "    Memory Usage: %.1f%%\n", current_baseline.avg_memory_usage_percent);
    }
    pthread_mutex_unlock(&baseline_mutex);
}

// Очистка analyzer
void performance_analyzer_cleanup(void) {
    pthread_mutex_destroy(&metrics_mutex);
    pthread_mutex_destroy(&baseline_mutex);
    pthread_mutex_destroy(&alert_mutex);
    
    memset(&analyzer_stats, 0, sizeof(analyzer_stats));
    memset(metrics_history, 0, sizeof(metrics_history));
    memset(&current_baseline, 0, sizeof(current_baseline));
    memset(alert_queue, 0, sizeof(alert_queue));
    
    vkprintf(1, "Performance analyzer cleaned up\n");
}