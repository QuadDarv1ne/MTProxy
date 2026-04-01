/*
    MTProxy Distributed Tracing
    Реализация системы трассировки запросов между узлами
*/

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <pthread.h>
    #include <sys/time.h>
#endif

#include "system/monitoring/distributed-tracing.h"
#include "common/utils.h"
#include "common/kprintf.h"

// ============================================================================
// Глобальные переменные
// ============================================================================

static struct {
    bool initialized;
    tracing_config_t config;
    tracing_stats_t stats;
    
    trace_context_t *active_traces[1024];
    int active_trace_count;
    
    span_t *active_spans[4096];
    int active_span_count;
    
    tracing_export_callback_t export_callback;
    tracing_span_end_callback_t span_end_callback;
    
    // Для генерации ID
    uint64_t random_seed;
    int id_counter;
} g_tracing = {0};

// ============================================================================
// Внутренние функции
// ============================================================================

static int64_t get_time_ns(void) {
#ifdef _WIN32
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    // 100-наносекундные интервалы с 1601 года
    return (int64_t)((uli.QuadPart - 116444736000000000ULL) * 100);
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
#endif
}

static void generate_random_hex(char *buffer, size_t length, size_t buffer_size) {
    static const char hex_chars[] = "0123456789abcdef";
    
    if (buffer_size < length + 1) return;
    
    for (size_t i = 0; i < length; i++) {
        buffer[i] = hex_chars[rand() % 16];
    }
    buffer[length] = '\0';
}

static trace_context_t* find_trace(const char *trace_id) {
    for (int i = 0; i < g_tracing.active_trace_count; i++) {
        if (g_tracing.active_traces[i] && 
            strcmp(g_tracing.active_traces[i]->id, trace_id) == 0) {
            return g_tracing.active_traces[i];
        }
    }
    return NULL;
}

static span_t* create_span(void) {
    span_t *span = (span_t*)calloc(1, sizeof(span_t));
    if (!span) return NULL;
    
    if (g_tracing.active_span_count < 4096) {
        g_tracing.active_spans[g_tracing.active_span_count++] = span;
        g_tracing.stats.total_spans++;
        g_tracing.stats.active_spans++;
    }
    
    return span;
}

static void free_span(span_t *span) {
    if (!span) return;
    
    // Освобождение атрибутов
    for (int i = 0; i < span->attribute_count; i++) {
        if (span->attribute_keys[i]) free(span->attribute_keys[i]);
        if (span->attribute_values[i]) free(span->attribute_values[i]);
    }
    
    // Освобождение событий
    for (int i = 0; i < span->event_count; i++) {
        if (span->events[i]) free(span->events[i]);
    }
    
    free(span);
}

static bool should_sample(void) {
    if (g_tracing.config.sampling_rate >= 1.0) return true;
    if (g_tracing.config.sampling_rate <= 0.0) return false;
    
    return ((double)rand() / RAND_MAX) < g_tracing.config.sampling_rate;
}

// ============================================================================
// Инициализация и очистка
// ============================================================================

int tracing_init(const char *service_name) {
    if (!service_name) {
        return -1;
    }
    
    if (g_tracing.initialized) {
        return 0;
    }
    
    memset(&g_tracing, 0, sizeof(g_tracing));
    
    snprintf(g_tracing.config.service_name, sizeof(g_tracing.config.service_name), 
             "%s", service_name);
    g_tracing.config.sampling_rate = 0.1;  // 10% по умолчанию
    g_tracing.config.enabled = true;
    g_tracing.config.max_spans_per_trace = TRACING_MAX_SPANS_PER_TRACE;
    g_tracing.config.export_interval_sec = 30;
    
    g_tracing.stats.start_time = get_time_ns();
    
    // Инициализация генератора случайных чисел
    g_tracing.random_seed = (uint64_t)time(NULL);
    srand((unsigned int)g_tracing.random_seed);
    
    g_tracing.initialized = true;
    
    kprintf("[TRACING] Initialized for service: %s (sampling: %.1f%%)\n", 
            service_name, g_tracing.config.sampling_rate * 100.0);
    
    return 0;
}

