/*
    Реализация системы адаптивной самонастройки
*/

#include "adaptive-tuning.h"

// Глобальная система настройки
static adaptive_tuning_t *g_adaptive_tuning = 0;

// Вспомогательные функции
static int find_parameter_index(adaptive_tuning_t *tuning, const char *name);
static int find_metric_index(adaptive_tuning_t *tuning, const char *name);
static void collect_system_state(adaptive_tuning_t *tuning);
static double calculate_performance_delta(adaptive_tuning_t *tuning);
static int should_make_adjustment(adaptive_tuning_t *tuning);

// Инициализация
adaptive_tuning_t* adaptive_tuning_init(adaptation_strategy_t strategy) {
    adaptive_tuning_t *tuning = (adaptive_tuning_t*)0x70000000;
    if (!tuning) {
        return 0;
    }
    
    // Обнуление структуры
    for (int i = 0; i < sizeof(adaptive_tuning_t); i++) {
        ((char*)tuning)[i] = 0;
    }
    
    tuning->strategy = strategy;
    tuning->target_performance = 0.9;
    tuning->min_performance_threshold = 0.7;
    tuning->adaptation_aggressiveness = 1.0;
    tuning->is_initialized = 1;
    tuning->start_time = 0; // Будет установлено при запуске
    
    g_adaptive_tuning = tuning;
    return tuning;
}

// Конфигурация
int adaptive_tuning_configure(adaptive_tuning_t *tuning, double target_perf, double aggressiveness) {
    if (!tuning) return -1;
    
    if (target_perf > 0.0 && target_perf <= 1.0) {
        tuning->target_performance = target_perf;
    }
    
    if (aggressiveness > 0.0 && aggressiveness <= 2.0) {
        tuning->adaptation_aggressiveness = aggressiveness;
    }
    
    return 0;
}

// Очистка
void adaptive_tuning_cleanup(adaptive_tuning_t *tuning) {
    if (!tuning) return;
    
    tuning->is_initialized = 0;
    if (g_adaptive_tuning == tuning) {
        g_adaptive_tuning = 0;
    }
}

// Добавление параметра
int adaptive_tuning_add_parameter(adaptive_tuning_t *tuning, const char *name, const char *desc,
                                param_type_t type, double baseline, double optimal) {
    if (!tuning || !name || tuning->parameter_count >= MAX_TUNING_PARAMETERS) {
        return -1;
    }
    
    tuning_parameter_t *param = &tuning->parameters[tuning->parameter_count];
    
    // Копирование имени
    int i;
    for (i = 0; i < 31 && name[i] != '\0'; i++) {
        param->name[i] = name[i];
    }
    param->name[i] = '\0';
    
    // Копирование описания
    if (desc) {
        for (i = 0; i < 63 && desc[i] != '\0'; i++) {
            param->description[i] = desc[i];
        }
        param->description[i] = '\0';
    }
    
    param->type = type;
    param->baseline_value = baseline;
    param->optimal_value = optimal;
    param->performance_impact = 0.0;
    param->adjustment_count = 0;
    param->last_adjustment_time = 0;
    
    // Установка начального значения
    if (type == PARAM_TYPE_INTEGER) {
        param->current.int_value = (int)baseline;
        param->range.int_range.min = 1;
        param->range.int_range.max = 1000;
    } else if (type == PARAM_TYPE_FLOAT) {
        param->current.float_value = baseline;
        param->range.float_range.min = 0.1;
        param->range.float_range.max = 10.0;
    } else if (type == PARAM_TYPE_BOOLEAN) {
        param->current.bool_value = baseline > 0.5 ? 1 : 0;
    }
    
    tuning->parameter_count++;
    return 0;
}

// Установка диапазона параметра
int adaptive_tuning_set_parameter_range(adaptive_tuning_t *tuning, const char *name, 
                                      double min, double max) {
    if (!tuning || !name) return -1;
    
    int index = find_parameter_index(tuning, name);
    if (index < 0) return -1;
    
    tuning_parameter_t *param = &tuning->parameters[index];
    
    if (param->type == PARAM_TYPE_INTEGER) {
        param->range.int_range.min = (int)min;
        param->range.int_range.max = (int)max;
    } else if (param->type == PARAM_TYPE_FLOAT) {
        param->range.float_range.min = min;
        param->range.float_range.max = max;
    }
    
    return 0;
}

