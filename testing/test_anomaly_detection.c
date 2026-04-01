/**
 * @file test_anomaly_detection.c
 * @brief Тесты для системы ML-детекции аномалий
 * 
 * @version 1.0.32
 * @date 29 марта 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#include "../system/ml/anomaly-detection.h"
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
 * @brief Генерация нормальных данных
 */
static void generate_normal_data(double* data, size_t n_samples, size_t n_features,
                                  double mean, double std_dev) {
    srand(42); /* Фиксированный seed для воспроизводимости */
    
    for (size_t i = 0; i < n_samples; i++) {
        for (size_t f = 0; f < n_features; f++) {
            /* Box-Muller transform для нормального распределения */
            double u1 = (double)rand() / RAND_MAX;
            double u2 = (double)rand() / RAND_MAX;
            if (u1 < 1e-10) u1 = 1e-10;
            
            double z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
            data[i * n_features + f] = mean + std_dev * z;
        }
    }
}

/**
 * @brief Генерация данных с аномалиями
 */
static void generate_anomalous_data(double* data, size_t n_samples, size_t n_features,
                                    double normal_mean, double normal_std_dev,
                                    size_t anomaly_indices[], size_t n_anomalies) {
    /* Сначала генерируем нормальные данные */
    generate_normal_data(data, n_samples, n_features, normal_mean, normal_std_dev);
    
    /* Добавляем аномалии */
    for (size_t i = 0; i < n_anomalies; i++) {
        size_t idx = anomaly_indices[i];
        if (idx < n_samples) {
            /* Аномалия: значение за пределами 4 сигм */
            for (size_t f = 0; f < n_features; f++) {
                double sign = (rand() % 2 == 0) ? 1.0 : -1.0;
                data[idx * n_features + f] = normal_mean + sign * (5.0 + rand() % 3) * normal_std_dev;
            }
        }
    }
}

/* ============================================
 * Тесты инициализации и очистки
 * ============================================ */

static void test_anomaly_init_cleanup(void) {
    printf("\n=== Test: Anomaly Detector Init/Cleanup ===\n");
    
    anomaly_detector_t detector;
    anomaly_config_t config;
    
    /* Тест 1: Инициализация с конфигурацией по умолчанию */
    memset(&config, 0, sizeof(config));
    config.algorithm = ANOMALY_ALGO_ISOLATION_FOREST;
    config.n_features = 3;
    config.max_samples = 1000;
    config.threshold = 0.6f;
    config.n_trees = 50;
    config.tree_height = 15;
    
    int ret = anomaly_detector_init(&detector, &config);
    TEST_ASSERT(ret == 0, "Init with config returns 0");
    TEST_ASSERT(detector.n_features == 3, "n_features set correctly");
    TEST_ASSERT(detector.config.threshold == 0.6f, "threshold set correctly");
    
    anomaly_detector_cleanup(&detector);
    
    /* Тест 2: Инициализация с NULL конфигурацией (по умолчанию) */
    ret = anomaly_detector_init(&detector, NULL);
    TEST_ASSERT(ret == 0, "Init with NULL config returns 0");
    TEST_ASSERT(detector.n_features == 1, "Default n_features is 1");
    
    anomaly_detector_cleanup(&detector);
    
    /* Тест 3: Очистка NULL детектора */
    anomaly_detector_cleanup(NULL);
    TEST_ASSERT(1, "Cleanup with NULL doesn't crash");
    
    /* Тест 4: Инициализация NULL детектора */
    ret = anomaly_detector_init(NULL, &config);
    TEST_ASSERT(ret == -1, "Init with NULL detector returns -1");
}

/* ============================================
 * Тесты обучения (train)
 * ============================================ */

