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

#include "common/runtime-tuner.h"
#include "common/kprintf.h"
#include "common/common-stats.h"
#include "common/config-manager.h"

// Runtime tuner statistics
struct runtime_tuner_stats {
    long long total_tuning_operations;
    long long successful_tunings;
    long long failed_tunings;
    long long auto_tunings;
    long long manual_tunings;
    long long performance_improvements;
    long long rollback_operations;
};

static struct runtime_tuner_stats tuner_stats = {0};

// Tuning parameter structure
struct tuning_parameter {
    char name[128];
    char description[256];
    void *current_value_ptr;
    enum config_param_type type;
    double min_value;
    double max_value;
    double step_size;
    double current_value;
    double optimal_value;
    double baseline_value;
    int is_tunable;
    int auto_tune_enabled;
    time_t last_tuned;
    int tuning_attempts;
    double performance_impact; // -1.0 to 1.0
};

// Tuning strategy
enum tuning_strategy {
    TUNING_STRATEGY_CONSERVATIVE = 0,
    TUNING_STRATEGY_AGGRESSIVE,
    TUNING_STRATEGY_ADAPTIVE,
    TUNING_STRATEGY_PREDICTIVE
};

// Performance metric
struct performance_metric {
    char name[64];
    double current_value;
    double baseline_value;
    double weight; // Вес метрики в общей оценке
    time_t timestamp;
    int is_degraded;
};

// Tuning context
struct tuning_context {
    struct tuning_parameter *parameters;
    int param_count;
    int param_capacity;
    struct performance_metric *metrics;
    int metric_count;
    int metric_capacity;
    enum tuning_strategy strategy;
    double performance_threshold;
    int auto_tuning_enabled;
    time_t last_tuning_cycle;
    int tuning_cycle_interval_seconds;
    pthread_mutex_t tuner_mutex;
    int tuner_initialized;
};

static struct tuning_context global_tuner_ctx = {0};

// Built-in tunable parameters
static const struct builtin_tunable_param {
    const char *name;
    const char *description;
    enum config_param_type type;
    double min_value;
    double max_value;
    double step_size;
    int auto_tune_default;
} builtin_tunable_params[] = {
    {"network.buffer_size", "Network buffer size in bytes", CONFIG_TYPE_INT, 1024, 65536, 1024, 1},
    {"network.connection_timeout", "Connection timeout in seconds", CONFIG_TYPE_INT, 1, 300, 5, 1},
    {"network.max_connections", "Maximum concurrent connections", CONFIG_TYPE_INT, 10, 10000, 50, 1},
    {"performance.thread_pool_size", "Number of worker threads", CONFIG_TYPE_INT, 1, 128, 1, 1},
    {"performance.cache_size", "Cache size in MB", CONFIG_TYPE_INT, 1, 1024, 10, 1},
    {"security.encryption_level", "Encryption security level", CONFIG_TYPE_INT, 1, 3, 1, 0},
    {"monitoring.log_level", "Logging verbosity level", CONFIG_TYPE_INT, 0, 4, 1, 0}
};

#define BUILTIN_TUNABLE_COUNT (sizeof(builtin_tunable_params) / sizeof(builtin_tunable_params[0]))

// Инициализация runtime tuner
int runtime_tuner_init(enum tuning_strategy strategy) {
    if (global_tuner_ctx.tuner_initialized) {
        return 0; // Уже инициализирован
    }
    
    pthread_mutex_init(&global_tuner_ctx.tuner_mutex, NULL);
    
    // Инициализация параметров
    global_tuner_ctx.param_capacity = BUILTIN_TUNABLE_COUNT + 20;
    global_tuner_ctx.parameters = calloc(global_tuner_ctx.param_capacity, 
                                        sizeof(struct tuning_parameter));
    if (!global_tuner_ctx.parameters) {
        pthread_mutex_destroy(&global_tuner_ctx.tuner_mutex);
        return -1;
    }
    
    // Инициализация метрик
    global_tuner_ctx.metric_capacity = 20;
    global_tuner_ctx.metrics = calloc(global_tuner_ctx.metric_capacity, 
                                     sizeof(struct performance_metric));
    if (!global_tuner_ctx.metrics) {
        free(global_tuner_ctx.parameters);
        pthread_mutex_destroy(&global_tuner_ctx.tuner_mutex);
        return -1;
    }
    
    global_tuner_ctx.strategy = strategy;
    global_tuner_ctx.performance_threshold = 0.1; // 10% улучшение
    global_tuner_ctx.auto_tuning_enabled = 1;
    global_tuner_ctx.tuning_cycle_interval_seconds = 60;
    global_tuner_ctx.last_tuning_cycle = time(NULL);
    global_tuner_ctx.tuner_initialized = 1;
    
    // Регистрация builtin параметров
    for (int i = 0; i < BUILTIN_TUNABLE_COUNT; i++) {
        const struct builtin_tunable_param *param = &builtin_tunable_params[i];
        runtime_tuner_register_parameter(
            param->name,
            param->description,
            param->type,
            param->min_value,
            param->max_value,
            param->step_size,
            param->auto_tune_default
        );
    }
    
    vkprintf(1, "Runtime tuner initialized with strategy %d and %d builtin parameters\n", 
             strategy, BUILTIN_TUNABLE_COUNT);
    
    return 0;
}

