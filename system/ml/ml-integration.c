/**
 * @file ml-integration.c
 * @brief Реализация интеграции ML-систем с Alert Manager
 * 
 * @version 1.0.32
 * @date 29 марта 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#include "ml-integration.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

/* ============================================
 * Внутренние функции
 * ============================================ */

/**
 * @brief Получить текущее время в миллисекундах
 */
static uint64_t get_time_ms(void) {
#ifdef _WIN32
    return GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

/**
 * @brief Инициализация мьютекса
 */
static int init_mutex(void** mutex) {
#ifdef _WIN32
    *mutex = CreateMutex(NULL, FALSE, NULL);
    return (*mutex == NULL) ? -1 : 0;
#else
    *mutex = malloc(sizeof(pthread_mutex_t));
    if (!*mutex) return -1;
    return pthread_mutex_init((pthread_mutex_t*)*mutex, NULL);
#endif
}

static void lock_mutex(void* mutex) {
#ifdef _WIN32
    WaitForSingleObject((HANDLE)mutex, INFINITE);
#else
    pthread_mutex_lock((pthread_mutex_t*)mutex);
#endif
}

static void unlock_mutex(void* mutex) {
#ifdef _WIN32
    ReleaseMutex((HANDLE)mutex);
#else
    pthread_mutex_unlock((pthread_mutex_t*)mutex);
#endif
}

static void cleanup_mutex(void* mutex) {
#ifdef _WIN32
    CloseHandle((HANDLE)mutex);
#else
    pthread_mutex_destroy((pthread_mutex_t*)mutex);
    free(mutex);
#endif
}

/**
 * @brief Выделение памяти с проверкой
 */
static void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "ML Integration: Failed to allocate %zu bytes\n", size);
    }
    return ptr;
}

/**
 * @brief Логирование
 */
static void ml_log(ml_integration_t* ml, int level, const char* format, ...) {
    if (!ml || !ml->log_callback || !ml->log_user_data) return;
    
    char message[512];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    ml->log_callback(level, message, ml->log_user_data);
}

/**
 * @brief Отправка алерта
 */
static void ml_send_alert(ml_integration_t* ml, int level, const char* type, 
                          const char* format, ...) {
    if (!ml || !ml->alert_callback) return;
    
    char message[512];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    ml->alert_callback(level, type, message, ml->alert_user_data);
    ml->total_alerts++;
}

/* ============================================
 * Основные функции API
 * ============================================ */

int ml_integration_init(ml_integration_t* ml, size_t max_monitors) {
    if (!ml) return -1;
    
    memset(ml, 0, sizeof(ml_integration_t));
    
    ml->max_monitors = max_monitors;
    ml->monitors = (ml_monitor_config_t*)safe_malloc(
        max_monitors * sizeof(ml_monitor_config_t));
    ml->monitor_stats = (ml_monitor_stats_t*)safe_malloc(
        max_monitors * sizeof(ml_monitor_stats_t));
    ml->anomaly_detectors = (anomaly_detector_t*)safe_malloc(
        max_monitors * sizeof(anomaly_detector_t));
    ml->predictors = (predictive_analytics_t*)safe_malloc(
        max_monitors * sizeof(predictive_analytics_t));
    
    if (!ml->monitors || !ml->monitor_stats || 
        !ml->anomaly_detectors || !ml->predictors) {
        ml_integration_cleanup(ml);
        return -1;
    }
    
    memset(ml->monitors, 0, max_monitors * sizeof(ml_monitor_config_t));
    memset(ml->monitor_stats, 0, max_monitors * sizeof(ml_monitor_stats_t));
    memset(ml->anomaly_detectors, 0, max_monitors * sizeof(anomaly_detector_t));
    memset(ml->predictors, 0, max_monitors * sizeof(predictive_analytics_t));
    
    /* Инициализация мьютекса */
    if (init_mutex(&ml->mutex) != 0) {
        ml_integration_cleanup(ml);
        return -1;
    }
    
    ml->n_monitors = 0;
    
    return 0;
}

void ml_integration_cleanup(ml_integration_t* ml) {
    if (!ml) return;

    lock_mutex(ml->mutex);

    /* Очистка мониторов */
    for (size_t i = 0; i < ml->n_monitors; i++) {
        anomaly_detector_cleanup(&ml->anomaly_detectors[i]);
        predictive_analytics_cleanup(&ml->predictors[i]);
    }

    free(ml->monitors);
    free(ml->monitor_stats);
    free(ml->anomaly_detectors);
    free(ml->predictors);

    /* Очистка мьютекса */
    if (ml->mutex) {
        cleanup_mutex(ml->mutex);
        ml->mutex = NULL;
    }

    ml->n_monitors = 0;
    ml->max_monitors = 0;
    ml->monitors = NULL;
    ml->monitor_stats = NULL;
    ml->anomaly_detectors = NULL;
    ml->predictors = NULL;
}