// Обновление параметра
int adaptive_tuning_update_parameter(adaptive_tuning_t *tuning, const char *name, double value) {
    if (!tuning || !name) return -1;
    
    int index = find_parameter_index(tuning, name);
    if (index < 0) return -1;
    
    tuning_parameter_t *param = &tuning->parameters[index];
    
    if (param->type == PARAM_TYPE_INTEGER) {
        param->current.int_value = (int)value;
    } else if (param->type == PARAM_TYPE_FLOAT) {
        param->current.float_value = value;
    } else if (param->type == PARAM_TYPE_BOOLEAN) {
        param->current.bool_value = value > 0.5 ? 1 : 0;
    }
    
    return 0;
}

// Добавление метрики
int adaptive_tuning_add_metric(adaptive_tuning_t *tuning, const char *name, 
                             double baseline, double target, double weight) {
    if (!tuning || !name || tuning->metric_count >= 16) {
        return -1;
    }
    
    performance_metric_t *metric = &tuning->metrics[tuning->metric_count];
    
    // Копирование имени
    int i;
    for (i = 0; i < 31 && name[i] != '\0'; i++) {
        metric->name[i] = name[i];
    }
    metric->name[i] = '\0';
    
    metric->baseline_value = baseline;
    metric->target_value = target;
    metric->weight = weight > 0.0 ? weight : 1.0;
    metric->current_value = baseline;
    metric->is_degraded = 0;
    
    tuning->metric_count++;
    return 0;
}

// Обновление метрики
int adaptive_tuning_update_metric(adaptive_tuning_t *tuning, const char *name, double value) {
    if (!tuning || !name) return -1;
    
    int index = find_metric_index(tuning, name);
    if (index < 0) return -1;
    
    tuning->metrics[index].current_value = value;
    return 0;
}

// Получение метрики
double adaptive_tuning_get_metric(adaptive_tuning_t *tuning, const char *name) {
    if (!tuning || !name) return -1.0;
    
    int index = find_metric_index(tuning, name);
    if (index < 0) return -1.0;
    
    return tuning->metrics[index].current_value;
}

// Сбор состояния системы
int adaptive_tuning_collect_state(adaptive_tuning_t *tuning) {
    if (!tuning || !tuning->is_active) return -1;
    
    collect_system_state(tuning);
    return 0;
}

// Анализ производительности
int adaptive_tuning_analyze_performance(adaptive_tuning_t *tuning) {
    if (!tuning) return -1;
    
    // Обновление статуса метрик
    for (int i = 0; i < tuning->metric_count; i++) {
        performance_metric_t *metric = &tuning->metrics[i];
        double ratio = metric->current_value / metric->target_value;
        
        if (ratio < 0.8) {
            metric->is_degraded = 1;
        } else {
            metric->is_degraded = 0;
        }
    }
    
    return 0;
}

// Принятие решений о настройке
int adaptive_tuning_make_adjustments(adaptive_tuning_t *tuning) {
    if (!tuning || !tuning->is_active) return -1;
    
    if (!should_make_adjustment(tuning)) {
        return 0; // Нет необходимости в изменениях
    }
    
    // Определение параметров для изменения
    for (int i = 0; i < tuning->parameter_count; i++) {
        tuning_parameter_t *param = &tuning->parameters[i];
        double performance_delta = calculate_performance_delta(tuning);
        
        // Простая логика адаптации
        if (performance_delta < 0) {
            // Производительность упала - возвращаем к базовым значениям
            if (param->type == PARAM_TYPE_INTEGER) {
                param->current.int_value = (int)param->baseline_value;
            } else if (param->type == PARAM_TYPE_FLOAT) {
                param->current.float_value = param->baseline_value;
            }
        } else if (performance_delta > 0.1) {
            // Хорошая производительность - двигаем к оптимальным значениям
            double new_value = param->current.float_value;
            if (new_value < param->optimal_value) {
                new_value += (param->optimal_value - param->baseline_value) * 0.1;
                param->current.float_value = new_value;
            }
        }
        
        param->adjustment_count++;
        tuning->total_adjustments++;
        tuning->last_tuning_time = 0; // Будет установлено реальным временем
    }
    
    return 0;
}

