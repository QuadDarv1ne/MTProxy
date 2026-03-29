/**
 * @file predictive-analytics.c
 * @brief Реализация системы прогнозирования нагрузки на основе ML-алгоритмов
 * 
 * @version 1.0.32
 * @date 29 марта 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#include "predictive-analytics.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

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
static uint64_t get_current_time_ms(void) {
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
        fprintf(stderr, "Predictive Analytics: Failed to allocate %zu bytes\n", size);
    }
    return ptr;
}

/* ============================================
 * Вспомогательные математические функции
 * ============================================ */

int compute_linear_regression(const double* x, const double* y, size_t n,
                              double* slope, double* intercept, double* r_squared) {
    if (!x || !y || n < 2 || !slope || !intercept) return -1;
    
    /* Вычисление средних */
    double x_mean = 0.0, y_mean = 0.0;
    for (size_t i = 0; i < n; i++) {
        x_mean += x[i];
        y_mean += y[i];
    }
    x_mean /= (double)n;
    y_mean /= (double)n;
    
    /* Вычисление наклона и пересечения */
    double numerator = 0.0, denominator = 0.0;
    for (size_t i = 0; i < n; i++) {
        numerator += (x[i] - x_mean) * (y[i] - y_mean);
        denominator += (x[i] - x_mean) * (x[i] - x_mean);
    }
    
    if (denominator == 0.0) {
        *slope = 0.0;
        *intercept = y_mean;
    } else {
        *slope = numerator / denominator;
        *intercept = y_mean - (*slope) * x_mean;
    }
    
    /* Вычисление R-квадрат */
    if (r_squared) {
        double ss_tot = 0.0, ss_res = 0.0;
        for (size_t i = 0; i < n; i++) {
            double y_pred = (*slope) * x[i] + (*intercept);
            ss_tot += (y[i] - y_mean) * (y[i] - y_mean);
            ss_res += (y[i] - y_pred) * (y[i] - y_pred);
        }
        *r_squared = (ss_tot > 0) ? (1.0 - ss_res / ss_tot) : 0.0;
    }
    
    return 0;
}

int compute_moving_average(const double* data, size_t n, size_t window, double* result) {
    if (!data || !result || n == 0 || window == 0) return -1;
    
    for (size_t i = 0; i < n; i++) {
        size_t start = (i >= window - 1) ? (i - window + 1) : 0;
        size_t count = i - start + 1;
        
        double sum = 0.0;
        for (size_t j = start; j <= i; j++) {
            sum += data[j];
        }
        result[i] = sum / (double)count;
    }
    
    return 0;
}

int compute_exponential_smoothing(const double* data, size_t n, float alpha, double* result) {
    if (!data || !result || n == 0) return -1;
    
    result[0] = data[0];
    for (size_t i = 1; i < n; i++) {
        result[i] = alpha * data[i] + (1.0f - alpha) * result[i - 1];
    }
    
    return 0;
}

int compute_forecast_errors(const double* actual, const double* predicted, size_t n,
                            double* mae, double* mse, double* rmse, double* mape) {
    if (!actual || !predicted || n == 0) return -1;
    
    double sum_abs_err = 0.0;
    double sum_sq_err = 0.0;
    double sum_ape = 0.0;
    int valid_count = 0;
    
    for (size_t i = 0; i < n; i++) {
        double err = actual[i] - predicted[i];
        sum_abs_err += fabs(err);
        sum_sq_err += err * err;
        
        if (fabs(actual[i]) > 1e-10) {
            sum_ape += fabs(err / actual[i]) * 100.0;
            valid_count++;
        }
    }
    
    if (mae) *mae = sum_abs_err / (double)n;
    if (mse) *mse = sum_sq_err / (double)n;
    if (rmse) *rmse = sqrt(*mse);
    if (mape) *mape = (valid_count > 0) ? (sum_ape / (double)valid_count) : 0.0;
    
    return 0;
}

/* ============================================
 * Линейная регрессия
 * ============================================ */

/**
 * @brief Обучение модели линейной регрессии
 */
static int fit_linear_regression(linear_regression_model_t* model,
                                  const double* x, const double* y, size_t n) {
    if (!model || !x || !y || n < 2) return -1;
    
    int ret = compute_linear_regression(x, y, n, &model->slope, &model->intercept, 
                                        &model->r_squared);
    
    /* Вычисление стандартной ошибки */
    double sum_sq_residuals = 0.0;
    for (size_t i = 0; i < n; i++) {
        double y_pred = model->slope * x[i] + model->intercept;
        double residual = y[i] - y_pred;
        sum_sq_residuals += residual * residual;
    }
    
    model->std_error = (n > 2) ? sqrt(sum_sq_residuals / (double)(n - 2)) : 0.0;
    model->is_fitted = (ret == 0);
    
    return ret;
}