static void test_anomaly_training(void) {
    printf("\n=== Test: Anomaly Detector Training ===\n");
    
    anomaly_detector_t detector;
    anomaly_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = ANOMALY_ALGO_ISOLATION_FOREST;
    config.n_features = 2;
    config.max_samples = 500;
    config.n_trees = 30;
    config.tree_height = 10;
    
    anomaly_detector_init(&detector, &config);
    
    /* Тест 1: Обучение на нормальных данных */
    size_t n_samples = 200;
    size_t n_features = 2;
    double* train_data = (double*)malloc(n_samples * n_features * sizeof(double));
    
    generate_normal_data(train_data, n_samples, n_features, 100.0, 15.0);
    
    int ret = anomaly_detector_train(&detector, train_data, n_samples, n_features);
    TEST_ASSERT(ret == 0, "Training on normal data succeeds");
    TEST_ASSERT(detector.n_samples == n_samples, "n_samples updated correctly");
    
    /* Тест 2: Проверка статистики признаков */
    anomaly_feature_stats_t stats;
    ret = anomaly_detector_get_feature_stats(&detector, 0, &stats);
    TEST_ASSERT(ret == 0, "Get feature stats succeeds");
    TEST_ASSERT(stats.count == n_samples, "Stats count matches training samples");
    TEST_ASSERT(stats.mean > 85.0 && stats.mean < 115.0, "Mean is within expected range");
    TEST_ASSERT(stats.std_dev > 0.0, "Std dev is positive");
    
    /* Тест 3: Обучение с неправильным количеством признаков */
    ret = anomaly_detector_train(&detector, train_data, n_samples, 5);
    TEST_ASSERT(ret == -1, "Training with wrong n_features returns -1");
    
    /* Тест 4: Обучение с NULL данными */
    ret = anomaly_detector_train(&detector, NULL, n_samples, n_features);
    TEST_ASSERT(ret == -1, "Training with NULL data returns -1");
    
    /* Тест 5: Обучение с нулевым количеством образцов */
    ret = anomaly_detector_train(&detector, train_data, 0, n_features);
    TEST_ASSERT(ret == -1, "Training with 0 samples returns -1");
    
    free(train_data);
    anomaly_detector_cleanup(&detector);
}

/* ============================================
 * Тесты предсказания (predict)
 * ============================================ */

static void test_anomaly_prediction(void) {
    printf("\n=== Test: Anomaly Detector Prediction ===\n");
    
    anomaly_detector_t detector;
    anomaly_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = ANOMALY_ALGO_ISOLATION_FOREST;
    config.n_features = 1;
    config.max_samples = 500;
    config.n_trees = 50;
    config.threshold = 0.6f;
    
    anomaly_detector_init(&detector, &config);
    
    /* Обучение на нормальных данных */
    size_t n_samples = 300;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    for (size_t i = 0; i < n_samples; i++) {
        train_data[i] = 100.0 + 10.0 * sin((double)i / 10.0);
    }
    
    anomaly_detector_train(&detector, train_data, n_samples, 1);
    
    /* Тест 1: Предсказание нормального значения */
    anomaly_result_t result;
    double normal_value = 102.0;
    
    int ret = anomaly_detector_predict(&detector, &normal_value, &result);
    TEST_ASSERT(ret == 0, "Predict returns 0");
    TEST_ASSERT(result.status == ANOMALY_STATUS_NORMAL || 
                result.status == ANOMALY_STATUS_SUSPICIOUS,
                "Normal value detected as normal or suspicious");
    TEST_ASSERT(result.anomaly_score >= 0.0 && result.anomaly_score <= 1.0,
                "Anomaly score in valid range [0, 1]");
    
    /* Тест 2: Предсказание аномального значения */
    double anomalous_value = 200.0; /* Значительно отклоняется от 100 */
    
    ret = anomaly_detector_predict(&detector, &anomalous_value, &result);
    TEST_ASSERT(ret == 0, "Predict for anomaly returns 0");
    TEST_ASSERT(result.status >= ANOMALY_STATUS_SUSPICIOUS,
                "Anomalous value detected as suspicious or higher");
    TEST_ASSERT(result.anomaly_score > 0.5, "Anomaly score is high for anomaly");
    
    /* Тест 3: Предсказание с NULL параметрами */
    ret = anomaly_detector_predict(&detector, NULL, &result);
    TEST_ASSERT(ret == -1, "Predict with NULL values returns -1");
    
    ret = anomaly_detector_predict(&detector, &normal_value, NULL);
    TEST_ASSERT(ret == -1, "Predict with NULL result returns -1");
    
    /* Тест 4: Проверка описания результата */
    TEST_ASSERT(strlen(result.description) > 0, "Result description is not empty");
    TEST_ASSERT(strstr(result.description, "Anomaly score") != NULL,
                "Description contains anomaly score");
    
    free(train_data);
    anomaly_detector_cleanup(&detector);
}

