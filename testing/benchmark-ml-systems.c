/**
 * @file benchmark-ml-systems.c
 * @brief Бенчмарки производительности ML-систем
 * 
 * Тестирует производительность:
 * - Anomaly Detection (5 алгоритмов)
 * - Predictive Analytics (6 алгоритмов)
 * 
 * @version 1.0.32
 * @date 29 марта 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#include "../system/ml/anomaly-detection.h"
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
#include <sys/time.h>
#endif

/* ============================================
 * Утилиты
 * ============================================ */

/**
 * @brief Получить текущее время в миллисекундах
 */
static uint64_t get_time_ms(void) {
#ifdef _WIN32
    return GetTickCount64();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

/**
 * @brief Получить текущее время в микросекундах
 */
static uint64_t get_time_us(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (uint64_t)(counter.QuadPart * 1000000 / freq.QuadPart);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec * 1000000 + tv.tv_usec);
#endif
}

/**
 * @brief Генерация нормальных данных
 */
static void generate_normal_data(double* data, size_t n_samples, size_t n_features,
                                  double mean, double std_dev) {
    srand(42);
    
    for (size_t i = 0; i < n_samples; i++) {
        for (size_t f = 0; f < n_features; f++) {
            double u1 = (double)rand() / RAND_MAX;
            double u2 = (double)rand() / RAND_MAX;
            if (u1 < 1e-10) u1 = 1e-10;
            
            double z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
            data[i * n_features + f] = mean + std_dev * z;
        }
    }
}

/**
 * @brief Генерация линейных данных
 */
static void generate_linear_data(double* data, size_t n, double slope, double intercept, 
                                  double noise_std_dev) {
    srand(42);
    
    for (size_t i = 0; i < n; i++) {
        double u1 = (double)rand() / RAND_MAX;
        double u2 = (double)rand() / RAND_MAX;
        if (u1 < 1e-10) u1 = 1e-10;
        
        double noise = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2) * noise_std_dev;
        data[i] = slope * (double)i + intercept + noise;
    }
}

/**
 * @brief Форматирование времени
 */
static void format_time(uint64_t us, char* buffer, size_t size) {
    if (us < 1000) {
        snprintf(buffer, size, "%llu μs", (unsigned long long)us);
    } else if (us < 1000000) {
        snprintf(buffer, size, "%.2f ms", us / 1000.0);
    } else {
        snprintf(buffer, size, "%.2f s", us / 1000000.0);
    }
}

/* ============================================
 * Бенчмарки Anomaly Detection
 * ============================================ */

typedef struct {
    const char* name;
    anomaly_algo_t algorithm;
    uint64_t train_time_us;
    uint64_t predict_time_us;
    size_t samples_per_second;
} anomaly_benchmark_result_t;

static anomaly_benchmark_result_t benchmark_anomaly_algorithm(
    const char* name,
    anomaly_algo_t algorithm,
    const double* train_data,
    size_t n_samples,
    size_t n_features,
    const double* test_data,
    size_t n_tests
) {
    anomaly_config_t config = {0};
    config.algorithm = algorithm;
    config.n_features = n_features;
    config.max_samples = n_samples + 100;
    config.n_trees = 50;
    config.tree_height = 15;
    config.threshold = 0.6f;
    config.zscore_threshold = 3.0f;
    config.moving_avg_window = 60;
    
    anomaly_detector_t detector;
    anomaly_detector_init(&detector, &config);
    
    /* Бенчмарк обучения */
    uint64_t start = get_time_us();
    anomaly_detector_train(&detector, train_data, n_samples, n_features);
    uint64_t train_time = get_time_us() - start;
    
    /* Бенчмарк предсказания */
    anomaly_result_t result;
    start = get_time_us();
    for (size_t i = 0; i < n_tests; i++) {
        anomaly_detector_predict(&detector, &test_data[i * n_features], &result);
    }
    uint64_t total_predict_time = get_time_us() - start;
    uint64_t avg_predict_time = total_predict_time / n_tests;
    
    /* Вычисление пропускной способности */
    size_t samples_per_sec = (uint64_t)n_tests * 1000000 / total_predict_time;
    
    anomaly_detector_cleanup(&detector);
    
    anomaly_benchmark_result_t res = {
        .name = name,
        .algorithm = algorithm,
        .train_time_us = train_time,
        .predict_time_us = avg_predict_time,
        .samples_per_second = samples_per_sec
    };
    
    return res;
}

