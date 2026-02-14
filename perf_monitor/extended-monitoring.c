#include "extended-monitoring.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/time.h>
    #include <pthread.h>
#endif

// Получение текущего времени
time_t monitoring_get_current_time(void) {
    return time(NULL);
}

// Инициализация мониторинга
extended_monitoring_t* monitoring_init(int max_metrics, int max_alerts, int event_buffer_size) {
    extended_monitoring_t *monitoring = calloc(1, sizeof(extended_monitoring_t));
    if (!monitoring) {
        return NULL;
    }
    
    // Инициализация массивов
    monitoring->metrics = calloc(max_metrics, sizeof(metric_t));
    monitoring->alert_rules = calloc(max_alerts, sizeof(alert_rule_t));
    monitoring->event_buffer = calloc(event_buffer_size, sizeof(monitoring_event_t));
    monitoring->component_stats = calloc(64, sizeof(component_stats_t));
    
    if (!monitoring->metrics || !monitoring->alert_rules || 
        !monitoring->event_buffer || !monitoring->component_stats) {
        monitoring_cleanup(monitoring);
        return NULL;
    }
    
    monitoring->max_metrics = max_metrics;
    monitoring->max_alerts = max_alerts;
    monitoring->event_buffer_size = event_buffer_size;
    monitoring->max_components = 64;
    
    // Конфигурация по умолчанию
    monitoring->config.enable_auto_alerting = 1;
    monitoring->config.enable_performance_monitoring = 1;
    monitoring->config.enable_security_monitoring = 1;
    monitoring->config.enable_resource_monitoring = 1;
    monitoring->config.sampling_rate = 1.0;
    monitoring->config.retention_days = 7;
    
    // Экспорт по умолчанию
    monitoring->exporters.enable_prometheus_export = 0;
    monitoring->exporters.prometheus_port = 9090;
    monitoring->exporters.enable_json_export = 1;
    strcpy(monitoring->exporters.json_file_path, "/tmp/mtproxy_metrics.json");
    
    monitoring->is_initialized = 1;
    monitoring->is_running = 0;
    monitoring->start_time = monitoring_get_current_time();
    monitoring->last_collection_time = monitoring->start_time;
    
    return monitoring;
}

// Регистрация метрики
metric_t* monitoring_register_metric(extended_monitoring_t *monitoring,
                                   const char *name, 
                                   const char *description,
                                   const char *labels,
                                   metric_type_t type) {
    if (!monitoring || monitoring->metric_count >= monitoring->max_metrics) {
        return NULL;
    }
    
    metric_t *metric = &monitoring->metrics[monitoring->metric_count];
    
    strncpy(metric->name, name, sizeof(metric->name) - 1);
    strncpy(metric->description, description, sizeof(metric->description) - 1);
    if (labels) {
        strncpy(metric->labels, labels, sizeof(metric->labels) - 1);
    }
    metric->type = type;
    metric->value = 0.0;
    metric->min_value = 0.0;
    metric->max_value = 0.0;
    metric->sum = 0.0;
    metric->count = 0;
    metric->last_update = monitoring_get_current_time();
    metric->update_count = 0;
    metric->rate_per_second = 0.0;
    
    monitoring->metric_count++;
    return metric;
}

// Обновление метрики
int monitoring_update_metric(extended_monitoring_t *monitoring, 
                           const char *name,
                           double value) {
    if (!monitoring || !name) return -1;
    
    // Поиск метрики
    metric_t *metric = NULL;
    for (int i = 0; i < monitoring->metric_count; i++) {
        if (strcmp(monitoring->metrics[i].name, name) == 0) {
            metric = &monitoring->metrics[i];
            break;
        }
    }
    
    if (!metric) return -1;
    
    // Обновление значений
    metric->value = value;
    metric->sum += value;
    metric->count++;
    metric->update_count++;
    
    if (metric->count == 1) {
        metric->min_value = value;
        metric->max_value = value;
    } else {
        if (value < metric->min_value) metric->min_value = value;
        if (value > metric->max_value) metric->max_value = value;
    }
    
    // Расчет rate
    time_t current_time = monitoring_get_current_time();
    double time_diff = difftime(current_time, metric->last_update);
    if (time_diff > 0) {
        metric->rate_per_second = metric->update_count / time_diff;
    }
    metric->last_update = current_time;
    
    monitoring->stats.total_metrics_collected++;
    return 0;
}

