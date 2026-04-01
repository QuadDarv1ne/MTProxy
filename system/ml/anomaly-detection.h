/**
 * @file anomaly-detection.h
 * @brief Система ML-детекции аномалий в сетевом трафике
 * 
 * Поддерживаемые алгоритмы:
 * - Isolation Forest: обнаружение аномалий через изоляцию точек
 * - Z-Score: статистический метод обнаружения выбросов
 * - DBSCAN: кластеризация для обнаружения аномалий
 * - Moving Average: скользящее среднее для временных рядов
 * - Exponential Smoothing: экспоненциальное сглаживание
 * 
 * @version 1.0.32
 * @date 29 марта 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#ifndef ANOMALY_DETECTION_H
#define ANOMALY_DETECTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * Конфигурация и константы
 * ============================================ */

/** Максимальное количество признаков для анализа */
#define ANOMALY_MAX_FEATURES        32

/** Максимальный размер выборки для обучения */
#define ANOMALY_MAX_SAMPLES         10000

/** Максимальное количество деревьев в Isolation Forest */
#define ANOMALY_MAX_TREES           100

/** Максимальная высота дерева */
#define ANOMALY_MAX_TREE_HEIGHT     20

/** Максимальное количество кластеров в DBSCAN */
#define ANOMALY_MAX_CLUSTERS        64

/** Порог аномалии по умолчанию (0.0 - 1.0) */
#define ANOMALY_DEFAULT_THRESHOLD   0.6f

/** Минимальный порог Z-Score */
#define ANOMALY_ZSCORE_MIN_THRESHOLD    2.0f

/** Максимальный порог Z-Score */
#define ANOMALY_ZSCORE_MAX_THRESHOLD    4.0f

/** Размер окна для скользящего среднего */
#define ANOMALY_MOVING_AVG_WINDOW     60

/** Коэффициент сглаживания для экспоненциального сглаживания */
#define ANOMALY_EXP_SMOOTH_ALPHA      0.3f

/* ============================================
 * Типы данных
 * ============================================ */

/**
 * @brief Типы алгоритмов обнаружения аномалий
 */
typedef enum {
    ANOMALY_ALGO_ISOLATION_FOREST = 0,    /**< Isolation Forest */
    ANOMALY_ALGO_ZSCORE = 1,              /**< Z-Score статистический метод */
    ANOMALY_ALGO_DBSCAN = 2,              /**< DBSCAN кластеризация */
    ANOMALY_ALGO_MOVING_AVG = 3,          /**< Скользящее среднее */
    ANOMALY_ALGO_EXP_SMOOTH = 4,          /**< Экспоненциальное сглаживание */
    ANOMALY_ALGO_ENSEMBLE = 5             /**< Ансамбль алгоритмов */
} anomaly_algo_t;

/**
 * @brief Статус аномалии
 */
typedef enum {
    ANOMALY_STATUS_NORMAL = 0,            /**< Нормальное значение */
    ANOMALY_STATUS_SUSPICIOUS = 1,        /**< Подозрительное значение */
    ANOMALY_STATUS_ANOMALY = 2,           /**< Аномалия обнаружена */
    ANOMALY_STATUS_CRITICAL = 3           /**< Критическая аномалия */
} anomaly_status_t;

/**
 * @brief Типы метрик для анализа
 */
typedef enum {
    ANOMALY_METRIC_CONNECTIONS = 0,       /**< Количество подключений */
    ANOMALY_METRIC_REQUESTS_PER_SEC = 1,  /**< Запросов в секунду */
    ANOMALY_METRIC_BYTES_IN = 2,          /**< Входной трафик (байты) */
    ANOMALY_METRIC_BYTES_OUT = 3,         /**< Выходной трафик (байты) */
    ANOMALY_METRIC_LATЕНСИ = 4,           /**< Латентность (мс) */
    ANOMALY_METRIC_CPU_USAGE = 5,         /**< Использование CPU (%) */
    ANOMALY_METRIC_MEMORY_USAGE = 6,      /**< Использование памяти (%) */
    ANOMALY_METRIC_ERROR_RATE = 7,        /**< Частота ошибок */
    ANOMALY_METRIC_CUSTOM = 8             /**< Пользовательская метрика */
} anomaly_metric_t;

/**
 * @brief Конфигурация детектора аномалий
 */