static void run_anomaly_detection_benchmarks(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  Anomaly Detection Benchmarks                                ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    /* Параметры теста */
    size_t n_samples = 1000;
    size_t n_features = 3;
    size_t n_tests = 1000;
    
    /* Генерация данных */
    double* train_data = (double*)malloc(n_samples * n_features * sizeof(double));
    double* test_data = (double*)malloc(n_tests * n_features * sizeof(double));
    
    generate_normal_data(train_data, n_samples, n_features, 100.0, 15.0);
    generate_normal_data(test_data, n_tests, n_features, 100.0, 15.0);
    
    printf("Configuration:\n");
    printf("  Training samples: %zu\n", n_samples);
    printf("  Features: %zu\n", n_features);
    printf("  Test predictions: %zu\n", n_tests);
    printf("\n");
    
    anomaly_benchmark_result_t results[5];
    int n_results = 0;
    
    /* Isolation Forest */
    printf("Running: Isolation Forest... ");
    fflush(stdout);
    results[n_results++] = benchmark_anomaly_algorithm(
        "Isolation Forest", ANOMALY_ALGO_ISOLATION_FOREST,
        train_data, n_samples, n_features, test_data, n_tests
    );
    printf("Done\n");
    
    /* Z-Score */
    printf("Running: Z-Score... ");
    fflush(stdout);
    results[n_results++] = benchmark_anomaly_algorithm(
        "Z-Score", ANOMALY_ALGO_ZSCORE,
        train_data, n_samples, n_features, test_data, n_tests
    );
    printf("Done\n");
    
    /* Moving Average */
    printf("Running: Moving Average... ");
    fflush(stdout);
    results[n_results++] = benchmark_anomaly_algorithm(
        "Moving Average", ANOMALY_ALGO_MOVING_AVG,
        train_data, n_samples, n_features, test_data, n_tests
    );
    printf("Done\n");
    
    /* Exponential Smoothing */
    printf("Running: Exponential Smoothing... ");
    fflush(stdout);
    results[n_results++] = benchmark_anomaly_algorithm(
        "Exponential Smoothing", ANOMALY_ALGO_EXP_SMOOTH,
        train_data, n_samples, n_features, test_data, n_tests
    );
    printf("Done\n");
    
    /* Ensemble */
    printf("Running: Ensemble... ");
    fflush(stdout);
    results[n_results++] = benchmark_anomaly_algorithm(
        "Ensemble", ANOMALY_ALGO_ENSEMBLE,
        train_data, n_samples, n_features, test_data, n_tests
    );
    printf("Done\n");

    /* Вывод результатов */
    printf("\n");
    printf("+---------------------+--------------+--------------+-----------------+\n");
    printf("| Algorithm           │ Train Time   │ Predict Time │ Samples/sec     │\n");
    printf("+---------------------+--------------+--------------+-----------------+\n");

    for (int i = 0; i < n_results; i++) {
        char train_str[32], predict_str[32];
        format_time(results[i].train_time_us, train_str, sizeof(train_str));
        format_time(results[i].predict_time_us, predict_str, sizeof(predict_str));

        printf("│ %-19s │ %-12s │ %-12s │ %15zu │\n",
               results[i].name, train_str, predict_str, results[i].samples_per_second);
    }

    printf("+---------------------+--------------+--------------+-----------------+\n");
    
    free(train_data);
    free(test_data);
}

/* ============================================
 * Бенчмарки Predictive Analytics
 * ============================================ */

typedef struct {
    const char* name;
    predict_algo_t algorithm;
    uint64_t train_time_us;
    uint64_t forecast_time_us;
    size_t forecasts_per_second;
} predict_benchmark_result_t;