// Инкремент метрики
int monitoring_increment_metric(extended_monitoring_t *monitoring,
                              const char *name,
                              double increment) {
    if (!monitoring || !name) return -1;
    
    double current_value = monitoring_get_metric_value(monitoring, name);
    return monitoring_update_metric(monitoring, name, current_value + increment);
}

// Получение значения метрики
double monitoring_get_metric_value(extended_monitoring_t *monitoring,
                                 const char *name) {
    if (!monitoring || !name) return 0.0;
    
    for (int i = 0; i < monitoring->metric_count; i++) {
        if (strcmp(monitoring->metrics[i].name, name) == 0) {
            return monitoring->metrics[i].value;
        }
    }
    return 0.0;
}

// Создание правила алерта
alert_rule_t* monitoring_create_alert_rule(extended_monitoring_t *monitoring,
                                         const char *name,
                                         const char *metric_name,
                                         alert_type_t type,
                                         alert_level_t level) {
    if (!monitoring || monitoring->alert_count >= monitoring->max_alerts) {
        return NULL;
    }
    
    alert_rule_t *rule = &monitoring->alert_rules[monitoring->alert_count];
    
    strncpy(rule->name, name, sizeof(rule->name) - 1);
    strncpy(rule->metric_name, metric_name, sizeof(rule->metric_name) - 1);
    rule->type = type;
    rule->level = level;
    rule->threshold_value = 0.0;
    rule->warning_threshold = 0.0;
    rule->critical_threshold = 0.0;
    rule->evaluation_period_seconds = 60;
    rule->cooldown_period_seconds = 300;
    rule->last_triggered = 0;
    rule->trigger_count = 0;
    rule->is_active = 0;
    
    monitoring->alert_count++;
    return rule;
}

// Обновление порога алерта
int monitoring_update_alert_threshold(extended_monitoring_t *monitoring,
                                    const char *alert_name,
                                    double threshold) {
    if (!monitoring || !alert_name) return -1;
    
    for (int i = 0; i < monitoring->alert_count; i++) {
        if (strcmp(monitoring->alert_rules[i].name, alert_name) == 0) {
            monitoring->alert_rules[i].threshold_value = threshold;
            return 0;
        }
    }
    return -1;
}

// Проверка алертов
int monitoring_check_alerts(extended_monitoring_t *monitoring) {
    if (!monitoring || !monitoring->config.enable_auto_alerting) {
        return 0;
    }
    
    time_t current_time = monitoring_get_current_time();
    int triggered_count = 0;
    
    for (int i = 0; i < monitoring->alert_count; i++) {
        alert_rule_t *rule = &monitoring->alert_rules[i];
        
        // Проверка cooldown периода
        if (difftime(current_time, rule->last_triggered) < rule->cooldown_period_seconds) {
            continue;
        }
        
        double metric_value = monitoring_get_metric_value(monitoring, rule->metric_name);
        
        // Проверка порога
        if (metric_value >= rule->threshold_value) {
            rule->is_active = 1;
            rule->last_triggered = current_time;
            rule->trigger_count++;
            triggered_count++;
            
            // Логирование события
            char message[256];
            snprintf(message, sizeof(message), 
                    "Alert triggered: %s (value: %.2f, threshold: %.2f)",
                    rule->name, metric_value, rule->threshold_value);
            
            monitoring_log_event(monitoring, "monitoring", rule->level, 
                               message, rule->metric_name);
        }
    }
    
    monitoring->stats.total_alerts_triggered += triggered_count;
    return triggered_count;
}

// Логирование события
int monitoring_log_event(extended_monitoring_t *monitoring,
                        const char *component,
                        alert_level_t level,
                        const char *message,
                        const char *details) {
    if (!monitoring || monitoring->event_count >= monitoring->event_buffer_size) {
        monitoring->stats.dropped_events++;
        return -1;
    }
    
    monitoring_event_t *event = &monitoring->event_buffer[monitoring->event_head];
    
    event->timestamp = monitoring_get_current_time();
    strncpy(event->component, component, sizeof(event->component) - 1);
    strncpy(event->message, message, sizeof(event->message) - 1);
    event->level = level;
    if (details) {
        strncpy(event->details, details, sizeof(event->details) - 1);
    }
    
    monitoring->event_head = (monitoring->event_head + 1) % monitoring->event_buffer_size;
    if (monitoring->event_count < monitoring->event_buffer_size) {
        monitoring->event_count++;
    } else {
        monitoring->event_tail = (monitoring->event_tail + 1) % monitoring->event_buffer_size;
    }
    
    monitoring->stats.total_events_logged++;
    return 0;
}

