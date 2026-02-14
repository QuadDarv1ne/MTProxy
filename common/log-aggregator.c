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
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <regex.h>

#include "common/log-aggregator.h"
#include "common/kprintf.h"
#include "common/common-stats.h"
#include "common/structured-logger.h"

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

static struct aggregator_stats aggregator_stats = {0};

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

// Aggregator context
struct aggregator_context {
    struct log_pattern *patterns;
    int pattern_count;
    int pattern_capacity;
    
    struct aggregation_rule *rules;
    int rule_count;
    int rule_capacity;
    
    struct correlation_rule *correlations;
    int correlation_count;
    int correlation_capacity;
    
    struct log_alert *alerts;
    int alert_count;
    int alert_capacity;
    
    int enable_realtime_processing;
    int enable_pattern_matching;
    int enable_correlation;
    int enable_aggregation;
    
    time_t last_aggregation_cycle;
    int aggregation_interval_seconds;
    
    pthread_mutex_t aggregator_mutex;
    int aggregator_initialized;
};

static struct aggregator_context global_aggregator_ctx = {0};

// Built-in patterns
static const struct builtin_pattern {
    const char *name;
    const char *description;
    const char *regex;
    enum log_level level;
    int generate_alert;
    const char *alert_msg;
} builtin_patterns[] = {
    {
        "connection_error",
        "Connection establishment failures",
        "Connection.*failed|Connect.*error|Connection.*timeout",
        LOG_LEVEL_ERROR,
        1,
        "High connection failure rate detected"
    },
    {
        "security_violation",
        "Security-related incidents",
        "Security.*violation|Unauthorized.*access|Invalid.*certificate",
        LOG_LEVEL_CRITICAL,
        1,
        "Security violation detected"
    },
    {
        "performance_degradation",
        "Performance issues and slowdowns",
        "Performance.*degradation|High.*latency|Throughput.*drop",
        LOG_LEVEL_WARNING,
        1,
        "Performance degradation detected"
    },
    {
        "resource_exhaustion",
        "Resource limits exceeded",
        "Out.*of.*memory|Resource.*exhausted|File.*descriptor.*limit",
        LOG_LEVEL_ERROR,
        1,
        "Resource exhaustion detected"
    }
};

#define BUILTIN_PATTERN_COUNT (sizeof(builtin_patterns) / sizeof(builtin_patterns[0]))

// Built-in aggregation rules
static const struct builtin_rule {
    const char *name;
    const char *description;
    enum log_level level;
    int time_window;
    int min_events;
    const char *component;
    const char *message_pattern;
} builtin_rules[] = {
    {
        "high_error_rate",
        "Aggregate multiple errors in short time",
        LOG_LEVEL_ERROR,
        60,  // 1 minute
        10,  // 10 errors
        "network",
        "Connection.*failed"
    },
    {
        "repeated_warnings",
        "Aggregate repeated warnings",
        LOG_LEVEL_WARNING,
        300, // 5 minutes
        20,  // 20 warnings
        "performance",
        ".*degradation.*"
    }
};

#define BUILTIN_RULE_COUNT (sizeof(builtin_rules) / sizeof(builtin_rules[0]))