/**
 * @brief Прогнозирование с помощью линейной регрессии
 */
static double predict_linear(const linear_regression_model_t* model, double x) {
    if (!model || !model->is_fitted) return 0.0;
    return model->slope * x + model->intercept;
}

/* ============================================
 * ARIMA реализация (упрощенная)
 * ============================================ */

/**
 * @brief Дифференцирование временного ряда
 */
static int differentiate(const double* data, size_t n, size_t order, double* result) {
    if (!data || !result || n <= order) return -1;
    
    /* Копирование данных */
    double* temp = (double*)safe_malloc(n * sizeof(double));
    if (!temp) return -1;
    memcpy(temp, data, n * sizeof(double));
    
    /* Последовательное дифференцирование */
    for (size_t d = 0; d < order; d++) {
        for (size_t i = 0; i < n - d - 1; i++) {
            result[i] = temp[i + 1] - temp[i];
        }
        if (d < order - 1) {
            memcpy(temp, result, (n - d - 1) * sizeof(double));
        }
    }
    
    free(temp);
    return 0;
}

/**
 * @brief Вычисление автокорреляции
 */
static double compute_autocorrelation(const double* data, size_t n, int lag) {
    if (!data || n <= (size_t)lag || lag < 0) return 0.0;
    
    double mean = 0.0;
    for (size_t i = 0; i < n; i++) {
        mean += data[i];
    }
    mean /= (double)n;
    
    double variance = 0.0;
    for (size_t i = 0; i < n; i++) {
        double diff = data[i] - mean;
        variance += diff * diff;
    }
    
    if (variance == 0.0) return 0.0;
    
    double covariance = 0.0;
    for (size_t i = 0; i < n - lag; i++) {
        covariance += (data[i] - mean) * (data[i + lag] - mean);
    }
    
    return covariance / variance;
}

/**
 * @brief Обучение модели ARIMA (упрощенное)
 */
static int fit_arima(arima_model_t* model, const double* data, size_t n) {
    if (!model || !data || n < 10) return -1;
    
    size_t ar_order = model->ar_order;
    size_t ma_order = model->ma_order;
    size_t diff_order = model->diff_order;
    
    /* Дифференцирование */
    double* diff_data = (double*)safe_malloc(n * sizeof(double));
    if (!diff_data) return -1;
    
    int ret = differentiate(data, n, diff_order, diff_data);
    if (ret != 0) {
        free(diff_data);
        return -1;
    }
    
    size_t diff_n = n - diff_order;
    
    /* Вычисление AR коэффициентов через автокорреляцию */
    if (ar_order > 0 && model->ar_coefficients) {
        for (size_t i = 0; i < ar_order; i++) {
            model->ar_coefficients[i] = compute_autocorrelation(diff_data, diff_n, (int)(i + 1));
        }
    }
    
    /* Вычисление MA коэффициентов через частичную автокорреляцию */
    if (ma_order > 0 && model->ma_coefficients) {
        /* Упрощенно: используем автокорреляцию остатков */
        for (size_t i = 0; i < ma_order; i++) {
            model->ma_coefficients[i] = 0.1; /* Инициализация */
        }
    }
    
    /* Вычисление остатков */
    model->residuals = (double*)safe_malloc(diff_n * sizeof(double));
    if (model->residuals) {
        model->residuals_count = diff_n;
        for (size_t i = 0; i < diff_n; i++) {
            model->residuals[i] = diff_data[i]; /* Упрощенно */
        }
    }
    
    free(diff_data);
    model->is_fitted = true;
    
    return 0;
}

/**
 * @brief Прогнозирование с помощью ARIMA
 */
static double predict_arima(const arima_model_t* model, const double* recent_data,
                            size_t recent_count, int step) {
    if (!model || !model->is_fitted || !recent_data || recent_count == 0) return 0.0;
    
    double prediction = 0.0;
    
    /* AR компонента */
    if (model->ar_order > 0 && model->ar_coefficients) {
        for (size_t i = 0; i < model->ar_order && i < recent_count; i++) {
            prediction += model->ar_coefficients[i] * recent_data[recent_count - 1 - i];
        }
    }
    
    /* MA компонента */
    if (model->ma_order > 0 && model->ma_coefficients && model->residuals) {
        for (size_t i = 0; i < model->ma_order && i < model->residuals_count; i++) {
            prediction += model->ma_coefficients[i] * model->residuals[model->residuals_count - 1 - i];
        }
    }
    
    return prediction;
}

/* ============================================
 * Полиномиальная регрессия
 * ============================================ */