/* ============================================
 * Тесты Z-Score алгоритма
 * ============================================ */

static void test_anomaly_zscore(void) {
    printf("\n=== Test: Z-Score Anomaly Detection ===\n");
    
    anomaly_detector_t detector;
    anomaly_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = ANOMALY_ALGO_ZSCORE;
    config.n_features = 1;
    config.max_samples = 500;
    config.zscore_threshold = 3.0f;
    config.threshold = 0.6f;
    
    anomaly_detector_init(&detector, &config);
    
    /* Обучение */
    size_t n_samples = 200;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    generate_normal_data(train_data, n_samples, 1, 50.0, 5.0);
    
    anomaly_detector_train(&detector, train_data, n_samples, 1);
    
    /* Тест 1: Нормальное значение (в пределах 1 сигмы) */
    anomaly_result_t result;
    double normal_value = 51.0;
    
    int ret = anomaly_detector_predict(&detector, &normal_value, &result);
    TEST_ASSERT(ret == 0, "Z-Score predict returns 0");
    TEST_ASSERT(result.detected_by == ANOMALY_ALGO_ZSCORE,
                "Detected by Z-Score algorithm");
    
    /* Тест 2: Аномальное значение (за пределами 3 сигм) */
    double anomalous_value = 80.0; /* 6 сигм от среднего */
    
    ret = anomaly_detector_predict(&detector, &anomalous_value, &result);
    TEST_ASSERT(ret == 0, "Z-Score predict for anomaly returns 0");
    TEST_ASSERT(result.status >= ANOMALY_STATUS_SUSPICIOUS,
                "Anomaly detected by Z-Score");
    
    /* Тест 3: Проверка Z-Score значения */
    TEST_ASSERT(fabs(result.deviation) > 2.0, "Z-Score deviation is significant");
    
    free(train_data);
    anomaly_detector_cleanup(&detector);
}

/* ============================================
 * Тесты Moving Average
 * ============================================ */

static void test_anomaly_moving_avg(void) {
    printf("\n=== Test: Moving Average Anomaly Detection ===\n");
    
    anomaly_detector_t detector;
    anomaly_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = ANOMALY_ALGO_MOVING_AVG;
    config.n_features = 1;
    config.max_samples = 500;
    config.moving_avg_window = 20;
    config.threshold = 0.6f;
    
    anomaly_detector_init(&detector, &config);
    
    /* Обучение: данные с трендом */
    size_t n_samples = 100;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    for (size_t i = 0; i < n_samples; i++) {
        train_data[i] = 50.0 + 0.1 * (double)i; /* Линейный тренд */
    }
    
    anomaly_detector_train(&detector, train_data, n_samples, 1);
    
    /* Тест 1: Предсказание с использованием Moving Average */
    anomaly_result_t result;
    double value = 55.0;
    
    int ret = anomaly_detector_predict(&detector, &value, &result);
    TEST_ASSERT(ret == 0, "Moving Avg predict returns 0");
    TEST_ASSERT(result.detected_by == ANOMALY_ALGO_MOVING_AVG,
                "Detected by Moving Average algorithm");
    
    /* Тест 2: Добавление образца для онлайн-обучения */
    ret = anomaly_detector_add_sample(&detector, &value);
    TEST_ASSERT(ret == 0, "Add sample returns 0");
    
    free(train_data);
    anomaly_detector_cleanup(&detector);
}

/* ============================================
 * Тесты Exponential Smoothing
 * ============================================ */