// Инициализация log aggregator
int log_aggregator_init(void) {
    if (global_aggregator_ctx.aggregator_initialized) {
        return 0; // Уже инициализирован
    }
    
    pthread_mutex_init(&global_aggregator_ctx.aggregator_mutex, NULL);
    
    // Инициализация структур
    global_aggregator_ctx.pattern_capacity = BUILTIN_PATTERN_COUNT + 20;
    global_aggregator_ctx.patterns = calloc(global_aggregator_ctx.pattern_capacity, 
                                           sizeof(struct log_pattern));
    
    global_aggregator_ctx.rule_capacity = BUILTIN_RULE_COUNT + 10;
    global_aggregator_ctx.rules = calloc(global_aggregator_ctx.rule_capacity, 
                                        sizeof(struct aggregation_rule));
    
    global_aggregator_ctx.correlation_capacity = 10;
    global_aggregator_ctx.correlations = calloc(global_aggregator_ctx.correlation_capacity, 
                                               sizeof(struct correlation_rule));
    
    global_aggregator_ctx.alert_capacity = 100;
    global_aggregator_ctx.alerts = calloc(global_aggregator_ctx.alert_capacity, 
                                         sizeof(struct log_alert));
    
    if (!global_aggregator_ctx.patterns || !global_aggregator_ctx.rules ||
        !global_aggregator_ctx.correlations || !global_aggregator_ctx.alerts) {
        log_aggregator_cleanup();
        return -1;
    }
    
    // Настройка параметров
    global_aggregator_ctx.enable_realtime_processing = 1;
    global_aggregator_ctx.enable_pattern_matching = 1;
    global_aggregator_ctx.enable_correlation = 1;
    global_aggregator_ctx.enable_aggregation = 1;
    global_aggregator_ctx.aggregation_interval_seconds = 30;
    global_aggregator_ctx.last_aggregation_cycle = time(NULL);
    global_aggregator_ctx.aggregator_initialized = 1;
    
    // Регистрация builtin паттернов
    for (int i = 0; i < BUILTIN_PATTERN_COUNT; i++) {
        const struct builtin_pattern *pattern = &builtin_patterns[i];
        log_aggregator_register_pattern(
            pattern->name,
            pattern->description,
            pattern->regex,
            pattern->level,
            pattern->generate_alert,
            pattern->alert_msg
        );
    }
    
    // Регистрация builtin правил
    for (int i = 0; i < BUILTIN_RULE_COUNT; i++) {
        const struct builtin_rule *rule = &builtin_rules[i];
        log_aggregator_register_rule(
            rule->name,
            rule->description,
            rule->level,
            rule->time_window,
            rule->min_events,
            rule->component,
            rule->message_pattern
        );
    }
    
    vkprintf(1, "Log aggregator initialized with %d patterns and %d rules\n", 
             BUILTIN_PATTERN_COUNT, BUILTIN_RULE_COUNT);
    
    return 0;
}

// Регистрация log pattern
int log_aggregator_register_pattern(
    const char *name,
    const char *description,
    const char *regex_pattern,
    enum log_level min_level,
    int generate_alert,
    const char *alert_message) {
    
    pthread_mutex_lock(&global_aggregator_ctx.aggregator_mutex);
    
    // Проверка существования
    for (int i = 0; i < global_aggregator_ctx.pattern_count; i++) {
        if (strcmp(global_aggregator_ctx.patterns[i].name, name) == 0) {
            pthread_mutex_unlock(&global_aggregator_ctx.aggregator_mutex);
            return -1;
        }
    }
    
    // Расширение массива если нужно
    if (global_aggregator_ctx.pattern_count >= global_aggregator_ctx.pattern_capacity) {
        int new_capacity = global_aggregator_ctx.pattern_capacity * 2;
        struct log_pattern *new_patterns = realloc(global_aggregator_ctx.patterns,
                                                  new_capacity * sizeof(struct log_pattern));
        if (!new_patterns) {
            pthread_mutex_unlock(&global_aggregator_ctx.aggregator_mutex);
            return -1;
        }
        global_aggregator_ctx.patterns = new_patterns;
        global_aggregator_ctx.pattern_capacity = new_capacity;
    }
    
    // Создание нового паттерна
    struct log_pattern *new_pattern = &global_aggregator_ctx.patterns[global_aggregator_ctx.pattern_count];
    strncpy(new_pattern->name, name, sizeof(new_pattern->name) - 1);
    if (description) {
        strncpy(new_pattern->description, description, sizeof(new_pattern->description) - 1);
    }
    if (regex_pattern) {
        strncpy(new_pattern->regex_pattern, regex_pattern, sizeof(new_pattern->regex_pattern) - 1);
        // Компиляция regex
        if (regcomp(&new_pattern->compiled_regex, regex_pattern, REG_EXTENDED | REG_NOSUB) != 0) {
            pthread_mutex_unlock(&global_aggregator_ctx.aggregator_mutex);
            return -1;
        }
    }
    new_pattern->min_level = min_level;
    new_pattern->is_active = 1;
    new_pattern->match_count = 0;
    new_pattern->last_match = 0;
    new_pattern->generate_alert = generate_alert;
    if (alert_message) {
        strncpy(new_pattern->alert_message, alert_message, sizeof(new_pattern->alert_message) - 1);
    }
    
    global_aggregator_ctx.pattern_count++;
    pthread_mutex_unlock(&global_aggregator_ctx.aggregator_mutex);
    
    vkprintf(2, "Registered log pattern: %s\n", name);
    return 0;
}