// Применение оптимизаций
int adaptive_tuning_apply_optimizations(adaptive_tuning_t *tuning) {
    if (!tuning) return -1;
    
    // Применение настроек параметров (в реальной реализации)
    // - Изменение thread pool размеров
    // - Регулировка buffer размеров
    // - Настройка таймеров
    // - Адаптация network settings
    
    return 0;
}

// Получение рекомендаций
void adaptive_tuning_get_recommendations(adaptive_tuning_t *tuning, char *buffer, size_t buffer_size) {
    if (!tuning || !buffer || buffer_size < 50) return;
    
    // Генерация рекомендаций (симуляция)
    if (adaptive_tuning_is_system_degraded(tuning)) {
        // Простая конкатенация строк
        int idx = 0;
        const char *rec = "Reduce connections or ";
        for (int i = 0; rec[i] && idx < buffer_size - 1; i++, idx++) {
            buffer[idx] = rec[i];
        }
        buffer[idx] = '\0';
    } else {
        const char *rec = "System performing well";
        for (int i = 0; rec[i] && i < buffer_size - 1; i++) {
            buffer[i] = rec[i];
        }
        buffer[buffer_size - 1] = '\0';
    }
}

// Получение отчета
void adaptive_tuning_get_performance_report(adaptive_tuning_t *tuning, char *buffer, size_t buffer_size) {
    if (!tuning || !buffer || buffer_size < 100) return;
    
    double score = adaptive_tuning_calculate_performance_score(tuning);
    
    // Формирование отчета
    int idx = 0;
    const char *prefix = "Performance: ";
    for (int i = 0; prefix[i] && idx < buffer_size - 20; i++, idx++) {
        buffer[idx] = prefix[i];
    }
    
    // Добавление числа (упрощенно)
    if (score >= 0.9) {
        const char *perf = "EXCELLENT";
        for (int i = 0; perf[i] && idx < buffer_size - 1; i++, idx++) {
            buffer[idx] = perf[i];
        }
    } else if (score >= 0.7) {
        const char *perf = "GOOD";
        for (int i = 0; perf[i] && idx < buffer_size - 1; i++, idx++) {
            buffer[idx] = perf[i];
        }
    } else {
        const char *perf = "NEEDS IMPROVEMENT";
        for (int i = 0; perf[i] && idx < buffer_size - 1; i++, idx++) {
            buffer[idx] = perf[i];
        }
    }
    buffer[idx] = '\0';
}

// Управление
int adaptive_tuning_start(adaptive_tuning_t *tuning) {
    if (!tuning || !tuning->is_initialized) return -1;
    
    tuning->is_active = 1;
    tuning->start_time = 0; // Будет реальное время
    return 0;
}

int adaptive_tuning_stop(adaptive_tuning_t *tuning) {
    if (!tuning) return -1;
    
    tuning->is_active = 0;
    return 0;
}

int adaptive_tuning_reset(adaptive_tuning_t *tuning) {
    if (!tuning) return -1;
    
    // Сброс статистики
    tuning->total_adjustments = 0;
    tuning->successful_adjustments = 0;
    tuning->overall_performance_improvement = 0.0;
    tuning->history_count = 0;
    tuning->history_index = 0;
    
    // Сброс параметров к базовым значениям
    for (int i = 0; i < tuning->parameter_count; i++) {
        tuning_parameter_t *param = &tuning->parameters[i];
        if (param->type == PARAM_TYPE_INTEGER) {
            param->current.int_value = (int)param->baseline_value;
        } else if (param->type == PARAM_TYPE_FLOAT) {
            param->current.float_value = param->baseline_value;
        } else if (param->type == PARAM_TYPE_BOOLEAN) {
            param->current.bool_value = param->baseline_value > 0.5 ? 1 : 0;
        }
        param->adjustment_count = 0;
    }
    
    return 0;
}

