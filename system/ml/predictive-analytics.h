/**
 * @file predictive-analytics.h
 * @brief Система прогнозирования нагрузки на основе ML-алгоритмов
 * 
 * Поддерживаемые алгоритмы:
 * - Linear Regression: линейная регрессия для трендов
 * - Moving Average: скользящее среднее для временных рядов
 * - Exponential Smoothing: экспоненциальное сглаживание
 * - ARIMA: авторегрессионная интегрированная скользящая средняя
 * - Polynomial Regression: полиномиальная регрессия
 * - Seasonal Decomposition: сезонная декомпозиция
 * 
 * @version 1.0.32
 * @date 29 марта 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#ifndef PREDICTIVE_ANALYTICS_H
#define PREDICTIVE_ANALYTICS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * Конфигурация и константы
 * ============================================ */

/** Максимальное количество точек данных для обучения */
#define PREDICT_MAX_DATA_POINTS     10000

/** Максимальный порядок полинома */
#define PREDICT_MAX_POLY_ORDER      5

/** Максимальное количество лагов для ARIMA */
#define PREDICT_MAX_LAGS            50

/** Максимальное количество прогнозов */
#define PREDICT_MAX_FORECAST        1000

/** Размер окна по умолчанию */
#define PREDICT_DEFAULT_WINDOW      60

/** Коэффициент сглаживания по умолчанию */
#define PREDICT_DEFAULT_ALPHA       0.3f

/* ============================================
 * Типы данных
 * ============================================ */

/**
 * @brief Типы алгоритмов прогнозирования
 */
typedef enum {
    PREDICT_ALGO_LINEAR_REGRESSION = 0,     /**< Линейная регрессия */
    PREDICT_ALGO_MOVING_AVG = 1,            /**< Простое скользящее среднее */
    PREDICT_ALGO_EXP_SMOOTH = 2,            /**< Экспоненциальное сглаживание */
    PREDICT_ALGO_ARIMA = 3,                 /**< ARIMA */
    PREDICT_ALGO_POLY_REGRESSION = 4,       /**< Полиномиальная регрессия */
    PREDICT_ALGO_SEASONAL = 5,              /**< Сезонная декомпозиция */
    PREDICT_ALGO_ENSEMBLE = 6               /**< Ансамбль алгоритмов */
} predict_algo_t;

/**
 * @brief Статус прогноза
 */
typedef enum {
    PREDICT_STATUS_SUCCESS = 0,             /**< Успешный прогноз */
    PREDICT_STATUS_WARNING = 1,             /**< Предупреждение (низкая уверенность) */
    PREDICT_STATUS_ERROR = 2,               /**< Ошибка прогнозирования */
    PREDICT_STATUS_INSUFFICIENT_DATA = 3    /**< Недостаточно данных */
} predict_status_t;

/**
 * @brief Типы метрик для прогнозирования
 */
typedef enum {
    PREDICT_METRIC_CONNECTIONS = 0,         /**< Количество подключений */
    PREDICT_METRIC_REQUESTS_PER_SEC = 1,    /**< Запросов в секунду */
    PREDICT_METRIC_BANDWIDTH = 2,           /**< Пропускная способность */
    PREDICT_METRIC_CPU_USAGE = 3,           /**< Использование CPU */
    PREDICT_METRIC_MEMORY_USAGE = 4,        /**< Использование памяти */
    PREDICT_METRIC_LATENCY = 5,             /**< Латентность */
    PREDICT_METRIC_ERROR_RATE = 6,          /**< Частота ошибок */
    PREDICT_METRIC_CUSTOM = 7               /**< Пользовательская метрика */
} predict_metric_t;

/**
 * @brief Конфигурация системы прогнозирования
 */
typedef struct {
    predict_algo_t algorithm;               /**< Алгоритм прогнозирования */
    size_t window_size;                     /**< Размер окна данных */
    size_t forecast_horizon;                /**< Горизонт прогнозирования */
    float alpha;                            /**< Коэффициент сглаживания (0.0 - 1.0) */
    float beta;                             /**< Коэффициент тренда (0.0 - 1.0) */
    float gamma;                            /**< Коэффициент сезонности (0.0 - 1.0) */
    size_t ar_order;                        /**< Порядок AR (для ARIMA) */
    size_t ma_order;                        /**< Порядок MA (для ARIMA) */
    size_t diff_order;                      /**< Порядок дифференцирования (для ARIMA) */
    size_t poly_order;                      /**< Порядок полинома (для полиномиальной регрессии) */
    size_t seasonal_period;                 /**< Период сезонности */
    bool enable_confidence_interval;        /**< Включить доверительный интервал */
    float confidence_level;                 /**< Уровень доверия (0.0 - 1.0) */
    bool enable_ensemble;                   /**< Включить ансамбль алгоритмов */
    bool enable_adaptive;                   /**< Адаптивная подстройка параметров */
} predict_config_t;