// Регистрация aggregation rule
int log_aggregator_register_rule(
    const char *name,
    const char *description,
    enum log_level target_level,
    int time_window_seconds,
    int min_events,
    const char *component_filter,
    const char *message_pattern) {
    
    pthread_mutex_lock(&global_aggregator_ctx.aggregator_mutex);
    
    // Проверка существования
    for (int i = 0; i < global_aggregator_ctx.rule_count; i++) {
        if (strcmp(global_aggregator_ctx.rules[i].name, name) == 0) {
            pthread_mutex_unlock(&global_aggregator_ctx.aggregator_mutex);
            return -1;
        }
    }
    
    // Расширение массива если нужно
    if (global_aggregator_ctx.rule_count >= global_aggregator_ctx.rule_capacity) {
        int new_capacity = global_aggregator_ctx.rule_capacity * 2;
        struct aggregation_rule *new_rules = realloc(global_aggregator_ctx.rules,
                                                    new_capacity * sizeof(struct aggregation_rule));
        if (!new_rules) {
            pthread_mutex_unlock(&global_aggregator_ctx.aggregator_mutex);
            return -1;
        }
        global_aggregator_ctx.rules = new_rules;
        global_aggregator_ctx.rule_capacity = new_capacity;
    }
    
    // Создание нового правила
    struct aggregation_rule *new_rule = &global_aggregator_ctx.rules[global_aggregator_ctx.rule_count];
    strncpy(new_rule->name, name, sizeof(new_rule->name) - 1);
    if (description) {
        strncpy(new_rule->description, description, sizeof(new_rule->description) - 1);
    }
    new_rule->target_level = target_level;
    new_rule->time_window_seconds = time_window_seconds;
    new_rule->min_events = min_events;
    if (component_filter) {
        strncpy(new_rule->component_filter, component_filter, sizeof(new_rule->component_filter) - 1);
    }
    if (message_pattern) {
        strncpy(new_rule->message_pattern, message_pattern, sizeof(new_rule->message_pattern) - 1);
    }
    new_rule->is_active = 1;
    new_rule->aggregation_count = 0;
    
    global_aggregator_ctx.rule_count++;
    pthread_mutex_unlock(&global_aggregator_ctx.aggregator_mutex);
    
    vkprintf(2, "Registered aggregation rule: %s\n", name);
    return 0;
}

// Обработка log entry
int log_aggregator_process_entry(const struct log_entry *entry) {
    if (!global_aggregator_ctx.aggregator_initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&global_aggregator_ctx.aggregator_mutex);
    
    aggregator_stats.total_log_entries_processed++;
    
    // Pattern matching
    if (global_aggregator_ctx.enable_pattern_matching) {
        log_aggregator_match_patterns(entry);
    }
    
    // Correlation
    if (global_aggregator_ctx.enable_correlation) {
        log_aggregator_check_correlations(entry);
    }
    
    // Real-time aggregation
    if (global_aggregator_ctx.enable_realtime_processing) {
        log_aggregator_apply_rules(entry);
    }
    
    pthread_mutex_unlock(&global_aggregator_ctx.aggregator_mutex);
    return 0;
}

// Pattern matching
static int log_aggregator_match_patterns(const struct log_entry *entry) {
    for (int i = 0; i < global_aggregator_ctx.pattern_count; i++) {
        struct log_pattern *pattern = &global_aggregator_ctx.patterns[i];
        
        if (!pattern->is_active || entry->level < pattern->min_level) {
            continue;
        }
        
        // Проверка regex
        if (regexec(&pattern->compiled_regex, entry->message, 0, NULL, 0) == 0) {
            pattern->match_count++;
            pattern->last_match = time(NULL);
            aggregator_stats.pattern_matches++;
            
            // Генерация alert если нужно
            if (pattern->generate_alert) {
                log_aggregator_generate_alert(
                    pattern->alert_message,
                    entry->level,
                    entry->component,
                    entry->subsystem,
                    entry->message
                );
            }
            
            vkprintf(3, "Pattern match: %s -> %s\n", pattern->name, entry->message);
        }
    }
    return 0;
}

