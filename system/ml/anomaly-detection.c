/**
 * @file anomaly-detection.c
 * @brief Реализация системы ML-детекции аномалий в сетевом трафике
 * 
 * @version 1.0.32
 * @date 29 марта 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#include "anomaly-detection.h"
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
 * Внутренние структуры и функции
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

/**
 * @brief Блокировка мьютекса
 */
static void lock_mutex(void* mutex) {
#ifdef _WIN32
    WaitForSingleObject((HANDLE)mutex, INFINITE);
#else
    pthread_mutex_lock((pthread_mutex_t*)mutex);
#endif
}

/**
 * @brief Разблокировка мьютекса
 */
static void unlock_mutex(void* mutex) {
#ifdef _WIN32
    ReleaseMutex((HANDLE)mutex);
#else
    pthread_mutex_unlock((pthread_mutex_t*)mutex);
#endif
}

/**
 * @brief Очистка мьютекса
 */
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
        fprintf(stderr, "Anomaly Detection: Failed to allocate %zu bytes\n", size);
    }
    return ptr;
}

/* ============================================
 * Вспомогательные математические функции
 * ============================================ */

double anomaly_compute_mean(const double* data, size_t count) {
    if (!data || count == 0) return 0.0;
    
    double sum = 0.0;
    for (size_t i = 0; i < count; i++) {
        sum += data[i];
    }
    return sum / (double)count;
}

double anomaly_compute_std_dev(const double* data, size_t count, double mean) {
    if (!data || count <= 1) return 0.0;
    
    double sum_sq = 0.0;
    for (size_t i = 0; i < count; i++) {
        double diff = data[i] - mean;
        sum_sq += diff * diff;
    }
    return sqrt(sum_sq / (double)(count - 1));
}

double anomaly_compute_zscore(double value, double mean, double std_dev) {
    if (std_dev == 0.0) return 0.0;
    return (value - mean) / std_dev;
}

double anomaly_normalize_value(double value, double min, double max) {
    if (max == min) return 0.5;
    double normalized = (value - min) / (max - min);
    return (normalized < 0.0) ? 0.0 : ((normalized > 1.0) ? 1.0 : normalized);
}

double anomaly_compute_moving_avg(const double* buffer, size_t count) {
    if (!buffer || count == 0) return 0.0;
    
    double sum = 0.0;
    for (size_t i = 0; i < count; i++) {
        sum += buffer[i];
    }
    return sum / (double)count;
}

double anomaly_exp_smooth(double prev_value, double current_value, float alpha) {
    return alpha * current_value + (1.0f - alpha) * prev_value;
}

/* ============================================
 * Статистические функции
 * ============================================ */

/**
 * @brief Обновление статистики по признаку
 */
static void update_feature_stats(anomaly_feature_stats_t* stats, double value) {
    if (!stats) return;
    
    /* Обновление мин/макс */
    if (stats->count == 0 || value < stats->min) stats->min = value;
    if (stats->count == 0 || value > stats->max) stats->max = value;
    
    /* Обновление суммы и суммы квадратов для mean/stddev */
    stats->sum += value;
    stats->sum_squares += value * value;
    stats->count++;
    
    /* Вычисление среднего и стандартного отклонения */
    stats->mean = stats->sum / (double)stats->count;
    
    if (stats->count > 1) {
        double variance = (stats->sum_squares - (stats->sum * stats->sum) / stats->count) 
                         / (double)(stats->count - 1);
        stats->std_dev = sqrt(variance > 0 ? variance : 0);
    }
    
    /* IQR вычисляется отдельно при необходимости */
    stats->iqr = stats->q3 - stats->q1;
}

/**
 * @brief Сортировка массива для вычисления перцентилей
 */