/**
 * @brief Точка данных временного ряда
 */
typedef struct {
    double value;                           /**< Значение метрики */
    uint64_t timestamp;                     /**< Временная метка (мс) */
    bool is_valid;                          /**< Валидность данных */
} predict_data_point_t;

/**
 * @brief Результат прогноза
 */
typedef struct {
    predict_status_t status;                /**< Статус прогноза */
    predict_algo_t algorithm;               /**< Использованный алгоритм */
    
    /* Прогнозируемые значения */
    double* forecast_values;                /**< Массив прогнозов */
    size_t forecast_count;                  /**< Количество прогнозов */
    
    /* Доверительные интервалы */
    double* lower_bound;                    /**< Нижняя граница */
    double* upper_bound;                    /**< Верхняя граница */
    
    /* Метрики качества */
    double mae;                             /**< Mean Absolute Error */
    double mse;                             /**< Mean Squared Error */
    double rmse;                            /**< Root Mean Squared Error */
    double mape;                            /**< Mean Absolute Percentage Error */
    double r_squared;                       /**< R-квадрат (коэффициент детерминации) */
    
    /* Дополнительная информация */
    double trend_slope;                     /**< Наклон тренда */
    double trend_intercept;                 /**< Пересечение тренда */
    float confidence;                       /**< Уверенность прогноза (0.0 - 1.0) */
    uint64_t forecast_timestamp;            /**< Временная метка прогноза */
    
    char description[256];                  /**< Описание прогноза */
} predict_result_t;

/**
 * @brief Модель линейной регрессии
 */
typedef struct {
    double slope;                           /**< Наклон */
    double intercept;                       /**< Пересечение */
    double r_squared;                       /**< R-квадрат */
    double std_error;                       /**< Стандартная ошибка */
    bool is_fitted;                         /**< Обучена ли модель */
} linear_regression_model_t;

/**
 * @brief Модель ARIMA
 */
typedef struct {
    double* ar_coefficients;                /**< AR коэффициенты */
    double* ma_coefficients;                /**< MA коэффициенты */
    size_t ar_order;                        /**< Порядок AR */
    size_t ma_order;                        /**< Порядок MA */
    size_t diff_order;                      /**< Порядок дифференцирования */
    double* residuals;                      /**< Остатки */
    size_t residuals_count;                 /**< Количество остатков */
    bool is_fitted;                         /**< Обучена ли модель */
} arima_model_t;

/**
 * @brief Модель полиномиальной регрессии
 */
typedef struct {
    double* coefficients;                   /**< Коэффициенты полинома */
    size_t order;                           /**< Порядок полинома */
    double r_squared;                       /**< R-квадрат */
    bool is_fitted;                         /**< Обучена ли модель */
} poly_regression_model_t;

/**
 * @brief Компоненты сезонной декомпозиции
 */
typedef struct {
    double* trend;                          /**< Тренд компонента */
    double* seasonal;                       /**< Сезонная компонента */
    double* residual;                       /**< Остаточная компонента */
    size_t period;                          /**< Период сезонности */
    size_t length;                          /**< Длина данных */
} seasonal_decomposition_t;

/**
 * @brief Основной контекст системы прогнозирования
 */