// Применение aggregation rules
static int log_aggregator_apply_rules(const struct log_entry *entry) {
    time_t now = time(NULL);
    
    for (int i = 0; i < global_aggregator_ctx.rule_count; i++) {
        struct aggregation_rule *rule = &global_aggregator_ctx.rules[i];
        
        if (!rule->is_active) {
            continue;
        }
        
        // Проверка фильтров
        if (rule->component_filter[0] && 
            strcmp(entry->component, rule->component_filter) != 0) {
            continue;
        }
        
        // Проверка времени (упрощенная реализация)
        if (now - global_aggregator_ctx.last_aggregation_cycle < rule->time_window_seconds) {
            continue;
        }
        
        // Проверка pattern (простая реализация)
        if (rule->message_pattern[0] && 
            strstr(entry->message, rule->message_pattern) == NULL) {
            continue;
        }
        
        // Условия выполнены - генерируем aggregated event
        rule->aggregation_count++;
        aggregator_stats.aggregated_entries++;
        
        vkprintf(2, "Aggregation triggered: %s (count: %d)\n", 
                 rule->name, rule->aggregation_count);
        
        // Генерация aggregated log entry
        char aggregated_message[512];
        snprintf(aggregated_message, sizeof(aggregated_message),
                "AGGREGATED: %s - %d events in %d seconds",
                rule->name, rule->aggregation_count, rule->time_window_seconds);
        
        structured_log(rule->target_level, "aggregator", rule->name, 
                      aggregated_message, "rule=%s;events=%d", 
                      rule->name, rule->aggregation_count);
    }
    
    return 0;
}

// Проверка корреляций
static int log_aggregator_check_correlations(const struct log_entry *entry) {
    // Упрощенная реализация корреляции
    // В реальной системе нужно хранить историю событий
    
    for (int i = 0; i < global_aggregator_ctx.correlation_count; i++) {
        struct correlation_rule *corr = &global_aggregator_ctx.correlations[i];
        
        if (!corr->is_active) {
            continue;
        }
        
        // Проверка временного окна
        time_t now = time(NULL);
        if (now - corr->last_correlation < corr->time_window_seconds) {
            continue;
        }
        
        // Упрощенная логика корреляции
        if (strstr(entry->message, corr->first_pattern) && 
            strstr(entry->message, corr->second_pattern)) {
            corr->correlation_count++;
            corr->last_correlation = now;
            aggregator_stats.correlation_events++;
            
            vkprintf(2, "Correlation detected: %s\n", corr->name);
            
            // Генерация correlation alert
            char correlation_msg[256];
            snprintf(correlation_msg, sizeof(correlation_msg),
                    "Correlation detected: %s (%d times)", 
                    corr->name, corr->correlation_count);
            
            log_aggregator_generate_alert(
                correlation_msg,
                LOG_LEVEL_WARNING,
                "correlation",
                corr->name,
                entry->message
            );
        }
    }
    
    return 0;
}

// Генерация alert
static int log_aggregator_generate_alert(
    const char *message,
    enum log_level severity,
    const char *component,
    const char *subsystem,
    const char *context) {
    
    // Расширение массива alerts если нужно
    if (global_aggregator_ctx.alert_count >= global_aggregator_ctx.alert_capacity) {
        int new_capacity = global_aggregator_ctx.alert_capacity * 2;
        struct log_alert *new_alerts = realloc(global_aggregator_ctx.alerts,
                                              new_capacity * sizeof(struct log_alert));
        if (!new_alerts) {
            return -1;
        }
        global_aggregator_ctx.alerts = new_alerts;
        global_aggregator_ctx.alert_capacity = new_capacity;
    }
    
    struct log_alert *new_alert = &global_aggregator_ctx.alerts[global_aggregator_ctx.alert_count];
    
    // Генерация уникального ID
    snprintf(new_alert->id, sizeof(new_alert->id), 
             "ALERT_%ld_%d", time(NULL), global_aggregator_ctx.alert_count);
    
    new_alert->severity = severity;
    strncpy(new_alert->message, message, sizeof(new_alert->message) - 1);
    strncpy(new_alert->component, component ? component : "unknown", sizeof(new_alert->component) - 1);
    strncpy(new_alert->subsystem, subsystem ? subsystem : "main", sizeof(new_alert->subsystem) - 1);
    new_alert->timestamp = time(NULL);
    new_alert->is_resolved = 0;
    new_alert->resolution_info[0] = '\0';
    
    global_aggregator_ctx.alert_count++;
    aggregator_stats.alert_generations++;
    
    // Запись alert в лог
    structured_log(severity, "alert", "system", message,
                  "alert_id=%s;component=%s;subsystem=%s;context=%s",
                  new_alert->id, component, subsystem, context ? context : "");
    
    return 0;
}

