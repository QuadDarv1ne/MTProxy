/*
    Система адаптивной самонастройки MTProxy
    Автоматическая оптимизация под текущую нагрузку и условия
*/

#ifndef ADAPTIVE_TUNING_H
#define ADAPTIVE_TUNING_H

#include <stdint.h>
#include <stddef.h>

// Конфигурация адаптивной настройки
#define MAX_TUNING_PARAMETERS 64
#define TUNING_HISTORY_SIZE 100
#define ADAPTATION_INTERVAL_MS 5000
#define PERFORMANCE_THRESHOLD 0.85

// Типы параметров настройки
typedef enum {
    PARAM_TYPE_INTEGER = 0,
    PARAM_TYPE_FLOAT = 1,
    PARAM_TYPE_BOOLEAN = 2,
    PARAM_TYPE_ENUM = 3
} param_type_t;

// Стратегии адаптации
typedef enum {
    STRATEGY_CONSERVATIVE = 0,  // Консервативная (медленные изменения)
    STRATEGY_AGGRESSIVE = 1,    // Агрессивная (быстрые изменения)
    STRATEGY_BALANCED = 2,      // Сбалансированная (умеренные изменения)
    STRATEGY_LEARNING = 3       // Обучающаяся (на основе истории)
} adaptation_strategy_t;

// Параметр настройки
typedef struct tuning_parameter {
    char name[32];
    char description[64];
    param_type_t type;
    
    // Текущее значение
    union {
        int int_value;
        double float_value;
        int bool_value;
        int enum_value;
    } current;
    
    // Диапазон значений
    union {
        struct { int min; int max; } int_range;
        struct { double min; double max; } float_range;
        struct { int count; char **enum_names; } enum_range;
    } range;
    
    // Оптимальные значения
    double optimal_value;
    double baseline_value;
    
    // Статистика
    double performance_impact;
    int adjustment_count;
    long long last_adjustment_time;
} tuning_parameter_t;

// Метрика производительности
typedef struct performance_metric {
    char name[32];
    double current_value;
    double baseline_value;
    double target_value;
    double weight;  // Вес в общей оценке
    int is_degraded;
} performance_metric_t;

// Состояние системы
typedef struct system_state {
    double cpu_usage;
    double memory_usage;
    double network_throughput;
    int active_connections;
    double response_time_ms;
    int error_rate;
    long long timestamp;
} system_state_t;

// Адаптивная настройка
typedef struct adaptive_tuning {
    // Параметры
    tuning_parameter_t parameters[MAX_TUNING_PARAMETERS];
    int parameter_count;
    
    // Метрики производительности
    performance_metric_t metrics[16];
    int metric_count;
    
    // История состояний
    system_state_t state_history[TUNING_HISTORY_SIZE];
    int history_index;
    int history_count;
    
    // Стратегия адаптации
    adaptation_strategy_t strategy;
    double adaptation_aggressiveness;
    
    // Целевые показатели
    double target_performance;
    double min_performance_threshold;
    
    // Статус
    int is_initialized;
    int is_active;
    long long last_tuning_time;
    long long start_time;
    
    // Счетчики
    int total_adjustments;
    int successful_adjustments;
    double overall_performance_improvement;
} adaptive_tuning_t;

// Инициализация
adaptive_tuning_t* adaptive_tuning_init(adaptation_strategy_t strategy);
int adaptive_tuning_configure(adaptive_tuning_t *tuning, double target_perf, double aggressiveness);
void adaptive_tuning_cleanup(adaptive_tuning_t *tuning);

// Управление параметрами
int adaptive_tuning_add_parameter(adaptive_tuning_t *tuning, const char *name, const char *desc,
                                param_type_t type, double baseline, double optimal);
int adaptive_tuning_set_parameter_range(adaptive_tuning_t *tuning, const char *name, 
                                      double min, double max);
int adaptive_tuning_update_parameter(adaptive_tuning_t *tuning, const char *name, double value);

// Управление метриками
int adaptive_tuning_add_metric(adaptive_tuning_t *tuning, const char *name, 
                             double baseline, double target, double weight);
int adaptive_tuning_update_metric(adaptive_tuning_t *tuning, const char *name, double value);
double adaptive_tuning_get_metric(adaptive_tuning_t *tuning, const char *name);

// Адаптация
int adaptive_tuning_collect_state(adaptive_tuning_t *tuning);
int adaptive_tuning_analyze_performance(adaptive_tuning_t *tuning);
int adaptive_tuning_make_adjustments(adaptive_tuning_t *tuning);
int adaptive_tuning_apply_optimizations(adaptive_tuning_t *tuning);

// Получение рекомендаций
void adaptive_tuning_get_recommendations(adaptive_tuning_t *tuning, char *buffer, size_t buffer_size);
void adaptive_tuning_get_performance_report(adaptive_tuning_t *tuning, char *buffer, size_t buffer_size);

// Управление
int adaptive_tuning_start(adaptive_tuning_t *tuning);
int adaptive_tuning_stop(adaptive_tuning_t *tuning);
int adaptive_tuning_reset(adaptive_tuning_t *tuning);

// Утилиты
const char* adaptive_tuning_strategy_to_string(adaptation_strategy_t strategy);
double adaptive_tuning_calculate_performance_score(adaptive_tuning_t *tuning);
int adaptive_tuning_is_system_degraded(adaptive_tuning_t *tuning);

#endif // ADAPTIVE_TUNING_H