void tracing_cleanup(void) {
    if (!g_tracing.initialized) {
        return;
    }
    
    // Завершение активных trace
    for (int i = 0; i < g_tracing.active_trace_count; i++) {
        if (g_tracing.active_traces[i]) {
            tracing_end_trace(g_tracing.active_traces[i]);
        }
    }
    
    // Освобождение активных span
    for (int i = 0; i < g_tracing.active_span_count; i++) {
        if (g_tracing.active_spans[i]) {
            free_span(g_tracing.active_spans[i]);
        }
    }
    
    memset(&g_tracing, 0, sizeof(g_tracing));
    kprintf("[TRACING] Cleaned up\n");
}

bool tracing_is_initialized(void) {
    return g_tracing.initialized;
}

// ============================================================================
// Конфигурация
// ============================================================================

int tracing_set_sampling_rate(double rate) {
    if (!g_tracing.initialized || rate < 0.0 || rate > 1.0) {
        return -1;
    }
    
    g_tracing.config.sampling_rate = rate;
    kprintf("[TRACING] Sampling rate set to: %.1f%%\n", rate * 100.0);
    return 0;
}

void tracing_set_enabled(bool enabled) {
    g_tracing.config.enabled = enabled;
    kprintf("[TRACING] Tracing %s\n", enabled ? "enabled" : "disabled");
}

void tracing_set_export_callback(tracing_export_callback_t callback) {
    g_tracing.export_callback = callback;
    kprintf("[TRACING] Export callback set\n");
}

void tracing_set_span_end_callback(tracing_span_end_callback_t callback) {
    g_tracing.span_end_callback = callback;
    kprintf("[TRACING] Span end callback set\n");
}

// ============================================================================
// Trace управление
// ============================================================================

trace_context_t* tracing_start_trace(const char *name) {
    if (!g_tracing.initialized || !g_tracing.config.enabled || !name) {
        return NULL;
    }
    
    trace_context_t *trace = (trace_context_t*)calloc(1, sizeof(trace_context_t));
    if (!trace) return NULL;
    
    // Генерация trace ID
    tracing_generate_trace_id(trace->id, sizeof(trace->id));
    
    trace->start_time = get_time_ns();
    trace->sampled = should_sample();
    trace->completed = false;
    
    if (g_tracing.active_trace_count < 1024) {
        g_tracing.active_traces[g_tracing.active_trace_count++] = trace;
        g_tracing.stats.total_traces++;
        if (trace->sampled) {
            g_tracing.stats.sampled_traces++;
        }
    }
    
    kprintf("[TRACING] Started trace: %s (sampled: %s)\n", 
            name, trace->sampled ? "yes" : "no");
    
    return trace;
}

trace_context_t* tracing_start_trace_with_context(const char *trace_id, const char *parent_span_id) {
    if (!g_tracing.initialized || !trace_id) {
        return NULL;
    }
    
    // Проверка существующего trace
    trace_context_t *existing = find_trace(trace_id);
    if (existing) {
        return existing;
    }
    
    trace_context_t *trace = (trace_context_t*)calloc(1, sizeof(trace_context_t));
    if (!trace) return NULL;
    
    snprintf(trace->id, sizeof(trace->id), "%s", trace_id);
    if (parent_span_id) {
        snprintf(trace->root_span_id, sizeof(trace->root_span_id), "%s", parent_span_id);
    }
    
    trace->start_time = get_time_ns();
    trace->sampled = true;  // Уже сэмплируется на upstream
    trace->completed = false;
    
    if (g_tracing.active_trace_count < 1024) {
        g_tracing.active_traces[g_tracing.active_trace_count++] = trace;
        g_tracing.stats.total_traces++;
        g_tracing.stats.sampled_traces++;
    }
    
    return trace;
}

int tracing_end_trace(trace_context_t *trace) {
    if (!trace || !g_tracing.initialized) {
        return -1;
    }
    
    trace->end_time = get_time_ns();
    trace->completed = true;
    
    // Удаление из активных
    for (int i = 0; i < g_tracing.active_trace_count; i++) {
        if (g_tracing.active_traces[i] == trace) {
            // Сдвиг массива
            for (int j = i; j < g_tracing.active_trace_count - 1; j++) {
                g_tracing.active_traces[j] = g_tracing.active_traces[j + 1];
            }
            g_tracing.active_trace_count--;
            break;
        }
    }
    
    g_tracing.stats.completed_traces++;
    
    // Экспорт через callback
    if (g_tracing.export_callback && trace->sampled) {
        g_tracing.export_callback(trace, NULL, 0);
        g_tracing.stats.exported_traces++;
    }
    
    free(trace);
    
    return 0;
}