typedef struct {
    anomaly_algo_t algorithm;             /**< Алгоритм обнаружения */
    float threshold;                      /**< Порог обнаружения (0.0 - 1.0) */
    size_t max_samples;                   /**< Максимальное количество образцов */
    size_t n_features;                    /**< Количество признаков */
    size_t n_trees;                       /**< Количество деревьев (Isolation Forest) */
    size_t tree_height;                   /**< Высота дерева */
    float zscore_threshold;               /**< Порог Z-Score */
    size_t moving_avg_window;             /**< Размер окна скользящего среднего */
    float exp_smooth_alpha;               /**< Коэффициент экспоненциального сглаживания */
    bool enable_ensemble;                 /**< Включить ансамбль алгоритмов */
    bool enable_adaptive_threshold;       /**< Адаптивный порог */
    bool enable_real_time;                /**< Режим реального времени */
} anomaly_config_t;

/**
 * @brief Статистика по признаку
 */
typedef struct {
    double mean;                          /**< Среднее значение */
    double std_dev;                       /**< Стандартное отклонение */
    double min;                           /**< Минимальное значение */
    double max;                           /**< Максимальное значение */
    double q1;                            /**< Первый квартиль (25%) */
    double q3;                            /**< Третий квартиль (75%) */
    double iqr;                           /**< Интерквартильный размах */
    size_t count;                         /**< Количество образцов */
    double sum;                           /**< Сумма значений */
    double sum_squares;                   /**< Сумма квадратов */
} anomaly_feature_stats_t;

/**
 * @brief Данные признака
 */
typedef struct {
    double value;                         /**< Текущее значение */
    double normalized;                    /**< Нормализованное значение */
    double zscore;                        /**< Z-Score значение */
    bool is_outlier;                      /**< Является ли выбросом */
    uint64_t timestamp;                   /**< Временная метка */
} anomaly_feature_data_t;

/**
 * @brief Результат обнаружения аномалии
 */
typedef struct {
    anomaly_status_t status;              /**< Статус аномалии */
    float anomaly_score;                  /**< Оценка аномалии (0.0 - 1.0) */
    anomaly_algo_t detected_by;           /**< Алгоритм, обнаруживший аномалию */
    size_t feature_index;                 /**< Индекс признака с аномалией */
    double feature_value;                 /**< Значение признака */
    double expected_value;                /**< Ожидаемое значение */
    double deviation;                     /**< Отклонение от нормы */
    uint64_t timestamp;                   /**< Временная метка обнаружения */
    char description[256];                /**< Описание аномалии */
} anomaly_result_t;

/**
 * @brief Узел дерева изоляции
 */
typedef struct anomaly_tree_node {
    size_t feature_index;                 /**< Индекс признака для разделения */
    double split_value;                   /**< Значение разделения */
    struct anomaly_tree_node* left;       /**< Левый потомок */
    struct anomaly_tree_node* right;      /**< Правый потомок */
    size_t size;                          /**< Размер узла (количество точек) */
    size_t depth;                         /**< Глубина узла */
} anomaly_tree_node_t;

/**
 * @brief Дерево изоляции
 */
typedef struct {
    anomaly_tree_node_t* root;            /**< Корневой узел */
    size_t height;                        /**< Высота дерева */
    size_t n_features;                    /**< Количество признаков */
} anomaly_tree_t;

/**
 * @brief Кластер DBSCAN
 */
typedef struct {
    size_t id;                            /**< ID кластера */
    size_t* point_indices;                /**< Индексы точек в кластере */
    size_t point_count;                   /**< Количество точек */
    size_t capacity;                      /**< Вместимость массива */
    double centroid[ANOMALY_MAX_FEATURES];/**< Центроид кластера */
    bool is_noise;                        /**< Является ли шумом */
} anomaly_cluster_t;

/**
 * @brief Основной контекст детектора аномалий
 */