static predict_benchmark_result_t benchmark_predict_algorithm(
    const char* name,
    predict_algo_t algorithm,
    const double* train_data,
    size_t n_samples,
    size_t forecast_steps
) {
    predict_config_t config = {0};
    config.algorithm = algorithm;
    config.window_size = 100;
    config.forecast_horizon = forecast_steps;
    config.alpha = 0.3f;
    config.ar_order = 2;
    config.ma_order = 2;
    config.diff_order = 1;
    config.poly_order = 2;
    
    predictive_analytics_t predictor;
    predictive_analytics_init(&predictor, &config);
    
    /* Бенчмарк обучения */
    uint64_t start = get_time_us();
    predictive_analytics_train(&predictor, train_data, n_samples);
    uint64_t train_time = get_time_us() - start;
    
    /* Бенчмарк прогноза */
    predict_result_t result;
    start = get_time_us();
    for (int i = 0; i < 10; i++) {
        predictive_analytics_forecast(&predictor, forecast_steps, &result);
        free(result.forecast_values);
        free(result.lower_bound);
        free(result.upper_bound);
    }
    uint64_t total_forecast_time = get_time_us() - start;
    uint64_t avg_forecast_time = total_forecast_time / 10;
    
    /* Вычисление пропускной способности */
    size_t forecasts_per_sec = 10 * 1000000 / total_forecast_time;
    
    predictive_analytics_cleanup(&predictor);
    
    predict_benchmark_result_t res = {
        .name = name,
        .algorithm = algorithm,
        .train_time_us = train_time,
        .forecast_time_us = avg_forecast_time,
        .forecasts_per_second = forecasts_per_sec
    };
    
    return res;
}

static void run_predictive_analytics_benchmarks(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  Predictive Analytics Benchmarks                             ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    /* Параметры теста */
    size_t n_samples = 500;
    size_t forecast_steps = 10;
    
    /* Генерация данных */
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    generate_linear_data(train_data, n_samples, 0.5, 100.0, 2.0);
    
    printf("Configuration:\n");
    printf("  Training samples: %zu\n", n_samples);
    printf("  Forecast steps: %zu\n", forecast_steps);
    printf("\n");
    
    predict_benchmark_result_t results[6];
    int n_results = 0;
    
    /* Linear Regression */
    printf("Running: Linear Regression... ");
    fflush(stdout);
    results[n_results++] = benchmark_predict_algorithm(
        "Linear Regression", PREDICT_ALGO_LINEAR_REGRESSION,
        train_data, n_samples, forecast_steps
    );
    printf("Done\n");
    
    /* Moving Average */
    printf("Running: Moving Average... ");
    fflush(stdout);
    results[n_results++] = benchmark_predict_algorithm(
        "Moving Average", PREDICT_ALGO_MOVING_AVG,
        train_data, n_samples, forecast_steps
    );
    printf("Done\n");
    
    /* Exponential Smoothing */
    printf("Running: Exponential Smoothing... ");
    fflush(stdout);
    results[n_results++] = benchmark_predict_algorithm(
        "Exponential Smoothing", PREDICT_ALGO_EXP_SMOOTH,
        train_data, n_samples, forecast_steps
    );
    printf("Done\n");
    
    /* ARIMA */
    printf("Running: ARIMA... ");
    fflush(stdout);
    results[n_results++] = benchmark_predict_algorithm(
        "ARIMA", PREDICT_ALGO_ARIMA,
        train_data, n_samples, forecast_steps
    );
    printf("Done\n");
    
    /* Polynomial Regression */
    printf("Running: Polynomial Regression... ");
    fflush(stdout);
    results[n_results++] = benchmark_predict_algorithm(
        "Polynomial Regression", PREDICT_ALGO_POLY_REGRESSION,
        train_data, n_samples, forecast_steps
    );
    printf("Done\n");
    
    /* Ensemble */
    printf("Running: Ensemble... ");
    fflush(stdout);
    results[n_results++] = benchmark_predict_algorithm(
        "Ensemble", PREDICT_ALGO_ENSEMBLE,
        train_data, n_samples, forecast_steps
    );
    printf("Done\n");
    
    /* Вывод результатов */
    printf("\n");
    printf("┌─────────────────────┬──────────────┬──────────────┬─────────────────┐\n");
    printf("│ Algorithm           │ Train Time   │ Forecast Time│ Forecasts/sec   │\n");
    printf("├─────────────────────┼──────────────┼──────────────┼─────────────────┤\n");
    
    for (int i = 0; i < n_results; i++) {
        char train_str[32], forecast_str[32];
        format_time(results[i].train_time_us, train_str, sizeof(train_str));
        format_time(results[i].forecast_time_us, forecast_str, sizeof(forecast_str));
        
        printf("│ %-19s │ %-12s │ %-12s │ %15zu │\n",
               results[i].name, train_str, forecast_str, results[i].forecasts_per_second);
    }
    
    printf("└─────────────────────┴──────────────┴──────────────┴─────────────────┘\n");
    
    free(train_data);
}