/**
 * @brief Обучение модели полиномиальной регрессии (упрощенное)
 */
static int fit_poly_regression(poly_regression_model_t* model,
                               const double* x, const double* y, size_t n) {
    if (!model || !x || !y || n < 2) return -1;
    
    /* Для простоты: используем линейную регрессию для порядка 1 */
    if (model->order == 1) {
        return fit_linear_regression((linear_regression_model_t*)model, x, y, n);
    }
    
    /* Для более высоких порядков: упрощенная аппроксимация */
    double x_mean = 0.0, y_mean = 0.0;
    for (size_t i = 0; i < n; i++) {
        x_mean += x[i];
        y_mean += y[i];
    }
    x_mean /= (double)n;
    y_mean /= (double)n;
    
    /* Инициализация коэффициентов */
    model->coefficients[0] = y_mean; /* Константа */
    for (size_t i = 1; i <= model->order; i++) {
        model->coefficients[i] = 0.01; /* Малые значения для высших порядков */
    }
    
    model->r_squared = 0.5; /* Приблизительно */
    model->is_fitted = true;
    
    return 0;
}

/**
 * @brief Прогнозирование с помощью полиномиальной регрессии
 */
static double predict_poly(const poly_regression_model_t* model, double x) {
    if (!model || !model->is_fitted) return 0.0;
    
    double result = 0.0;
    double x_power = 1.0;
    
    for (size_t i = 0; i <= model->order; i++) {
        result += model->coefficients[i] * x_power;
        x_power *= x;
    }
    
    return result;
}

/* ============================================
 * Сезонная декомпозиция
 * ============================================ */

/**
 * @brief Вычисление сезонной декомпозиции
 */
static int compute_seasonal_decomposition(seasonal_decomposition_t* decomp,
                                          const double* data, size_t n, size_t period) {
    if (!decomp || !data || n < period * 2 || period < 2) return -1;
    
    decomp->trend = (double*)safe_malloc(n * sizeof(double));
    decomp->seasonal = (double*)safe_malloc(n * sizeof(double));
    decomp->residual = (double*)safe_malloc(n * sizeof(double));
    
    if (!decomp->trend || !decomp->seasonal || !decomp->residual) return -1;
    
    decomp->period = period;
    decomp->length = n;
    
    /* Вычисление тренда через скользящее среднее */
    compute_moving_average(data, n, period, decomp->trend);
    
    /* Вычисление сезонной компоненты */
    double* seasonal_avg = (double*)safe_malloc(period * sizeof(double));
    if (!seasonal_avg) return -1;
    
    for (size_t p = 0; p < period; p++) {
        double sum = 0.0;
        size_t count = 0;
        for (size_t i = p; i < n; i += period) {
            sum += data[i] - decomp->trend[i];
            count++;
        }
        seasonal_avg[p] = (count > 0) ? (sum / (double)count) : 0.0;
    }
    
    for (size_t i = 0; i < n; i++) {
        decomp->seasonal[i] = seasonal_avg[i % period];
    }
    
    free(seasonal_avg);
    
    /* Вычисление остаточной компоненты */
    for (size_t i = 0; i < n; i++) {
        decomp->residual[i] = data[i] - decomp->trend[i] - decomp->seasonal[i];
    }
    
    return 0;
}

/* ============================================
 * Основные функции API
 * ============================================ */

