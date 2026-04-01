/*
    MTProxy Distributed Tracing
    Система трассировки запросов между узлами кластера
    
    Поддерживаемые функции:
    - Генерация trace ID
    - Span контекст
    - Propagation между узлами
    - Сбор метрик производительности
    - Экспорт trace данных
    
    Примеры использования:
    tracing_init("mtproxy-cluster");
    tracing_set_sampling_rate(0.1);  // 10% запросов
    trace_context_t *ctx = tracing_start_trace("request");
    tracing_add_attribute(ctx, "client_ip", "192.168.1.100");
    span_t *span = tracing_start_span(ctx, "process_request");
    // ... обработка запроса ...
    tracing_end_span(span);
    tracing_end_trace(ctx);
    tracing_cleanup();
*/

#ifndef DISTRIBUTED_TRACING_H
#define DISTRIBUTED_TRACING_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Версия Distributed Tracing
#define TRACING_VERSION "1.0.0"

// Максимальные размеры
#define TRACING_MAX_TRACE_ID_LEN 32
#define TRACING_MAX_SPAN_ID_LEN 16
#define TRACING_MAX_SPAN_NAME_LEN 128
#define TRACING_MAX_ATTRIBUTES 32
#define TRACING_MAX_ATTRIBUTE_VALUE_LEN 256
#define TRACING_MAX_EVENTS 64
#define TRACING_MAX_SPANS_PER_TRACE 256

// Типы span
typedef enum {
    TRACING_SPAN_SERVER = 0,
    TRACING_SPAN_CLIENT = 1,
    TRACING_SPAN_PRODUCER = 2,
    TRACING_SPAN_CONSUMER = 3,
    TRACING_SPAN_INTERNAL = 4
} tracing_span_kind_t;

// Статус span
typedef enum {
    TRACING_STATUS_UNSET = 0,
    TRACING_STATUS_OK = 1,
    TRACING_STATUS_ERROR = 2
} tracing_status_t;

// Trace ID
typedef struct {
    char id[TRACING_MAX_TRACE_ID_LEN + 1];
    int64_t start_time;
    int64_t end_time;
    char root_span_id[TRACING_MAX_SPAN_ID_LEN + 1];
    bool sampled;
    bool completed;
} trace_context_t;

// Span
typedef struct {
    char span_id[TRACING_MAX_SPAN_ID_LEN + 1];
    char parent_span_id[TRACING_MAX_SPAN_ID_LEN + 1];
    char trace_id[TRACING_MAX_TRACE_ID_LEN + 1];
    char name[TRACING_MAX_SPAN_NAME_LEN];
    tracing_span_kind_t kind;
    tracing_status_t status;
    int64_t start_time;
    int64_t end_time;
    int64_t duration_ns;
    
    // Атрибуты
    char *attribute_keys[TRACING_MAX_ATTRIBUTES];
    char *attribute_values[TRACING_MAX_ATTRIBUTES];
    int attribute_count;
    
    // События
    char *events[TRACING_MAX_EVENTS];
    int64_t event_timestamps[TRACING_MAX_EVENTS];
    int event_count;
    
    trace_context_t *trace;
} span_t;

// Конфигурация tracing
typedef struct {
    char service_name[64];
    double sampling_rate;  // 0.0 - 1.0
    bool enabled;
    int max_spans_per_trace;
    int export_interval_sec;
    char export_endpoint[256];
} tracing_config_t;

// Статистика tracing
typedef struct {
    int64_t start_time;
    uint64_t total_traces;
    uint64_t sampled_traces;
    uint64_t completed_traces;
    uint64_t total_spans;
    uint64_t active_spans;
    uint64_t dropped_spans;
    uint64_t exported_traces;
    double avg_trace_duration_ms;
    double avg_span_duration_ms;
} tracing_stats_t;

// Callback функции
typedef void (*tracing_export_callback_t)(const trace_context_t *trace, span_t **spans, int span_count);
typedef void (*tracing_span_end_callback_t)(const span_t *span);

// ============================================================================
// Инициализация и очистка
// ============================================================================

/**
 * Инициализация системы tracing
 * @param service_name Имя сервиса
 * @return 0 при successo, -1 при ошибке
 */
int tracing_init(const char *service_name);

/**
 * Очистка системы tracing
 */
void tracing_cleanup(void);

/**
 * Проверка инициализации
 * @return true если инициализирована
 */
bool tracing_is_initialized(void);

// ============================================================================
// Конфигурация
// ============================================================================

/**
 * Установить частоту сэмплирования
 * @param rate Частота (0.0 - 1.0)
 * @return 0 при успехе, -1 при ошибке
 */
int tracing_set_sampling_rate(double rate);

/**
 * Включить/выключить tracing
 * @param enabled Статус
 */
void tracing_set_enabled(bool enabled);

/**
 * Установить callback для экспорта trace
 * @param callback Функция экспорта
 */
void tracing_set_export_callback(tracing_export_callback_t callback);

/**
 * Установить callback для окончания span
 * @param callback Функция окончания span
 */
void tracing_set_span_end_callback(tracing_span_end_callback_t callback);

// ============================================================================
// Trace управление
// ============================================================================

/**
 * Начать новый trace
 * @param name Имя trace
 * @return Контекст trace или NULL
 */
trace_context_t* tracing_start_trace(const char *name);