int ml_integration_add_monitor(ml_integration_t* ml,
                               const ml_monitor_config_t* config,
                               int* monitor_id) {
    if (!ml || !config) return -1;
    
    lock_mutex(ml->mutex);
    
    if (ml->n_monitors >= ml->max_monitors) {
        unlock_mutex(ml->mutex);
        return -1; /* Превышен лимит */
    }
    
    size_t idx = ml->n_monitors;
    
    /* Копирование конфигурации */
    memcpy(&ml->monitors[idx], config, sizeof(ml_monitor_config_t));
    ml->monitors[idx].enabled = true;
    
    /* Инициализация статистики */
    memset(&ml->monitor_stats[idx], 0, sizeof(ml_monitor_stats_t));
    ml->monitor_stats[idx].status = ML_MONITOR_STATUS_ACTIVE;
    
    /* Инициализация ML-детектора */
    if (config->type == ML_MONITOR_ANOMALY) {
        anomaly_config_t aconfig = {0};
        aconfig.algorithm = config->anomaly_algorithm;
        aconfig.n_features = 1;
        aconfig.max_samples = 1000;
        aconfig.threshold = config->anomaly_threshold;
        aconfig.zscore_threshold = 3.0f; /* Default Z-Score threshold */

        anomaly_detector_init(&ml->anomaly_detectors[idx], &aconfig);
    } else if (config->type == ML_MONITOR_FORECAST) {
        predict_config_t pconfig = {0};
        pconfig.algorithm = config->forecast_algorithm;
        pconfig.window_size = 100;
        pconfig.forecast_horizon = config->forecast_horizon;
        pconfig.enable_confidence_interval = true;
        
        predictive_analytics_init(&ml->predictors[idx], &pconfig);
    }
    
    ml->n_monitors++;
    
    if (monitor_id) {
        *monitor_id = (int)idx;
    }
    
    ml_log(ml, 1, "Added ML monitor '%s' (type=%d, id=%d)", 
           config->name, config->type, (int)idx);
    
    unlock_mutex(ml->mutex);
    return 0;
}

int ml_integration_remove_monitor(ml_integration_t* ml, int monitor_id) {
    if (!ml || monitor_id < 0 || (size_t)monitor_id >= ml->n_monitors) return -1;
    
    lock_mutex(ml->mutex);
    
    size_t idx = (size_t)monitor_id;
    
    ml_log(ml, 1, "Removing ML monitor '%s' (id=%d)", 
           ml->monitors[idx].name, monitor_id);
    
    /* Очистка ML-детекторов */
    anomaly_detector_cleanup(&ml->anomaly_detectors[idx]);
    predictive_analytics_cleanup(&ml->predictors[idx]);
    
    /* Сдвиг массивов */
    for (size_t i = idx; i < ml->n_monitors - 1; i++) {
        memcpy(&ml->monitors[i], &ml->monitors[i + 1], sizeof(ml_monitor_config_t));
        memcpy(&ml->monitor_stats[i], &ml->monitor_stats[i + 1], sizeof(ml_monitor_stats_t));
        memcpy(&ml->anomaly_detectors[i], &ml->anomaly_detectors[i + 1], sizeof(anomaly_detector_t));
        memcpy(&ml->predictors[i], &ml->predictors[i + 1], sizeof(predictive_analytics_t));
    }
    
    ml->n_monitors--;

    memset(&ml->monitors[ml->n_monitors], 0, sizeof(ml_monitor_config_t));
    memset(&ml->monitor_stats[ml->n_monitors], 0, sizeof(ml_monitor_stats_t));
    memset(&ml->anomaly_detectors[ml->n_monitors], 0, sizeof(anomaly_detector_t));
    memset(&ml->predictors[ml->n_monitors], 0, sizeof(predictive_analytics_t));

    unlock_mutex(ml->mutex);
    return 0;
}