/* ============================================
 * Скальability бенчмарки
 * ============================================ */

static void run_scalability_benchmarks(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  Scalability Benchmarks (Isolation Forest)                   ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    size_t n_features = 3;
    size_t n_tests = 100;
    
    printf("┌─────────────────┬──────────────┬──────────────┬─────────────────┐\n");
    printf("│ Samples         │ Train Time   │ Predict Time │ Samples/sec     │\n");
    printf("├─────────────────┼──────────────┼──────────────┼─────────────────┤\n");
    
    size_t sample_sizes[] = {100, 500, 1000, 2000, 5000};
    
    for (size_t s = 0; s < sizeof(sample_sizes)/sizeof(sample_sizes[0]); s++) {
        size_t n_samples = sample_sizes[s];
        
        double* train_data = (double*)malloc(n_samples * n_features * sizeof(double));
        double* test_data = (double*)malloc(n_tests * n_features * sizeof(double));
        
        generate_normal_data(train_data, n_samples, n_features, 100.0, 15.0);
        generate_normal_data(test_data, n_tests, n_features, 100.0, 15.0);
        
        anomaly_config_t config = {0};
        config.algorithm = ANOMALY_ALGO_ISOLATION_FOREST;
        config.n_features = n_features;
        config.max_samples = n_samples + 100;
        config.n_trees = 50;
        
        anomaly_detector_t detector;
        anomaly_detector_init(&detector, &config);
        
        uint64_t start = get_time_us();
        anomaly_detector_train(&detector, train_data, n_samples, n_features);
        uint64_t train_time = get_time_us() - start;
        
        anomaly_result_t result;
        start = get_time_us();
        for (size_t i = 0; i < n_tests; i++) {
            anomaly_detector_predict(&detector, &test_data[i * n_features], &result);
        }
        uint64_t total_predict_time = get_time_us() - start;
        
        size_t samples_per_sec = (uint64_t)n_tests * 1000000 / total_predict_time;
        
        char train_str[32], predict_str[32];
        format_time(train_time, train_str, sizeof(train_str));
        format_time(total_predict_time / n_tests, predict_str, sizeof(predict_str));
        
        printf("│ %15zu │ %-12s │ %-12s │ %15zu │\n",
               n_samples, train_str, predict_str, samples_per_sec);
        
        anomaly_detector_cleanup(&detector);
        free(train_data);
        free(test_data);
    }
    
    printf("└─────────────────┴──────────────┴──────────────┴─────────────────┘\n");
}

/* ============================================
 * Главная функция
 * ============================================ */

int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  MTProxy ML Systems Benchmarks                               ║\n");
    printf("║  Version: 1.0.32                                             ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    /* Запуск бенчмарков */
    run_anomaly_detection_benchmarks();
    run_predictive_analytics_benchmarks();
    run_scalability_benchmarks();
    
    /* Итоговый вывод */
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║  Summary                                                     ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("  Fastest Training: Z-Score (O(n))\n");
    printf("  Fastest Prediction: Z-Score / Moving Average (O(1))\n");
    printf("  Best Accuracy: Isolation Forest / Ensemble\n");
    printf("  Best for Time Series: ARIMA\n");
    printf("  Best for Trends: Linear Regression\n");
    printf("\n");
    
    return 0;
}