/**
 * Начать trace с существующим ID (для propagation)
 * @param trace_id Существующий trace ID
 * @param parent_span_id ID родительского span
 * @return Контекст trace или NULL
 */
trace_context_t* tracing_start_trace_with_context(const char *trace_id, const char *parent_span_id);

/**
 * Завершить trace
 * @param trace Контекст trace
 * @return 0 при успехе, -1 при ошибке
 */
int tracing_end_trace(trace_context_t *trace);

/**
 * Проверить, сэмплируется ли trace
 * @param trace Контекст trace
 * @return true если сэмплируется
 */
bool tracing_is_sampled(const trace_context_t *trace);

/**
 * Получить trace ID в формате hex строки
 * @param trace Контекст trace
 * @param buffer Буфер для строки
 * @param buffer_size Размер буфера
 */
void tracing_get_trace_id_hex(const trace_context_t *trace, char *buffer, size_t buffer_size);

// ============================================================================
// Span управление
// ============================================================================

/**
 * Начать новый span
 * @param trace Контекст trace
 * @param name Имя span
 * @param kind Тип span
 * @return Span или NULL
 */
span_t* tracing_start_span(trace_context_t *trace, const char *name, tracing_span_kind_t kind);

/**
 * Завершить span
 * @param span Span
 * @return 0 при успехе, -1 при ошибке
 */
int tracing_end_span(span_t *span);

/**
 * Установить статус span
 * @param span Span
 * @param status Статус
 * @return 0 при успехе, -1 при ошибке
 */
int tracing_set_span_status(span_t *span, tracing_status_t status);

/**
 * Добавить атрибут к span
 * @param span Span
 * @param key Ключ атрибута
 * @param value Значение атрибута
 * @return 0 при успехе, -1 при ошибке
 */
int tracing_add_span_attribute(span_t *span, const char *key, const char *value);

/**
 * Добавить событие к span
 * @param span Span
 * @param event_name Имя события
 * @return 0 при успехе, -1 при ошибке
 */
int tracing_add_span_event(span_t *span, const char *event_name);

/**
 * Получить ID span в формате hex
 * @param span Span
 * @param buffer Буфер для строки
 * @param buffer_size Размер буфера
 */
void tracing_get_span_id_hex(const span_t *span, char *buffer, size_t buffer_size);

// ============================================================================
// Propagation
// ============================================================================

/**
 * Извлечь trace контекст из заголовков
 * @param traceparent Заголовок traceparent (W3C format)
 * @param tracestate Заголовок tracestate
 * @return Контекст trace или NULL
 */
trace_context_t* tracing_extract_context(const char *traceparent, const char *tracestate);

/**
 * Инжектировать trace контекст в заголовки
 * @param trace Контекст trace
 * @param span Текущий span
 * @param traceparent Буфер для traceparent
 * @param traceparent_size Размер буфера
 * @param tracestate Буфер для tracestate
 * @param tracestate_size Размер буфера
 * @return 0 при успехе, -1 при ошибке
 */
int tracing_inject_context(const trace_context_t *trace, const span_t *span,
                           char *traceparent, size_t traceparent_size,
                           char *tracestate, size_t tracestate_size);

/**
 * Создать заголовок traceparent для текущего span
 * @param span Span
 * @param buffer Буфер для строки
 * @param buffer_size Размер буфера
 * @return 0 при успехе, -1 при ошибке
 */
int tracing_create_traceparent(const span_t *span, char *buffer, size_t buffer_size);

// ============================================================================
// Метрики и статистика
// ============================================================================

/**
 * Получить статистику tracing
 * @param stats Структура статистики
 * @return 0 при успехе, -1 при ошибке
 */
int tracing_get_stats(tracing_stats_t *stats);

/**
 * Получить строку статистики
 * @param buffer Буфер для вывода
 * @param buffer_size Размер буфера
 * @return 0 при успехе, -1 при ошибке
 */
int tracing_get_stats_string(char *buffer, size_t buffer_size);

/**
 * Сбросить статистику
 */
void tracing_reset_stats(void);

/**
 * Получить текущее количество активных trace
 * @return Количество активных trace
 */
int tracing_get_active_trace_count(void);

/**
 * Получить текущее количество активных span
 * @return Количество активных span
 */
int tracing_get_active_span_count(void);

// ============================================================================
// Утилиты
// ============================================================================

/**
 * Сгенерировать случайный trace ID
 * @param buffer Буфер для строки (минимум 33 байта)
 * @param buffer_size Размер буфера
 */
void tracing_generate_trace_id(char *buffer, size_t buffer_size);

/**
 * Сгенерировать случайный span ID
 * @param buffer Буфер для строки (минимум 17 байт)
 * @param buffer_size Размер буфера
 */
void tracing_generate_span_id(char *buffer, size_t buffer_size);

/**
 * Конвертировать статус в строку
 * @param status Статус
 * @return Строка статуса
 */
const char* tracing_status_to_string(tracing_status_t status);

/**
 * Конвертировать тип span в строку
 * @param kind Тип span
 * @return Строка типа
 */
const char* tracing_span_kind_to_string(tracing_span_kind_t kind);

/**
 * Получить текущее время в наносекундах
 * @return Время в наносекундах
 */
int64_t tracing_get_time_ns(void);

#ifdef __cplusplus
}
#endif

#endif // DISTRIBUTED_TRACING_H