static void test_anomaly_exp_smoothing(void) {
    printf("\n=== Test: Exponential Smoothing Anomaly Detection ===\n");
    
    anomaly_detector_t detector;
    anomaly_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = ANOMALY_ALGO_EXP_SMOOTH;
    config.n_features = 1;
    config.max_samples = 500;
    config.exp_smooth_alpha = 0.3f;
    config.threshold = 0.6f;
    
    anomaly_detector_init(&detector, &config);
    
    /* Обучение */
    size_t n_samples = 100;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    for (size_t i = 0; i < n_samples; i++) {
        train_data[i] = 100.0 + 5.0 * sin((double)i / 5.0);
    }
    
    anomaly_detector_train(&detector, train_data, n_samples, 1);
    
    /* Тест 1: Предсказание */
    anomaly_result_t result;
    double value = 102.0;
    
    int ret = anomaly_detector_predict(&detector, &value, &result);
    TEST_ASSERT(ret == 0, "Exp Smoothing predict returns 0");
    TEST_ASSERT(result.detected_by == ANOMALY_ALGO_EXP_SMOOTH,
                "Detected by Exponential Smoothing");
    
    free(train_data);
    anomaly_detector_cleanup(&detector);
}

/* ============================================
 * Тесты Ensemble алгоритма
 * ============================================ */

static void test_anomaly_ensemble(void) {
    printf("\n=== Test: Ensemble Anomaly Detection ===\n");
    
    anomaly_detector_t detector;
    anomaly_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = ANOMALY_ALGO_ENSEMBLE;
    config.n_features = 2;
    config.max_samples = 500;
    config.enable_ensemble = true;
    config.threshold = 0.6f;
    
    anomaly_detector_init(&detector, &config);
    
    /* Обучение */
    size_t n_samples = 200;
    size_t n_features = 2;
    double* train_data = (double*)malloc(n_samples * n_features * sizeof(double));
    generate_normal_data(train_data, n_samples, n_features, 50.0, 10.0);
    
    anomaly_detector_train(&detector, train_data, n_samples, n_features);
    
    /* Тест 1: Предсказание с использованием ансамбля */
    anomaly_result_t result;
    double values[2] = {52.0, 48.0};
    
    int ret = anomaly_detector_predict(&detector, values, &result);
    TEST_ASSERT(ret == 0, "Ensemble predict returns 0");
    TEST_ASSERT(result.detected_by == ANOMALY_ALGO_ENSEMBLE,
                "Detected by Ensemble algorithm");
    
    /* Тест 2: Аномальное значение */
    double anomalous[2] = {150.0, 160.0};
    
    ret = anomaly_detector_predict(&detector, anomalous, &result);
    TEST_ASSERT(ret == 0, "Ensemble predict for anomaly returns 0");
    TEST_ASSERT(result.status >= ANOMALY_STATUS_SUSPICIOUS,
                "Anomaly detected by Ensemble");
    
    free(train_data);
    anomaly_detector_cleanup(&detector);
}

/* ============================================
 * Тесты callback функций
 * ============================================ */

static int callback_triggered = 0;
static anomaly_status_t callback_status;

static void anomaly_callback(const anomaly_result_t* result, void* user_data) {
    callback_triggered = 1;
    callback_status = result->status;
    (void)user_data;
}

static void test_anomaly_callback(void) {
    printf("\n=== Test: Anomaly Detection Callback ===\n");
    
    anomaly_detector_t detector;
    anomaly_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = ANOMALY_ALGO_ZSCORE;
    config.n_features = 1;
    config.max_samples = 200;
    config.zscore_threshold = 2.0f; /* Низкий порог для срабатывания */
    config.threshold = 0.5f;
    
    anomaly_detector_init(&detector, &config);
    
    /* Обучение */
    size_t n_samples = 100;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    generate_normal_data(train_data, n_samples, 1, 50.0, 5.0);
    
    anomaly_detector_train(&detector, train_data, n_samples, 1);
    
    /* Установка callback */
    callback_triggered = 0;
    anomaly_detector_set_callback(&detector, anomaly_callback, NULL);
    
    /* Тест 1: Callback не срабатывает для нормальных значений */
    double normal_value = 51.0;
    anomaly_result_t result;
    
    anomaly_detector_predict(&detector, &normal_value, &result);
    TEST_ASSERT(callback_triggered == 0, "Callback not triggered for normal value");
    
    /* Тест 2: Callback срабатывает для аномальных значений */
    double anomalous_value = 80.0;
    
    anomaly_detector_predict(&detector, &anomalous_value, &result);
    TEST_ASSERT(callback_triggered == 1, "Callback triggered for anomalous value");
    TEST_ASSERT(callback_status >= ANOMALY_STATUS_SUSPICIOUS,
                "Callback status is suspicious or higher");
    
    free(train_data);
    anomaly_detector_cleanup(&detector);
}