// Сбор системных метрик
int monitoring_collect_system_metrics(extended_monitoring_t *monitoring) {
    if (!monitoring || !monitoring->config.enable_resource_monitoring) {
        return 0;
    }
    
#ifdef _WIN32
    // Windows системные метрики
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    monitoring_update_metric(monitoring, "system_memory_total_bytes", 
                           memInfo.ullTotalPhys);
    monitoring_update_metric(monitoring, "system_memory_available_bytes", 
                           memInfo.ullAvailPhys);
    
    // CPU usage (приблизительно)
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER idle, kernel, user;
        idle.HighPart = idleTime.dwHighDateTime;
        idle.LowPart = idleTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        kernel.LowPart = kernelTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        
        double cpu_usage = ((double)(kernel.QuadPart + user.QuadPart - idle.QuadPart) / 
                           (kernel.QuadPart + user.QuadPart)) * 100.0;
        monitoring_update_metric(monitoring, "system_cpu_usage_percent", cpu_usage);
    }
#else
    // Linux/Unix системные метрики
    FILE *file = fopen("/proc/meminfo", "r");
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                unsigned long mem_total;
                sscanf(line, "MemTotal: %lu kB", &mem_total);
                monitoring_update_metric(monitoring, "system_memory_total_bytes", 
                                       mem_total * 1024);
            } else if (strncmp(line, "MemAvailable:", 13) == 0) {
                unsigned long mem_available;
                sscanf(line, "MemAvailable: %lu kB", &mem_available);
                monitoring_update_metric(monitoring, "system_memory_available_bytes", 
                                       mem_available * 1024);
            }
        }
        fclose(file);
    }
    
    // CPU usage
    file = fopen("/proc/stat", "r");
    if (file) {
        char line[256];
        if (fgets(line, sizeof(line), file)) {
            unsigned long user, nice, system, idle;
            sscanf(line, "cpu %lu %lu %lu %lu", &user, &nice, &system, &idle);
            unsigned long total = user + nice + system + idle;
            double cpu_usage = ((double)(user + nice + system) / total) * 100.0;
            monitoring_update_metric(monitoring, "system_cpu_usage_percent", cpu_usage);
        }
        fclose(file);
    }
#endif
    
    return 0;
}

// Сбор метрик процесса
int monitoring_collect_process_metrics(extended_monitoring_t *monitoring) {
    if (!monitoring) return 0;
    
#ifdef _WIN32
    HANDLE process = GetCurrentProcess();
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(process, &pmc, sizeof(pmc))) {
        monitoring_update_metric(monitoring, "process_memory_working_set_bytes", 
                               pmc.WorkingSetSize);
        monitoring_update_metric(monitoring, "process_memory_pagefile_bytes", 
                               pmc.PagefileUsage);
    }
    
    // CPU время процесса
    FILETIME creation_time, exit_time, kernel_time, user_time;
    if (GetProcessTimes(process, &creation_time, &exit_time, &kernel_time, &user_time)) {
        ULARGE_INTEGER kernel, user;
        kernel.HighPart = kernel_time.dwHighDateTime;
        kernel.LowPart = kernel_time.dwLowDateTime;
        user.HighPart = user_time.dwHighDateTime;
        user.LowPart = user_time.dwLowDateTime;
        
        double cpu_time = (double)(kernel.QuadPart + user.QuadPart) / 10000000.0; // В секундах
        monitoring_update_metric(monitoring, "process_cpu_time_seconds", cpu_time);
    }
