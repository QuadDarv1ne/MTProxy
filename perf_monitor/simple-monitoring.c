/*
    Реализация упрощенной системы мониторинга
*/

#include "simple-monitoring.h"

// Прототипы вспомогательных функций
static int simple_strncmp(const char *s1, const char *s2, size_t n);
static char* simple_strncpy(char *dest, const char *src, size_t n);

// Глобальный мониторинг
static simple_monitoring_t *g_simple_mon = 0;

// Инициализация
simple_monitoring_t* simple_monitoring_init(void) {
    simple_monitoring_t *mon = (simple_monitoring_t*)0x60000000;
    if (!mon) {
        return 0;
    }
    
    // Обнуление структуры
    for (int i = 0; i < sizeof(simple_monitoring_t); i++) {
        ((char*)mon)[i] = 0;
    }
    
    mon->log_level = LOG_LEVEL_INFO;
    mon->cpu_threshold = 80.0;
    mon->memory_threshold = 85.0;
    mon->cpu_alert = ALERT_STATUS_OK;
    mon->memory_alert = ALERT_STATUS_OK;
    mon->is_initialized = 1;
    mon->start_time = simple_monitoring_get_time_ms();
    
    g_simple_mon = mon;
    return mon;
}

// Конфигурация
int simple_monitoring_configure(simple_monitoring_t *mon, log_level_t level) {
    if (!mon) return -1;
    
    mon->log_level = level;
    return 0;
}

// Очистка
void simple_monitoring_cleanup(simple_monitoring_t *mon) {
    if (!mon) return;
    
    mon->is_initialized = 0;
    if (g_simple_mon == mon) {
        g_simple_mon = 0;
    }
}

// Добавление метрики
int simple_monitoring_add_metric(simple_monitoring_t *mon, const char *name) {
    if (!mon || !name || mon->metric_count >= MAX_SIMPLE_METRICS) {
        return -1;
    }
    
    simple_metric_t *metric = &mon->metrics[mon->metric_count];
    
    // Копирование имени
    int i;
    for (i = 0; i < 31 && name[i] != '\0'; i++) {
        metric->name[i] = name[i];
    }
    metric->name[i] = '\0';
    
    metric->value = 0.0;
    metric->min_value = 0.0;
    metric->max_value = 0.0;
    metric->sample_count = 0;
    metric->last_update = simple_monitoring_get_time_ms();
    
    mon->metric_count++;
    return 0;
}

// Обновление метрики
int simple_monitoring_update_metric(simple_monitoring_t *mon, const char *name, double value) {
    if (!mon || !name) return -1;
    
    // Поиск метрики
    for (int i = 0; i < mon->metric_count; i++) {
        if (simple_strncmp(mon->metrics[i].name, name, 32) == 0) {
            simple_metric_t *metric = &mon->metrics[i];
            metric->value = value;
            metric->sample_count++;
            mon->metrics_samples++;
            
            if (metric->sample_count == 1) {
                metric->min_value = value;
                metric->max_value = value;
            } else {
                if (value < metric->min_value) metric->min_value = value;
                if (value > metric->max_value) metric->max_value = value;
            }
            
            metric->last_update = simple_monitoring_get_time_ms();
            return 0;
        }
    }
    
    return -1;
}

// Инкремент метрики
int simple_monitoring_increment_metric(simple_monitoring_t *mon, const char *name, double increment) {
    if (!mon || !name) return -1;
    
    for (int i = 0; i < mon->metric_count; i++) {
        if (simple_strncmp(mon->metrics[i].name, name, 32) == 0) {
            return simple_monitoring_update_metric(mon, name, mon->metrics[i].value + increment);
        }
    }
    
    return -1;
}

// Получение метрики
simple_metric_t* simple_monitoring_get_metric(simple_monitoring_t *mon, const char *name) {
    if (!mon || !name) return 0;
    
    for (int i = 0; i < mon->metric_count; i++) {
        if (simple_strncmp(mon->metrics[i].name, name, 32) == 0) {
            return &mon->metrics[i];
        }
    }
    
    return 0;
}

