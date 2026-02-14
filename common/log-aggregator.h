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

#ifndef __LOG_AGGREGATOR_H__
#define __LOG_AGGREGATOR_H__

#include <time.h>
#include <regex.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct log_entry;
enum log_level;

// Aggregator statistics
struct aggregator_stats {
    long long total_log_entries_processed;
    long long aggregated_entries;
    long long pattern_matches;
    long long correlation_events;
    long long alert_generations;
    long long filter_operations;
    long long aggregation_cycles;
};

// Log pattern structure
struct log_pattern {
    char name[128];
    char description[256];
    char regex_pattern[512];
    regex_t compiled_regex;
    enum log_level min_level;
    int is_active;
    int match_count;
    time_t last_match;
    int generate_alert;
    char alert_message[256];
};

// Aggregation rule
struct aggregation_rule {
    char name[128];
    char description[256];
    enum log_level target_level;
    int time_window_seconds;
    int min_events;
    char component_filter[64];
    char subsystem_filter[64];
    char message_pattern[256];
    int is_active;
    int aggregation_count;
};

// Correlation rule
struct correlation_rule {
    char name[128];
    char description[256];
    char first_pattern[128];
    char second_pattern[128];
    int time_window_seconds;
    int is_active;
    int correlation_count;
    time_t last_correlation;
};

// Alert structure
struct log_alert {
    char id[64];
    enum log_level severity;
    char message[512];
    char component[64];
    char subsystem[64];
    time_t timestamp;
    int is_resolved;
    char resolution_info[256];
};

// Инициализация log aggregator
int log_aggregator_init(void);

// Регистрация log pattern
int log_aggregator_register_pattern(
    const char *name,
    const char *description,
    const char *regex_pattern,
    enum log_level min_level,
    int generate_alert,
    const char *alert_message);

// Регистрация aggregation rule
int log_aggregator_register_rule(
    const char *name,
    const char *description,
    enum log_level target_level,
    int time_window_seconds,
    int min_events,
    const char *component_filter,
    const char *message_pattern);

// Обработка log entry
int log_aggregator_process_entry(const struct log_entry *entry);

// Выполнение цикла агрегации
int log_aggregator_run_cycle(void);

// Получение статистики
void log_aggregator_get_stats(struct aggregator_stats *stats);

// Вывод статистики
void log_aggregator_print_stats(void);

// Очистка aggregator
void log_aggregator_cleanup(void);

// Расширенные функции
int log_aggregator_register_correlation(
    const char *name,
    const char *description,
    const char *first_pattern,
    const char *second_pattern,
    int time_window_seconds);

int log_aggregator_get_alerts(struct log_alert *alerts, int max_alerts);
int log_aggregator_resolve_alert(const char *alert_id, const char *resolution_info);
int log_aggregator_export_report(const char *filename, time_t start_time, time_t end_time);
int log_aggregator_enable_pattern(const char *pattern_name, int enable);
int log_aggregator_enable_rule(const char *rule_name, int enable);

// Query functions
int log_aggregator_query_patterns(struct log_pattern *patterns, int max_patterns);
int log_aggregator_query_rules(struct aggregation_rule *rules, int max_rules);
int log_aggregator_query_correlations(struct correlation_rule *correlations, int max_correlations);
int log_aggregator_get_active_alerts_count(void);

// Pattern management
int log_aggregator_update_pattern_regex(const char *pattern_name, const char *new_regex);
int log_aggregator_update_rule_threshold(const char *rule_name, int new_threshold);
int log_aggregator_get_pattern_stats(const char *pattern_name, int *match_count, time_t *last_match);

// Real-time analysis
int log_aggregator_enable_realtime(int enable);
int log_aggregator_set_aggregation_interval(int seconds);
int log_aggregator_force_aggregation_cycle(void);

#ifdef __cplusplus
}
#endif

#endif // __LOG_AGGREGATOR_H__