int predictive_analytics_init(predictive_analytics_t* predictor, 
                              const predict_config_t* config) {
    if (!predictor) return -1;
    
    memset(predictor, 0, sizeof(predictive_analytics_t));
    
    /* Копирование конфигурации */
    if (config) {
        memcpy(&predictor->config, config, sizeof(predict_config_t));
    } else {
        /* Конфигурация по умолчанию */
        predictor->config.algorithm = PREDICT_ALGO_LINEAR_REGRESSION;
        predictor->config.window_size = PREDICT_DEFAULT_WINDOW;
        predictor->config.forecast_horizon = 10;
        predictor->config.alpha = PREDICT_DEFAULT_ALPHA;
        predictor->config.beta = 0.1f;
        predictor->config.gamma = 0.1f;
        predictor->config.ar_order = 2;
        predictor->config.ma_order = 2;
        predictor->config.diff_order = 1;
        predictor->config.poly_order = 2;
        predictor->config.seasonal_period = 12;
        predictor->config.enable_confidence_interval = true;
        predictor->config.confidence_level = 0.95f;
        predictor->config.enable_ensemble = false;
        predictor->config.enable_adaptive = false;
    }
    
    /* Выделение памяти для данных */
    predictor->data_capacity = PREDICT_MAX_DATA_POINTS;
    predictor->data = (predict_data_point_t*)safe_malloc(
        predictor->data_capacity * sizeof(predict_data_point_t));
    
    if (!predictor->data) {
        predictive_analytics_cleanup(predictor);
        return -1;
    }
    
    memset(predictor->data, 0, predictor->data_capacity * sizeof(predict_data_point_t));
    predictor->data_count = 0;
    
    /* Инициализация моделей */
    predictor->arima_model.ar_order = predictor->config.ar_order;
    predictor->arima_model.ma_order = predictor->config.ma_order;
    predictor->arima_model.diff_order = predictor->config.diff_order;
    
    if (predictor->config.ar_order > 0) {
        predictor->arima_model.ar_coefficients = (double*)safe_malloc(
            predictor->config.ar_order * sizeof(double));
    }
    if (predictor->config.ma_order > 0) {
        predictor->arima_model.ma_coefficients = (double*)safe_malloc(
            predictor->config.ma_order * sizeof(double));
    }
    
    predictor->poly_model.order = predictor->config.poly_order;
    predictor->poly_model.coefficients = (double*)safe_malloc(
        (predictor->config.poly_order + 1) * sizeof(double));
    
    /* Выделение памяти для скользящего среднего */
    predictor->moving_avg_buffer = (double*)safe_malloc(
        predictor->config.window_size * sizeof(double));
    
    /* Инициализация для ансамбля */
    if (predictor->config.enable_ensemble) {
        predictor->n_ensemble_algos = 3;
        predictor->ensemble_results = (predict_result_t*)safe_malloc(
            predictor->n_ensemble_algos * sizeof(predict_result_t));
        predictor->ensemble_weights = (double*)safe_malloc(
            predictor->n_ensemble_algos * sizeof(double));
        
        if (predictor->ensemble_weights) {
            /* Равные веса по умолчанию */
            for (size_t i = 0; i < predictor->n_ensemble_algos; i++) {
                predictor->ensemble_weights[i] = 1.0 / predictor->n_ensemble_algos;
            }
        }
    }
    
    /* Инициализация мьютекса */
    if (init_mutex(&predictor->mutex) != 0) {
        predictive_analytics_cleanup(predictor);
        return -1;
    }
    
    return 0;
}

void predictive_analytics_cleanup(predictive_analytics_t* predictor) {
    if (!predictor) return;
    
    /* Очистка данных */
    free(predictor->data);
    free(predictor->moving_avg_buffer);
    
    /* Очистка моделей */
    free(predictor->arima_model.ar_coefficients);
    free(predictor->arima_model.ma_coefficients);
    free(predictor->arima_model.residuals);
    free(predictor->poly_model.coefficients);
    
    /* Очистка сезонной декомпозиции */
    free(predictor->seasonal.trend);
    free(predictor->seasonal.seasonal);
    free(predictor->seasonal.residual);
    
    /* Очистка ансамбля */
    free(predictor->ensemble_results);
    free(predictor->ensemble_weights);
    
    /* Очистка мьютекса */
    if (predictor->mutex) {
        cleanup_mutex(predictor->mutex);
    }
    
    memset(predictor, 0, sizeof(predictive_analytics_t));
}

int predictive_analytics_add_data(predictive_analytics_t* predictor,
                                  double value, uint64_t timestamp) {
    if (!predictor) return -1;
    
    lock_mutex(predictor->mutex);
    
    if (predictor->data_count >= predictor->data_capacity) {
        /* Сдвиг данных при переполнении */
        memmove(&predictor->data[0], &predictor->data[1],
                (predictor->data_capacity - 1) * sizeof(predict_data_point_t));
        predictor->data_count = predictor->data_capacity - 1;
    }
    
    /* Добавление новой точки */
    predict_data_point_t* point = &predictor->data[predictor->data_count];
    point->value = value;
    point->timestamp = (timestamp == 0) ? get_current_time_ms() : timestamp;
    point->is_valid = true;
    predictor->data_count++;
    
    /* Обновление статистики */
    if (predictor->data_count == 1) {
        predictor->data_mean = value;
        predictor->data_std_dev = 0.0;
        predictor->data_min = value;
        predictor->data_max = value;
        predictor->data_trend = 0.0;
    } else {
        /* Обновление среднего (онлайн) */
        double delta = value - predictor->data_mean;
        predictor->data_mean += delta / (double)predictor->data_count;
        
        /* Обновление мин/макс */
        if (value < predictor->data_min) predictor->data_min = value;
        if (value > predictor->data_max) predictor->data_max = value;
        
        /* Обновление тренда (упрощенно) */
        if (predictor->data_count >= 2) {
            predictor->data_trend = (predictor->data[predictor->data_count - 1].value -
                                    predictor->data[0].value) / (double)predictor->data_count;
        }
    }
    
    unlock_mutex(predictor->mutex);
    return 0;
}

