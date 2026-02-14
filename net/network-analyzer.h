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

#ifndef __NETWORK_ANALYZER_H__
#define __NETWORK_ANALYZER_H__

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Alert types
enum alert_type {
    ALERT_TYPE_UNKNOWN = 0,
    ALERT_TYPE_HIGH_LATENCY,
    ALERT_TYPE_LOW_THROUGHPUT,
    ALERT_TYPE_HIGH_PACKET_LOSS,
    ALERT_TYPE_HIGH_CPU_USAGE,
    ALERT_TYPE_HIGH_MEMORY_USAGE,
    ALERT_TYPE_THROUGHPUT_DEGRADATION,
    ALERT_TYPE_LATENCY_DEGRADATION,
    ALERT_TYPE_CONNECTION_ANOMALY,
    ALERT_TYPE_SECURITY_INCIDENT
};

// Alert severity levels
enum alert_severity {
    ALERT_SEVERITY_INFO = 0,
    ALERT_SEVERITY_WARNING,
    ALERT_SEVERITY_CRITICAL,
    ALERT_SEVERITY_EMERGENCY
};

// Performance analyzer statistics
struct performance_analyzer_stats {
    long long total_analysis_runs;
    long long performance_degradations_detected;
    long long optimizations_applied;
    long long false_positives;
    long long alert_generations;
    long long auto_adjustments;
};

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

// Alert structure
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

// Инициализация performance analyzer
int performance_analyzer_init(void);

// Сбор метрик производительности
int performance_analyzer_collect_metrics(
    double throughput_mbps,
    double latency_ms,
    double packet_loss_rate,
    double cpu_usage_percent,
    double memory_usage_percent);

// Анализ производительности
int performance_analyzer_run_analysis(void);

// Генерация alert
int performance_analyzer_generate_alert(
    enum alert_type type,
    enum alert_severity severity,
    const char *message,
    int connection_id,
    double current_value,
    double threshold_value);

// Получение статистики analyzer
void performance_analyzer_get_stats(struct performance_analyzer_stats *stats);

// Вывод статистики
void performance_analyzer_print_stats(void);

// Очистка analyzer
void performance_analyzer_cleanup(void);

// Настройка конфигурации
int performance_analyzer_set_config(const struct analyzer_config *config);
int performance_analyzer_get_config(struct analyzer_config *config);

// Получение текущих метрик
int performance_analyzer_get_current_metrics(struct performance_metrics *metrics);

// Получение baseline
int performance_analyzer_get_baseline(struct performance_baseline *baseline);

// Получение активных alerts
int performance_analyzer_get_active_alerts(struct performance_alert *alerts, int max_count);

// Разрешение alert
int performance_analyzer_resolve_alert(int alert_id);

// Прогнозирование производительности
int performance_analyzer_predict_performance(struct performance_metrics *prediction, int seconds_ahead);

// Экспорт данных для внешнего анализа
int performance_analyzer_export_data(const char *filename);

#ifdef __cplusplus
}
#endif

#endif // __NETWORK_ANALYZER_H__