/* ============================================
 * Тесты статистики
 * ============================================ */

static void test_anomaly_statistics(void) {
    printf("\n=== Test: Anomaly Detection Statistics ===\n");
    
    anomaly_detector_t detector;
    anomaly_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = ANOMALY_ALGO_ISOLATION_FOREST;
    config.n_features = 1;
    config.max_samples = 500;
    
    anomaly_detector_init(&detector, &config);
    
    /* Обучение */
    size_t n_samples = 100;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    for (size_t i = 0; i < n_samples; i++) {
        train_data[i] = 50.0 + (double)(i % 10);
    }
    
    anomaly_detector_train(&detector, train_data, n_samples, 1);
    
    /* Множественные предсказания */
    for (int i = 0; i < 20; i++) {
        double value = 50.0 + (double)(i % 5);
        anomaly_result_t result;
        anomaly_detector_predict(&detector, &value, &result);
    }
    
    /* Тест 1: Проверка статистики */
    size_t total_predictions;
    size_t anomalies_detected;
    float accuracy;
    
    anomaly_detector_get_stats(&detector, &total_predictions, &anomalies_detected, &accuracy);
    TEST_ASSERT(total_predictions == 20, "Total predictions count is correct");
    TEST_ASSERT(accuracy >= 0.0 && accuracy <= 1.0, "Accuracy in valid range");
    
    /* Тест 2: Сброс статистики */
    anomaly_detector_reset_stats(&detector);
    anomaly_detector_get_stats(&detector, &total_predictions, &anomalies_detected, &accuracy);
    TEST_ASSERT(total_predictions == 0, "Stats reset clears total_predictions");
    
    /* Тест 3: Установка порога */
    int ret = anomaly_detector_set_threshold(&detector, 0.8f);
    TEST_ASSERT(ret == 0, "Set threshold returns 0");
    
    ret = anomaly_detector_set_threshold(&detector, 1.5f); /* Недопустимое значение */
    TEST_ASSERT(ret == -1, "Set invalid threshold returns -1");
    
    free(train_data);
    anomaly_detector_cleanup(&detector);
}

/* ============================================
 * Тесты вспомогательных функций
 * ============================================ */

static void test_anomaly_helper_functions(void) {
    printf("\n=== Test: Anomaly Detection Helper Functions ===\n");
    
    /* Тест 1: Вычисление среднего */
    double data[] = {10.0, 20.0, 30.0, 40.0, 50.0};
    double mean = anomaly_compute_mean(data, 5);
    TEST_ASSERT(fabs(mean - 30.0) < 0.001, "Compute mean is correct");
    
    /* Тест 2: Вычисление стандартного отклонения */
    double std_dev = anomaly_compute_std_dev(data, 5, mean);
    TEST_ASSERT(std_dev > 15.0 && std_dev < 16.0, "Compute std dev is in expected range");
    
    /* Тест 3: Вычисление Z-Score */
    double zscore = anomaly_compute_zscore(50.0, mean, std_dev);
    TEST_ASSERT(zscore > 1.0 && zscore < 2.0, "Compute Z-Score is correct");
    
    /* Тест 4: Нормализация значения */
    double normalized = anomaly_normalize_value(30.0, 10.0, 50.0);
    TEST_ASSERT(fabs(normalized - 0.5) < 0.01, "Normalize value is correct");
    
    /* Тест 5: Скользящее среднее */
    double buffer[] = {10.0, 20.0, 30.0};
    double ma = anomaly_compute_moving_avg(buffer, 3);
    TEST_ASSERT(fabs(ma - 20.0) < 0.001, "Compute moving avg is correct");
    
    /* Тест 6: Экспоненциальное сглаживание */
    double smoothed = anomaly_exp_smooth(20.0, 30.0, 0.5f);
    TEST_ASSERT(fabs(smoothed - 25.0) < 0.001, "Exp smooth is correct");
    
    /* Тест 7: Конвертация статуса в строку */
    const char* status_str = anomaly_status_to_string(ANOMALY_STATUS_CRITICAL);
    TEST_ASSERT(strcmp(status_str, "Critical") == 0, "Status to string is correct");
    
    /* Тест 8: Конвертация алгоритма в строку */
    const char* algo_str = anomaly_algo_to_string(ANOMALY_ALGO_ISOLATION_FOREST);
    TEST_ASSERT(strstr(algo_str, "Isolation Forest") != NULL,
                "Algo to string is correct");
}