static void sort_array(double* arr, size_t n) {
    if (n <= 1) return;
    
    /* Простая сортировка пузырьком для небольших массивов */
    for (size_t i = 0; i < n - 1; i++) {
        for (size_t j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                double temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

/**
 * @brief Вычисление перцентилей
 */
static void compute_percentiles(double* data, size_t count, double* q1, double* q3) {
    if (!data || count == 0) {
        if (q1) *q1 = 0;
        if (q3) *q3 = 0;
        return;
    }
    
    /* Копирование и сортировка */
    double* sorted = (double*)safe_malloc(count * sizeof(double));
    if (!sorted) return;
    memcpy(sorted, data, count * sizeof(double));
    sort_array(sorted, count);
    
    /* Q1 (25-й перцентиль) */
    size_t q1_idx = count / 4;
    *q1 = sorted[q1_idx];
    
    /* Q3 (75-й перцентиль) */
    size_t q3_idx = (3 * count) / 4;
    *q3 = sorted[q3_idx];
    
    free(sorted);
}

/* ============================================
 * Isolation Forest реализация
 * ============================================ */

/**
 * @brief Создание узла дерева
 */
static anomaly_tree_node_t* create_tree_node(size_t feature_index, double split_value, 
                                             size_t depth) {
    anomaly_tree_node_t* node = (anomaly_tree_node_t*)safe_malloc(sizeof(anomaly_tree_node_t));
    if (!node) return NULL;
    
    node->feature_index = feature_index;
    node->split_value = split_value;
    node->left = NULL;
    node->right = NULL;
    node->size = 0;
    node->depth = depth;
    
    return node;
}

/**
 * @brief Построение дерева изоляции
 */
static anomaly_tree_node_t* build_isolation_tree(const double* data, size_t n_samples,
                                                  size_t n_features, size_t height_limit,
                                                  size_t current_depth) {
    if (current_depth >= height_limit || n_samples <= 1) {
        return create_tree_node(0, 0, current_depth);
    }
    
    /* Выбор случайного признака */
    size_t feature_idx = rand() % n_features;
    
    /* Поиск мин/макс для выбранного признака */
    double min_val = data[feature_idx];
    double max_val = data[feature_idx];
    for (size_t i = 0; i < n_samples; i++) {
        double val = data[i * n_features + feature_idx];
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }
    
    if (min_val == max_val) {
        return create_tree_node(feature_idx, min_val, current_depth);
    }
    
    /* Случайное значение разделения */
    double split_value = min_val + ((double)rand() / RAND_MAX) * (max_val - min_val);
    
    anomaly_tree_node_t* node = create_tree_node(feature_idx, split_value, current_depth);
    if (!node) return NULL;
    
    /* Разделение данных */
    double* left_data = (double*)safe_malloc(n_samples * n_features * sizeof(double));
    double* right_data = (double*)safe_malloc(n_samples * n_features * sizeof(double));
    
    if (!left_data || !right_data) {
        free(left_data);
        free(right_data);
        free(node);
        return NULL;
    }
    
    size_t left_count = 0;
    size_t right_count = 0;
    
    for (size_t i = 0; i < n_samples; i++) {
        double val = data[i * n_features + feature_idx];
        if (val < split_value) {
            memcpy(&left_data[left_count * n_features], &data[i * n_features], 
                   n_features * sizeof(double));
            left_count++;
        } else {
            memcpy(&right_data[right_count * n_features], &data[i * n_features],
                   n_features * sizeof(double));
            right_count++;
        }
    }
    
    /* Рекурсивное построение поддеревьев */
    node->left = build_isolation_tree(left_data, left_count, n_features, 
                                      height_limit, current_depth + 1);
    node->right = build_isolation_tree(right_data, right_count, n_features,
                                       height_limit, current_depth + 1);
    
    node->size = n_samples;
    
    free(left_data);
    free(right_data);
    
    return node;
}

/**
 * @brief Очистка дерева
 */
static void cleanup_tree_node(anomaly_tree_node_t* node) {
    if (!node) return;
    cleanup_tree_node(node->left);
    cleanup_tree_node(node->right);
    free(node);
}

/**
 * @brief Вычисление пути длины для точки
 */
static double path_length(anomaly_tree_node_t* node, const double* point, 
                          size_t n_features, size_t current_depth) {
    if (!node || !node->left || !node->right) {
        /* Листовой узел - добавляем коррекцию */
        size_t size = node ? node->size : 0;
        if (size <= 1) return (double)current_depth;
        
        /* Коррекция для размера */
        double c = 2.0 * (log((double)(size - 1)) + 0.5772156649) - (2.0 * (size - 1) / (double)size);
        return current_depth + c;
    }
    
    double val = point[node->feature_index];
    if (val < node->split_value) {
        return path_length(node->left, point, n_features, current_depth + 1);
    } else {
        return path_length(node->right, point, n_features, current_depth + 1);
    }
}

/**
 * @brief Построение Isolation Forest
 */
static int build_isolation_forest(anomaly_detector_t* detector, const double* data) {
    if (!detector || !data) return -1;
    
    size_t n_trees = detector->config.n_trees;
    size_t n_features = detector->n_features;
    size_t n_samples = detector->n_samples;
    size_t height_limit = detector->config.tree_height;
    
    /* Выделение памяти для деревьев */
    detector->trees = (anomaly_tree_t*)safe_malloc(n_trees * sizeof(anomaly_tree_t));
    if (!detector->trees) return -1;
    
    detector->n_trees = n_trees;
    
    /* Построение каждого дерева */
    for (size_t t = 0; t < n_trees; t++) {
        /* Выборка подмножества данных (bootstrap) */
        size_t sample_size = (n_samples < 256) ? n_samples : 256;
        double* sample_data = (double*)safe_malloc(sample_size * n_features * sizeof(double));
        if (!sample_data) continue;
        
        for (size_t i = 0; i < sample_size; i++) {
            size_t idx = rand() % n_samples;
            memcpy(&sample_data[i * n_features], &data[idx * n_features],
                   n_features * sizeof(double));
        }
        
        /* Построение дерева */
        detector->trees[t].root = build_isolation_tree(sample_data, sample_size, 
                                                       n_features, height_limit, 0);
        detector->trees[t].height = height_limit;
        detector->trees[t].n_features = n_features;
        
        free(sample_data);
    }
    
    return 0;
}

/**
 * @brief Вычисление оценки аномалии через Isolation Forest
 */
static float compute_isolation_forest_score(anomaly_detector_t* detector, 
                                           const double* point) {
    if (!detector || !detector->trees || detector->n_trees == 0) return 0.5f;
    
    double avg_path_length = 0.0;
    
    for (size_t t = 0; t < detector->n_trees; t++) {
        avg_path_length += path_length(detector->trees[t].root, point, 
                                       detector->n_features, 0);
    }
    avg_path_length /= (double)detector->n_trees;
    
    /* Нормализация: более короткий путь = более высокая аномалия */
    double c = 2.0 * (log((double)(detector->n_samples - 1)) + 0.5772156649) 
               - (2.0 * (detector->n_samples - 1) / (double)detector->n_samples);
    
    double score = pow(2.0, -avg_path_length / c);
    
    return (float)score;
}

/* ============================================
 * DBSCAN реализация
 * ============================================ */

/**
 * @brief Вычисление евклидова расстояния
 */
static double euclidean_distance(const double* a, const double* b, size_t n_features) {
    double sum = 0.0;
    for (size_t i = 0; i < n_features; i++) {
        double diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sqrt(sum);
}

/**
 * @brief Поиск соседей в радиусе eps
 */
static size_t find_neighbors(anomaly_detector_t* detector, const double* point,
                             double eps, size_t* neighbors, size_t max_neighbors) {
    size_t count = 0;
    
    for (size_t i = 0; i < detector->n_samples && count < max_neighbors; i++) {
        double dist = euclidean_distance(point, &detector->training_data[i * detector->n_features],
                                         detector->n_features);
        if (dist <= eps) {
            neighbors[count] = i;
            count++;
        }
    }
    
    return count;
}

/**
 * @brief DBSCAN кластеризация
 */
static int run_dbscan(anomaly_detector_t* detector, double eps, size_t min_pts) {
    if (!detector || !detector->training_data) return -1;
    
    size_t n_samples = detector->n_samples;
    size_t n_features = detector->n_features;
    
    /* Выделение памяти для кластеров */
    detector->clusters = (anomaly_cluster_t*)safe_malloc(ANOMALY_MAX_CLUSTERS * sizeof(anomaly_cluster_t));
    detector->cluster_labels = (int*)safe_malloc(n_samples * sizeof(int));
    
    if (!detector->clusters || !detector->cluster_labels) return -1;
    
    /* Инициализация */
    for (size_t i = 0; i < n_samples; i++) {
        detector->cluster_labels[i] = -1; /* Не помечен */
    }
    detector->n_clusters = 0;
    
    /* Временный массив соседей */
    size_t* neighbors = (size_t*)safe_malloc(n_samples * sizeof(size_t));
    if (!neighbors) return -1;
    
    int cluster_id = 0;
    
    for (size_t i = 0; i < n_samples; i++) {
        if (detector->cluster_labels[i] != -1) continue; /* Уже помечен */
        
        /* Поиск соседей */
        size_t neighbor_count = find_neighbors(detector, 
                                               &detector->training_data[i * n_features],
                                               eps, neighbors, n_samples);
        
        if (neighbor_count < min_pts) {
            detector->cluster_labels[i] = -2; /* Шум */
            continue;
        }
        
        /* Создание нового кластера */
        if (detector->n_clusters >= ANOMALY_MAX_CLUSTERS) break;
        
        anomaly_cluster_t* cluster = &detector->clusters[detector->n_clusters];
        cluster->id = cluster_id;
        cluster->point_count = 0;
        cluster->capacity = n_samples;
        cluster->point_indices = (size_t*)safe_malloc(cluster->capacity * sizeof(size_t));
        cluster->is_noise = false;
        
        if (!cluster->point_indices) continue;
        
        /* Добавление точки в кластер */
        detector->cluster_labels[i] = cluster_id;
        cluster->point_indices[cluster->point_count++] = i;
        
        /* Обработка соседей */
        for (size_t j = 0; j < neighbor_count; j++) {
            size_t neighbor_idx = neighbors[j];
            
            if (detector->cluster_labels[neighbor_idx] == -2) {
                detector->cluster_labels[neighbor_idx] = cluster_id; /* Из шума в кластер */
            }
            
            if (detector->cluster_labels[neighbor_idx] == -1) {
                detector->cluster_labels[neighbor_idx] = cluster_id;
                cluster->point_indices[cluster->point_count++] = neighbor_idx;
                
                /* Рекурсивный поиск соседей */
                size_t* sub_neighbors = (size_t*)safe_malloc(n_samples * sizeof(size_t));
                if (sub_neighbors) {
                    size_t sub_count = find_neighbors(detector,
                                                      &detector->training_data[neighbor_idx * n_features],
                                                      eps, sub_neighbors, n_samples);
                    
                    if (sub_count >= min_pts) {
                        for (size_t k = 0; k < sub_count; k++) {
                            size_t found = 0;
                            for (size_t m = 0; m < neighbor_count; m++) {
                                if (neighbors[m] == sub_neighbors[k]) {
                                    found = 1;
                                    break;
                                }
                            }
                            if (!found) {
                                neighbors[neighbor_count++] = sub_neighbors[k];
                            }
                        }
                    }
                    free(sub_neighbors);
                }
            }
        }
        
        /* Вычисление центроида */
        memset(cluster->centroid, 0, n_features * sizeof(double));
        for (size_t j = 0; j < cluster->point_count; j++) {
            size_t idx = cluster->point_indices[j];
            for (size_t f = 0; f < n_features; f++) {
                cluster->centroid[f] += detector->training_data[idx * n_features + f];
            }
        }
        for (size_t f = 0; f < n_features; f++) {
            cluster->centroid[f] /= (double)cluster->point_count;
        }
        
        detector->n_clusters++;
        cluster_id++;
    }
    
    free(neighbors);
    return 0;
}

/**
 * @brief Определение кластера для новой точки
 */
static int find_cluster_for_point(anomaly_detector_t* detector, const double* point, 
                                  double eps) {
    int best_cluster = -1;
    double min_dist = INFINITY;
    
    for (size_t c = 0; c < detector->n_clusters; c++) {
        double dist = euclidean_distance(point, detector->clusters[c].centroid, 
                                         detector->n_features);
        if (dist < min_dist) {
            min_dist = dist;
            best_cluster = (int)c;
        }
    }
    
    if (min_dist > eps) {
        return -2; /* Аномалия - не принадлежит ни одному кластеру */
    }
    
    return best_cluster;
}

/* ============================================
 * Основные функции API
 * ============================================ */

int anomaly_detector_init(anomaly_detector_t* detector, const anomaly_config_t* config) {
    if (!detector) return -1;
    
    memset(detector, 0, sizeof(anomaly_detector_t));
    
    /* Копирование конфигурации */
    if (config) {
        memcpy(&detector->config, config, sizeof(anomaly_config_t));
    } else {
        /* Конфигурация по умолчанию */
        detector->config.algorithm = ANOMALY_ALGO_ISOLATION_FOREST;
        detector->config.threshold = ANOMALY_DEFAULT_THRESHOLD;
        detector->config.max_samples = ANOMALY_MAX_SAMPLES;
        detector->config.n_features = 1;
        detector->config.n_trees = 50;
        detector->config.tree_height = ANOMALY_MAX_TREE_HEIGHT;
        detector->config.zscore_threshold = 3.0f;
        detector->config.moving_avg_window = ANOMALY_MOVING_AVG_WINDOW;
        detector->config.exp_smooth_alpha = ANOMALY_EXP_SMOOTH_ALPHA;
        detector->config.enable_ensemble = false;
        detector->config.enable_adaptive_threshold = false;
        detector->config.enable_real_time = false;
    }
    
    detector->n_features = detector->config.n_features;
    detector->n_samples = 0;
    
    /* Выделение памяти для данных */
    detector->training_data = (double*)safe_malloc(
        detector->config.max_samples * detector->n_features * sizeof(double));
    detector->feature_stats = (anomaly_feature_stats_t*)safe_malloc(
        detector->n_features * sizeof(anomaly_feature_stats_t));
    
    if (!detector->training_data || !detector->feature_stats) {
        anomaly_detector_cleanup(detector);
        return -1;
    }
    
    memset(detector->training_data, 0, 
           detector->config.max_samples * detector->n_features * sizeof(double));
    memset(detector->feature_stats, 0, 
           detector->n_features * sizeof(anomaly_feature_stats_t));
    
    /* Инициализация Moving Average буфера */
    detector->moving_avg_buffer = (double*)safe_malloc(
        detector->config.moving_avg_window * sizeof(double));
    if (detector->moving_avg_buffer) {
        memset(detector->moving_avg_buffer, 0, 
               detector->config.moving_avg_window * sizeof(double));
    }
    
    /* Инициализация мьютекса */
    if (init_mutex(&detector->mutex) != 0) {
        anomaly_detector_cleanup(detector);
        return -1;
    }
    
    /* Инициализация для ансамбля */
    if (detector->config.enable_ensemble) {
        detector->n_algos = 3; /* IF, Z-Score, DBSCAN */
        detector->algo_scores = (float*)safe_malloc(detector->n_algos * sizeof(float));
    }
    
    return 0;
}

void anomaly_detector_cleanup(anomaly_detector_t* detector) {
    if (!detector) return;
    
    /* Очистка данных */
    free(detector->training_data);
    free(detector->feature_stats);
    free(detector->moving_avg_buffer);
    free(detector->algo_scores);
    
    /* Очистка деревьев Isolation Forest */
    if (detector->trees) {
        for (size_t t = 0; t < detector->n_trees; t++) {
            cleanup_tree_node(detector->trees[t].root);
        }
        free(detector->trees);
    }
    
    /* Очистка кластеров DBSCAN */
    if (detector->clusters) {
        for (size_t c = 0; c < detector->n_clusters; c++) {
            free(detector->clusters[c].point_indices);
        }
        free(detector->clusters);
    }
    free(detector->cluster_labels);
    
    /* Очистка мьютекса */
    if (detector->mutex) {
        cleanup_mutex(detector->mutex);
    }
    
    memset(detector, 0, sizeof(anomaly_detector_t));
}

int anomaly_detector_train(anomaly_detector_t* detector, const double* data,
                           size_t n_samples, size_t n_features) {
    if (!detector || !data || n_samples == 0 || n_features == 0) return -1;
    
    lock_mutex(detector->mutex);
    
    /* Проверка размера */
    if (n_samples > detector->config.max_samples) {
        n_samples = detector->config.max_samples;
    }
    
    if (n_features != detector->n_features) {
        unlock_mutex(detector->mutex);
        return -1; /* Несовпадение количества признаков */
    }
    
    /* Копирование данных */
    memcpy(detector->training_data, data, n_samples * n_features * sizeof(double));
    detector->n_samples = n_samples;
    
    /* Вычисление статистики по признакам */
    for (size_t f = 0; f < n_features; f++) {
        /* Сбор значений для признака */
        double* feature_values = (double*)safe_malloc(n_samples * sizeof(double));
        if (!feature_values) continue;
        
        for (size_t i = 0; i < n_samples; i++) {
            feature_values[i] = data[i * n_features + f];
        }
        
        /* Вычисление статистики */
        detector->feature_stats[f].mean = anomaly_compute_mean(feature_values, n_samples);
        detector->feature_stats[f].std_dev = anomaly_compute_std_dev(
            feature_values, n_samples, detector->feature_stats[f].mean);
        detector->feature_stats[f].min = feature_values[0];
        detector->feature_stats[f].max = feature_values[0];
        for (size_t i = 1; i < n_samples; i++) {
            if (feature_values[i] < detector->feature_stats[f].min)
                detector->feature_stats[f].min = feature_values[i];
            if (feature_values[i] > detector->feature_stats[f].max)
                detector->feature_stats[f].max = feature_values[i];
        }
        
        compute_percentiles(feature_values, n_samples, 
                           &detector->feature_stats[f].q1,
                           &detector->feature_stats[f].q3);
        detector->feature_stats[f].iqr = detector->feature_stats[f].q3 - detector->feature_stats[f].q1;
        detector->feature_stats[f].count = n_samples;
        
        free(feature_values);
    }
    
    /* Построение Isolation Forest */
    if (detector->config.algorithm == ANOMALY_ALGO_ISOLATION_FOREST ||
        detector->config.enable_ensemble) {
        build_isolation_forest(detector, data);
    }
    
    /* Запуск DBSCAN */
    if (detector->config.algorithm == ANOMALY_ALGO_DBSCAN ||
        detector->config.enable_ensemble) {
        double eps = 2.0; /* Настройка зависит от данных */
        size_t min_pts = 4;
        run_dbscan(detector, eps, min_pts);
    }
    
    unlock_mutex(detector->mutex);
    
    return 0;
}

/**
 * @brief Вычисление Z-Score оценки аномалии
 */
static float compute_zscore_anomaly(anomaly_detector_t* detector, const double* values) {
    float max_zscore = 0.0f;
    
    for (size_t f = 0; f < detector->n_features; f++) {
        double zscore = anomaly_compute_zscore(values[f], 
                                               detector->feature_stats[f].mean,
                                               detector->feature_stats[f].std_dev);
        float abs_zscore = (float)fabs(zscore);
        if (abs_zscore > max_zscore) {
            max_zscore = abs_zscore;
        }
    }
    
    /* Нормализация к диапазону 0-1 */
    float threshold = detector->config.zscore_threshold;
    float score = (max_zscore >= threshold) ? 1.0f : (max_zscore / threshold);
    
    return score;
}

/**
 * @brief Вычисление DBSCAN оценки аномалии
 */
static float compute_dbscan_score(anomaly_detector_t* detector, const double* values) {
    if (!detector || detector->n_clusters == 0) return 0.5f;
    
    int cluster_id = find_cluster_for_point(detector, values, 2.0);
    
    if (cluster_id == -2) {
        return 1.0f; /* Аномалия - не принадлежит кластеру */
    }
    
    /* Вычисление расстояния до центроида */
    double dist = euclidean_distance(values, detector->clusters[cluster_id].centroid,
                                     detector->n_features);
    
    /* Нормализация */
    return (float)(1.0 - 1.0 / (1.0 + dist));
}

int anomaly_detector_predict(anomaly_detector_t* detector, const double* values,
                             anomaly_result_t* result) {
    if (!detector || !values || !result) return -1;
    
    lock_mutex(detector->mutex);
    
    memset(result, 0, sizeof(anomaly_result_t));
    result->timestamp = get_current_time_ms();
    detector->total_predictions++;
    
    float anomaly_score = 0.0f;
    
    /* Вычисление оценок по каждому алгоритму */
    if (detector->config.enable_ensemble) {
        /* Isolation Forest */
        detector->algo_scores[0] = compute_isolation_forest_score(detector, values);
        
        /* Z-Score */
        detector->algo_scores[1] = compute_zscore_anomaly(detector, values);
        
        /* DBSCAN */
        detector->algo_scores[2] = compute_dbscan_score(detector, values);
        
        /* Усреднение */
        for (size_t i = 0; i < detector->n_algos; i++) {
            anomaly_score += detector->algo_scores[i];
        }
        anomaly_score /= (float)detector->n_algos;
        
        result->detected_by = ANOMALY_ALGO_ENSEMBLE;
    } else {
        switch (detector->config.algorithm) {
            case ANOMALY_ALGO_ISOLATION_FOREST:
                anomaly_score = compute_isolation_forest_score(detector, values);
                result->detected_by = ANOMALY_ALGO_ISOLATION_FOREST;
                break;
                
            case ANOMALY_ALGO_ZSCORE:
                anomaly_score = compute_zscore_anomaly(detector, values);
                result->detected_by = ANOMALY_ALGO_ZSCORE;
                break;
                
            case ANOMALY_ALGO_DBSCAN:
                anomaly_score = compute_dbscan_score(detector, values);
                result->detected_by = ANOMALY_ALGO_DBSCAN;
                break;
                
            case ANOMALY_ALGO_MOVING_AVG:
                /* Простое сравнение со скользящим средним */
                if (detector->moving_avg_count > 0) {
                    double ma = anomaly_compute_moving_avg(detector->moving_avg_buffer, 
                                                          detector->moving_avg_count);
                    double deviation = fabs(values[0] - ma) / (ma > 0 ? ma : 1.0);
                    anomaly_score = (float)(1.0 - 1.0 / (1.0 + deviation));
                } else {
                    anomaly_score = 0.5f;
                }
                result->detected_by = ANOMALY_ALGO_MOVING_AVG;
                break;
                
            case ANOMALY_ALGO_EXP_SMOOTH:
                if (detector->exp_smooth_initialized) {
                    double deviation = fabs(values[0] - detector->exp_smooth_value);
                    anomaly_score = (float)(1.0 - 1.0 / (1.0 + deviation));
                } else {
                    anomaly_score = 0.5f;
                }
                result->detected_by = ANOMALY_ALGO_EXP_SMOOTH;
                break;
                
            default:
                anomaly_score = 0.5f;
                result->detected_by = ANOMALY_ALGO_ISOLATION_FOREST;
        }
    }
    
    result->anomaly_score = anomaly_score;
    
    /* Определение статуса */
    if (anomaly_score >= detector->config.threshold) {
        if (anomaly_score >= 0.9f) {
            result->status = ANOMALY_STATUS_CRITICAL;
            detector->anomalies_detected++;
        } else if (anomaly_score >= 0.8f) {
            result->status = ANOMALY_STATUS_ANOMALY;
            detector->anomalies_detected++;
        } else {
            result->status = ANOMALY_STATUS_SUSPICIOUS;
        }
    } else {
        result->status = ANOMALY_STATUS_NORMAL;
    }
    
    /* Поиск признака с наибольшей аномалией */
    double max_deviation = 0.0;
    for (size_t f = 0; f < detector->n_features; f++) {
        double zscore = anomaly_compute_zscore(values[f],
                                               detector->feature_stats[f].mean,
                                               detector->feature_stats[f].std_dev);
        if (fabs(zscore) > max_deviation) {
            max_deviation = fabs(zscore);
            result->feature_index = f;
            result->feature_value = values[f];
            result->expected_value = detector->feature_stats[f].mean;
            result->deviation = zscore;
        }
    }
    
    /* Формирование описания */
    snprintf(result->description, sizeof(result->description),
             "Anomaly score: %.3f, Status: %s, Algorithm: %s",
             anomaly_score,
             anomaly_status_to_string(result->status),
             anomaly_algo_to_string(result->detected_by));
    
    /* Обновление последнего результата */
    memcpy(&detector->last_result, result, sizeof(anomaly_result_t));
    
    /* Callback */
    if (result->status >= ANOMALY_STATUS_SUSPICIOUS && 
        detector->on_anomaly_detected) {
        detector->on_anomaly_detected(result, detector->user_data);
    }
    
    unlock_mutex(detector->mutex);
    
    return 0;
}

int anomaly_detector_add_sample(anomaly_detector_t* detector, const double* values) {
    if (!detector || !values) return -1;
    
    lock_mutex(detector->mutex);
    
    /* Добавление в буфер */
    if (detector->n_samples < detector->config.max_samples) {
        memcpy(&detector->training_data[detector->n_samples * detector->n_features],
               values, detector->n_features * sizeof(double));
        detector->n_samples++;
        
        /* Обновление статистики */
        for (size_t f = 0; f < detector->n_features; f++) {
            update_feature_stats(&detector->feature_stats[f], values[f]);
        }
    }
    
    /* Обновление Moving Average */
    if (detector->moving_avg_buffer) {
        detector->moving_avg_sum -= detector->moving_avg_buffer[detector->moving_avg_index];
        detector->moving_avg_buffer[detector->moving_avg_index] = values[0];
        detector->moving_avg_sum += values[0];
        detector->moving_avg_index = (detector->moving_avg_index + 1) % 
                                     detector->config.moving_avg_window;
        
        if (detector->moving_avg_count < detector->config.moving_avg_window) {
            detector->moving_avg_count++;
        }
    }
    
    /* Обновление Exponential Smoothing */
    if (!detector->exp_smooth_initialized) {
        detector->exp_smooth_value = values[0];
        detector->exp_smooth_initialized = true;
    } else {
        detector->exp_smooth_value = anomaly_exp_smooth(
            detector->exp_smooth_value, values[0], detector->config.exp_smooth_alpha);
    }
    
    unlock_mutex(detector->mutex);
    
    return 0;
}

int anomaly_detector_predict_realtime(anomaly_detector_t* detector,
                                      const anomaly_input_t* input,
                                      anomaly_result_t* result) {
    if (!detector || !input || !result) return -1;
    
    /* Предсказание */
    int ret = anomaly_detector_predict(detector, input->values, result);
    
    if (ret == 0 && input->update_model) {
        /* Добавление образца для онлайн-обучения */
        anomaly_detector_add_sample(detector, input->values);
    }
    
    return ret;
}

int anomaly_detector_get_feature_stats(anomaly_detector_t* detector,
                                       size_t feature_index,
                                       anomaly_feature_stats_t* stats) {
    if (!detector || !stats || feature_index >= detector->n_features) return -1;
    
    lock_mutex(detector->mutex);
    memcpy(stats, &detector->feature_stats[feature_index], sizeof(anomaly_feature_stats_t));
    unlock_mutex(detector->mutex);
    
    return 0;
}

int anomaly_detector_set_threshold(anomaly_detector_t* detector, float threshold) {
    if (!detector || threshold < 0.0f || threshold > 1.0f) return -1;
    
    lock_mutex(detector->mutex);
    detector->config.threshold = threshold;
    unlock_mutex(detector->mutex);
    
    return 0;
}

void anomaly_detector_set_callback(anomaly_detector_t* detector,
                                   void (*callback)(const anomaly_result_t*, void*),
                                   void* user_data) {
    if (!detector) return;
    
    lock_mutex(detector->mutex);
    detector->on_anomaly_detected = callback;
    detector->user_data = user_data;
    unlock_mutex(detector->mutex);
}

void anomaly_detector_get_stats(anomaly_detector_t* detector,
                                size_t* total_predictions,
                                size_t* anomalies_detected,
                                float* accuracy) {
    if (!detector) return;
    
    lock_mutex(detector->mutex);
    
    if (total_predictions) *total_predictions = detector->total_predictions;
    if (anomalies_detected) *anomalies_detected = detector->anomalies_detected;
    
    if (accuracy) {
        if (detector->total_predictions > 0) {
            *accuracy = 1.0f - ((float)detector->anomalies_detected / 
                               (float)detector->total_predictions);
        } else {
            *accuracy = 1.0f;
        }
    }
    
    unlock_mutex(detector->mutex);
}

void anomaly_detector_reset_stats(anomaly_detector_t* detector) {
    if (!detector) return;
    
    lock_mutex(detector->mutex);
    detector->total_predictions = 0;
    detector->anomalies_detected = 0;
    detector->false_positives = 0;
    detector->true_positives = 0;
    unlock_mutex(detector->mutex);
}

/* ============================================
 * Функции конвертации в строку
 * ============================================ */

const char* anomaly_status_to_string(anomaly_status_t status) {
    switch (status) {
        case ANOMALY_STATUS_NORMAL: return "Normal";
        case ANOMALY_STATUS_SUSPICIOUS: return "Suspicious";
        case ANOMALY_STATUS_ANOMALY: return "Anomaly";
        case ANOMALY_STATUS_CRITICAL: return "Critical";
        default: return "Unknown";
    }
}

const char* anomaly_algo_to_string(anomaly_algo_t algo) {
    switch (algo) {
        case ANOMALY_ALGO_ISOLATION_FOREST: return "Isolation Forest";
        case ANOMALY_ALGO_ZSCORE: return "Z-Score";
        case ANOMALY_ALGO_DBSCAN: return "DBSCAN";
        case ANOMALY_ALGO_MOVING_AVG: return "Moving Average";
        case ANOMALY_ALGO_EXP_SMOOTH: return "Exponential Smoothing";
        case ANOMALY_ALGO_ENSEMBLE: return "Ensemble";
        default: return "Unknown";
    }
}

const char* anomaly_metric_to_string(anomaly_metric_t metric) {
    switch (metric) {
        case ANOMALY_METRIC_CONNECTIONS: return "Connections";
        case ANOMALY_METRIC_REQUESTS_PER_SEC: return "Requests/sec";
        case ANOMALY_METRIC_BYTES_IN: return "Bytes In";
        case ANOMALY_METRIC_BYTES_OUT: return "Bytes Out";
        case ANOMALY_METRIC_LATЕНСИ: return "Latency";
        case ANOMALY_METRIC_CPU_USAGE: return "CPU Usage";
        case ANOMALY_METRIC_MEMORY_USAGE: return "Memory Usage";
        case ANOMALY_METRIC_ERROR_RATE: return "Error Rate";
        case ANOMALY_METRIC_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

/* ============================================
 * JSON экспорт/импорт (упрощенная реализация)
 * ============================================ */

int anomaly_detector_export_json(anomaly_detector_t* detector, char* buffer, size_t buffer_size) {
    if (!detector || !buffer || buffer_size == 0) return -1;
    
    int written = 0;
    
    written += snprintf(buffer + written, buffer_size - written, "{\n");
    written += snprintf(buffer + written, buffer_size - written,
        "  \"config\": {\n");
    written += snprintf(buffer + written, buffer_size - written,
        "    \"algorithm\": %d,\n", (int)detector->config.algorithm);
    written += snprintf(buffer + written, buffer_size - written,
        "    \"threshold\": %.3f,\n", detector->config.threshold);
    written += snprintf(buffer + written, buffer_size - written,
        "    \"n_features\": %zu,\n", detector->n_features);
    written += snprintf(buffer + written, buffer_size - written,
        "    \"n_trees\": %zu,\n", detector->config.n_trees);
    written += snprintf(buffer + written, buffer_size - written,
        "    \"zscore_threshold\": %.2f\n", detector->config.zscore_threshold);
    written += snprintf(buffer + written, buffer_size - written,
        "  },\n");
    
    written += snprintf(buffer + written, buffer_size - written,
        "  \"stats\": {\n");
    written += snprintf(buffer + written, buffer_size - written,
        "    \"total_predictions\": %zu,\n", detector->total_predictions);
    written += snprintf(buffer + written, buffer_size - written,
        "    \"anomalies_detected\": %zu\n", detector->anomalies_detected);
    written += snprintf(buffer + written, buffer_size - written,
        "  }\n");
    
    written += snprintf(buffer + written, buffer_size - written, "}\n");
    
    return written;
}

int anomaly_detector_import_json(anomaly_detector_t* detector, const char* json) {
    /* Упрощенная заглушка - полная реализация требует JSON парсера */
    if (!detector || !json) return -1;
    
    /* В реальной реализации здесь будет парсинг JSON */
    /* и восстановление состояния детектора */
    
    return 0;
}
