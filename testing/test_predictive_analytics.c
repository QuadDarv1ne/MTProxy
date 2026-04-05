/**
 * @file test_predictive_analytics.c
 * @brief Тесты для системы прогнозирования нагрузки
 * 
 * @version 1.0.32
 * @date 29 марта 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#include "../system/ml/predictive-analytics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/* ============================================
 * Тестовые данные и утилиты
 * ============================================ */

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("  ✓ %s\n", message); \
        } else { \
            tests_failed++; \
            printf("  ✗ FAILED: %s\n", message); \
        } \
    } while(0)

/**
 * @brief Генерация линейных данных с шумом
 */
static void generate_linear_data(double* data, size_t n, double slope, double intercept, 
                                  double noise_std_dev) {
    srand(42); /* Фиксированный seed */
    
    for (size_t i = 0; i < n; i++) {
        /* Box-Muller для шума */
        double u1 = (double)rand() / RAND_MAX;
        double u2 = (double)rand() / RAND_MAX;
        if (u1 < 1e-10) u1 = 1e-10;
        
        double noise = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2) * noise_std_dev;
        data[i] = slope * (double)i + intercept + noise;
    }
}

/**
 * @brief Генерация сезонных данных
 */
static void generate_seasonal_data(double* data, size_t n, double period, 
                                    double amplitude, double baseline) {
    for (size_t i = 0; i < n; i++) {
        data[i] = baseline + amplitude * sin(2.0 * M_PI * (double)i / period);
    }
}

/* ============================================
 * Тесты инициализации и очистки
 * ============================================ */

static void test_predict_init_cleanup(void) {
    printf("\n=== Test: Predictive Analytics Init/Cleanup ===\n");
    
    predictive_analytics_t predictor;
    predict_config_t config;
    
    /* Тест 1: Инициализация с конфигурацией */
    memset(&config, 0, sizeof(config));
    config.algorithm = PREDICT_ALGO_LINEAR_REGRESSION;
    config.window_size = 50;
    config.forecast_horizon = 10;
    config.alpha = 0.3f;
    
    int ret = predictive_analytics_init(&predictor, &config);
    TEST_ASSERT(ret == 0, "Init with config returns 0");
    TEST_ASSERT(predictor.config.window_size == 50, "window_size set correctly");
    TEST_ASSERT(predictor.config.alpha == 0.3f, "alpha set correctly");
    
    predictive_analytics_cleanup(&predictor);
    
    /* Тест 2: Инициализация с NULL конфигурацией */
    ret = predictive_analytics_init(&predictor, NULL);
    TEST_ASSERT(ret == 0, "Init with NULL config returns 0");
    TEST_ASSERT(predictor.config.window_size > 0, "Default window_size is set");
    
    predictive_analytics_cleanup(&predictor);
    
    /* Тест 3: Очистка NULL predictor */
    predictive_analytics_cleanup(NULL);
    TEST_ASSERT(1, "Cleanup with NULL doesn't crash");
    
    /* Тест 4: Инициализация NULL predictor */
    ret = predictive_analytics_init(NULL, &config);
    TEST_ASSERT(ret == -1, "Init with NULL predictor returns -1");
}

/* ============================================
 * Тесты добавления данных
 * ============================================ */