/* ============================================
 * Тесты производительности
 * ============================================ */

static void test_anomaly_performance(void) {
    printf("\n=== Test: Anomaly Detection Performance ===\n");
    
    anomaly_detector_t detector;
    anomaly_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = ANOMALY_ALGO_ISOLATION_FOREST;
    config.n_features = 5;
    config.max_samples = 2000;
    config.n_trees = 100;
    
    anomaly_detector_init(&detector, &config);
    
    /* Тест 1: Обучение на больших данных */
    size_t n_samples = 1000;
    size_t n_features = 5;
    double* train_data = (double*)malloc(n_samples * n_features * sizeof(double));
    generate_normal_data(train_data, n_samples, n_features, 100.0, 20.0);
    
    clock_t start = clock();
    int ret = anomaly_detector_train(&detector, train_data, n_samples, n_features);
    clock_t end = clock();
    
    double train_time = (double)(end - start) / CLOCKS_PER_SEC;
    TEST_ASSERT(ret == 0, "Training on large dataset succeeds");
    TEST_ASSERT(train_time < 5.0, "Training completes in reasonable time");
    printf("    Training time: %.3f seconds\n", train_time);
    
    /* Тест 2: Скорость предсказания */
    double values[5] = {100.0, 105.0, 98.0, 102.0, 97.0};
    anomaly_result_t result;
    
    start = clock();
    for (int i = 0; i < 1000; i++) {
        anomaly_detector_predict(&detector, values, &result);
    }
    end = clock();
    
    double predict_time = (double)(end - start) / CLOCKS_PER_SEC;
    double avg_predict_time = (predict_time * 1000.0) / 1000.0; /* ms */
    TEST_ASSERT(avg_predict_time < 10.0, "Average predict time < 10ms");
    printf("    Avg predict time: %.3f ms\n", avg_predict_time);
    
    free(train_data);
    anomaly_detector_cleanup(&detector);
}

/* ============================================
 * Тесты JSON экспорта/импорта
 * ============================================ */

static void test_anomaly_json_export(void) {
    printf("\n=== Test: Anomaly Detection JSON Export ===\n");
    
    anomaly_detector_t detector;
    anomaly_config_t config;
    
    memset(&config, 0, sizeof(config));
    config.algorithm = ANOMALY_ALGO_ZSCORE;
    config.n_features = 1;
    config.threshold = 0.7f;
    config.zscore_threshold = 3.0f;
    
    anomaly_detector_init(&detector, &config);
    
    /* Обучение */
    size_t n_samples = 50;
    double* train_data = (double*)malloc(n_samples * sizeof(double));
    for (size_t i = 0; i < n_samples; i++) {
        train_data[i] = 50.0 + (double)i;
    }
    
    anomaly_detector_train(&detector, train_data, n_samples, 1);
    
    /* Тест 1: Экспорт в JSON */
    char buffer[1024];
    int written = anomaly_detector_export_json(&detector, buffer, sizeof(buffer));
    TEST_ASSERT(written > 0, "JSON export returns positive bytes");
    TEST_ASSERT(written < (int)sizeof(buffer), "JSON fits in buffer");
    TEST_ASSERT(strstr(buffer, "\"config\"") != NULL, "JSON contains config");
    TEST_ASSERT(strstr(buffer, "\"stats\"") != NULL, "JSON contains stats");
    
    printf("    JSON output (%d bytes):\n", written);
    printf("    %.*s\n", (written < 200) ? written : 200, buffer);
    
    /* Тест 2: Импорт из JSON (заглушка) */
    int ret = anomaly_detector_import_json(&detector, buffer);
    TEST_ASSERT(ret == 0, "JSON import returns 0 (stub)");
    
    /* Тест 3: Экспорт с NULL буфером */
    written = anomaly_detector_export_json(&detector, NULL, 0);
    TEST_ASSERT(written == -1, "Export with NULL buffer returns -1");
    
    free(train_data);
    anomaly_detector_cleanup(&detector);
}