bool tracing_is_sampled(const trace_context_t *trace) {
    return trace ? trace->sampled : false;
}

void tracing_get_trace_id_hex(const trace_context_t *trace, char *buffer, size_t buffer_size) {
    if (!trace || !buffer || buffer_size < TRACING_MAX_TRACE_ID_LEN + 1) {
        return;
    }
    snprintf(buffer, buffer_size, "%s", trace->id);
}

// ============================================================================
// Span управление
// ============================================================================

span_t* tracing_start_span(trace_context_t *trace, const char *name, tracing_span_kind_t kind) {
    if (!g_tracing.initialized || !g_tracing.config.enabled || !trace || !name) {
        return NULL;
    }
    
    if (!trace->sampled) {
        return NULL;  // Не сэмплируется
    }

    span_t *span = create_span();
    if (!span) return NULL;

    // Генерация span ID
    tracing_generate_span_id(span->span_id, sizeof(span->span_id));
    snprintf(span->trace_id, sizeof(span->trace_id), "%s", trace->id);
    snprintf(span->name, sizeof(span->name), "%s", name);

    span->kind = kind;
    span->status = TRACING_STATUS_UNSET;
    span->start_time = get_time_ns();
    span->trace = trace;

    // Parent span ID
    if (trace->root_span_id[0] != '\0') {
        snprintf(span->parent_span_id, sizeof(span->parent_span_id), "%s", trace->root_span_id);
    }

    kprintf("[TRACING] Started span: %s (kind: %s)\n", name, tracing_span_kind_to_string(kind));

    return span;
}

int tracing_end_span(span_t *span) {
    if (!span || !g_tracing.initialized) {
        return -1;
    }
    
    span->end_time = get_time_ns();
    span->duration_ns = span->end_time - span->start_time;
    
    // Удаление из активных
    for (int i = 0; i < g_tracing.active_span_count; i++) {
        if (g_tracing.active_spans[i] == span) {
            for (int j = i; j < g_tracing.active_span_count - 1; j++) {
                g_tracing.active_spans[j] = g_tracing.active_spans[j + 1];
            }
            g_tracing.active_span_count--;
            break;
        }
    }
    
    g_tracing.stats.active_spans--;
    
    // Callback
    if (g_tracing.span_end_callback) {
        g_tracing.span_end_callback(span);
    }
    
    // Обновление средней длительности
    double duration_ms = (double)span->duration_ns / 1000000.0;
    g_tracing.stats.avg_span_duration_ms = 
        (g_tracing.stats.avg_span_duration_ms * 0.9) + (duration_ms * 0.1);
    
    return 0;
}

int tracing_set_span_status(span_t *span, tracing_status_t status) {
    if (!span) return -1;
    span->status = status;
    return 0;
}

int tracing_add_span_attribute(span_t *span, const char *key, const char *value) {
    if (!span || !key || !value || span->attribute_count >= TRACING_MAX_ATTRIBUTES) {
        return -1;
    }
    
    span->attribute_keys[span->attribute_count] = strdup(key);
    span->attribute_values[span->attribute_count] = strdup(value);
    span->attribute_count++;
    
    return 0;
}

int tracing_add_span_event(span_t *span, const char *event_name) {
    if (!span || !event_name || span->event_count >= TRACING_MAX_EVENTS) {
        return -1;
    }
    
    span->events[span->event_count] = strdup(event_name);
    span->event_timestamps[span->event_count] = get_time_ns();
    span->event_count++;
    
    return 0;
}

void tracing_get_span_id_hex(const span_t *span, char *buffer, size_t buffer_size) {
    if (!span || !buffer || buffer_size < TRACING_MAX_SPAN_ID_LEN + 1) {
        return;
    }
    snprintf(buffer, buffer_size, "%s", span->span_id);
}

// ============================================================================
// Propagation
// ============================================================================