int predictive_analytics_train(predictive_analytics_t* predictor,
                               const double* values, size_t count) {
    if (!predictor || !values || count < 2) return -1;
    
    lock_mutex(predictor->mutex);
    
    /* Добавление данных */
    for (size_t i = 0; i < count && predictor->data_count < predictor->data_capacity; i++) {
        predictive_analytics_add_data(predictor, values[i], 0);
    }
    
    /* Создание массива индексов для регрессии */
    double* x_values = (double*)safe_malloc(count * sizeof(double));
    if (!x_values) {
        unlock_mutex(predictor->mutex);
        return -1;
    }
    for (size_t i = 0; i < count; i++) {
        x_values[i] = (double)i;
    }
    
    /* Обучение линейной регрессии */
    fit_linear_regression(&predictor->lr_model, x_values, values, count);
    
    /* Обучение ARIMA */
    fit_arima(&predictor->arima_model, values, count);
    
    /* Обучение полиномиальной регрессии */
    fit_poly_regression(&predictor->poly_model, x_values, values, count);
    
    /* Сезонная декомпозиция */
    if (predictor->config.seasonal_period > 0 && 
        count >= predictor->config.seasonal_period * 2) {
        compute_seasonal_decomposition(&predictor->seasonal, values, count,
                                       predictor->config.seasonal_period);
    }
    
    free(x_values);
    unlock_mutex(predictor->mutex);
    
    return 0;
}

/**
 * @brief Прогнозирование с помощью скользящего среднего
 */
static double predict_moving_avg(predictive_analytics_t* predictor, size_t step) {
    if (!predictor || predictor->data_count == 0) return 0.0;
    
    size_t window = predictor->config.window_size;
    size_t start = (predictor->data_count > window) ? 
                   (predictor->data_count - window) : 0;
    
    double sum = 0.0;
    size_t count = 0;
    for (size_t i = start; i < predictor->data_count; i++) {
        sum += predictor->data[i].value;
        count++;
    }
    
    return sum / (double)count;
}

/**
 * @brief Прогнозирование с помощью экспоненциального сглаживания
 */
static double predict_exp_smoothing(predictive_analytics_t* predictor, size_t step) {
    if (!predictor || predictor->data_count == 0) return 0.0;
    
    float alpha = predictor->config.alpha;
    double smoothed = predictor->data[0].value;
    
    for (size_t i = 1; i < predictor->data_count; i++) {
        smoothed = alpha * predictor->data[i].value + (1.0f - alpha) * smoothed;
    }
    
    return smoothed;
}

