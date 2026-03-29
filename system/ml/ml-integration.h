/**
 * @file ml-integration.h
 * @brief Интеграция ML-систем с Alert Manager и мониторингом
 * 
 * Возможности:
 * - Автоматические алерты при обнаружении аномалий
 * - Прогнозирование критических событий
 * - Адаптивные пороги срабатывания
 * - Статистика и мониторинг ML-моделей
 * 
 * @version 1.0.32
 * @date 29 марта 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#ifndef ML_INTEGRATION_H
#define ML_INTEGRATION_H

#include "ml/anomaly-detection.h"
#include "ml/predictive-analytics.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * Конфигурация и константы
 * ============================================ */

/** Максимальное количество ML-мониторов */
#define ML_INTEGRATION_MAX_MONITORS     32

/** Максимальное имя метрики */
#define ML_MAX_METRIC_NAME_LEN          64

/** Интервал проверки прогнозов (мс) */
#define ML_FORECAST_CHECK_INTERVAL      60000

/** Порог критического прогноза (%) */
#define ML_CRITICAL_FORECAST_THRESHOLD  90.0f

/** Порог предупреждения прогноза (%) */
#define ML_WARNING_FORECAST_THRESHOLD   75.0f

/* ============================================
 * Типы данных
 * ============================================ */

/**
 * @brief Типы ML-мониторов
 */
typedef enum {
    ML_MONITOR_ANOMALY = 0,             /**< Монитор аномалий */
    ML_MONITOR_FORECAST = 1,            /**< Монитор прогнозов */
    ML_MONITOR_CUSTOM = 2               /**< Пользовательский монитор */
} ml_monitor_type_t;

/**
 * @brief Статус ML-монитора
 */
typedef enum {
    ML_MONITOR_STATUS_ACTIVE = 0,       /**< Активен */
    ML_MONITOR_STATUS_PAUSED = 1,       /**< На паузе */
    ML_MONITOR_STATUS_ERROR = 2,        /**< Ошибка */
    ML_MONITOR_STATUS_DISABLED = 3      /**< Отключен */
} ml_monitor_status_t;

/**
 * @brief Конфигурация ML-монитора
 */
typedef struct {
    char name[ML_MAX_METRIC_NAME_LEN];  /**< Имя монитора */
    ml_monitor_type_t type;             /**< Тип монитора */
    bool enabled;                       /**< Включен ли */
    
    /* Настройки аномалий */
    anomaly_algo_t anomaly_algorithm;   /**< Алгоритм детекции */
    float anomaly_threshold;            /**< Порог аномалии */
    
    /* Настройки прогнозов */
    predict_algo_t forecast_algorithm;  /**< Алгоритм прогноза */
    size_t forecast_horizon;            /**< Горизонт прогноза (шаги) */
    float warning_threshold;            /**< Порог предупреждения */
    float critical_threshold;           /**< Порог критичности */
    
    /* Настройки уведомлений */
    bool enable_alerts;                 /**< Включить алерты */
    bool enable_logging;                /**< Включить логирование */
    int alert_cooldown_sec;             /**< Задержка между алертами (сек) */
    
    /* Пользовательские данные */
    void* user_data;                    /**< Пользовательские данные */
} ml_monitor_config_t;

/**
 * @brief Статистика ML-монитора
 */
typedef struct {
    uint64_t total_checks;              /**< Всего проверок */
    uint64_t anomalies_detected;        /**< Обнаружено аномалий */
    uint64_t warnings_triggered;        /**< Сработало предупреждений */
    uint64_t criticals_triggered;       /**< Сработало критических */
    uint64_t alerts_sent;               /**< Отправлено алертов */
    uint64_t last_check_time;           /**< Время последней проверки */
    double last_anomaly_score;          /**< Последняя оценка аномалии */
    double last_forecast_value;         /**< Последнее прогнозное значение */
    ml_monitor_status_t status;         /**< Статус монитора */
} ml_monitor_stats_t;

/**
 * @brief Контекст ML-интеграции
 */
typedef struct {
    /* Мониторы */
    ml_monitor_config_t* monitors;      /**< Конфигурации мониторов */
    ml_monitor_stats_t* monitor_stats;  /**< Статистика мониторов */
    size_t n_monitors;                  /**< Количество мониторов */
    size_t max_monitors;                /**< Максимум мониторов */
    
    /* ML-детекторы */
    anomaly_detector_t* anomaly_detectors;  /**< Детекторы аномалий */
    predictive_analytics_t* predictors;     /**< Предикторы */
    
    /* Alert Manager callback */
    void (*alert_callback)(int level, const char* type, const char* message, void* user_data);
    void* alert_user_data;              /**< Данные для callback */
    
    /* Логирование */
    void (*log_callback)(int level, const char* message, void* user_data);
    void* log_user_data;                /**< Данные для логирования */
    
    /* Глобальная статистика */
    uint64_t total_anomalies;           /**< Всего аномалий */
    uint64_t total_forecasts;           /**< Всего прогнозов */
    uint64_t total_alerts;              /**< Всего алертов */
    
    /* Блокировка */
    void* mutex;                        /**< Мьютекс */
} ml_integration_t;