int ml_integration_update_monitor(ml_integration_t* ml,
                                  int monitor_id,
                                  double value,
                                  uint64_t timestamp) {
    if (!ml || monitor_id < 0 || (size_t)monitor_id >= ml->n_monitors) return -1;
    
    lock_mutex(ml->mutex);
    
    size_t idx = (size_t)monitor_id;
    ml_monitor_config_t* config = &ml->monitors[idx];
    ml_monitor_stats_t* stats = &ml->monitor_stats[idx];
    
    if (!config->enabled || config->type == ML_MONITOR_CUSTOM) {
        unlock_mutex(ml->mutex);
        return 0;
    }
    
    stats->total_checks++;
    stats->last_check_time = get_time_ms();
    
    if (config->type == ML_MONITOR_ANOMALY) {
        /* Проверка аномалии */
        anomaly_result_t result;
        if (anomaly_detector_predict(&ml->anomaly_detectors[idx], &value, &result) == 0) {
            stats->last_anomaly_score = result.anomaly_score;
            
            if (result.status >= ANOMALY_STATUS_ANOMALY) {
                stats->anomalies_detected++;
                ml->total_anomalies++;
                
                if (config->enable_alerts) {
                    ml_send_alert(ml, 
                                  (result.status == ANOMALY_STATUS_CRITICAL) ? 4 : 3,
                                  "MLAnomaly",
                                  "Monitor '%s': Anomaly detected (score=%.3f, algorithm=%s)",
                                  config->name, result.anomaly_score,
                                  anomaly_algo_to_string(result.detected_by));
                    stats->alerts_sent++;
                }
                
                if (config->enable_logging) {
                    ml_log(ml, 2, "Monitor '%s': Anomaly score=%.3f",
                           config->name, result.anomaly_score);
                }
            }
        }
        
        /* Добавление образца для онлайн-обучения */
        anomaly_detector_add_sample(&ml->anomaly_detectors[idx], &value);
        
    } else if (config->type == ML_MONITOR_FORECAST) {
        /* Добавление данных для прогноза */
        predictive_analytics_add_data(&ml->predictors[idx], value, timestamp);
        
        /* Прогноз на forecast_horizon шагов */
        if (stats->total_checks % 10 == 0) { /* Каждые 10 проверок */
            predict_result_t forecast;
            if (predictive_analytics_forecast(&ml->predictors[idx], 
                                              config->forecast_horizon, &forecast) == 0) {
                ml->total_forecasts++;
                
                /* Проверка прогноза на критичность */
                if (forecast.forecast_count > 0) {
                    double last_forecast = forecast.forecast_values[forecast.forecast_count - 1];
                    stats->last_forecast_value = last_forecast;
                    
                    /* Простая эвристика: если прогноз превышает порог */
                    if (last_forecast > config->critical_threshold) {
                        stats->criticals_triggered++;
                        
                        if (config->enable_alerts) {
                            ml_send_alert(ml, 4, "MLCriticalForecast",
                                          "Monitor '%s': Critical forecast (%.2f > %.2f)",
                                          config->name, last_forecast, 
                                          config->critical_threshold);
                            stats->alerts_sent++;
                        }
                    } else if (last_forecast > config->warning_threshold) {
                        stats->warnings_triggered++;
                        
                        if (config->enable_alerts) {
                            ml_send_alert(ml, 2, "MLWarningForecast",
                                          "Monitor '%s': Warning forecast (%.2f > %.2f)",
                                          config->name, last_forecast,
                                          config->warning_threshold);
                            stats->alerts_sent++;
                        }
                    }
                }
                
                free(forecast.forecast_values);
                free(forecast.lower_bound);
                free(forecast.upper_bound);
            }
        }
    }
    
    unlock_mutex(ml->mutex);
    return 0;
}

int ml_integration_check_all(ml_integration_t* ml) {
    if (!ml) return 0;
    
    int alerts_count = 0;
    
    lock_mutex(ml->mutex);
    
    for (size_t i = 0; i < ml->n_monitors; i++) {
        if (ml->monitors[i].enabled && ml->monitors[i].type == ML_MONITOR_ANOMALY) {
            /* Проверка каждого монитора аномалий */
            /* В реальной реализации здесь была бы проверка последних данных */
        }
    }
    
    unlock_mutex(ml->mutex);
    
    return alerts_count;
}

void ml_integration_set_alert_callback(ml_integration_t* ml,
                                       void (*callback)(int, const char*, const char*, void*),
                                       void* user_data) {
    if (!ml) return;
    
    lock_mutex(ml->mutex);
    ml->alert_callback = callback;
    ml->alert_user_data = user_data;
    unlock_mutex(ml->mutex);
}

void ml_integration_set_log_callback(ml_integration_t* ml,
                                     void (*callback)(int, const char*, void*),
                                     void* user_data) {
    if (!ml) return;
    
    lock_mutex(ml->mutex);
    ml->log_callback = callback;
    ml->log_user_data = user_data;
    unlock_mutex(ml->mutex);
}

int ml_integration_get_monitor_stats(ml_integration_t* ml,
                                     int monitor_id,
                                     ml_monitor_stats_t* stats) {
    if (!ml || monitor_id < 0 || (size_t)monitor_id >= ml->n_monitors || !stats) return -1;
    
    lock_mutex(ml->mutex);
    memcpy(stats, &ml->monitor_stats[monitor_id], sizeof(ml_monitor_stats_t));
    unlock_mutex(ml->mutex);
    
    return 0;
}