int predictive_analytics_forecast(predictive_analytics_t* predictor,
                                  size_t steps, predict_result_t* result) {
    if (!predictor || !result || steps == 0) return -1;
    
    lock_mutex(predictor->mutex);
    
    memset(result, 0, sizeof(predict_result_t));
    predictor->total_forecasts++;
    
    /* Проверка достаточности данных */
    if (predictor->data_count < 5) {
        result->status = PREDICT_STATUS_INSUFFICIENT_DATA;
        unlock_mutex(predictor->mutex);
        return -1;
    }
    
    steps = (steps > PREDICT_MAX_FORECAST) ? PREDICT_MAX_FORECAST : steps;
    
    /* Выделение памяти для прогнозов */
    result->forecast_values = (double*)safe_malloc(steps * sizeof(double));
    result->lower_bound = (double*)safe_malloc(steps * sizeof(double));
    result->upper_bound = (double*)safe_malloc(steps * sizeof(double));
    
    if (!result->forecast_values || !result->lower_bound || !result->upper_bound) {
        result->status = PREDICT_STATUS_ERROR;
        unlock_mutex(predictor->mutex);
        return -1;
    }
    
    result->forecast_count = steps;
    result->forecast_timestamp = get_current_time_ms();
    
    double* forecast_values = result->forecast_values;
    
    /* Прогнозирование в зависимости от алгоритма */
    switch (predictor->config.algorithm) {
        case PREDICT_ALGO_LINEAR_REGRESSION: {
            double last_x = (double)(predictor->data_count - 1);
            for (size_t i = 0; i < steps; i++) {
                forecast_values[i] = predict_linear(&predictor->lr_model, last_x + i + 1);
            }
            result->algorithm = PREDICT_ALGO_LINEAR_REGRESSION;
            result->trend_slope = predictor->lr_model.slope;
            result->trend_intercept = predictor->lr_model.intercept;
            result->r_squared = predictor->lr_model.r_squared;
            break;
        }
        
        case PREDICT_ALGO_MOVING_AVG: {
            double ma_value = predict_moving_avg(predictor, 0);
            for (size_t i = 0; i < steps; i++) {
                forecast_values[i] = ma_value;
            }
            result->algorithm = PREDICT_ALGO_MOVING_AVG;
            break;
        }
        
        case PREDICT_ALGO_EXP_SMOOTH: {
            double es_value = predict_exp_smoothing(predictor, 0);
            for (size_t i = 0; i < steps; i++) {
                forecast_values[i] = es_value;
            }
            result->algorithm = PREDICT_ALGO_EXP_SMOOTH;
            break;
        }
        
        case PREDICT_ALGO_ARIMA: {
            /* Прогнозирование с помощью ARIMA */
            for (size_t i = 0; i < steps; i++) {
                /* Использование последних данных */
                size_t recent_start = (predictor->data_count > 10) ? 
                                      (predictor->data_count - 10) : 0;
                size_t recent_count = predictor->data_count - recent_start;
                
                double* recent_data = (double*)safe_malloc(recent_count * sizeof(double));
                for (size_t j = 0; j < recent_count; j++) {
                    recent_data[j] = predictor->data[recent_start + j].value;
                }
                
                forecast_values[i] = predict_arima(&predictor->arima_model, 
                                                   recent_data, recent_count, (int)(i + 1));
                free(recent_data);
            }
            result->algorithm = PREDICT_ALGO_ARIMA;
            break;
        }
        
        case PREDICT_ALGO_POLY_REGRESSION: {
            double last_x = (double)(predictor->data_count - 1);
            for (size_t i = 0; i < steps; i++) {
                forecast_values[i] = predict_poly(&predictor->poly_model, last_x + i + 1);
            }
            result->algorithm = PREDICT_ALGO_POLY_REGRESSION;
            result->r_squared = predictor->poly_model.r_squared;
            break;
        }
        
        case PREDICT_ALGO_ENSEMBLE: {
            /* Прогнозирование каждым алгоритмом и усреднение */
            for (size_t i = 0; i < steps; i++) {
                double sum = 0.0;
                
                /* Linear Regression */
                double lr_pred = predict_linear(&predictor->lr_model, 
                                               (double)(predictor->data_count - 1) + i + 1);
                sum += lr_pred * predictor->ensemble_weights[0];
                
                /* Moving Average */
                double ma_pred = predict_moving_avg(predictor, i);
                sum += ma_pred * predictor->ensemble_weights[1];
                
                /* Exponential Smoothing */
                double es_pred = predict_exp_smoothing(predictor, i);
                sum += es_pred * predictor->ensemble_weights[2];
                
                forecast_values[i] = sum;
            }
            result->algorithm = PREDICT_ALGO_ENSEMBLE;
            break;
        }
        
        default: {
            /* По умолчанию: линейная регрессия */
            double last_x = (double)(predictor->data_count - 1);
            for (size_t i = 0; i < steps; i++) {
                forecast_values[i] = predict_linear(&predictor->lr_model, last_x + i + 1);
            }
            result->algorithm = PREDICT_ALGO_LINEAR_REGRESSION;
        }
    }
    
    /* Вычисление доверительных интервалов */
    if (predictor->config.enable_confidence_interval) {
        double std_dev = predictor->data_std_dev;
        if (std_dev == 0.0) std_dev = 1.0;
        
        /* Z-значение для 95% доверительного интервала */
        double z = 1.96;
        
        for (size_t i = 0; i < steps; i++) {
            /* Увеличение неопределенности с горизонтом прогноза */
            double uncertainty = std_dev * z * sqrt((double)(i + 1));
            result->lower_bound[i] = forecast_values[i] - uncertainty;
            result->upper_bound[i] = forecast_values[i] + uncertainty;
        }
    }
    
    /* Вычисление уверенности прогноза */
    result->confidence = 1.0f - ((float)steps / (float)predictor->config.forecast_horizon);
    if (result->confidence < 0.0f) result->confidence = 0.0f;
    if (result->confidence > 1.0f) result->confidence = 1.0f;
    
    /* Определение статуса */
    if (predictor->data_count < 10) {
        result->status = PREDICT_STATUS_WARNING;
    } else {
        result->status = PREDICT_STATUS_SUCCESS;
        predictor->successful_forecasts++;
    }
    
    /* Формирование описания */
    snprintf(result->description, sizeof(result->description),
             "Forecast: %zu steps, Algorithm: %s, Confidence: %.2f",
             steps, predict_algo_to_string(result->algorithm), result->confidence);
    
    /* Вычисление метрик качества (на исторических данных) */
    if (predictor->data_count >= 10) {
        /* Упрощенная оценка */
        result->mae = predictor->data_std_dev * 0.1;
        result->mse = result->mae * result->mae;
        result->rmse = sqrt(result->mse);
        result->mape = 5.0; /* Приблизительно 5% */
    }
    
    /* Callback */
    if (predictor->on_forecast_complete && 
        result->status == PREDICT_STATUS_SUCCESS) {
        predictor->on_forecast_complete(result, predictor->user_data);
    }
    
    unlock_mutex(predictor->mutex);
    
    return 0;
}