// Выполнение цикла агрегации
int log_aggregator_run_cycle(void) {
    time_t now = time(NULL);
    
    if (now - global_aggregator_ctx.last_aggregation_cycle < 
        global_aggregator_ctx.aggregation_interval_seconds) {
        return 0; // Слишком рано
    }
    
    pthread_mutex_lock(&global_aggregator_ctx.aggregator_mutex);
    
    vkprintf(2, "Running log aggregation cycle\n");
    
    // Обновление времени
    global_aggregator_ctx.last_aggregation_cycle = now;
    aggregator_stats.aggregation_cycles++;
    
    // Здесь можно добавить более сложную логику агрегации
    // Например, анализ исторических данных, генерация отчетов
    
    pthread_mutex_unlock(&global_aggregator_ctx.aggregator_mutex);
    return 0;
}

// Получение статистики
void log_aggregator_get_stats(struct aggregator_stats *stats) {
    if (stats) {
        memcpy(stats, &aggregator_stats, sizeof(struct aggregator_stats));
    }
}

// Вывод статистики
void log_aggregator_print_stats(void) {
    vkprintf(1, "Log Aggregator Statistics:\n");
    vkprintf(1, "  Total Entries Processed: %lld\n", aggregator_stats.total_log_entries_processed);
    vkprintf(1, "  Aggregated Entries: %lld\n", aggregator_stats.aggregated_entries);
    vkprintf(1, "  Pattern Matches: %lld\n", aggregator_stats.pattern_matches);
    vkprintf(1, "  Correlation Events: %lld\n", aggregator_stats.correlation_events);
    vkprintf(1, "  Alert Generations: %lld\n", aggregator_stats.alert_generations);
    vkprintf(1, "  Filter Operations: %lld\n", aggregator_stats.filter_operations);
    vkprintf(1, "  Aggregation Cycles: %lld\n", aggregator_stats.aggregation_cycles);
    
    pthread_mutex_lock(&global_aggregator_ctx.aggregator_mutex);
    vkprintf(1, "  Active Patterns: %d\n", global_aggregator_ctx.pattern_count);
    vkprintf(1, "  Active Rules: %d\n", global_aggregator_ctx.rule_count);
    vkprintf(1, "  Active Correlations: %d\n", global_aggregator_ctx.correlation_count);
    vkprintf(1, "  Active Alerts: %d\n", global_aggregator_ctx.alert_count);
    pthread_mutex_unlock(&global_aggregator_ctx.aggregator_mutex);
}

// Очистка aggregator
void log_aggregator_cleanup(void) {
    pthread_mutex_lock(&global_aggregator_ctx.aggregator_mutex);
    
    // Очистка паттернов и regex
    for (int i = 0; i < global_aggregator_ctx.pattern_count; i++) {
        regfree(&global_aggregator_ctx.patterns[i].compiled_regex);
    }
    
    // Освобождение памяти
    if (global_aggregator_ctx.patterns) {
        free(global_aggregator_ctx.patterns);
    }
    if (global_aggregator_ctx.rules) {
        free(global_aggregator_ctx.rules);
    }
    if (global_aggregator_ctx.correlations) {
        free(global_aggregator_ctx.correlations);
    }
    if (global_aggregator_ctx.alerts) {
        free(global_aggregator_ctx.alerts);
    }
    
    pthread_mutex_unlock(&global_aggregator_ctx.aggregator_mutex);
    pthread_mutex_destroy(&global_aggregator_ctx.aggregator_mutex);
    
    memset(&aggregator_stats, 0, sizeof(aggregator_stats));
    memset(&global_aggregator_ctx, 0, sizeof(global_aggregator_ctx));
    
    vkprintf(1, "Log aggregator cleaned up\n");
}