#else
    // Linux процесс метрики
    char stat_path[64];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", getpid());
    
    FILE *file = fopen(stat_path, "r");
    if (file) {
        char line[1024];
        if (fgets(line, sizeof(line), file)) {
            unsigned long utime, stime;
            // Извлекаем 14 и 15 поля (user time и system time)
            char *token = strtok(line, " ");
            for (int i = 1; i < 14 && token; i++) {
                token = strtok(NULL, " ");
            }
            if (token) {
                utime = strtoul(token, NULL, 10);
                token = strtok(NULL, " ");
                if (token) {
                    stime = strtoul(token, NULL, 10);
                    double cpu_time = (double)(utime + stime) / sysconf(_SC_CLK_TCK);
                    monitoring_update_metric(monitoring, "process_cpu_time_seconds", cpu_time);
                }
            }
        }
        fclose(file);
    }
    
    // Память процесса
    char status_path[64];
    snprintf(status_path, sizeof(status_path), "/proc/%d/status", getpid());
    
    file = fopen(status_path, "r");
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                unsigned long vm_rss;
                sscanf(line, "VmRSS: %lu kB", &vm_rss);
                monitoring_update_metric(monitoring, "process_memory_rss_bytes", 
                                       vm_rss * 1024);
            } else if (strncmp(line, "VmSize:", 7) == 0) {
                unsigned long vm_size;
                sscanf(line, "VmSize: %lu kB", &vm_size);
                monitoring_update_metric(monitoring, "process_memory_vms_bytes", 
                                       vm_size * 1024);
            }
        }
        fclose(file);
    }
#endif
    
    return 0;
}

// Обновление статистики компонента
int monitoring_update_component_stats(extended_monitoring_t *monitoring,
                                     const char *component_name,
                                     int success,
                                     double response_time_ms) {
    if (!monitoring || !component_name) return -1;
    
    // Поиск или создание статистики компонента
    component_stats_t *stats = NULL;
    for (int i = 0; i < monitoring->component_count; i++) {
        if (strcmp(monitoring->component_stats[i].name, component_name) == 0) {
            stats = &monitoring->component_stats[i];
            break;
        }
    }
    
    if (!stats && monitoring->component_count < monitoring->max_components) {
        stats = &monitoring->component_stats[monitoring->component_count];
        strncpy(stats->name, component_name, sizeof(stats->name) - 1);
        monitoring->component_count++;
    }
    
    if (!stats) return -1;
    
    // Обновление статистики
    stats->total_requests++;
    if (success) {
        stats->successful_requests++;
    } else {
        stats->failed_requests++;
    }
    
    // Обновление среднего времени ответа (экспоненциальное сглаживание)
    if (stats->total_requests == 1) {
        stats->avg_response_time_ms = response_time_ms;
    } else {
        stats->avg_response_time_ms = 
            (stats->avg_response_time_ms * 0.9) + (response_time_ms * 0.1);
    }
    
    // Расчет error rate
    if (stats->total_requests > 0) {
        stats->error_rate_percent = 
            (double)stats->failed_requests * 100.0 / stats->total_requests;
    }
    
    // Расчет throughput
    time_t current_time = monitoring_get_current_time();
    double time_diff = difftime(current_time, stats->last_activity);
    if (time_diff > 0) {
        stats->throughput_rps = 1.0 / time_diff;
    }
    stats->last_activity = current_time;
    
    return 0;
}

// Печать статистики
void monitoring_print_stats(extended_monitoring_t *monitoring) {
    if (!monitoring) return;
    
    printf("=== Extended Monitoring Statistics ===\n");
    printf("Total metrics collected: %llu\n", monitoring->stats.total_metrics_collected);
    printf("Total alerts triggered: %llu\n", monitoring->stats.total_alerts_triggered);
    printf("Total events logged: %llu\n", monitoring->stats.total_events_logged);
    printf("Dropped events: %llu\n", monitoring->stats.dropped_events);
    printf("Active metrics: %d\n", monitoring->metric_count);
    printf("Active alerts: %d\n", monitoring->alert_count);
    printf("Active components: %d\n", monitoring->component_count);
    
    // Печать метрик
    printf("\nMetrics:\n");
    for (int i = 0; i < monitoring->metric_count; i++) {
        metric_t *metric = &monitoring->metrics[i];
        printf("  %s: %.2f (min: %.2f, max: %.2f, count: %llu)\n",
               metric->name, metric->value, metric->min_value, 
               metric->max_value, metric->count);
    }
    
    // Печать активных алертов
    printf("\nActive Alerts:\n");
    for (int i = 0; i < monitoring->alert_count; i++) {
        alert_rule_t *rule = &monitoring->alert_rules[i];
        if (rule->is_active) {
            printf("  %s: %s (triggered %llu times)\n", 
                   rule->name, rule->metric_name, rule->trigger_count);
        }
    }
    
    printf("=====================================\n");
}

