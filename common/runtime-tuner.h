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

#ifndef __RUNTIME_TUNER_H__
#define __RUNTIME_TUNER_H__

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
enum config_param_type;

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

// Tuning strategy
enum tuning_strategy {
    TUNING_STRATEGY_CONSERVATIVE = 0,
    TUNING_STRATEGY_AGGRESSIVE,
    TUNING_STRATEGY_ADAPTIVE,
    TUNING_STRATEGY_PREDICTIVE
};

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
    double performance_impact;
};

// Performance metric
struct performance_metric {
    char name[64];
    double current_value;
    double baseline_value;
    double weight;
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
};

// Инициализация runtime tuner
int runtime_tuner_init(enum tuning_strategy strategy);

// Регистрация tunable параметра
int runtime_tuner_register_parameter(
    const char *name,
    const char *description,
    enum config_param_type type,
    double min_value,
    double max_value,
    double step_size,
    int auto_tune_enabled);

// Регистрация performance метрики
int runtime_tuner_register_metric(
    const char *name,
    double weight,
    double baseline_value);

// Обновление значения метрики
int runtime_tuner_update_metric(const char *name, double value);

// Выполнение цикла автоматической настройки
int runtime_tuner_run_auto_tuning(void);

// Ручная настройка параметра
int runtime_tuner_manual_tune(const char *param_name, double target_value);

// Получение статистики tuner
void runtime_tuner_get_stats(struct runtime_tuner_stats *stats);

// Вывод статистики
void runtime_tuner_print_stats(void);

// Очистка tuner
void runtime_tuner_cleanup(void);

// Расширенные функции
int runtime_tuner_enable_auto_tuning(int enable);
int runtime_tuner_set_strategy(enum tuning_strategy strategy);
int runtime_tuner_set_performance_threshold(double threshold);
int runtime_tuner_get_parameter_info(const char *param_name, 
                                    struct tuning_parameter *info_out);
int runtime_tuner_list_tunable_parameters(char param_names[][128], 
                                         int max_parameters);
int runtime_tuner_export_tuning_history(const char *filename);
int runtime_tuner_import_tuning_profile(const char *filename);

// Вспомогательные функции
double runtime_tuner_get_current_performance(void);
int runtime_tuner_reset_parameter(const char *param_name);
int runtime_tuner_lock_parameter(const char *param_name, int lock);
int runtime_tuner_set_parameter_bounds(const char *param_name, 
                                      double min_value, 
                                      double max_value);

#ifdef __cplusplus
}
#endif

#endif // __RUNTIME_TUNER_H__