trace_context_t* tracing_extract_context(const char *traceparent, const char *tracestate) {
    if (!traceparent || strlen(traceparent) < 55) {
        return NULL;
    }
    
    // Парсинг W3C traceparent: version-trace_id-parent_id-flags
    char version[3] = {0};
    char trace_id[33] = {0};
    char parent_id[17] = {0};
    char flags[3] = {0};
    
    if (sscanf(traceparent, "%2[^-]-%32[^-]-%16[^-]-%2[^-]", 
               version, trace_id, parent_id, flags) != 4) {
        return NULL;
    }
    
    return tracing_start_trace_with_context(trace_id, parent_id);
}

int tracing_inject_context(const trace_context_t *trace, const span_t *span,
                           char *traceparent, size_t traceparent_size,
                           char *tracestate, size_t tracestate_size) {
    if (!trace || !span || !traceparent || traceparent_size < 56) {
        return -1;
    }
    
    // Формат W3C traceparent
    snprintf(traceparent, traceparent_size, "00-%s-%s-01", trace->id, span->span_id);
    
    if (tracestate && tracestate_size > 0) {
        snprintf(tracestate, tracestate_size, "%s=1", g_tracing.config.service_name);
    }
    
    return 0;
}

int tracing_create_traceparent(const span_t *span, char *buffer, size_t buffer_size) {
    if (!span || !buffer || buffer_size < 56) {
        return -1;
    }
    
    snprintf(buffer, buffer_size, "00-%s-%s-01", span->trace_id, span->span_id);
    return 0;
}

// ============================================================================
// Метрики и статистика
// ============================================================================

int tracing_get_stats(tracing_stats_t *stats) {
    if (!g_tracing.initialized || !stats) {
        return -1;
    }
    
    memcpy(stats, &g_tracing.stats, sizeof(tracing_stats_t));
    return 0;
}

int tracing_get_stats_string(char *buffer, size_t buffer_size) {
    if (!g_tracing.initialized || !buffer || buffer_size < 256) {
        return -1;
    }
    
    tracing_stats_t stats;
    tracing_get_stats(&stats);
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "Distributed Tracing Statistics:\n");
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Service: %s\n", g_tracing.config.service_name);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Sampling Rate: %.1f%%\n", g_tracing.config.sampling_rate * 100.0);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Traces: Total %llu, Sampled %llu, Completed %llu\n",
                       (unsigned long long)stats.total_traces,
                       (unsigned long long)stats.sampled_traces,
                       (unsigned long long)stats.completed_traces);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Spans: Total %llu, Active %llu, Exported %llu\n",
                       (unsigned long long)stats.total_spans,
                       (unsigned long long)stats.active_spans,
                       (unsigned long long)stats.exported_traces);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Avg Duration: Span %.2f ms\n", stats.avg_span_duration_ms);
    
    return 0;
}

void tracing_reset_stats(void) {
    if (!g_tracing.initialized) return;
    
    memset(&g_tracing.stats, 0, sizeof(g_tracing.stats));
    g_tracing.stats.start_time = get_time_ns();
    kprintf("[TRACING] Statistics reset\n");
}

int tracing_get_active_trace_count(void) {
    return g_tracing.active_trace_count;
}

int tracing_get_active_span_count(void) {
    return g_tracing.active_span_count;
}

// ============================================================================
// Утилиты
// ============================================================================

void tracing_generate_trace_id(char *buffer, size_t buffer_size) {
    if (buffer_size < 33) return;
    generate_random_hex(buffer, 32, buffer_size);
}

void tracing_generate_span_id(char *buffer, size_t buffer_size) {
    if (buffer_size < 17) return;
    generate_random_hex(buffer, 16, buffer_size);
}

const char* tracing_status_to_string(tracing_status_t status) {
    switch (status) {
        case TRACING_STATUS_UNSET: return "Unset";
        case TRACING_STATUS_OK: return "OK";
        case TRACING_STATUS_ERROR: return "Error";
        default: return "Unknown";
    }
}

const char* tracing_span_kind_to_string(tracing_span_kind_t kind) {
    switch (kind) {
        case TRACING_SPAN_SERVER: return "Server";
        case TRACING_SPAN_CLIENT: return "Client";
        case TRACING_SPAN_PRODUCER: return "Producer";
        case TRACING_SPAN_CONSUMER: return "Consumer";
        case TRACING_SPAN_INTERNAL: return "Internal";
        default: return "Unknown";
    }
}

int64_t tracing_get_time_ns(void) {
    return get_time_ns();
}