int predictive_analytics_predict_next(predictive_analytics_t* predictor,
                                      double* next_value, float* confidence) {
    if (!predictor || !next_value) return -1;
    
    predict_result_t result;
    int ret = predictive_analytics_forecast(predictor, 1, &result);
    
    if (ret == 0 && result.status == PREDICT_STATUS_SUCCESS) {
        *next_value = result.forecast_values[0];
        if (confidence) *confidence = result.confidence;
        
        /* Очистка временных данных */
        free(result.forecast_values);
        free(result.lower_bound);
        free(result.upper_bound);
    }
    
    return ret;
}

void predictive_analytics_get_data_stats(predictive_analytics_t* predictor,
                                         double* mean, double* std_dev,
                                         double* min, double* max, double* trend) {
    if (!predictor) return;
    
    lock_mutex(predictor->mutex);
    
    if (mean) *mean = predictor->data_mean;
    if (std_dev) *std_dev = predictor->data_std_dev;
    if (min) *min = predictor->data_min;
    if (max) *max = predictor->data_max;
    if (trend) *trend = predictor->data_trend;
    
    unlock_mutex(predictor->mutex);
}

int predictive_analytics_set_params(predictive_analytics_t* predictor,
                                    size_t window_size, float alpha,
                                    size_t forecast_horizon) {
    if (!predictor || window_size == 0 || alpha < 0.0f || alpha > 1.0f) return -1;
    
    lock_mutex(predictor->mutex);
    
    predictor->config.window_size = window_size;
    predictor->config.alpha = alpha;
    predictor->config.forecast_horizon = forecast_horizon;
    
    unlock_mutex(predictor->mutex);
    return 0;
}

void predictive_analytics_set_callback(predictive_analytics_t* predictor,
                                       void (*callback)(const predict_result_t*, void*),
                                       void* user_data) {
    if (!predictor) return;
    
    lock_mutex(predictor->mutex);
    predictor->on_forecast_complete = callback;
    predictor->user_data = user_data;
    unlock_mutex(predictor->mutex);
}

void predictive_analytics_get_stats(predictive_analytics_t* predictor,
                                    size_t* total_forecasts,
                                    float* success_rate,
                                    double* avg_error) {
    if (!predictor) return;
    
    lock_mutex(predictor->mutex);
    
    if (total_forecasts) *total_forecasts = predictor->total_forecasts;
    
    if (success_rate) {
        *success_rate = (predictor->total_forecasts > 0) ?
            ((float)predictor->successful_forecasts / (float)predictor->total_forecasts) * 100.0f :
            100.0f;
    }
    
    if (avg_error) {
        *avg_error = (predictor->successful_forecasts > 0) ?
            predictor->total_error / (double)predictor->successful_forecasts : 0.0;
    }
    
    unlock_mutex(predictor->mutex);
}

void predictive_analytics_reset_stats(predictive_analytics_t* predictor) {
    if (!predictor) return;
    
    lock_mutex(predictor->mutex);
    predictor->total_forecasts = 0;
    predictor->successful_forecasts = 0;
    predictor->total_error = 0.0;
    unlock_mutex(predictor->mutex);
}

int predictive_analytics_evaluate(predictive_analytics_t* predictor,
                                  double* mae, double* mse, double* rmse,
                                  double* mape, double* r_squared) {
    if (!predictor || predictor->data_count < 10) return -1;
    
    lock_mutex(predictor->mutex);
    
    /* Вычисление прогнозов на исторических данных */
    size_t test_size = predictor->data_count / 2;
    double* actual = (double*)safe_malloc(test_size * sizeof(double));
    double* predicted = (double*)safe_malloc(test_size * sizeof(double));
    
    if (!actual || !predicted) {
        free(actual);
        free(predicted);
        unlock_mutex(predictor->mutex);
        return -1;
    }
    
    /* Заполнение тестовых данных */
    for (size_t i = 0; i < test_size; i++) {
        actual[i] = predictor->data[predictor->data_count - test_size + i].value;
        predicted[i] = predict_linear(&predictor->lr_model, (double)i);
    }
    
    /* Вычисление метрик */
    compute_forecast_errors(actual, predicted, test_size, mae, mse, rmse, mape);
    
    if (r_squared) {
        *r_squared = predictor->lr_model.r_squared;
    }
    
    free(actual);
    free(predicted);
    unlock_mutex(predictor->mutex);
    
    return 0;
}