// Сбор системных метрик
void simple_monitoring_collect_system_metrics(simple_monitoring_t *mon) {
    if (!mon) return;
    
    // Симуляция сбора метрик
    simple_monitoring_update_metric(mon, "cpu_usage", 45.5);
    simple_monitoring_update_metric(mon, "memory_usage", 65.2);
    simple_monitoring_update_metric(mon, "active_connections", 1250);
    simple_monitoring_update_metric(mon, "throughput_mbps", 150.5);
}

// Логирование
int simple_monitoring_log(simple_monitoring_t *mon, log_level_t level, const char *component, const char *message) {
    if (!mon || level > mon->log_level) return 0;
    
    if (mon->log_count >= MAX_SIMPLE_LOGS) {
        // Циклический буфер
        mon->log_head = (mon->log_head + 1) % MAX_SIMPLE_LOGS;
        mon->log_count = MAX_SIMPLE_LOGS - 1;
    }
    
    simple_log_entry_t *entry = &mon->log_buffer[(mon->log_head + mon->log_count) % MAX_SIMPLE_LOGS];
    entry->timestamp = simple_monitoring_get_time_ms();
    entry->level = level;
    
    // Копирование компонента
    if (component) {
        int i;
        for (i = 0; i < 15 && component[i] != '\0'; i++) {
            entry->component[i] = component[i];
        }
        entry->component[i] = '\0';
    }
    
    // Копирование сообщения
    if (message) {
        int i;
        for (i = 0; i < 127 && message[i] != '\0'; i++) {
            entry->message[i] = message[i];
        }
        entry->message[i] = '\0';
    }
    
    mon->log_count++;
    mon->total_logs++;
    
    return 0;
}

// Упрощенные функции логирования
int simple_monitoring_log_error(simple_monitoring_t *mon, const char *component, const char *message) {
    return simple_monitoring_log(mon, LOG_LEVEL_ERROR, component, message);
}

int simple_monitoring_log_warning(simple_monitoring_t *mon, const char *component, const char *message) {
    return simple_monitoring_log(mon, LOG_LEVEL_WARNING, component, message);
}

int simple_monitoring_log_info(simple_monitoring_t *mon, const char *component, const char *message) {
    return simple_monitoring_log(mon, LOG_LEVEL_INFO, component, message);
}

// Регистрация компонента
int simple_monitoring_register_component(simple_monitoring_t *mon, const char *name) {
    if (!mon || !name || mon->component_count >= MAX_COMPONENTS) {
        return -1;
    }
    
    simple_component_stats_t *comp = &mon->components[mon->component_count];
    
    // Копирование имени
    int i;
    for (i = 0; i < 15 && name[i] != '\0'; i++) {
        comp->name[i] = name[i];
    }
    comp->name[i] = '\0';
    
    comp->total_requests = 0;
    comp->successful_requests = 0;
    comp->failed_requests = 0;
    comp->avg_response_time_ms = 0.0;
    
    mon->component_count++;
    return 0;
}

// Обновление статистики компонента
int simple_monitoring_update_component_stats(simple_monitoring_t *mon, const char *name, 
                                           int success, double response_time) {
    if (!mon || !name) return -1;
    
    for (int i = 0; i < mon->component_count; i++) {
        if (simple_strncmp(mon->components[i].name, name, 16) == 0) {
            simple_component_stats_t *comp = &mon->components[i];
            comp->total_requests++;
            
            if (success) {
                comp->successful_requests++;
            } else {
                comp->failed_requests++;
            }
            
            // Обновление среднего времени ответа
            comp->avg_response_time_ms = 
                (comp->avg_response_time_ms * (comp->total_requests - 1) + response_time) / 
                comp->total_requests;
            
            return 0;
        }
    }
    
    return -1;
}

// Получение статистики компонента
simple_component_stats_t* simple_monitoring_get_component_stats(simple_monitoring_t *mon, const char *name) {
    if (!mon || !name) return 0;
    
    for (int i = 0; i < mon->component_count; i++) {
        if (simple_strncmp(mon->components[i].name, name, 16) == 0) {
            return &mon->components[i];
        }
    }
    
    return 0;
}