void ml_integration_get_stats(ml_integration_t* ml,
                              uint64_t* total_anomalies,
                              uint64_t* total_forecasts,
                              uint64_t* total_alerts) {
    if (!ml) return;
    
    lock_mutex(ml->mutex);
    
    if (total_anomalies) *total_anomalies = ml->total_anomalies;
    if (total_forecasts) *total_forecasts = ml->total_forecasts;
    if (total_alerts) *total_alerts = ml->total_alerts;
    
    unlock_mutex(ml->mutex);
}

int ml_integration_pause_monitor(ml_integration_t* ml, int monitor_id) {
    if (!ml || monitor_id < 0 || (size_t)monitor_id >= ml->n_monitors) return -1;
    
    lock_mutex(ml->mutex);
    ml->monitors[monitor_id].enabled = false;
    ml->monitor_stats[monitor_id].status = ML_MONITOR_STATUS_PAUSED;
    ml_log(ml, 1, "Paused monitor '%s' (id=%d)", 
           ml->monitors[monitor_id].name, monitor_id);
    unlock_mutex(ml->mutex);
    
    return 0;
}

int ml_integration_resume_monitor(ml_integration_t* ml, int monitor_id) {
    if (!ml || monitor_id < 0 || (size_t)monitor_id >= ml->n_monitors) return -1;
    
    lock_mutex(ml->mutex);
    ml->monitors[monitor_id].enabled = true;
    ml->monitor_stats[monitor_id].status = ML_MONITOR_STATUS_ACTIVE;
    ml_log(ml, 1, "Resumed monitor '%s' (id=%d)", 
           ml->monitors[monitor_id].name, monitor_id);
    unlock_mutex(ml->mutex);
    
    return 0;
}

int ml_integration_export_json(ml_integration_t* ml, char* buffer, size_t buffer_size) {
    if (!ml || !buffer || buffer_size == 0) return -1;
    
    int written = 0;
    
    lock_mutex(ml->mutex);
    
    written += snprintf(buffer + written, buffer_size - written, "{\n");
    written += snprintf(buffer + written, buffer_size - written,
        "  \"monitors\": %zu,\n", ml->n_monitors);
    written += snprintf(buffer + written, buffer_size - written,
        "  \"total_anomalies\": %llu,\n", (unsigned long long)ml->total_anomalies);
    written += snprintf(buffer + written, buffer_size - written,
        "  \"total_forecasts\": %llu,\n", (unsigned long long)ml->total_forecasts);
    written += snprintf(buffer + written, buffer_size - written,
        "  \"total_alerts\": %llu,\n", (unsigned long long)ml->total_alerts);
    
    written += snprintf(buffer + written, buffer_size - written,
        "  \"monitor_details\": [\n");
    
    for (size_t i = 0; i < ml->n_monitors && i < 10; i++) {
        written += snprintf(buffer + written, buffer_size - written,
            "    {\n");
        written += snprintf(buffer + written, buffer_size - written,
            "      \"name\": \"%s\",\n", ml->monitors[i].name);
        written += snprintf(buffer + written, buffer_size - written,
            "      \"type\": %d,\n", ml->monitors[i].type);
        written += snprintf(buffer + written, buffer_size - written,
            "      \"enabled\": %s,\n", ml->monitors[i].enabled ? "true" : "false");
        written += snprintf(buffer + written, buffer_size - written,
            "      \"total_checks\": %llu\n", 
            (unsigned long long)ml->monitor_stats[i].total_checks);
        written += snprintf(buffer + written, buffer_size - written,
            "    }%s\n", (i < ml->n_monitors - 1 && i < 9) ? "," : "");
    }
    
    written += snprintf(buffer + written, buffer_size - written,
        "  ]\n");
    written += snprintf(buffer + written, buffer_size - written, "}\n");
    
    unlock_mutex(ml->mutex);
    
    return written;
}

const char* ml_monitor_type_to_string(ml_monitor_type_t type) {
    switch (type) {
        case ML_MONITOR_ANOMALY: return "Anomaly";
        case ML_MONITOR_FORECAST: return "Forecast";
        case ML_MONITOR_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

const char* ml_monitor_status_to_string(ml_monitor_status_t status) {
    switch (status) {
        case ML_MONITOR_STATUS_ACTIVE: return "Active";
        case ML_MONITOR_STATUS_PAUSED: return "Paused";
        case ML_MONITOR_STATUS_ERROR: return "Error";
        case ML_MONITOR_STATUS_DISABLED: return "Disabled";
        default: return "Unknown";
    }
}