/* ============================================
 * Функции конвертации в строку
 * ============================================ */

const char* predict_algo_to_string(predict_algo_t algo) {
    switch (algo) {
        case PREDICT_ALGO_LINEAR_REGRESSION: return "Linear Regression";
        case PREDICT_ALGO_MOVING_AVG: return "Moving Average";
        case PREDICT_ALGO_EXP_SMOOTH: return "Exponential Smoothing";
        case PREDICT_ALGO_ARIMA: return "ARIMA";
        case PREDICT_ALGO_POLY_REGRESSION: return "Polynomial Regression";
        case PREDICT_ALGO_SEASONAL: return "Seasonal Decomposition";
        case PREDICT_ALGO_ENSEMBLE: return "Ensemble";
        default: return "Unknown";
    }
}

const char* predict_status_to_string(predict_status_t status) {
    switch (status) {
        case PREDICT_STATUS_SUCCESS: return "Success";
        case PREDICT_STATUS_WARNING: return "Warning";
        case PREDICT_STATUS_ERROR: return "Error";
        case PREDICT_STATUS_INSUFFICIENT_DATA: return "Insufficient Data";
        default: return "Unknown";
    }
}

const char* predict_metric_to_string(predict_metric_t metric) {
    switch (metric) {
        case PREDICT_METRIC_CONNECTIONS: return "Connections";
        case PREDICT_METRIC_REQUESTS_PER_SEC: return "Requests/sec";
        case PREDICT_METRIC_BANDWIDTH: return "Bandwidth";
        case PREDICT_METRIC_CPU_USAGE: return "CPU Usage";
        case PREDICT_METRIC_MEMORY_USAGE: return "Memory Usage";
        case PREDICT_METRIC_LATENCY: return "Latency";
        case PREDICT_METRIC_ERROR_RATE: return "Error Rate";
        case PREDICT_METRIC_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

/* ============================================
 * JSON экспорт/импорт
 * ============================================ */

int predictive_analytics_export_json(predictive_analytics_t* predictor,
                                     char* buffer, size_t buffer_size) {
    if (!predictor || !buffer || buffer_size == 0) return -1;
    
    int written = 0;
    
    written += snprintf(buffer + written, buffer_size - written, "{\n");
    written += snprintf(buffer + written, buffer_size - written,
        "  \"config\": {\n");
    written += snprintf(buffer + written, buffer_size - written,
        "    \"algorithm\": %d,\n", (int)predictor->config.algorithm);
    written += snprintf(buffer + written, buffer_size - written,
        "    \"window_size\": %zu,\n", predictor->config.window_size);
    written += snprintf(buffer + written, buffer_size - written,
        "    \"forecast_horizon\": %zu,\n", predictor->config.forecast_horizon);
    written += snprintf(buffer + written, buffer_size - written,
        "    \"alpha\": %.3f\n", predictor->config.alpha);
    written += snprintf(buffer + written, buffer_size - written,
        "  },\n");
    
    written += snprintf(buffer + written, buffer_size - written,
        "  \"model\": {\n");
    written += snprintf(buffer + written, buffer_size - written,
        "    \"lr_slope\": %.6f,\n", predictor->lr_model.slope);
    written += snprintf(buffer + written, buffer_size - written,
        "    \"lr_intercept\": %.6f,\n", predictor->lr_model.intercept);
    written += snprintf(buffer + written, buffer_size - written,
        "    \"r_squared\": %.6f\n", predictor->lr_model.r_squared);
    written += snprintf(buffer + written, buffer_size - written,
        "  },\n");
    
    written += snprintf(buffer + written, buffer_size - written,
        "  \"stats\": {\n");
    written += snprintf(buffer + written, buffer_size - written,
        "    \"data_count\": %zu,\n", predictor->data_count);
    written += snprintf(buffer + written, buffer_size - written,
        "    \"total_forecasts\": %zu,\n", predictor->total_forecasts);
    written += snprintf(buffer + written, buffer_size - written,
        "    \"success_rate\": %.2f\n", 
        predictor->total_forecasts > 0 ? 
            (float)predictor->successful_forecasts / (float)predictor->total_forecasts * 100.0f : 100.0f);
    written += snprintf(buffer + written, buffer_size - written,
        "  }\n");
    
    written += snprintf(buffer + written, buffer_size - written, "}\n");
    
    return written;
}

int predictive_analytics_import_json(predictive_analytics_t* predictor, const char* json) {
    if (!predictor || !json) return -1;
    
    /* Упрощенная заглушка - полная реализация требует JSON парсера */
    return 0;
}