// Проверка алертов
void simple_monitoring_check_alerts(simple_monitoring_t *mon) {
    if (!mon) return;
    
    // Получение текущих значений (симуляция)
    double cpu_usage = 45.5;
    double memory_usage = 65.2;
    
    // Проверка CPU алерта
    if (cpu_usage > mon->cpu_threshold) {
        mon->cpu_alert = ALERT_STATUS_WARNING;
        if (cpu_usage > mon->cpu_threshold + 10.0) {
            mon->cpu_alert = ALERT_STATUS_CRITICAL;
        }
    } else {
        mon->cpu_alert = ALERT_STATUS_OK;
    }
    
    // Проверка Memory алерта
    if (memory_usage > mon->memory_threshold) {
        mon->memory_alert = ALERT_STATUS_WARNING;
        if (memory_usage > mon->memory_threshold + 10.0) {
            mon->memory_alert = ALERT_STATUS_CRITICAL;
        }
    } else {
        mon->memory_alert = ALERT_STATUS_OK;
    }
    
    if (mon->cpu_alert != ALERT_STATUS_OK || mon->memory_alert != ALERT_STATUS_OK) {
        mon->total_alerts++;
    }
}

// Получение статуса алертов
alert_status_t simple_monitoring_get_cpu_alert(simple_monitoring_t *mon) {
    return mon ? mon->cpu_alert : ALERT_STATUS_OK;
}

alert_status_t simple_monitoring_get_memory_alert(simple_monitoring_t *mon) {
    return mon ? mon->memory_alert : ALERT_STATUS_OK;
}

// Установка порогов
void simple_monitoring_set_thresholds(simple_monitoring_t *mon, double cpu, double memory) {
    if (!mon) return;
    
    mon->cpu_threshold = cpu;
    mon->memory_threshold = memory;
}

// Получение отчетов
void simple_monitoring_get_report(simple_monitoring_t *mon, char *buffer, size_t buffer_size) {
    if (!mon || !buffer || buffer_size < 100) return;
    
    // Простое форматирование
    simple_strncpy(buffer, "Monitoring Report: Metrics=", 100);
    // Добавление чисел (в реальной реализации)
}

void simple_monitoring_get_metrics_report(simple_monitoring_t *mon, char *buffer, size_t buffer_size) {
    if (!mon || !buffer || buffer_size < 50) return;
    
    simple_strncpy(buffer, "Metrics Report", 50);
}

void simple_monitoring_get_alerts_report(simple_monitoring_t *mon, char *buffer, size_t buffer_size) {
    if (!mon || !buffer || buffer_size < 50) return;
    
    simple_strncpy(buffer, "Alerts Report", 50);
}

// Сброс статистики
void simple_monitoring_reset_stats(simple_monitoring_t *mon) {
    if (!mon) return;
    
    mon->total_logs = 0;
    mon->total_alerts = 0;
    mon->metrics_samples = 0;
    
    // Сброс метрик
    for (int i = 0; i < mon->metric_count; i++) {
        mon->metrics[i].value = 0.0;
        mon->metrics[i].sample_count = 0;
        mon->metrics[i].min_value = 0.0;
        mon->metrics[i].max_value = 0.0;
    }
    
    // Сброс компонентов
    for (int i = 0; i < mon->component_count; i++) {
        mon->components[i].total_requests = 0;
        mon->components[i].successful_requests = 0;
        mon->components[i].failed_requests = 0;
        mon->components[i].avg_response_time_ms = 0.0;
    }
}

// Получение времени
int64_t simple_monitoring_get_time_ms(void) {
    static int64_t base_time = 1700000000000LL;
    static int64_t counter = 0;
    counter += 100; // 100ms increments
    return base_time + counter;
}

// Утилиты

const char* simple_monitoring_level_to_string(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_NONE: return "NONE";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_WARNING: return "WARNING";
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        default: return "UNKNOWN";
    }
}

const char* simple_monitoring_alert_to_string(alert_status_t status) {
    switch (status) {
        case ALERT_STATUS_OK: return "OK";
        case ALERT_STATUS_WARNING: return "WARNING";
        case ALERT_STATUS_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

// Вспомогательные функции

static int simple_strncmp(const char *s1, const char *s2, size_t n) {
    if (!s1 || !s2) return -1;
    
    for (size_t i = 0; i < n && s1[i] && s2[i]; i++) {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
    }
    return 0;
}

static char* simple_strncpy(char *dest, const char *src, size_t n) {
    if (!dest || !src || n == 0) return dest;
    
    size_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return dest;
}