// Регистрация tunable параметра
int runtime_tuner_register_parameter(
    const char *name,
    const char *description,
    enum config_param_type type,
    double min_value,
    double max_value,
    double step_size,
    int auto_tune_enabled) {
    
    pthread_mutex_lock(&global_tuner_ctx.tuner_mutex);
    
    // Проверка существования
    for (int i = 0; i < global_tuner_ctx.param_count; i++) {
        if (strcmp(global_tuner_ctx.parameters[i].name, name) == 0) {
            pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
            return -1; // Параметр уже существует
        }
    }
    
    // Расширение массива если нужно
    if (global_tuner_ctx.param_count >= global_tuner_ctx.param_capacity) {
        int new_capacity = global_tuner_ctx.param_capacity * 2;
        struct tuning_parameter *new_params = realloc(global_tuner_ctx.parameters,
                                                     new_capacity * sizeof(struct tuning_parameter));
        if (!new_params) {
            pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
            return -1;
        }
        global_tuner_ctx.parameters = new_params;
        global_tuner_ctx.param_capacity = new_capacity;
    }
    
    // Создание нового параметра
    struct tuning_parameter *new_param = &global_tuner_ctx.parameters[global_tuner_ctx.param_count];
    strncpy(new_param->name, name, sizeof(new_param->name) - 1);
    if (description) {
        strncpy(new_param->description, description, sizeof(new_param->description) - 1);
    }
    new_param->type = type;
    new_param->min_value = min_value;
    new_param->max_value = max_value;
    new_param->step_size = step_size;
    new_param->is_tunable = 1;
    new_param->auto_tune_enabled = auto_tune_enabled;
    new_param->last_tuned = time(NULL);
    new_param->tuning_attempts = 0;
    new_param->performance_impact = 0.0;
    new_param->current_value_ptr = NULL; // Будет установлен позже
    
    // Получение текущего значения из config manager
    // TODO: Интеграция с config manager
    new_param->current_value = min_value;
    new_param->baseline_value = min_value;
    new_param->optimal_value = min_value;
    
    global_tuner_ctx.param_count++;
    pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
    
    vkprintf(2, "Registered tunable parameter: %s (range: %.2f-%.2f)\n", 
             name, min_value, max_value);
    return 0;
}

// Регистрация performance метрики
int runtime_tuner_register_metric(
    const char *name,
    double weight,
    double baseline_value) {
    
    pthread_mutex_lock(&global_tuner_ctx.tuner_mutex);
    
    // Проверка существования
    for (int i = 0; i < global_tuner_ctx.metric_count; i++) {
        if (strcmp(global_tuner_ctx.metrics[i].name, name) == 0) {
            pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
            return -1; // Метрика уже существует
        }
    }
    
    // Расширение массива если нужно
    if (global_tuner_ctx.metric_count >= global_tuner_ctx.metric_capacity) {
        int new_capacity = global_tuner_ctx.metric_capacity * 2;
        struct performance_metric *new_metrics = realloc(global_tuner_ctx.metrics,
                                                        new_capacity * sizeof(struct performance_metric));
        if (!new_metrics) {
            pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
            return -1;
        }
        global_tuner_ctx.metrics = new_metrics;
        global_tuner_ctx.metric_capacity = new_capacity;
    }
    
    // Создание новой метрики
    struct performance_metric *new_metric = &global_tuner_ctx.metrics[global_tuner_ctx.metric_count];
    strncpy(new_metric->name, name, sizeof(new_metric->name) - 1);
    new_metric->weight = weight;
    new_metric->baseline_value = baseline_value;
    new_metric->current_value = baseline_value;
    new_metric->timestamp = time(NULL);
    new_metric->is_degraded = 0;
    
    global_tuner_ctx.metric_count++;
    pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
    
    vkprintf(2, "Registered performance metric: %s (weight: %.2f)\n", name, weight);
    return 0;
}

// Обновление значения метрики
int runtime_tuner_update_metric(const char *name, double value) {
    pthread_mutex_lock(&global_tuner_ctx.tuner_mutex);
    
    struct performance_metric *metric = NULL;
    for (int i = 0; i < global_tuner_ctx.metric_count; i++) {
        if (strcmp(global_tuner_ctx.metrics[i].name, name) == 0) {
            metric = &global_tuner_ctx.metrics[i];
            break;
        }
    }
    
    if (!metric) {
        pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
        return -1;
    }
    
    // Обновление значения
    metric->current_value = value;
    metric->timestamp = time(NULL);
    
    // Проверка деградации
    double degradation = (metric->baseline_value - value) / metric->baseline_value;
    metric->is_degraded = (degradation > 0.1); // 10% деградация
    
    pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
    return 0;
}