/* ============================================
 * Основные функции API
 * ============================================ */

/**
 * @brief Инициализация ML-интеграции
 * 
 * @param ml ML-интеграция контекст
 * @param max_monitors Максимальное количество мониторов
 * @return 0 при успехе, -1 при ошибке
 */
int ml_integration_init(ml_integration_t* ml, size_t max_monitors);

/**
 * @brief Очистка ML-интеграции
 * 
 * @param ml ML-интеграция контекст
 */
void ml_integration_cleanup(ml_integration_t* ml);

/**
 * @brief Добавление ML-монитора
 * 
 * @param ml ML-интеграция контекст
 * @param config Конфигурация монитора
 * @param monitor_id ID монитора (выходной параметр)
 * @return 0 при успехе, -1 при ошибке
 */
int ml_integration_add_monitor(ml_integration_t* ml, 
                               const ml_monitor_config_t* config,
                               int* monitor_id);

/**
 * @brief Удаление ML-монитора
 * 
 * @param ml ML-интеграция контекст
 * @param monitor_id ID монитора
 * @return 0 при успехе, -1 при ошибке
 */
int ml_integration_remove_monitor(ml_integration_t* ml, int monitor_id);

/**
 * @brief Обновление данных монитора
 * 
 * @param ml ML-интеграция контекст
 * @param monitor_id ID монитора
 * @param value Текущее значение метрики
 * @param timestamp Временная метка
 * @return 0 при успехе, -1 при ошибке
 */
int ml_integration_update_monitor(ml_integration_t* ml,
                                  int monitor_id,
                                  double value,
                                  uint64_t timestamp);

/**
 * @brief Проверка всех мониторов
 * 
 * @param ml ML-интеграция контекст
 * @return Количество сработавших алертов
 */
int ml_integration_check_all(ml_integration_t* ml);

/**
 * @brief Установка callback для алертов
 * 
 * @param ml ML-интеграция контекст
 * @param callback Callback функция
 * @param user_data Пользовательские данные
 */
void ml_integration_set_alert_callback(ml_integration_t* ml,
                                       void (*callback)(int, const char*, const char*, void*),
                                       void* user_data);

/**
 * @brief Установка callback для логирования
 * 
 * @param ml ML-интеграция контекст
 * @param callback Callback функция
 * @param user_data Пользовательские данные
 */
void ml_integration_set_log_callback(ml_integration_t* ml,
                                     void (*callback)(int, const char*, void*),
                                     void* user_data);

/**
 * @brief Получение статистики монитора
 * 
 * @param ml ML-интеграция контекст
 * @param monitor_id ID монитора
 * @param stats Статистика (выходной параметр)
 * @return 0 при успехе, -1 при ошибке
 */
int ml_integration_get_monitor_stats(ml_integration_t* ml,
                                     int monitor_id,
                                     ml_monitor_stats_t* stats);

/**
 * @brief Получение общей статистики
 * 
 * @param ml ML-интеграция контекст
 * @param total_anomalies Всего аномалий
 * @param total_forecasts Всего прогнозов
 * @param total_alerts Всего алертов
 */
void ml_integration_get_stats(ml_integration_t* ml,
                              uint64_t* total_anomalies,
                              uint64_t* total_forecasts,
                              uint64_t* total_alerts);

/**
 * @brief Приостановка монитора
 * 
 * @param ml ML-интеграция контекст
 * @param monitor_id ID монитора
 * @return 0 при успехе, -1 при ошибке
 */
int ml_integration_pause_monitor(ml_integration_t* ml, int monitor_id);

/**
 * @brief Возобновление монитора
 * 
 * @param ml ML-интеграция контекст
 * @param monitor_id ID монитора
 * @return 0 при успехе, -1 при ошибке
 */
int ml_integration_resume_monitor(ml_integration_t* ml, int monitor_id);

/**
 * @brief Экспорт конфигурации в JSON
 * 
 * @param ml ML-интеграция контекст
 * @param buffer Буфер для JSON
 * @param buffer_size Размер буфера
 * @return Количество записанных байт, -1 при ошибке
 */
int ml_integration_export_json(ml_integration_t* ml, char* buffer, size_t buffer_size);

/**
 * @brief Получение названия типа монитора
 * 
 * @param type Тип монитора
 * @return Строковое название
 */
const char* ml_monitor_type_to_string(ml_monitor_type_t type);

/**
 * @brief Получение названия статуса монитора
 * 
 * @param status Статус монитора
 * @return Строковое название
 */
const char* ml_monitor_status_to_string(ml_monitor_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* ML_INTEGRATION_H */
