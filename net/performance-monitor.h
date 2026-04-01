/**
 * @file performance-monitor.h
 * @brief Система мониторинга производительности с ML-аналитикой
 *
 * @version 1.0.33
 * @date 1 апреля 2026
 * @copyright Copyright (c) 2026 MTProxy
 */

#ifndef __PERFORMANCE_MONITOR_H__
#define __PERFORMANCE_MONITOR_H__

#include <stdint.h>
#include <stddef.h>
#include "../system/ml/anomaly-detection.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
 * Конфигурация и константы
 * ============================================ */

/** Максимальное количество метрик в истории */
#define PERF_MAX_HISTORY 1000

/** Количество метрик для ML анализа */
#define PERF_NUM_METRICS 5

/** Структура метрик производительности */
typedef struct {
    uint64_t timestamp;           /**< Временная метка (ms) */
    double cpu_percent;           /**< Использование CPU (%) */
    uint64_t memory_bytes;        /**< Использование памяти (bytes) */
    double avg_latency_ms;        /**< Средняя задержка (ms) */
    double max_latency_ms;        /**< Максимальная задержка (ms) */
    uint64_t connections_active;  /**< Активные соединения */
    uint64_t connections_total;   /**< Всего соединений */
    uint64_t requests_per_second; /**< Запросов в секунду */
    uint64_t bytes_sent;          /**< Байт отправлено */
    uint64_t bytes_received;      /**< Байт получено */
    uint64_t errors_total;        /**< Всего ошибок */
    uint64_t cache_hits;          /**< Попаданий в кэш */
    uint64_t cache_misses;        /**< Промахов кэша */
} perf_metrics_t;

/** Конфигурация монитора производительности */
typedef struct {
    uint32_t sample_interval_ms;   /**< Интервал сбора метрик (ms) */
    int enable_ml_detection;       /**< Включить ML детекцию аномалий */
    double cpu_threshold;          /**< Порог CPU (%) */
    double memory_threshold;       /**< Порог памяти (%) */
    uint32_t latency_threshold_ms; /**< Порог задержки (ms) */
} perf_monitor_config_t;

/** Статистика монитора */
typedef struct {
    uint64_t total_samples;        /**< Всего собрано метрик */
    double avg_cpu_usage;          /**< Среднее CPU (%) */
    uint64_t avg_memory_usage;     /**< Средняя память (bytes) */
    double avg_latency_ms;         /**< Средняя задержка (ms) */
    double last_cpu_usage;         /**< Последнее CPU (%) */
    uint64_t last_memory_usage;    /**< Последняя память (bytes) */
    double last_latency_ms;        /**< Последняя задержка (ms) */
    uint64_t cpu_threshold_exceeded;    /**< Превышений CPU порога */
    uint64_t memory_threshold_exceeded; /**< Превышений памяти порога */
    uint64_t latency_threshold_exceeded;/**< Превышений задержки порога */
    uint64_t anomalies_detected;   /**< Обнаружено аномалий ML */
} perf_stats_t;

/** Контекст монитора производительности */
typedef struct {
    perf_metrics_t metrics_history[PERF_MAX_HISTORY]; /**< История метрик */
    uint64_t metrics_timestamps[PERF_MAX_HISTORY];    /**< Временные метки */
    int metrics_count;              /**< Количество метрик в истории */
    uint64_t total_samples;         /**< Всего собрано метрик */
    uint64_t start_time;            /**< Время запуска (ms) */
    
    perf_monitor_config_t config;   /**< Конфигурация */
    perf_stats_t stats;             /**< Статистика */
    
    /* ML детектор аномалий */
    struct anomaly_detector ml_detector;
    uint64_t last_anomaly_time;     /**< Время последней аномалии */
    float last_anomaly_score;       /**< Последний score аномалии */
    
    /* Синхронизация */
#ifdef _WIN32
    void *lock;                     /**< CRITICAL_SECTION */
    void *rwlock;                   /**< SRWLOCK */
#else
    pthread_mutex_t lock;           /**< Мьютекс */
    pthread_rwlock_t rwlock;        /**< Read-write lock */
#endif
    
    int initialized;                /**< Флаг инициализации */
} perf_monitor_ctx_t;

/* ============================================
 * Основные функции
 * ============================================ */

/**
 * @brief Инициализация монитора производительности
 *
 * @param ctx Контекст монитора (должен быть выделен вызывающим)
 * @param config Конфигурация (NULL для значений по умолчанию)
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int perf_monitor_init(perf_monitor_ctx_t *ctx, perf_monitor_config_t *config);

/**
 * @brief Запись метрик производительности
 *
 * @param ctx Контекст монитора
 * @param metrics Метрики для записи
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int perf_monitor_record(perf_monitor_ctx_t *ctx, perf_metrics_t *metrics);

/**
 * @brief Сбор текущих метрик системы
 *
 * @param ctx Контекст монитора
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int perf_monitor_sample(perf_monitor_ctx_t *ctx);

/**
 * @brief Получение статистики монитора
 *
 * @param ctx Контекст монитора
 * @param stats Структура для хранения статистики
 * @return 0 при успехе, отрицательное значение при ошибке
 */
int perf_monitor_get_stats(perf_monitor_ctx_t *ctx, perf_stats_t *stats);

/**
 * @brief Экспорт статистики в JSON формате
 *
 * @param ctx Контекст монитора
 * @param buffer Буфер для вывода
 * @param buffer_size Размер буфера
 * @return Количество записанных байт, отрицательное значение при ошибке
 */
int perf_monitor_export_json(perf_monitor_ctx_t *ctx, char *buffer, size_t buffer_size);

/**
 * @brief Очистка ресурсов монитора
 *
 * @param ctx Контекст монитора
 */
void perf_monitor_cleanup(perf_monitor_ctx_t *ctx);

/**
 * @brief Вывод статистики в stdout
 *
 * @param ctx Контекст монитора
 */
void perf_monitor_print_stats(perf_monitor_ctx_t *ctx);

/* ============================================
 * Inline функции для производительности
 * ============================================ */

/**
 * @brief Проверка необходимости сбора метрик (inline)
 *
 * @param ctx Контекст монитора
 * @param last_sample_time Время последнего сбора (ms)
 * @return 1 если пора собирать метрики, 0 иначе
 */
static inline int perf_monitor_should_sample(perf_monitor_ctx_t *ctx, uint64_t last_sample_time) {
    if (!ctx || !ctx->initialized) return 0;
    
    uint64_t now = 0;
#ifdef _WIN32
    now = GetTickCount64();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    now = (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
    
    return (now - last_sample_time) >= ctx->config.sample_interval_ms;
}

/**
 * @brief Проверка наличия аномалии (inline)
 *
 * @param ctx Контекст монитора
 * @return 1 если обнаружена аномалия, 0 иначе
 */
static inline int perf_monitor_has_anomaly(perf_monitor_ctx_t *ctx) {
    if (!ctx || !ctx->initialized || !ctx->config.enable_ml_detection) return 0;
    return ctx->stats.anomalies_detected > 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __PERFORMANCE_MONITOR_H__ */