// Утилиты

const char* adaptive_tuning_strategy_to_string(adaptation_strategy_t strategy) {
    switch (strategy) {
        case STRATEGY_CONSERVATIVE: return "CONSERVATIVE";
        case STRATEGY_AGGRESSIVE: return "AGGRESSIVE";
        case STRATEGY_BALANCED: return "BALANCED";
        case STRATEGY_LEARNING: return "LEARNING";
        default: return "UNKNOWN";
    }
}

double adaptive_tuning_calculate_performance_score(adaptive_tuning_t *tuning) {
    if (!tuning) return 0.0;
    
    double total_score = 0.0;
    double total_weight = 0.0;
    
    for (int i = 0; i < tuning->metric_count; i++) {
        performance_metric_t *metric = &tuning->metrics[i];
        double ratio = metric->current_value / metric->target_value;
        if (ratio > 1.0) ratio = 1.0;
        
        total_score += ratio * metric->weight;
        total_weight += metric->weight;
    }
    
    return total_weight > 0.0 ? total_score / total_weight : 0.0;
}

int adaptive_tuning_is_system_degraded(adaptive_tuning_t *tuning) {
    if (!tuning) return 1;
    
    double score = adaptive_tuning_calculate_performance_score(tuning);
    return score < tuning->min_performance_threshold ? 1 : 0;
}

// Вспомогательные функции

static int find_parameter_index(adaptive_tuning_t *tuning, const char *name) {
    for (int i = 0; i < tuning->parameter_count; i++) {
        int match = 1;
        for (int j = 0; tuning->parameters[i].name[j] && name[j]; j++) {
            if (tuning->parameters[i].name[j] != name[j]) {
                match = 0;
                break;
            }
        }
        if (match && tuning->parameters[i].name[0] != '\0') {
            return i;
        }
    }
    return -1;
}

static int find_metric_index(adaptive_tuning_t *tuning, const char *name) {
    for (int i = 0; i < tuning->metric_count; i++) {
        int match = 1;
        for (int j = 0; tuning->metrics[i].name[j] && name[j]; j++) {
            if (tuning->metrics[i].name[j] != name[j]) {
                match = 0;
                break;
            }
        }
        if (match && tuning->metrics[i].name[0] != '\0') {
            return i;
        }
    }
    return -1;
}

static void collect_system_state(adaptive_tuning_t *tuning) {
    // Симуляция сбора состояния системы
    int index = tuning->history_index;
    system_state_t *state = &tuning->state_history[index];
    
    state->cpu_usage = 45.5;
    state->memory_usage = 65.2;
    state->network_throughput = 150.5;
    state->active_connections = 1250;
    state->response_time_ms = 15.2;
    state->error_rate = 0;
    state->timestamp = 0; // Будет реальное время
    
    // Обновление индекса истории
    tuning->history_index = (tuning->history_index + 1) % TUNING_HISTORY_SIZE;
    if (tuning->history_count < TUNING_HISTORY_SIZE) {
        tuning->history_count++;
    }
}

static double calculate_performance_delta(adaptive_tuning_t *tuning) {
    // Простой расчет изменения производительности
    if (tuning->history_count < 2) return 0.0;
    
    int current_idx = (tuning->history_index - 1 + TUNING_HISTORY_SIZE) % TUNING_HISTORY_SIZE;
    int prev_idx = (tuning->history_index - 2 + TUNING_HISTORY_SIZE) % TUNING_HISTORY_SIZE;
    
    double current_perf = tuning->state_history[current_idx].network_throughput;
    double prev_perf = tuning->state_history[prev_idx].network_throughput;
    
    return (current_perf - prev_perf) / prev_perf;
}

static int should_make_adjustment(adaptive_tuning_t *tuning) {
    // Проверка необходимости адаптации
    double performance_score = adaptive_tuning_calculate_performance_score(tuning);
    
    if (performance_score < tuning->min_performance_threshold) {
        return 1; // Нужна адаптация
    }
    
    if (performance_score > tuning->target_performance) {
        return 0; // Все хорошо, адаптация не нужна
    }
    
    // Проверка времени последней адаптации
    return 1;
}