// Выполнение цикла автоматической настройки
int runtime_tuner_run_auto_tuning(void) {
    if (!global_tuner_ctx.auto_tuning_enabled) {
        return 0;
    }
    
    time_t now = time(NULL);
    if (now - global_tuner_ctx.last_tuning_cycle < global_tuner_ctx.tuning_cycle_interval_seconds) {
        return 0; // Слишком рано для нового цикла
    }
    
    pthread_mutex_lock(&global_tuner_ctx.tuner_mutex);
    
    vkprintf(2, "Starting auto-tuning cycle with %d parameters and %d metrics\n",
             global_tuner_ctx.param_count, global_tuner_ctx.metric_count);
    
    // Оценка текущей производительности
    double current_performance = runtime_tuner_evaluate_performance();
    
    // Для каждого tunable параметра
    for (int i = 0; i < global_tuner_ctx.param_count; i++) {
        struct tuning_parameter *param = &global_tuner_ctx.parameters[i];
        
        if (!param->auto_tune_enabled || !param->is_tunable) {
            continue;
        }
        
        // Пробуем изменить параметр
        if (runtime_tuner_try_parameter_change(param, current_performance)) {
            tuner_stats.successful_tunings++;
        } else {
            tuner_stats.failed_tunings++;
        }
        
        tuner_stats.auto_tunings++;
    }
    
    global_tuner_ctx.last_tuning_cycle = now;
    tuner_stats.total_tuning_operations++;
    
    pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
    
    vkprintf(2, "Auto-tuning cycle completed. Performance: %.2f\n", current_performance);
    return 0;
}

// Оценка общей производительности
static double runtime_tuner_evaluate_performance(void) {
    double total_weighted_score = 0.0;
    double total_weight = 0.0;
    
    for (int i = 0; i < global_tuner_ctx.metric_count; i++) {
        struct performance_metric *metric = &global_tuner_ctx.metrics[i];
        
        // Нормализация значения метрики (0.0 - 1.0, где 1.0 - оптимально)
        double normalized_value;
        if (metric->baseline_value > 0) {
            normalized_value = metric->current_value / metric->baseline_value;
        } else {
            normalized_value = 1.0; // Если baseline 0, считаем оптимальным текущее значение
        }
        
        // Ограничиваем диапазон
        if (normalized_value > 2.0) normalized_value = 2.0;
        if (normalized_value < 0.0) normalized_value = 0.0;
        
        // Для метрик, где меньше - лучше (например, latency)
        if (strstr(metric->name, "latency") || strstr(metric->name, "timeout")) {
            normalized_value = 2.0 - normalized_value;
        }
        
        // Применяем вес
        double weighted_score = normalized_value * metric->weight;
        total_weighted_score += weighted_score;
        total_weight += metric->weight;
    }
    
    return total_weight > 0 ? (total_weighted_score / total_weight) : 0.0;
}

// Попытка изменения параметра
static int runtime_tuner_try_parameter_change(
    struct tuning_parameter *param,
    double current_performance) {
    
    // Определяем направление изменения
    double direction = (param->current_value < param->optimal_value) ? 1.0 : -1.0;
    double new_value = param->current_value + (direction * param->step_size);
    
    // Проверка границ
    if (new_value < param->min_value) {
        new_value = param->min_value;
    }
    if (new_value > param->max_value) {
        new_value = param->max_value;
    }
    
    // Если значение не изменилось, пропускаем
    if (fabs(new_value - param->current_value) < 0.001) {
        return 0;
    }
    
    // Применяем новое значение
    if (runtime_tuner_apply_parameter_change(param, new_value)) {
        param->tuning_attempts++;
        param->last_tuned = time(NULL);
        
        // Оцениваем влияние на производительность
        double new_performance = runtime_tuner_evaluate_performance();
        param->performance_impact = new_performance - current_performance;
        
        vkprintf(3, "Parameter %s tuned: %.2f -> %.2f (impact: %.4f)\n",
                 param->name, param->current_value, new_value, param->performance_impact);
        
        // Если улучшение значительное, обновляем оптимальное значение
        if (param->performance_impact > global_tuner_ctx.performance_threshold) {
            param->optimal_value = new_value;
            tuner_stats.performance_improvements++;
            return 1;
        }
        
        // Если ухудшение, откатываем изменения
        if (param->performance_impact < -global_tuner_ctx.performance_threshold) {
            runtime_tuner_rollback_parameter(param);
            tuner_stats.rollback_operations++;
            return 0;
        }
        
        // Нейтральное изменение - сохраняем новое значение
        param->current_value = new_value;
        return 1;
    }
    
    return 0;
}