typedef struct {
    anomaly_config_t config;              /**< Конфигурация */
    
    /* Данные для обучения */
    double* training_data;                /**< Обучающие данные [n_samples][n_features] */
    size_t n_samples;                     /**< Текущее количество образцов */
    size_t n_features;                    /**< Количество признаков */
    
    /* Статистика по признакам */
    anomaly_feature_stats_t* feature_stats; /**< Статистика по признакам */
    
    /* Isolation Forest */
    anomaly_tree_t* trees;                /**< Массив деревьев */
    size_t n_trees;                       /**< Количество деревьев */
    
    /* DBSCAN */
    anomaly_cluster_t* clusters;          /**< Массив кластеров */
    size_t n_clusters;                    /**< Количество кластеров */
    int* cluster_labels;                  /**< Метки кластеров для каждой точки */
    
    /* Moving Average */
    double* moving_avg_buffer;            /**< Буфер для скользящего среднего */
    size_t moving_avg_index;              /**< Текущий индекс в буфере */
    size_t moving_avg_count;              /**< Количество элементов в буфере */
    double moving_avg_sum;                /**< Сумма для скользящего среднего */
    
    /* Exponential Smoothing */
    double exp_smooth_value;              /**< Текущее значение экспоненциального сглаживания */
    bool exp_smooth_initialized;          /**< Инициализировано ли */
    
    /* Ensemble */
    float* algo_scores;                   /**< Оценки от каждого алгоритма */
    size_t n_algos;                       /**< Количество алгоритмов в ансамбле */
    
    /* Кэширование */
    anomaly_result_t last_result;         /**< Последний результат */
    uint64_t last_update_time;            /**< Время последнего обновления */
    
    /* Статистика */
    size_t total_predictions;             /**< Всего предсказаний */
    size_t anomalies_detected;            /**< Обнаружено аномалий */
    size_t false_positives;               /**< Ложные срабатывания */
    size_t true_positives;                /**< Истинные срабатывания */
    
    /* Callback для уведомлений */
    void (*on_anomaly_detected)(const anomaly_result_t* result, void* user_data);
    void* user_data;                      /**< Пользовательские данные для callback */
    
    /* Блокировка для потокобезопасности */
    void* mutex;                          /**< Мьютекс */
} anomaly_detector_t;

/**
 * @brief Параметры для детектирования в реальном времени
 */
typedef struct {
    double values[ANOMALY_MAX_FEATURES];  /**< Текущие значения признаков */
    uint64_t timestamp;                   /**< Временная метка */
    bool update_model;                    /**< Обновлять ли модель */
} anomaly_input_t;

/* ============================================
 * Основные функции API
 * ============================================ */

/**
 * @brief Инициализация детектора аномалий
 * 
 * @param detector Указатель на детектор
 * @param config Конфигурация детектора
 * @return 0 при успехе, -1 при ошибке
 */
int anomaly_detector_init(anomaly_detector_t* detector, const anomaly_config_t* config);

/**
 * @brief Очистка детектора аномалий
 * 
 * @param detector Указатель на детектор
 */
void anomaly_detector_cleanup(anomaly_detector_t* detector);

/**
 * @brief Обучение детектора на исторических данных
 * 
 * @param detector Указатель на детектор
 * @param data Обучающие данные [n_samples][n_features]
 * @param n_samples Количество образцов
 * @param n_features Количество признаков
 * @return 0 при успехе, -1 при ошибке
 */
int anomaly_detector_train(anomaly_detector_t* detector, const double* data, 
                          size_t n_samples, size_t n_features);

/**
 * @brief Предсказание аномалии для одного образца
 * 
 * @param detector Указатель на детектор
 * @param values Значения признаков [n_features]
 * @param result Результат предсказания
 * @return 0 при успехе, -1 при ошибке
 */
int anomaly_detector_predict(anomaly_detector_t* detector, const double* values, 
                            anomaly_result_t* result);

/**
 * @brief Предсказание аномалии в реальном времени
 * 
 * @param detector Указатель на детектор
 * @param input Входные данные
 * @param result Результат предсказания
 * @return 0 при успехе, -1 при ошибке
 */
int anomaly_detector_predict_realtime(anomaly_detector_t* detector, 
                                      const anomaly_input_t* input,
                                      anomaly_result_t* result);

/**
 * @brief Добавление нового образца для онлайн-обучения
 * 
 * @param detector Указатель на детектор
 * @param values Значения признаков [n_features]
 * @return 0 при успехе, -1 при ошибке
 */
int anomaly_detector_add_sample(anomaly_detector_t* detector, const double* values);

/**
 * @brief Получение статистики по признакам
 * 
 * @param detector Указатель на детектор
 * @param feature_index Индекс признака
 * @param stats Указатель на структуру статистики
 * @return 0 при успехе, -1 при ошибке
 */