typedef struct {
    predict_config_t config;                /**< Конфигурация */
    
    /* Данные */
    predict_data_point_t* data;             /**< Массив данных */
    size_t data_count;                      /**< Текущее количество данных */
    size_t data_capacity;                   /**< Вместимость массива */
    
    /* Модели */
    linear_regression_model_t lr_model;     /**< Модель линейной регрессии */
    arima_model_t arima_model;              /**< Модель ARIMA */
    poly_regression_model_t poly_model;     /**< Модель полиномиальной регрессии */
    seasonal_decomposition_t seasonal;      /**< Сезонная декомпозиция */
    
    /* Кэширование */
    double* moving_avg_buffer;              /**< Буфер скользящего среднего */
    double exp_smooth_value;                /**< Значение экспоненциального сглаживания */
    bool exp_smooth_initialized;            /**< Инициализировано ли */
    
    /* Статистика данных */
    double data_mean;                       /**< Среднее значение */
    double data_std_dev;                    /**< Стандартное отклонение */
    double data_min;                        /**< Минимальное значение */
    double data_max;                        /**< Максимальное значение */
    double data_trend;                      /**< Тренд данных */
    
    /* Результаты ансамбля */
    predict_result_t* ensemble_results;     /**< Результаты от каждого алгоритма */
    size_t n_ensemble_algos;                /**< Количество алгоритмов в ансамбле */
    double* ensemble_weights;               /**< Веса алгоритмов */
    
    /* Статистика прогнозирования */
    size_t total_forecasts;                 /**< Всего прогнозов */
    size_t successful_forecasts;            /**< Успешные прогнозы */
    double total_error;                     /**< Суммарная ошибка */
    
    /* Callback для уведомлений */
    void (*on_forecast_complete)(const predict_result_t* result, void* user_data);
    void* user_data;                        /**< Пользовательские данные */
    
    /* Блокировка для потокобезопасности */
    void* mutex;                            /**< Мьютекс */
} predictive_analytics_t;

/* ============================================
 * Основные функции API
 * ============================================ */

/**
 * @brief Инициализация системы прогнозирования
 * 
 * @param predictor Указатель на систему прогнозирования
 * @param config Конфигурация
 * @return 0 при успехе, -1 при ошибке
 */
int predictive_analytics_init(predictive_analytics_t* predictor, 
                              const predict_config_t* config);

/**
 * @brief Очистка системы прогнозирования
 * 
 * @param predictor Указатель на систему прогнозирования
 */
void predictive_analytics_cleanup(predictive_analytics_t* predictor);

/**
 * @brief Добавление точки данных
 * 
 * @param predictor Указатель на систему прогнозирования
 * @param value Значение метрики
 * @param timestamp Временная метка (0 для текущего времени)
 * @return 0 при успехе, -1 при ошибке
 */
int predictive_analytics_add_data(predictive_analytics_t* predictor, 
                                  double value, uint64_t timestamp);

/**
 * @brief Обучение модели на исторических данных
 * 
 * @param predictor Указатель на систему прогнозирования
 * @param values Массив значений
 * @param count Количество точек
 * @return 0 при успехе, -1 при ошибке
 */
int predictive_analytics_train(predictive_analytics_t* predictor,
                               const double* values, size_t count);

/**
 * @brief Прогнозирование на N шагов вперед
 * 
 * @param predictor Указатель на систему прогнозирования
 * @param steps Количество шагов прогноза
 * @param result Результат прогноза
 * @return 0 при успехе, -1 при ошибке
 */
int predictive_analytics_forecast(predictive_analytics_t* predictor,
                                  size_t steps, predict_result_t* result);

/**
 * @brief Прогнозирование следующего значения
 * 
 * @param predictor Указатель на систему прогнозирования
 * @param next_value Прогнозируемое значение
 * @param confidence Уверенность прогноза (0.0 - 1.0)
 * @return 0 при успехе, -1 при ошибке
 */
int predictive_analytics_predict_next(predictive_analytics_t* predictor,
                                      double* next_value, float* confidence);

/**
 * @brief Получение статистики данных
 * 
 * @param predictor Указатель на систему прогнозирования
 * @param mean Среднее значение
 * @param std_dev Стандартное отклонение
 * @param min Минимальное значение
 * @param max Максимальное значение
 * @param trend Тренд
 */
void predictive_analytics_get_data_stats(predictive_analytics_t* predictor,
                                         double* mean, double* std_dev,
                                         double* min, double* max, double* trend);

/**
 * @brief Установка параметров конфигурации
 * 
 * @param predictor Указатель на систему прогнозирования
 * @param window_size Размер окна
 * @param alpha Коэффициент сглаживания
 * @param forecast_horizon Горизонт прогнозирования
 * @return 0 при успехе, -1 при ошибке
 */
int predictive_analytics_set_params(predictive_analytics_t* predictor,
                                    size_t window_size, float alpha,
                                    size_t forecast_horizon);

/**
 * @brief Установка callback функции
 * 
 * @param predictor Указатель на систему прогнозирования
 * @param callback Callback функция
 * @param user_data Пользовательские данные
 */
