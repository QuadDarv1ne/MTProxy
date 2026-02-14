/*
    Реализация расширенной системы мониторинга и логирования
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "advanced-monitoring.h"

// Глобальный мониторинг
static advanced_monitoring_t *g_monitoring = NULL;

// Вспомогательные функции
static int find_metric_index(advanced_monitoring_t *mon, const char *name);
static int find_alert_index(advanced_monitoring_t *mon, const char *name);
static int find_component_index(advanced_monitoring_t *mon, const char *name);
static void update_system_metrics(advanced_monitoring_t *mon);
static void check_threshold_alerts(advanced_monitoring_t *mon);

// Инициализация
advanced_monitoring_t* monitoring_init(int max_metrics, int max_alerts, int log_buffer_size) {
    advanced_monitoring_t *mon = malloc(sizeof(advanced_monitoring_t));
    if (!mon) {
        return NULL;
    }
    
    // Обнуление
    memset(mon, 0, sizeof(advanced_monitoring_t));
    
    // Инициализация параметров
    mon->max_metrics = max_metrics > 0 ? max_metrics : MAX_METRICS;
    mon->max_alerts = max_alerts > 0 ? max_alerts : MAX_ALERTS;
    mon->current_log_level = DEFAULT_LOG_LEVEL;
    mon->is_initialized = 1;
    mon->start_time = monitoring_get_current_time_ms();
    
    // Выделение памяти для метрик
    mon->metrics = malloc(sizeof(metric_t) * mon->max_metrics);
    if (mon->metrics) {
        memset(mon->metrics, 0, sizeof(metric_t) * mon->max_metrics);
    }
    
    // Выделение памяти для алертов
    mon->alert_rules = malloc(sizeof(alert_rule_t) * mon->max_alerts);
    if (mon->alert_rules) {
        memset(mon->alert_rules, 0, sizeof(alert_rule_t) * mon->max_alerts);
    }
    
    // Инициализация лог буфера
    mon->log_buffer = malloc(sizeof(log_buffer_t));
    if (mon->log_buffer) {
        mon->log_buffer->capacity = log_buffer_size > 0 ? log_buffer_size : MAX_LOG_BUFFER;
        mon->log_buffer->entries = malloc(sizeof(log_entry_t) * mon->log_buffer->capacity);
        if (mon->log_buffer->entries) {
            memset(mon->log_buffer->entries, 0, sizeof(log_entry_t) * mon->log_buffer->capacity);
        }
        mon->log_buffer->head = 0;
        mon->log_buffer->tail = 0;
        mon->log_buffer->count = 0;
        mon->log_buffer->dropped_entries = 0;
    }
    
    // Выделение памяти для статистики компонентов
    mon->component_stats = malloc(sizeof(component_stats_t) * 64); // Максимум 64 компонента
    if (mon->component_stats) {
        memset(mon->component_stats, 0, sizeof(component_stats_t) * 64);
        mon->max_components = 64;
    }
    
    g_monitoring = mon;
    return mon;
}

// Конфигурация
int monitoring_configure(advanced_monitoring_t *mon, log_level_t log_level, const char *log_file) {
    if (!mon) return -1;
    
    mon->current_log_level = log_level;
    
    if (log_file) {
        strncpy(mon->log_file_path, log_file, sizeof(mon->log_file_path) - 1);
        mon->log_file_path[sizeof(mon->log_file_path) - 1] = '\0';
        mon->enable_file_logging = 1;
    }
    
    return 0;
}

// Очистка
void monitoring_cleanup(advanced_monitoring_t *mon) {
    if (!mon) return;
    
    if (mon->metrics) {
        free(mon->metrics);
    }
    
    if (mon->alert_rules) {
        free(mon->alert_rules);
    }
    
    if (mon->log_buffer) {
        if (mon->log_buffer->entries) {
            free(mon->log_buffer->entries);
        }
        free(mon->log_buffer);
    }
    
    if (mon->component_stats) {
        free(mon->component_stats);
    }
    
    mon->is_initialized = 0;
    if (g_monitoring == mon) {
        g_monitoring = NULL;
    }
    
    free(mon);
}

// Регистрация метрики
int monitoring_register_metric(advanced_monitoring_t *mon, const char *name, const char *desc, metric_type_t type) {
    if (!mon || !name || mon->metric_count >= mon->max_metrics) {
        return -1;
    }
    
    metric_t *metric = &mon->metrics[mon->metric_count];
    strncpy(metric->name, name, sizeof(metric->name) - 1);
    metric->name[sizeof(metric->name) - 1] = '\0';
    
    if (desc) {
        strncpy(metric->description, desc, sizeof(metric->description) - 1);
        metric->description[sizeof(metric->description) - 1] = '\0';
    }
    
    metric->type = type;
    metric->value = 0.0;
    metric->min_value = 0.0;
    metric->max_value = 0.0;
    metric->sample_count = 0;
    metric->sum = 0.0;
    metric->last_update = monitoring_get_current_time_ms();
    
    mon->metric_count++;
    return 0;
}

// Обновление метрики
int monitoring_update_metric(advanced_monitoring_t *mon, const char *name, double value) {
    if (!mon || !name) return -1;
    
    int index = find_metric_index(mon, name);
    if (index < 0) return -1;
    
    metric_t *metric = &mon->metrics[index];
    metric->value = value;
    metric->sum += value;
    metric->sample_count++;
    
    if (metric->sample_count == 1) {
        metric->min_value = value;
        metric->max_value = value;
    } else {
        if (value < metric->min_value) metric->min_value = value;
        if (value > metric->max_value) metric->max_value = value;
    }
    
    metric->last_update = monitoring_get_current_time_ms();
    mon->metrics_samples_collected++;
    
    return 0;
}

// Инкремент метрики
int monitoring_increment_metric(advanced_monitoring_t *mon, const char *name, double increment) {
    if (!mon || !name) return -1;
    
    int index = find_metric_index(mon, name);
    if (index < 0) return -1;
    
    return monitoring_update_metric(mon, name, mon->metrics[index].value + increment);
}

// Получение метрики
metric_t* monitoring_get_metric(advanced_monitoring_t *mon, const char *name) {
    if (!mon || !name) return NULL;
    
    int index = find_metric_index(mon, name);
    if (index < 0) return NULL;
    
    return &mon->metrics[index];
}

// Сбор системных метрик
void monitoring_collect_system_metrics(advanced_monitoring_t *mon) {
    if (!mon) return;
    
    update_system_metrics(mon);
}

// Добавление правила алерта
int monitoring_add_alert_rule(advanced_monitoring_t *mon, const char *name, alert_type_t type, 
                           double threshold, double duration, void (*callback)(alert_rule_t*)) {
    if (!mon || !name || mon->alert_count >= mon->max_alerts) {
        return -1;
    }
    
    alert_rule_t *rule = &mon->alert_rules[mon->alert_count];
    strncpy(rule->name, name, sizeof(rule->name) - 1);
    rule->name[sizeof(rule->name) - 1] = '\0';
    
    rule->type = type;
    rule->threshold = threshold;
    rule->duration = duration;
    rule->status = ALERT_STATUS_OK;
    rule->triggered_time = 0;
    rule->notification_sent = 0;
    rule->callback = callback;
    
    mon->alert_count++;
    return 0;
}

// Проверка алертов
int monitoring_check_alerts(advanced_monitoring_t *mon) {
    if (!mon) return 0;
    
    int triggered_count = 0;
    check_threshold_alerts(mon);
    
    for (int i = 0; i < mon->alert_count; i++) {
        if (mon->alert_rules[i].status != ALERT_STATUS_OK) {
            triggered_count++;
            mon->total_alerts_triggered++;
        }
    }
    
    return triggered_count;
}

// Логирование
int monitoring_log(advanced_monitoring_t *mon, log_level_t level, const char *component, const char *format, ...) {
    if (!mon || level > mon->current_log_level) return 0;
    
    if (!mon->log_buffer || !mon->log_buffer->entries) return -1;
    
    // Проверка места в буфере
    if (mon->log_buffer->count >= mon->log_buffer->capacity) {
        mon->log_buffer->dropped_entries++;
        // Удаление старую запись
        mon->log_buffer->head = (mon->log_buffer->head + 1) % mon->log_buffer->capacity;
        mon->log_buffer->count--;
    }
    
    log_entry_t *entry = &mon->log_buffer->entries[mon->log_buffer->tail];
    entry->timestamp = monitoring_get_current_time_ms();
    entry->level = level;
    entry->thread_id = 0; // В реальной реализации получить ID потока
    entry->connection_id = 0;
    
    if (component) {
        strncpy(entry->component, component, sizeof(entry->component) - 1);
        entry->component[sizeof(entry->component) - 1] = '\0';
    }
    
    // Форматирование сообщения
    va_list args;
    va_start(args, format);
    vsnprintf(entry->message, sizeof(entry->message) - 1, format, args);
    va_end(args);
    
    entry->message[sizeof(entry->message) - 1] = '\0';
    
    // Обновление буфера
    mon->log_buffer->tail = (mon->log_buffer->tail + 1) % mon->log_buffer->capacity;
    mon->log_buffer->count++;
    mon->total_log_entries++;
    
    // Запись в файл если включено
    if (mon->enable_file_logging && mon->log_file_path[0]) {
        FILE *file = fopen(mon->log_file_path, "a");
        if (file) {
            fprintf(file, "[%lld] [%s] [%s] %s\n", 
                    entry->timestamp,
                    monitoring_level_to_string(entry->level),
                    entry->component,
                    entry->message);
            fclose(file);
        }
    }
    
    return 0;
}

// Получение отчета системы
void monitoring_get_system_report(advanced_monitoring_t *mon, char *buffer, size_t buffer_size) {
    if (!mon || !buffer || buffer_size < 200) return;
    
    snprintf(buffer, buffer_size,
        "System Monitoring Report:\n"
        "Metrics Collected: %lld\n"
        "Active Metrics: %d\n"
        "Alert Rules: %d\n"
        "Active Alerts: %d\n"
        "Log Entries: %lld\n"
        "Dropped Logs: %d\n"
        "Components: %d\n",
        mon->metrics_samples_collected,
        mon->metric_count,
        mon->alert_count,
        monitoring_check_alerts(mon),
        mon->total_log_entries,
        mon->log_buffer ? mon->log_buffer->dropped_entries : 0,
        mon->component_count);
}

// Сброс статистики
void monitoring_reset_stats(advanced_monitoring_t *mon) {
    if (!mon) return;
    
    mon->total_log_entries = 0;
    mon->total_alerts_triggered = 0;
    mon->metrics_samples_collected = 0;
    
    if (mon->log_buffer) {
        mon->log_buffer->dropped_entries = 0;
        mon->log_buffer->head = 0;
        mon->log_buffer->tail = 0;
        mon->log_buffer->count = 0;
    }
    
    for (int i = 0; i < mon->metric_count; i++) {
        mon->metrics[i].value = 0.0;
        mon->metrics[i].sum = 0.0;
        mon->metrics[i].sample_count = 0;
        mon->metrics[i].min_value = 0.0;
        mon->metrics[i].max_value = 0.0;
    }
}

// Вспомогательные функции

static int find_metric_index(advanced_monitoring_t *mon, const char *name) {
    for (int i = 0; i < mon->metric_count; i++) {
        if (strcmp(mon->metrics[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_alert_index(advanced_monitoring_t *mon, const char *name) {
    for (int i = 0; i < mon->alert_count; i++) {
        if (strcmp(mon->alert_rules[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_component_index(advanced_monitoring_t *mon, const char *name) {
    for (int i = 0; i < mon->component_count; i++) {
        if (strcmp(mon->component_stats[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static void update_system_metrics(advanced_monitoring_t *mon) {
    // В реальной реализации собирать системные метрики:
    // - CPU usage
    // - Memory usage
    // - Network statistics
    // - Disk I/O
    
    // Симуляция обновления
    monitoring_update_metric(mon, "cpu_usage", 45.5);
    monitoring_update_metric(mon, "memory_usage", 65.2);
    monitoring_update_metric(mon, "active_connections", 1250);
    monitoring_update_metric(mon, "throughput_mbps", 150.5);
}

static void check_threshold_alerts(advanced_monitoring_t *mon) {
    for (int i = 0; i < mon->alert_count; i++) {
        alert_rule_t *rule = &mon->alert_rules[i];
        
        // Проверка порогов (в реальной реализации)
        double current_value = 0.0;
        
        switch (rule->type) {
            case ALERT_TYPE_CPU_USAGE:
                current_value = 45.5; // Симуляция
                break;
            case ALERT_TYPE_MEMORY_USAGE:
                current_value = 65.2; // Симуляция
                break;
            case ALERT_TYPE_CONNECTION_COUNT:
                current_value = 1250; // Симуляция
                break;
            default:
                continue;
        }
        
        if (current_value > rule->threshold) {
            if (rule->status == ALERT_STATUS_OK) {
                rule->status = ALERT_STATUS_WARNING;
                rule->triggered_time = monitoring_get_current_time_ms();
                
                if (rule->callback) {
                    rule->callback(rule);
                }
            }
        } else {
            if (rule->status != ALERT_STATUS_OK) {
                rule->status = ALERT_STATUS_RESOLVED;
                rule->triggered_time = 0;
                rule->notification_sent = 0;
            }
        }
    }
}

// Утилиты

const char* monitoring_level_to_string(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_NONE: return "NONE";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_WARNING: return "WARNING";
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_TRACE: return "TRACE";
        default: return "UNKNOWN";
    }
}

const char* monitoring_alert_status_to_string(alert_status_t status) {
    switch (status) {
        case ALERT_STATUS_OK: return "OK";
        case ALERT_STATUS_WARNING: return "WARNING";
        case ALERT_STATUS_CRITICAL: return "CRITICAL";
        case ALERT_STATUS_RESOLVED: return "RESOLVED";
        default: return "UNKNOWN";
    }
}

const char* monitoring_metric_type_to_string(metric_type_t type) {
    switch (type) {
        case METRIC_TYPE_COUNTER: return "COUNTER";
        case METRIC_TYPE_GAUGE: return "GAUGE";
        case METRIC_TYPE_HISTOGRAM: return "HISTOGRAM";
        case METRIC_TYPE_SUMMARY: return "SUMMARY";
        default: return "UNKNOWN";
    }
}

long long monitoring_get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000LL);
}