// Применение изменения параметра
static int runtime_tuner_apply_parameter_change(
    struct tuning_parameter *param,
    double new_value) {
    
    // TODO: Интеграция с соответствующими subsystems
    // Например, для network.buffer_size нужно обновить буферы сети
    
    switch (param->type) {
        case CONFIG_TYPE_INT: {
            int int_val = (int)new_value;
            // config_manager_set_parameter("network", param->name, &int_val, sizeof(int));
            break;
        }
        case CONFIG_TYPE_DOUBLE: {
            // config_manager_set_parameter("network", param->name, &new_value, sizeof(double));
            break;
        }
        default:
            return 0;
    }
    
    param->current_value = new_value;
    return 1;
}

// Откат изменений параметра
static int runtime_tuner_rollback_parameter(struct tuning_parameter *param) {
    // Восстанавливаем предыдущее значение
    // TODO: Реализовать механизм сохранения предыдущих значений
    
    vkprintf(2, "Rolling back parameter: %s\n", param->name);
    return 1;
}

// Ручная настройка параметра
int runtime_tuner_manual_tune(const char *param_name, double target_value) {
    pthread_mutex_lock(&global_tuner_ctx.tuner_mutex);
    
    struct tuning_parameter *param = NULL;
    for (int i = 0; i < global_tuner_ctx.param_count; i++) {
        if (strcmp(global_tuner_ctx.parameters[i].name, param_name) == 0) {
            param = &global_tuner_ctx.parameters[i];
            break;
        }
    }
    
    if (!param) {
        pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
        return -1;
    }
    
    if (!param->is_tunable) {
        pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
        return -1;
    }
    
    // Проверка границ
    if (target_value < param->min_value || target_value > param->max_value) {
        pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
        return -1;
    }
    
    // Применяем изменение
    double old_value = param->current_value;
    if (runtime_tuner_apply_parameter_change(param, target_value)) {
        param->last_tuned = time(NULL);
        tuner_stats.manual_tunings++;
        tuner_stats.total_tuning_operations++;
        
        vkprintf(1, "Manual tuning: %s %.2f -> %.2f\n", 
                 param_name, old_value, target_value);
        pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
        return 0;
    }
    
    pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
    return -1;
}

// Получение статистики tuner
void runtime_tuner_get_stats(struct runtime_tuner_stats *stats) {
    if (stats) {
        memcpy(stats, &tuner_stats, sizeof(struct runtime_tuner_stats));
    }
}

// Вывод статистики
void runtime_tuner_print_stats(void) {
    vkprintf(1, "Runtime Tuner Statistics:\n");
    vkprintf(1, "  Total Tuning Operations: %lld\n", tuner_stats.total_tuning_operations);
    vkprintf(1, "  Successful Tunings: %lld\n", tuner_stats.successful_tunings);
    vkprintf(1, "  Failed Tunings: %lld\n", tuner_stats.failed_tunings);
    vkprintf(1, "  Auto Tunings: %lld\n", tuner_stats.auto_tunings);
    vkprintf(1, "  Manual Tunings: %lld\n", tuner_stats.manual_tunings);
    vkprintf(1, "  Performance Improvements: %lld\n", tuner_stats.performance_improvements);
    vkprintf(1, "  Rollback Operations: %lld\n", tuner_stats.rollback_operations);
    
    pthread_mutex_lock(&global_tuner_ctx.tuner_mutex);
    vkprintf(1, "  Tunable Parameters: %d\n", global_tuner_ctx.param_count);
    vkprintf(1, "  Performance Metrics: %d\n", global_tuner_ctx.metric_count);
    vkprintf(1, "  Auto-tuning: %s\n", global_tuner_ctx.auto_tuning_enabled ? "Enabled" : "Disabled");
    vkprintf(1, "  Strategy: %d\n", global_tuner_ctx.strategy);
    pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
}

// Очистка tuner
void runtime_tuner_cleanup(void) {
    pthread_mutex_lock(&global_tuner_ctx.tuner_mutex);
    
    if (global_tuner_ctx.parameters) {
        free(global_tuner_ctx.parameters);
    }
    
    if (global_tuner_ctx.metrics) {
        free(global_tuner_ctx.metrics);
    }
    
    pthread_mutex_unlock(&global_tuner_ctx.tuner_mutex);
    pthread_mutex_destroy(&global_tuner_ctx.tuner_mutex);
    
    memset(&tuner_stats, 0, sizeof(tuner_stats));
    memset(&global_tuner_ctx, 0, sizeof(global_tuner_ctx));
    
    vkprintf(1, "Runtime tuner cleaned up\n");
}