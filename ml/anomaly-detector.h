/*
 * Детектор аномалий на основе машинного обучения для MTProxy
 * Обнаружение подозрительной активности и адаптивная обфускация
 */

#ifndef _ANOMALY_DETECTOR_H_
#define _ANOMALY_DETECTOR_H_

#include <stdint.h>

// Типы аномалий
typedef enum {
    ANOMALY_TYPE_NONE = 0,
    ANOMALY_TYPE_TRAFFIC_SPIKE = 1,
    ANOMALY_TYPE_PATTERN_CHANGE = 2,
    ANOMALY_TYPE_SIZE_ANOMALY = 3,
    ANOMALY_TYPE_TIMING_ANOMALY = 4,
    ANOMALY_TYPE_BEHAVIORAL_CHANGE = 5,
    ANOMALY_TYPE_DDOS_PATTERN = 6
} anomaly_type_t;

// Статус детектора
typedef enum {
    ANOMALY_DETECTOR_STATUS_UNINITIALIZED = 0,
    ANOMALY_DETECTOR_STATUS_INITIALIZED = 1,
    ANOMALY_DETECTOR_STATUS_TRAINING = 2,
    ANOMALY_DETECTOR_STATUS_ACTIVE = 3,
    ANOMALY_DETECTOR_STATUS_ERROR = 4
} anomaly_detector_status_t;

// Статистика детектора аномалий
typedef struct {
    long long total_analyses;
    long long anomalies_detected;
    long long false_positives;
    long long true_positives;
    long long model_updates;
    long long training_samples;
    anomaly_detector_status_t current_status;
    int detection_accuracy_percent;
    int model_confidence;
} anomaly_detector_stats_t;

// Конфигурация детектора аномалий
typedef struct {
    int enable_anomaly_detection;
    int enable_adaptive_obfuscation;
    int sensitivity_level;  // 1-10, где 10 - максимальная чувствительность
    int training_window_minutes;
    int detection_threshold;
    int auto_update_model;
    int enable_logging;
    int max_training_samples;
    int min_confidence_threshold;
} anomaly_detector_config_t;

// Контекст детектора аномалий
typedef struct {
    anomaly_detector_config_t config;
    anomaly_detector_stats_t stats;
    anomaly_detector_status_t status;
    void *ml_model;  // Указатель на модель машинного обучения
    void *feature_extractor;
    void *normal_behavior_profile;
    int model_trained;
    int features_extracted;
    long long last_training_time;
    int current_confidence;
} anomaly_detector_context_t;

// Структура для данных трафика
typedef struct {
    long long timestamp;
    int packet_size;
    int inter_arrival_time;
    int connection_count;
    int data_rate;
    int protocol_type;
    int source_port;
    int destination_port;
    char source_ip[46];
    char destination_ip[46];
    int flags;
    int payload_entropy;
} traffic_data_t;

// Структура для результатов анализа
typedef struct {
    anomaly_type_t detected_anomaly;
    int confidence_level;  // 0-100
    int severity_level;    // 1-10
    long long detection_time;
    char description[256];
    int recommended_action;
    traffic_data_t *anomalous_data;
    int data_count;
} anomaly_analysis_result_t;

// Функции инициализации
int anomaly_detector_init(anomaly_detector_context_t *ctx);
int anomaly_detector_init_with_config(anomaly_detector_context_t *ctx, 
                                     const anomaly_detector_config_t *config);
void anomaly_detector_cleanup(anomaly_detector_context_t *ctx);

// Функции обучения модели
int anomaly_detector_train_model(anomaly_detector_context_t *ctx, 
                                const traffic_data_t *training_data, 
                                int sample_count);
int anomaly_detector_update_model(anomaly_detector_context_t *ctx, 
                                 const traffic_data_t *new_data, 
                                 int sample_count);
int anomaly_detector_reset_model(anomaly_detector_context_t *ctx);

// Функции анализа трафика
int anomaly_detector_analyze_traffic(anomaly_detector_context_t *ctx, 
                                    const traffic_data_t *traffic_data, 
                                    int data_count,
                                    anomaly_analysis_result_t *results, 
                                    int max_results);
int anomaly_detector_analyze_single_packet(anomaly_detector_context_t *ctx, 
                                          const traffic_data_t *packet_data,
                                          anomaly_analysis_result_t *result);

// Функции адаптивной обфускации
int anomaly_detector_get_adaptive_obfuscation_level(anomaly_detector_context_t *ctx);
int anomaly_detector_recommend_obfuscation_strategy(anomaly_detector_context_t *ctx, 
                                                   int *strategy_flags);
int anomaly_detector_apply_adaptive_changes(anomaly_detector_context_t *ctx);

// Функции управления
int anomaly_detector_enable_detection(anomaly_detector_context_t *ctx);
int anomaly_detector_disable_detection(anomaly_detector_context_t *ctx);
int anomaly_detector_set_sensitivity(anomaly_detector_context_t *ctx, int sensitivity);
int anomaly_detector_get_current_threat_level(anomaly_detector_context_t *ctx);

// Функции статистики
anomaly_detector_stats_t anomaly_detector_get_stats(anomaly_detector_context_t *ctx);
void anomaly_detector_reset_stats(anomaly_detector_context_t *ctx);

// Функции конфигурации
void anomaly_detector_get_config(anomaly_detector_context_t *ctx, 
                                anomaly_detector_config_t *config);
int anomaly_detector_update_config(anomaly_detector_context_t *ctx, 
                                  const anomaly_detector_config_t *new_config);

// Вспомогательные функции
int anomaly_detector_is_available(void);
int anomaly_detector_get_model_age(anomaly_detector_context_t *ctx);
const char* anomaly_detector_get_anomaly_string(anomaly_type_t anomaly_type);
int anomaly_detector_export_model(anomaly_detector_context_t *ctx, const char *filename);
int anomaly_detector_import_model(anomaly_detector_context_t *ctx, const char *filename);

#endif