// Расширенные функции мониторинга
int monitoring_add_custom_exporter(extended_monitoring_t *monitoring,
                                 const char *name,
                                 int (*export_func)(extended_monitoring_t*, const char*)) {
    if (!monitoring || !name || !export_func) return -1;
    
    // В реальной реализации здесь будет добавление пользовательского экспортера
    // Пока что просто возвращаем успех
    return 0;
}

int monitoring_set_dynamic_threshold(extended_monitoring_t *monitoring,
                                   const char *metric_name,
                                   double (*threshold_func)(void)) {
    if (!monitoring || !metric_name || !threshold_func) return -1;
    
    // Найти соответствующий алерт для обновления порога
    for (int i = 0; i < monitoring->alert_count; i++) {
        if (strcmp(monitoring->alert_rules[i].metric_name, metric_name) == 0) {
            // Установить динамический порог
            monitoring->alert_rules[i].threshold_value = threshold_func();
            return 0;
        }
    }
    return -1;
}

int monitoring_create_dashboard_endpoint(extended_monitoring_t *monitoring,
                                      int port) {
    if (!monitoring) return -1;
    
    // В реальной реализации создание HTTP endpoint для дашборда
    // Пока что просто возвращаем успех
    monitoring->exporters.prometheus_port = port;
    monitoring->exporters.enable_prometheus_export = 1;
    
    return 0;
}

int monitoring_export_for_grafana(extended_monitoring_t *monitoring,
                                 const char *output_file) {
    if (!monitoring || !output_file) return -1;
    
    // Создание JSON-файла с метриками в формате, подходящем для Grafana
    FILE *file = fopen(output_file, "w");
    if (!file) return -1;
    
    fprintf(file, "{\n");
    fprintf(file, "  \"timestamp\": %ld,\n", (long)monitoring_get_current_time());
    fprintf(file, "  \"uptime_seconds\": %.2f,\n", monitoring_get_uptime_seconds(monitoring));
    fprintf(file, "  \"metrics\": [\n");
    
    for (int i = 0; i < monitoring->metric_count; i++) {
        metric_t *metric = &monitoring->metrics[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"name\": \"%s\",\n", metric->name);
        fprintf(file, "      \"value\": %.2f,\n", metric->value);
        fprintf(file, "      \"type\": %d,\n", metric->type);
        fprintf(file, "      \"description\": \"%s\"\n", metric->description);
        fprintf(file, "    }%s\n", (i == monitoring->metric_count - 1) ? "" : ",");
    }
    
    fprintf(file, "  ],\n");
    fprintf(file, "  \"alerts\": [\n");
    
    for (int i = 0; i < monitoring->alert_count; i++) {
        alert_rule_t *rule = &monitoring->alert_rules[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"name\": \"%s\",\n", rule->name);
        fprintf(file, "      \"metric\": \"%s\",\n", rule->metric_name);
        fprintf(file, "      \"level\": %d,\n", rule->level);
        fprintf(file, "      \"threshold\": %.2f,\n", rule->threshold_value);
        fprintf(file, "      \"active\": %s\n", rule->is_active ? "true" : "false");
        fprintf(file, "    }%s\n", (i == monitoring->alert_count - 1) ? "" : ",");
    }
    
    fprintf(file, "  ]\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return 0;
}

void monitoring_set_retention_policy(extended_monitoring_t *monitoring,
                                   int days,
                                   int samples_per_day) {
    if (!monitoring) return;
    
    monitoring->config.retention_days = days;
    // В реальной системе здесь будет настройка политики хранения данных
    // В зависимости от количества дней и частоты сэмплирования
}

// Очистка мониторинга
void monitoring_cleanup(extended_monitoring_t *monitoring) {
    if (!monitoring) return;
    
    if (monitoring->metrics) free(monitoring->metrics);
    if (monitoring->alert_rules) free(monitoring->alert_rules);
    if (monitoring->event_buffer) free(monitoring->event_buffer);
    if (monitoring->component_stats) free(monitoring->component_stats);
    
    free(monitoring);
}