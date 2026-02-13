#ifndef PERF_MONITOR_H
#define PERF_MONITOR_H

#include <stdint.h>

/*
 * Структура для хранения метрик производительности
 */
typedef struct perf_metrics {
    // Временные метрики
    double avg_response_time;      // Среднее время ответа (мс)
    double max_response_time;      // Максимальное время ответа (мс)
    double min_response_time;      // Минимальное время ответа (мс)
    
    // Метрики пропускной способности
    uint64_t requests_per_second;  // Запросов в секунду
    uint64_t bytes_sent;          // Отправлено байт
    uint64_t bytes_received;      // Получено байт
    
    // Метрики использования ресурсов
    double cpu_usage;             // Использование CPU (%)
    uint64_t memory_used;         // Использовано памяти (байт)
    uint64_t memory_allocated;    // Всего выделено памяти (байт)
    
    // Метрики ошибок
    uint64_t error_count;         // Количество ошибок
    uint64_t timeout_count;       // Количество таймаутов
    
    // Метрики безопасности
    uint64_t blocked_connections; // Заблокированные соединения (DDoS)
    uint64_t suspicious_activities; // Подозрительная активность
    
    // Временные метки
    long last_update;           // Время последнего обновления
    long start_time;            // Время начала мониторинга
} perf_metrics_t;

/*
 * Инициализирует систему мониторинга производительности
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int init_perf_monitor();

/*
 * Обновляет метрики производительности
 * @param metrics: указатель на структуру метрик для обновления
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int update_perf_metrics(perf_metrics_t *metrics);

/*
 * Получает текущие метрики производительности
 * @return: указатель на структуру метрик или NULL в случае ошибки
 */
perf_metrics_t *get_current_metrics();

/*
 * Сбрасывает все метрики к начальным значениям
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int reset_perf_metrics();

/*
 * Выводит отчет о производительности в указанный поток
 * @param file_desc: дескриптор файла или потока для вывода
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int print_performance_report(void *file_desc);

/*
 * Деинициализирует систему мониторинга производительности
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int deinit_perf_monitor();

#endif