int anomaly_detector_get_feature_stats(anomaly_detector_t* detector, 
                                       size_t feature_index,
                                       anomaly_feature_stats_t* stats);

/**
 * @brief Установка порога обнаружения аномалий
 * 
 * @param detector Указатель на детектор
 * @param threshold Порог (0.0 - 1.0)
 * @return 0 при успехе, -1 при ошибке
 */
int anomaly_detector_set_threshold(anomaly_detector_t* detector, float threshold);

/**
 * @brief Установка callback функции для уведомлений об аномалиях
 * 
 * @param detector Указатель на детектор
 * @param callback Callback функция
 * @param user_data Пользовательские данные
 */
void anomaly_detector_set_callback(anomaly_detector_t* detector,
                                   void (*callback)(const anomaly_result_t*, void*),
                                   void* user_data);

/**
 * @brief Получение статистики детектора
 * 
 * @param detector Указатель на детектор
 * @param total_predictions Всего предсказаний
 * @param anomalies_detected Обнаружено аномалий
 * @param accuracy Точность (если доступна)
 */
void anomaly_detector_get_stats(anomaly_detector_t* detector, 
                               size_t* total_predictions,
                               size_t* anomalies_detected,
                               float* accuracy);

/**
 * @brief Сброс статистики детектора
 * 
 * @param detector Указатель на детектор
 */
void anomaly_detector_reset_stats(anomaly_detector_t* detector);

/**
 * @brief Экспорт модели в JSON формат
 * 
 * @param detector Указатель на детектор
 * @param buffer Буфер для JSON
 * @param buffer_size Размер буфера
 * @return Количество записанных байт, -1 при ошибке
 */
int anomaly_detector_export_json(anomaly_detector_t* detector, char* buffer, size_t buffer_size);

/**
 * @brief Импорт модели из JSON формата
 * 
 * @param detector Указатель на детектор
 * @param json JSON данные
 * @return 0 при успехе, -1 при ошибке
 */
int anomaly_detector_import_json(anomaly_detector_t* detector, const char* json);

/**
 * @brief Получение описания статуса аномалии
 * 
 * @param status Статус аномалии
 * @return Строковое описание
 */
const char* anomaly_status_to_string(anomaly_status_t status);

/**
 * @brief Получение названия алгоритма
 * 
 * @param algo Алгоритм
 * @return Строковое название
 */
const char* anomaly_algo_to_string(anomaly_algo_t algo);

/**
 * @brief Получение названия метрики
 * 
 * @param metric Метрика
 * @return Строковое название
 */
const char* anomaly_metric_to_string(anomaly_metric_t metric);

/* ============================================
 * Вспомогательные функции
 * ============================================ */

/**
 * @brief Вычисление среднего значения
 * 
 * @param data Массив данных
 * @param count Количество элементов
 * @return Среднее значение
 */
double anomaly_compute_mean(const double* data, size_t count);

/**
 * @brief Вычисление стандартного отклонения
 * 
 * @param data Массив данных
 * @param count Количество элементов
 * @param mean Среднее значение
 * @return Стандартное отклонение
 */
double anomaly_compute_std_dev(const double* data, size_t count, double mean);

/**
 * @brief Вычисление Z-Score
 * 
 * @param value Значение
 * @param mean Среднее значение
 * @param std_dev Стандартное отклонение
 * @return Z-Score
 */
double anomaly_compute_zscore(double value, double mean, double std_dev);

/**
 * @brief Нормализация значения (Min-Max scaling)
 * 
 * @param value Значение
 * @param min Минимальное значение
 * @param max Максимальное значение
 * @return Нормализованное значение (0.0 - 1.0)
 */
double anomaly_normalize_value(double value, double min, double max);

/**
 * @brief Вычисление скользящего среднего
 * 
 * @param buffer Буфер значений
 * @param count Количество элементов
 * @return Скользящее среднее
 */
double anomaly_compute_moving_avg(const double* buffer, size_t count);

/**
 * @brief Экспоненциальное сглаживание
 * 
 * @param prev_value Предыдущее сглаженное значение
 * @param current_value Текущее значение
 * @param alpha Коэффициент сглаживания (0.0 - 1.0)
 * @return Сглаженное значение
 */
double anomaly_exp_smooth(double prev_value, double current_value, float alpha);

#ifdef __cplusplus
}
#endif

#endif /* ANOMALY_DETECTION_H */