void predictive_analytics_set_callback(predictive_analytics_t* predictor,
                                       void (*callback)(const predict_result_t*, void*),
                                       void* user_data);

/**
 * @brief Получение статистики прогнозирования
 * 
 * @param predictor Указатель на систему прогнозирования
 * @param total_forecasts Всего прогнозов
 * @param success_rate Процент успешных прогнозов
 * @param avg_error Средняя ошибка
 */
void predictive_analytics_get_stats(predictive_analytics_t* predictor,
                                    size_t* total_forecasts,
                                    float* success_rate,
                                    double* avg_error);

/**
 * @brief Сброс статистики
 * 
 * @param predictor Указатель на систему прогнозирования
 */
void predictive_analytics_reset_stats(predictive_analytics_t* predictor);

/**
 * @brief Оценка качества модели
 * 
 * @param predictor Указатель на систему прогнозирования
 * @param mae Mean Absolute Error
 * @param mse Mean Squared Error
 * @param rmse Root Mean Squared Error
 * @param mape Mean Absolute Percentage Error
 * @param r_squared R-квадрат
 * @return 0 при успехе, -1 при ошибке
 */
int predictive_analytics_evaluate(predictive_analytics_t* predictor,
                                  double* mae, double* mse, double* rmse,
                                  double* mape, double* r_squared);

/**
 * @brief Экспорт модели в JSON
 * 
 * @param predictor Указатель на систему прогнозирования
 * @param buffer Буфер для JSON
 * @param buffer_size Размер буфера
 * @return Количество записанных байт, -1 при ошибке
 */
int predictive_analytics_export_json(predictive_analytics_t* predictor,
                                     char* buffer, size_t buffer_size);

/**
 * @brief Импорт модели из JSON
 * 
 * @param predictor Указатель на систему прогнозирования
 * @param json JSON данные
 * @return 0 при успехе, -1 при ошибке
 */
int predictive_analytics_import_json(predictive_analytics_t* predictor, const char* json);

/**
 * @brief Получение названия алгоритма
 * 
 * @param algo Алгоритм
 * @return Строковое название
 */
const char* predict_algo_to_string(predict_algo_t algo);

/**
 * @brief Получение статуса прогноза
 * 
 * @param status Статус
 * @return Строковое описание
 */
const char* predict_status_to_string(predict_status_t status);

/**
 * @brief Получение названия метрики
 * 
 * @param metric Метрика
 * @return Строковое название
 */
const char* predict_metric_to_string(predict_metric_t metric);

/* ============================================
 * Вспомогательные функции
 * ============================================ */

/**
 * @brief Вычисление линейной регрессии
 * 
 * @param x Независимая переменная
 * @param y Зависимая переменная
 * @param n Количество точек
 * @param slope Наклон
 * @param intercept Пересечение
 * @param r_squared R-квадрат
 * @return 0 при успехе, -1 при ошибке
 */
int compute_linear_regression(const double* x, const double* y, size_t n,
                              double* slope, double* intercept, double* r_squared);

/**
 * @brief Вычисление скользящего среднего
 * 
 * @param data Данные
 * @param n Количество точек
 * @param window Размер окна
 * @param result Результат (массив размера n)
 * @return 0 при успехе, -1 при ошибке
 */
int compute_moving_average(const double* data, size_t n, size_t window, double* result);

/**
 * @brief Экспоненциальное сглаживание
 * 
 * @param data Данные
 * @param n Количество точек
 * @param alpha Коэффициент сглаживания
 * @param result Результат
 * @return 0 при успехе, -1 при ошибке
 */
int compute_exponential_smoothing(const double* data, size_t n, float alpha, double* result);

/**
 * @brief Вычисление ошибок прогноза
 * 
 * @param actual Фактические значения
 * @param predicted Прогнозируемые значения
 * @param n Количество точек
 * @param mae Mean Absolute Error
 * @param mse Mean Squared Error
 * @param rmse Root Mean Squared Error
 * @param mape Mean Absolute Percentage Error
 * @return 0 при успехе, -1 при ошибке
 */
int compute_forecast_errors(const double* actual, const double* predicted, size_t n,
                            double* mae, double* mse, double* rmse, double* mape);

#ifdef __cplusplus
}
#endif

#endif /* PREDICTIVE_ANALYTICS_H */