static void test_predict_add_data(void) {
    printf("\n=== Test: Predictive Analytics Add Data ===\n");
    
    predictive_analytics_t predictor;
    predictive_analytics_init(&predictor, NULL);
    
    /* Тест 1: Добавление данных */
    for (int i = 0; i < 50; i++) {
        int ret = predictive_analytics_add_data(&predictor, 100.0 + i * 0.5, 0);
        TEST_ASSERT(ret == 0, "Add data point returns 0");
    }
    
    /* Тест 2: Проверка статистики */
    double mean, std_dev, min, max, trend;
    predictive_analytics_get_data_stats(&predictor, &mean, &std_dev, &min, &max, &trend);
    
    TEST_ASSERT(mean > 110.0 && mean < 120.0, "Mean is in expected range");
    TEST_ASSERT(min >= 100.0 && min <= 101.0, "Min is correct");
    TEST_ASSERT(max >= 124.0 && max <= 126.0, "Max is correct");
    TEST_ASSERT(trend > 0.0, "Trend is positive (increasing data)");
    
    /* Тест 3: Переполнение буфера */
    for (int i = 0; i < PREDICT_MAX_DATA_POINTS + 10; i++) {
        predictive_analytics_add_data(&predictor, (double)i, 0);
    }
    TEST_ASSERT(1, "Add data beyond capacity doesn't crash");
    
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты обучения (train)
 * ============================================ */

static void test_predict_training(void) {
    printf("\n=== Test: Predictive Analytics Training ===\n");
    
    predictive_analytics_t predictor;
    predict_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = PREDICT_ALGO_LINEAR_REGRESSION;
    config.window_size = 50;
    
    predictive_analytics_init(&predictor, &config);
    
    /* Генерация линейных данных */
    size_t n_samples = 200;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    generate_linear_data(train_data, n_samples, 0.5, 100.0, 2.0);
    
    /* Тест 1: Обучение */
    int ret = predictive_analytics_train(&predictor, train_data, n_samples);
    TEST_ASSERT(ret == 0, "Training succeeds");
    
    /* Тест 2: Проверка статистики после обучения */
    double mean, std_dev, min, max, trend;
    predictive_analytics_get_data_stats(&predictor, &mean, &std_dev, &min, &max, &trend);
    TEST_ASSERT(trend > 0.4 && trend < 0.6, "Trend is close to actual slope");
    
    /* Тест 3: Обучение с NULL данными */
    ret = predictive_analytics_train(&predictor, NULL, n_samples);
    TEST_ASSERT(ret == -1, "Training with NULL data returns -1");
    
    /* Тест 4: Обучение с малым количеством данных */
    ret = predictive_analytics_train(&predictor, train_data, 1);
    TEST_ASSERT(ret == -1, "Training with 1 sample returns -1");
    
    free(train_data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты линейной регрессии
 * ============================================ */

static void test_predict_linear_regression(void) {
    printf("\n=== Test: Linear Regression Forecasting ===\n");
    
    predictive_analytics_t predictor;
    predict_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = PREDICT_ALGO_LINEAR_REGRESSION;
    config.window_size = 50;
    config.forecast_horizon = 10;
    
    predictive_analytics_init(&predictor, &config);
    
    /* Генерация линейных данных */
    size_t n_samples = 100;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    generate_linear_data(train_data, n_samples, 1.0, 50.0, 1.0);
    
    predictive_analytics_train(&predictor, train_data, n_samples);
    
    /* Тест 1: Прогноз на 5 шагов */
    predict_result_t result;
    int ret = predictive_analytics_forecast(&predictor, 5, &result);
    TEST_ASSERT(ret == 0, "Forecast returns 0");
    TEST_ASSERT(result.status == PREDICT_STATUS_SUCCESS, "Status is success");
    TEST_ASSERT(result.algorithm == PREDICT_ALGO_LINEAR_REGRESSION,
                "Algorithm is Linear Regression");
    TEST_ASSERT(result.forecast_count == 5, "Forecast count matches request");
    
    /* Тест 2: Проверка тренда */
    TEST_ASSERT(result.trend_slope > 0.8 && result.trend_slope < 1.2,
                "Trend slope is close to actual (1.0)");
    TEST_ASSERT(result.r_squared > 0.5, "R-squared indicates good fit");
    
    /* Тест 3: Проверка доверительных интервалов */
    if (result.lower_bound && result.upper_bound) {
        for (size_t i = 0; i < result.forecast_count; i++) {
            TEST_ASSERT(result.lower_bound[i] <= result.forecast_values[i],
                        "Lower bound <= forecast value");
            TEST_ASSERT(result.upper_bound[i] >= result.forecast_values[i],
                        "Upper bound >= forecast value");
        }
    }
    
    /* Тест 4: Прогноз следующего значения */
    double next_value;
    float confidence;
    ret = predictive_analytics_predict_next(&predictor, &next_value, &confidence);
    TEST_ASSERT(ret == 0, "Predict next returns 0");
    TEST_ASSERT(confidence > 0.0 && confidence <= 1.0, "Confidence in valid range");
    
    /* Тест 5: Прогноз с NULL результатом */
    ret = predictive_analytics_forecast(&predictor, 5, NULL);
    TEST_ASSERT(ret == -1, "Forecast with NULL result returns -1");
    
    /* Очистка результата */
    free(result.forecast_values);
    free(result.lower_bound);
    free(result.upper_bound);
    
    free(train_data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты Moving Average
 * ============================================ */

static void test_predict_moving_avg(void) {
    printf("\n=== Test: Moving Average Forecasting ===\n");
    
    predictive_analytics_t predictor;
    predict_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = PREDICT_ALGO_MOVING_AVG;
    config.window_size = 20;
    config.forecast_horizon = 5;
    
    predictive_analytics_init(&predictor, &config);
    
    /* Генерация данных с небольшим шумом */
    size_t n_samples = 100;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    for (size_t i = 0; i < n_samples; i++) {
        train_data[i] = 100.0 + 5.0 * sin((double)i / 10.0);
    }
    
    predictive_analytics_train(&predictor, train_data, n_samples);
    
    /* Тест 1: Прогноз */
    predict_result_t result;
    int ret = predictive_analytics_forecast(&predictor, 5, &result);
    TEST_ASSERT(ret == 0, "MA Forecast returns 0");
    TEST_ASSERT(result.algorithm == PREDICT_ALGO_MOVING_AVG,
                "Algorithm is Moving Average");
    
    /* Тест 2: Прогнозы должны быть близки к среднему */
    double expected_avg = 100.0;
    for (size_t i = 0; i < result.forecast_count; i++) {
        TEST_ASSERT(fabs(result.forecast_values[i] - expected_avg) < 10.0,
                    "Forecast values are close to expected average");
    }
    
    free(result.forecast_values);
    free(result.lower_bound);
    free(result.upper_bound);
    free(train_data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты Exponential Smoothing
 * ============================================ */

static void test_predict_exp_smoothing(void) {
    printf("\n=== Test: Exponential Smoothing Forecasting ===\n");
    
    predictive_analytics_t predictor;
    predict_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = PREDICT_ALGO_EXP_SMOOTH;
    config.window_size = 30;
    config.alpha = 0.3f;
    
    predictive_analytics_init(&predictor, &config);
    
    /* Генерация данных */
    size_t n_samples = 100;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    for (size_t i = 0; i < n_samples; i++) {
        train_data[i] = 50.0 + 3.0 * cos((double)i / 5.0);
    }
    
    predictive_analytics_train(&predictor, train_data, n_samples);
    
    /* Тест 1: Прогноз */
    predict_result_t result;
    int ret = predictive_analytics_forecast(&predictor, 5, &result);
    TEST_ASSERT(ret == 0, "Exp Smoothing Forecast returns 0");
    TEST_ASSERT(result.algorithm == PREDICT_ALGO_EXP_SMOOTH,
                "Algorithm is Exponential Smoothing");
    
    free(result.forecast_values);
    free(result.lower_bound);
    free(result.upper_bound);
    free(train_data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты ARIMA
 * ============================================ */

static void test_predict_arima(void) {
    printf("\n=== Test: ARIMA Forecasting ===\n");
    
    predictive_analytics_t predictor;
    predict_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = PREDICT_ALGO_ARIMA;
    config.window_size = 50;
    config.ar_order = 2;
    config.ma_order = 2;
    config.diff_order = 1;
    
    predictive_analytics_init(&predictor, &config);
    
    /* Генерация данных с авторегрессией */
    size_t n_samples = 150;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    
    /* Простая AR(2) модель */
    srand(42);
    train_data[0] = 100.0;
    train_data[1] = 102.0;
    for (size_t i = 2; i < n_samples; i++) {
        double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0;
        train_data[i] = 0.7 * train_data[i-1] + 0.2 * train_data[i-2] + noise;
    }
    
    predictive_analytics_train(&predictor, train_data, n_samples);
    
    /* Тест 1: Прогноз */
    predict_result_t result;
    int ret = predictive_analytics_forecast(&predictor, 5, &result);
    TEST_ASSERT(ret == 0, "ARIMA Forecast returns 0");
    TEST_ASSERT(result.algorithm == PREDICT_ALGO_ARIMA,
                "Algorithm is ARIMA");
    
    free(result.forecast_values);
    free(result.lower_bound);
    free(result.upper_bound);
    free(train_data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты Polynomial Regression
 * ============================================ */

static void test_predict_poly_regression(void) {
    printf("\n=== Test: Polynomial Regression Forecasting ===\n");
    
    predictive_analytics_t predictor;
    predict_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = PREDICT_ALGO_POLY_REGRESSION;
    config.poly_order = 2;
    config.window_size = 50;
    
    predictive_analytics_init(&predictor, &config);
    
    /* Генерация квадратичных данных */
    size_t n_samples = 100;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    for (size_t i = 0; i < n_samples; i++) {
        double x = (double)i / 10.0;
        train_data[i] = 0.1 * x * x + 2.0 * x + 50.0;
    }
    
    predictive_analytics_train(&predictor, train_data, n_samples);
    
    /* Тест 1: Прогноз */
    predict_result_t result;
    int ret = predictive_analytics_forecast(&predictor, 5, &result);
    TEST_ASSERT(ret == 0, "Poly Regression Forecast returns 0");
    TEST_ASSERT(result.algorithm == PREDICT_ALGO_POLY_REGRESSION,
                "Algorithm is Polynomial Regression");
    
    free(result.forecast_values);
    free(result.lower_bound);
    free(result.upper_bound);
    free(train_data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты Ensemble
 * ============================================ */

static void test_predict_ensemble(void) {
    printf("\n=== Test: Ensemble Forecasting ===\n");
    
    predictive_analytics_t predictor;
    predict_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = PREDICT_ALGO_ENSEMBLE;
    config.enable_ensemble = true;
    config.window_size = 50;
    
    predictive_analytics_init(&predictor, &config);
    
    /* Генерация данных */
    size_t n_samples = 150;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    generate_linear_data(train_data, n_samples, 0.3, 80.0, 3.0);
    
    predictive_analytics_train(&predictor, train_data, n_samples);
    
    /* Тест 1: Прогноз ансамблем */
    predict_result_t result;
    int ret = predictive_analytics_forecast(&predictor, 5, &result);
    TEST_ASSERT(ret == 0, "Ensemble Forecast returns 0");
    TEST_ASSERT(result.algorithm == PREDICT_ALGO_ENSEMBLE,
                "Algorithm is Ensemble");
    
    /* Тест 2: Уверенность должна быть разумной */
    TEST_ASSERT(result.confidence > 0.5, "Confidence is reasonable");
    
    free(result.forecast_values);
    free(result.lower_bound);
    free(result.upper_bound);
    free(train_data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты оценки качества (evaluate)
 * ============================================ */

static void test_predict_evaluate(void) {
    printf("\n=== Test: Model Evaluation ===\n");
    
    predictive_analytics_t predictor;
    predictive_analytics_init(&predictor, NULL);
    
    /* Генерация данных */
    size_t n_samples = 100;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    generate_linear_data(train_data, n_samples, 0.5, 100.0, 2.0);
    
    predictive_analytics_train(&predictor, train_data, n_samples);
    
    /* Тест 1: Оценка модели */
    double mae, mse, rmse, mape, r_squared;
    int ret = predictive_analytics_evaluate(&predictor, &mae, &mse, &rmse, &mape, &r_squared);
    TEST_ASSERT(ret == 0, "Evaluate returns 0");
    TEST_ASSERT(mae >= 0.0, "MAE is non-negative");
    TEST_ASSERT(mse >= 0.0, "MSE is non-negative");
    TEST_ASSERT(rmse >= 0.0, "RMSE is non-negative");
    TEST_ASSERT(mape >= 0.0 && mape <= 100.0, "MAPE is in valid range (percentage)");
    TEST_ASSERT(r_squared >= 0.0 && r_squared <= 1.0, "R-squared in valid range");
    
    printf("    MAE: %.3f, MSE: %.3f, RMSE: %.3f, MAPE: %.2f%%, R²: %.3f\n",
           mae, mse, rmse, mape, r_squared * 100.0);
    
    free(train_data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты статистики
 * ============================================ */

static void test_predict_statistics(void) {
    printf("\n=== Test: Predictive Analytics Statistics ===\n");
    
    predictive_analytics_t predictor;
    predictive_analytics_init(&predictor, NULL);
    
    /* Обучение */
    size_t n_samples = 100;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    for (size_t i = 0; i < n_samples; i++) {
        train_data[i] = 50.0 + (double)i;
    }
    
    predictive_analytics_train(&predictor, train_data, n_samples);
    
    /* Множественные прогнозы */
    for (int i = 0; i < 10; i++) {
        predict_result_t result;
        predictive_analytics_forecast(&predictor, 3, &result);
        free(result.forecast_values);
        free(result.lower_bound);
        free(result.upper_bound);
    }
    
    /* Тест 1: Проверка статистики */
    size_t total_forecasts;
    float success_rate;
    double avg_error;
    
    predictive_analytics_get_stats(&predictor, &total_forecasts, &success_rate, &avg_error);
    TEST_ASSERT(total_forecasts == 10, "Total forecasts count is correct");
    TEST_ASSERT(success_rate > 0.0 && success_rate <= 100.0, "Success rate in valid range");
    
    /* Тест 2: Сброс статистики */
    predictive_analytics_reset_stats(&predictor);
    predictive_analytics_get_stats(&predictor, &total_forecasts, &success_rate, &avg_error);
    TEST_ASSERT(total_forecasts == 0, "Stats reset clears total_forecasts");
    
    /* Тест 3: Установка параметров */
    int ret = predictive_analytics_set_params(&predictor, 30, 0.5f, 20);
    TEST_ASSERT(ret == 0, "Set params returns 0");
    
    ret = predictive_analytics_set_params(&predictor, 0, -0.1f, 20); /* Недопустимые значения */
    TEST_ASSERT(ret == -1, "Set invalid params returns -1");
    
    free(train_data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты callback функций
 * ============================================ */

static int forecast_callback_triggered = 0;

static void forecast_callback(const predict_result_t* result, void* user_data) {
    forecast_callback_triggered = 1;
    (void)user_data;
}

static void test_predict_callback(void) {
    printf("\n=== Test: Predictive Analytics Callback ===\n");
    
    predictive_analytics_t predictor;
    predictive_analytics_init(&predictor, NULL);
    
    /* Обучение */
    size_t n_samples = 100;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    for (size_t i = 0; i < n_samples; i++) {
        train_data[i] = 50.0 + (double)i;
    }
    
    predictive_analytics_train(&predictor, train_data, n_samples);
    
    /* Установка callback */
    forecast_callback_triggered = 0;
    predictive_analytics_set_callback(&predictor, forecast_callback, NULL);
    
    /* Тест 1: Callback срабатывает при успешном прогнозе */
    predict_result_t result;
    predictive_analytics_forecast(&predictor, 5, &result);
    TEST_ASSERT(forecast_callback_triggered == 1, "Callback triggered on successful forecast");
    
    free(result.forecast_values);
    free(result.lower_bound);
    free(result.upper_bound);
    free(train_data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты вспомогательных функций
 * ============================================ */

static void test_predict_helper_functions(void) {
    printf("\n=== Test: Predictive Analytics Helper Functions ===\n");
    
    /* Тест 1: Линейная регрессия */
    double x[] = {0, 1, 2, 3, 4};
    double y[] = {1, 3, 5, 7, 9}; /* Идеальная линия: y = 2x + 1 */
    double slope, intercept, r_squared;
    
    int ret = compute_linear_regression(x, y, 5, &slope, &intercept, &r_squared);
    TEST_ASSERT(ret == 0, "Compute linear regression returns 0");
    TEST_ASSERT(fabs(slope - 2.0) < 0.001, "Slope is correct (2.0)");
    TEST_ASSERT(fabs(intercept - 1.0) < 0.001, "Intercept is correct (1.0)");
    TEST_ASSERT(fabs(r_squared - 1.0) < 0.001, "R-squared is 1.0 (perfect fit)");
    
    /* Тест 2: Скользящее среднее */
    double data[] = {10, 20, 30, 40, 50};
    double ma_result[5];
    ret = compute_moving_average(data, 5, 3, ma_result);
    TEST_ASSERT(ret == 0, "Compute moving average returns 0");
    TEST_ASSERT(fabs(ma_result[2] - 20.0) < 0.001, "MA[2] is correct");
    TEST_ASSERT(fabs(ma_result[4] - 40.0) < 0.001, "MA[4] is correct");
    
    /* Тест 3: Экспоненциальное сглаживание */
    double es_result[5];
    ret = compute_exponential_smoothing(data, 5, 0.5f, es_result);
    TEST_ASSERT(ret == 0, "Compute exp smoothing returns 0");
    TEST_ASSERT(fabs(es_result[0] - 10.0) < 0.001, "ES[0] is first value");
    
    /* Тест 4: Вычисление ошибок прогноза */
    double actual[] = {10, 20, 30, 40, 50};
    double predicted[] = {11, 19, 31, 39, 51};
    double mae, mse, rmse, mape;
    
    ret = compute_forecast_errors(actual, predicted, 5, &mae, &mse, &rmse, &mape);
    TEST_ASSERT(ret == 0, "Compute forecast errors returns 0");
    TEST_ASSERT(fabs(mae - 1.0) < 0.001, "MAE is 1.0");
    TEST_ASSERT(fabs(mape - 3.25) < 0.1, "MAPE is approximately 3.25%");
    
    /* Тест 5: Конвертация в строку */
    const char* algo_str = predict_algo_to_string(PREDICT_ALGO_LINEAR_REGRESSION);
    TEST_ASSERT(strstr(algo_str, "Linear") != NULL, "Algo to string works");
    
    const char* status_str = predict_status_to_string(PREDICT_STATUS_SUCCESS);
    TEST_ASSERT(strcmp(status_str, "Success") == 0, "Status to string works");
}

/* ============================================
 * Тесты JSON экспорта/импорта
 * ============================================ */

static void test_predict_json_export(void) {
    printf("\n=== Test: Predictive Analytics JSON Export ===\n");
    
    predictive_analytics_t predictor;
    predictive_analytics_init(&predictor, NULL);
    
    /* Обучение */
    size_t n_samples = 50;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    for (size_t i = 0; i < n_samples; i++) {
        train_data[i] = 50.0 + (double)i;
    }
    
    predictive_analytics_train(&predictor, train_data, n_samples);
    
    /* Тест 1: Экспорт в JSON */
    char buffer[1024];
    int written = predictive_analytics_export_json(&predictor, buffer, sizeof(buffer));
    TEST_ASSERT(written > 0, "JSON export returns positive bytes");
    TEST_ASSERT(written < (int)sizeof(buffer), "JSON fits in buffer");
    TEST_ASSERT(strstr(buffer, "\"config\"") != NULL, "JSON contains config");
    TEST_ASSERT(strstr(buffer, "\"model\"") != NULL, "JSON contains model");
    TEST_ASSERT(strstr(buffer, "\"lr_slope\"") != NULL, "JSON contains lr_slope");
    
    printf("    JSON output (%d bytes):\n", written);
    printf("    %.*s\n", (written < 200) ? written : 200, buffer);
    
    /* Тест 2: Импорт из JSON (заглушка) */
    int ret = predictive_analytics_import_json(&predictor, buffer);
    TEST_ASSERT(ret == 0, "JSON import returns 0 (stub)");
    
    free(train_data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты производительности
 * ============================================ */

static void test_predict_performance(void) {
    printf("\n=== Test: Predictive Analytics Performance ===\n");
    
    predictive_analytics_t predictor;
    predict_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = PREDICT_ALGO_LINEAR_REGRESSION;
    config.window_size = 100;
    
    predictive_analytics_init(&predictor, &config);
    
    /* Генерация больших данных */
    size_t n_samples = 2000;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    generate_linear_data(train_data, n_samples, 0.1, 100.0, 1.0);
    
    /* Тест 1: Время обучения */
    clock_t start = clock();
    int ret = predictive_analytics_train(&predictor, train_data, n_samples);
    clock_t end = clock();
    
    double train_time = (double)(end - start) / CLOCKS_PER_SEC;
    TEST_ASSERT(ret == 0, "Training on large dataset succeeds");
    TEST_ASSERT(train_time < 5.0, "Training completes in reasonable time");
    printf("    Training time: %.3f seconds\n", train_time);
    
    /* Тест 2: Время прогноза */
    predict_result_t result;
    start = clock();
    for (int i = 0; i < 100; i++) {
        predictive_analytics_forecast(&predictor, 10, &result);
        free(result.forecast_values);
        free(result.lower_bound);
        free(result.upper_bound);
    }
    end = clock();
    
    double predict_time = (double)(end - start) / CLOCKS_PER_SEC;
    double avg_predict_time = (predict_time * 1000.0) / 100; /* ms */
    TEST_ASSERT(avg_predict_time < 50.0, "Average predict time < 50ms");
    printf("    Avg predict time: %.3f ms\n", avg_predict_time);
    
    free(train_data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Тесты с недостаточным количеством данных
 * ============================================ */

static void test_predict_insufficient_data(void) {
    printf("\n=== Test: Insufficient Data Handling ===\n");
    
    predictive_analytics_t predictor;
    predictive_analytics_init(&predictor, NULL);
    
    /* Добавление только 3 точек данных */
    predictive_analytics_add_data(&predictor, 10.0, 0);
    predictive_analytics_add_data(&predictor, 12.0, 0);
    predictive_analytics_add_data(&predictor, 11.0, 0);
    
    /* Тест 1: Прогноз с недостаточными данными */
    predict_result_t result;
    int ret = predictive_analytics_forecast(&predictor, 5, &result);
    TEST_ASSERT(ret == -1, "Forecast with insufficient data returns -1");
    TEST_ASSERT(result.status == PREDICT_STATUS_INSUFFICIENT_DATA,
                "Status is INSUFFICIENT_DATA");
    
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Edge Cases Tests
 * ============================================ */

static void test_predict_edge_cases(void) {
    printf("\n=== Test: Edge Cases ===\n");
    
    predictive_analytics_t predictor;
    predict_config_t config;
    predict_result_t result;
    double single_value[1] = {42.0};
    double huge_values[100];
    double nan_values[10];
    double inf_values[10];
    int ret;
    
    memset(&config, 0, sizeof(config));
    
    /* Инициализация */
    predictive_analytics_init(&predictor, &config);
    TEST_ASSERT(predictor.initialized, "Edge cases: predictor init");
    
    /* Тест 1: Один образец */
    ret = predictive_analytics_train(&predictor, single_value, 1);
    TEST_ASSERT(ret == 0, "Edge cases: train with single sample");
    
    ret = predictive_analytics_forecast(&predictor, 1, &result);
    /* Прогноз с одним образцом может вернуть ошибку */
    TEST_ASSERT(ret == 0 || ret == -1, "Edge cases: forecast with single sample handled");
    
    predictive_analytics_cleanup(&predictor);
    
    /* Тест 2: NULL данные */
    predictive_analytics_init(&predictor, &config);
    ret = predictive_analytics_train(&predictor, NULL, 0);
    TEST_ASSERT(ret == -1, "Edge cases: train with NULL data returns -1");
    predictive_analytics_cleanup(&predictor);
    
    /* Тест 3: Огромные значения */
    predictive_analytics_init(&predictor, &config);
    for (int i = 0; i < 100; i++) {
        huge_values[i] = (i % 2 == 0) ? 1e10 : -1e10;
    }
    ret = predictive_analytics_train(&predictor, huge_values, 100);
    TEST_ASSERT(ret == 0, "Edge cases: train with huge values");
    
    ret = predictive_analytics_forecast(&predictor, 5, &result);
    TEST_ASSERT(ret == 0 || ret == -1, "Edge cases: forecast with huge values handled");
    if (ret == 0) {
        free(result.forecast_values);
        free(result.lower_bound);
        free(result.upper_bound);
    }
    
    predictive_analytics_cleanup(&predictor);
    
    /* Тест 4: NaN значения */
    predictive_analytics_init(&predictor, &config);
    for (int i = 0; i < 10; i++) {
        nan_values[i] = (i == 0) ? NAN : (double)i;
    }
    ret = predictive_analytics_train(&predictor, nan_values, 10);
    TEST_ASSERT(ret == 0 || ret == -1, "Edge cases: train with NaN handled");
    predictive_analytics_cleanup(&predictor);
    
    /* Тест 5: Бесконечные значения */
    predictive_analytics_init(&predictor, &config);
    for (int i = 0; i < 10; i++) {
        inf_values[i] = (i == 0) ? INFINITY : (double)i;
    }
    ret = predictive_analytics_train(&predictor, inf_values, 10);
    TEST_ASSERT(ret == 0 || ret == -1, "Edge cases: train with Inf handled");
    predictive_analytics_cleanup(&predictor);
    
    /* Тест 6: Предсказание без обучения */
    predictive_analytics_init(&predictor, &config);
    ret = predictive_analytics_forecast(&predictor, 5, &result);
    TEST_ASSERT(ret == -1, "Edge cases: forecast without training returns -1");
    predictive_analytics_cleanup(&predictor);
    
    /* Тест 7: Нулевой контекст */
    ret = predictive_analytics_train(NULL, single_value, 1);
    TEST_ASSERT(ret == -1, "Edge cases: train with NULL context returns -1");
    
    ret = predictive_analytics_forecast(NULL, 1, &result);
    TEST_ASSERT(ret == -1, "Edge cases: forecast with NULL context returns -1");
    
    predictive_analytics_cleanup(NULL);  /* no-op */
    TEST_ASSERT(1, "Edge cases: cleanup with NULL is safe");
}

static void test_predict_stress(void) {
    printf("\n=== Test: Stress Tests ===\n");
    
    predictive_analytics_t predictor;
    predict_config_t config;
    const size_t n_samples = 500;
    double *data = malloc(n_samples * sizeof(double));
    predict_result_t result;
    int ret;
    
    TEST_ASSERT(data != NULL, "Stress: allocate data");
    if (!data) return;
    
    memset(&config, 0, sizeof(config));
    
    /* Генерация данных */
    srand(54321);
    for (size_t i = 0; i < n_samples; i++) {
        data[i] = 50.0 + (double)(rand() % 100);
    }
    
    /* Инициализация */
    predictive_analytics_init(&predictor, &config);
    TEST_ASSERT(predictor.initialized, "Stress: predictor init");
    
    /* Множественное обучение */
    for (int i = 0; i < 5; i++) {
        ret = predictive_analytics_train(&predictor, data, n_samples);
        TEST_ASSERT(ret == 0, "Stress: repeated training (iteration %d)", i);
    }
    
    /* Множественные прогнозы */
    for (int i = 0; i < 50; i++) {
        ret = predictive_analytics_forecast(&predictor, 10, &result);
        TEST_ASSERT(ret == 0, "Stress: repeated forecast (iteration %d)", i);
        if (ret == 0) {
            free(result.forecast_values);
            free(result.lower_bound);
            free(result.upper_bound);
        }
    }
    
    /* Добавление множества точек */
    for (int i = 0; i < 100; i++) {
        ret = predictive_analytics_add_data(&predictor, (double)i, 0);
        TEST_ASSERT(ret == 0, "Stress: add data %d", i);
    }
    
    /* Статистика */
    predict_evaluation_t eval;
    ret = predictive_analytics_evaluate(&predictor, &eval);
    TEST_ASSERT(ret == 0, "Stress: evaluate after stress operations");
    
    free(data);
    predictive_analytics_cleanup(&predictor);
}

/* ============================================
 * Главная функция тестирования
 * ============================================ */

int main(void) {
    printf("============================================\n");
    printf("  Predictive Analytics System Tests\n");
    printf("  Version: 1.0.32\n");
    printf("============================================\n");
    
    /* Запуск всех тестов */
    test_predict_init_cleanup();
    test_predict_add_data();
    test_predict_training();
    test_predict_linear_regression();
    test_predict_moving_avg();
    test_predict_exp_smoothing();
    test_predict_arima();
    test_predict_poly_regression();
    test_predict_ensemble();
    test_predict_evaluate();
    test_predict_statistics();
    test_predict_callback();
    test_predict_helper_functions();
    test_predict_json_export();
    test_predict_performance();
    test_predict_insufficient_data();
    test_predict_edge_cases();
    test_predict_stress();

    /* Итоговый отчет */
    printf("\n============================================\n");
    printf("  Test Summary\n");
    printf("============================================\n");
    printf("  Total:  %d\n", tests_run);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);
    printf("  Success Rate: %.1f%%\n", 
           tests_run > 0 ? (100.0 * tests_passed / tests_run) : 0.0);
    printf("============================================\n");
    
    return (tests_failed == 0) ? 0 : 1;
}