/* ============================================
 * Edge Cases Tests
 * ============================================ */

/**
 * @brief Тест: обработка граничных случаев
 */
static void test_anomaly_edge_cases(void) {
    printf("\n--- Edge Cases Tests ---\n");
    
    struct anomaly_detector detector;
    double single_sample[4] = {1.0, 2.0, 3.0, 4.0};
    double empty_data[1] = {0.0};
    double huge_value[4] = {1e10, -1e10, 1e10, -1e10};
    double nan_value[4] = {NAN, 1.0, 2.0, 3.0};
    double inf_value[4] = {INFINITY, 1.0, 2.0, 3.0};
    float anomaly_score;
    int ret;
    
    /* Инициализация */
    ret = anomaly_detector_init(&detector, ANOMALY_ALGO_ZSCORE, 4, 100, 0.6f);
    TEST_ASSERT(ret == 0, "Edge cases: detector init");
    
    /* Тест 1: Один образец для обучения */
    ret = anomaly_detector_train(&detector, single_sample, 1);
    TEST_ASSERT(ret == 0, "Edge cases: train with single sample");
    
    /* Предсказание на одном образце должно работать */
    ret = anomaly_detector_predict(&detector, single_sample, &anomaly_score);
    TEST_ASSERT(ret == 0, "Edge cases: predict after single sample training");
    
    anomaly_detector_cleanup(&detector);
    
    /* Тест 2: Обучение с NULL данными */
    ret = anomaly_detector_init(&detector, ANOMALY_ALGO_ZSCORE, 4, 100, 0.6f);
    TEST_ASSERT(ret == 0, "Edge cases: detector init for NULL test");
    
    ret = anomaly_detector_train(&detector, NULL, 0);
    TEST_ASSERT(ret == -1, "Edge cases: train with NULL data returns -1");
    
    anomaly_detector_cleanup(&detector);
    
    /* Тест 3: Огромные значения (stress test) */
    ret = anomaly_detector_init(&detector, ANOMALY_ALGO_ZSCORE, 4, 100, 0.6f);
    TEST_ASSERT(ret == 0, "Edge cases: detector init for huge values");
    
    ret = anomaly_detector_train(&detector, huge_value, 10);
    TEST_ASSERT(ret == 0, "Edge cases: train with huge values");
    
    ret = anomaly_detector_predict(&detector, huge_value, &anomaly_score);
    TEST_ASSERT(ret == 0, "Edge cases: predict with huge values");
    
    anomaly_detector_cleanup(&detector);
    
    /* Тест 4: NaN значения */
    ret = anomaly_detector_init(&detector, ANOMALY_ALGO_ZSCORE, 4, 100, 0.6f);
    TEST_ASSERT(ret == 0, "Edge cases: detector init for NaN test");
    
    ret = anomaly_detector_train(&detector, nan_value, 10);
    /* Обучение с NaN может вернуть ошибку или обработать */
    TEST_ASSERT(ret == 0 || ret == -1, "Edge cases: train with NaN values handled");
    
    anomaly_detector_cleanup(&detector);
    
    /* Тест 5: Бесконечные значения */
    ret = anomaly_detector_init(&detector, ANOMALY_ALGO_ZSCORE, 4, 100, 0.6f);
    TEST_ASSERT(ret == 0, "Edge cases: detector init for Inf test");
    
    ret = anomaly_detector_train(&detector, inf_value, 10);
    /* Обучение с Inf может вернуть ошибку или обработать */
    TEST_ASSERT(ret == 0 || ret == -1, "Edge cases: train with Inf values handled");
    
    anomaly_detector_cleanup(&detector);
    
    /* Тест 6: Предсказание без обучения */
    ret = anomaly_detector_init(&detector, ANOMALY_ALGO_ZSCORE, 4, 100, 0.6f);
    TEST_ASSERT(ret == 0, "Edge cases: detector init for untrained test");
    
    ret = anomaly_detector_predict(&detector, single_sample, &anomaly_score);
    TEST_ASSERT(ret == -1, "Edge cases: predict without training returns -1");
    
    anomaly_detector_cleanup(&detector);
    
    /* Тест 7: Нулевой контекст */
    ret = anomaly_detector_train(NULL, single_sample, 10);
    TEST_ASSERT(ret == -1, "Edge cases: train with NULL context returns -1");
    
    ret = anomaly_detector_predict(NULL, single_sample, &anomaly_score);
    TEST_ASSERT(ret == -1, "Edge cases: predict with NULL context returns -1");
    
    ret = anomaly_detector_add_sample(NULL, single_sample);
    TEST_ASSERT(ret == -1, "Edge cases: add_sample with NULL context returns -1");
    
    anomaly_detector_cleanup(NULL);  /* Должно быть no-op */
    TEST_ASSERT(1, "Edge cases: cleanup with NULL context is safe");
}

/**
 * @brief Тест: стабильность при множественных операциях
 */
static void test_anomaly_stress(void) {
    printf("\n--- Stress Tests ---\n");
    
    struct anomaly_detector detector;
    const size_t n_features = 8;
    const size_t n_samples = 500;
    double *data = malloc(n_features * n_samples * sizeof(double));
    float score;
    int ret;
    
    TEST_ASSERT(data != NULL, "Stress: allocate data");
    if (!data) return;
    
    /* Генерация случайных данных */
    srand(12345);
    for (size_t i = 0; i < n_samples; i++) {
        for (size_t f = 0; f < n_features; f++) {
            data[i * n_features + f] = (double)rand() / RAND_MAX * 100.0;
        }
    }
    
    /* Инициализация */
    ret = anomaly_detector_init(&detector, ANOMALY_ALGO_ISOLATION_FOREST, 
                                 n_features, n_samples, 0.6f);
    TEST_ASSERT(ret == 0, "Stress: detector init");
    
    /* Множественное обучение */
    for (int i = 0; i < 5; i++) {
        ret = anomaly_detector_train(&detector, data, n_samples);
        TEST_ASSERT(ret == 0, "Stress: repeated training (iteration %d)", i);
    }
    
    /* Множественные предсказания */
    for (int i = 0; i < 100; i++) {
        ret = anomaly_detector_predict(&detector, data, &score);
        TEST_ASSERT(ret == 0, "Stress: repeated prediction (iteration %d)", i);
    }
    
    /* Добавление множества образцов */
    for (size_t i = 0; i < 50; i++) {
        ret = anomaly_detector_add_sample(&detector, data + (i * n_features));
        TEST_ASSERT(ret == 0, "Stress: add sample %d", (int)i);
    }
    
    /* Статистика */
    struct anomaly_stats stats;
    ret = anomaly_detector_get_stats(&detector, &stats);
    TEST_ASSERT(ret == 0, "Stress: get stats after stress operations");
    
    free(data);
    anomaly_detector_cleanup(&detector);
}

/* ============================================
 * Главная функция тестирования
 * ============================================ */

int main(void) {
    printf("============================================\n");
    printf("  Anomaly Detection System Tests\n");
    printf("  Version: 1.0.32\n");
    printf("============================================\n");
    
    /* Запуск всех тестов */
    test_anomaly_init_cleanup();
    test_anomaly_training();
    test_anomaly_prediction();
    test_anomaly_zscore();
    test_anomaly_moving_avg();
    test_anomaly_exp_smoothing();
    test_anomaly_ensemble();
    test_anomaly_callback();
    test_anomaly_statistics();
    test_anomaly_helper_functions();
    test_anomaly_performance();
    test_anomaly_json_export();
    test_anomaly_edge_cases();
    test_anomaly_stress();

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
