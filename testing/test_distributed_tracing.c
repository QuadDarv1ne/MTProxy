/*
    MTProxy Distributed Tracing Tests
    Тесты для системы трассировки запросов
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "system/monitoring/distributed-tracing.h"
#include "testing/test_common.h"

// ============================================================================
// Тесты инициализации
// ============================================================================

static int test_tracing_init(void) {
    TEST_START();
    
    int result = tracing_init("test-service");
    
    ASSERT(result == 0, "Инициализация успешна");
    ASSERT(tracing_is_initialized() == true, "Tracing инициализирован");
    
    // Повторная инициализация
    result = tracing_init("test-service");
    ASSERT(result == 0, "Повторная инициализация успешна");
    
    TEST_END();
}

static int test_tracing_init_null(void) {
    TEST_START();
    
    int result = tracing_init(NULL);
    ASSERT(result == -1, "NULL service_name отклонён");
    
    TEST_END();
}

static int test_tracing_cleanup(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_cleanup();
    
    ASSERT(tracing_is_initialized() == false, "Tracing очищен");
    
    TEST_END();
}

// ============================================================================
// Тесты конфигурации
// ============================================================================

static int test_tracing_set_sampling_rate(void) {
    TEST_START();
    
    tracing_init("test-service");
    
    int result = tracing_set_sampling_rate(0.5);
    ASSERT(result == 0, "Sampling rate установлен");
    
    result = tracing_set_sampling_rate(0.0);
    ASSERT(result == 0, "Sampling rate = 0 установлен");
    
    result = tracing_set_sampling_rate(1.0);
    ASSERT(result == 0, "Sampling rate = 1 установлен");
    
    result = tracing_set_sampling_rate(-0.1);
    ASSERT(result == -1, "Отрицательный rate отклонён");
    
    result = tracing_set_sampling_rate(1.5);
    ASSERT(result == -1, "Rate > 1 отклонён");
    
    TEST_END();
}

static int test_tracing_set_enabled(void) {
    TEST_START();
    
    tracing_init("test-service");
    
    tracing_set_enabled(false);
    // Выключено
    
    tracing_set_enabled(true);
    // Включено
    
    TEST_END();
}

// ============================================================================
// Тесты trace управления
// ============================================================================

static int test_tracing_start_end_trace(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);  // 100% сэмплирование
    
    trace_context_t *trace = tracing_start_trace("test-trace");
    ASSERT(trace != NULL, "Trace создан");
    ASSERT(strlen(trace->id) == 32, "Trace ID имеет правильную длину");
    
    int result = tracing_end_trace(trace);
    ASSERT(result == 0, "Trace завершён");
    
    TEST_END();
}

static int test_tracing_start_trace_with_context(void) {
    TEST_START();
    
    tracing_init("test-service");
    
    char trace_id[33] = "1234567890abcdef1234567890abcdef";
    trace_context_t *trace = tracing_start_trace_with_context(trace_id, "parent-span-id");
    
    ASSERT(trace != NULL, "Trace с контекстом создан");
    ASSERT(strcmp(trace->id, trace_id) == 0, "Trace ID совпадает");
    
    TEST_END();
}

static int test_tracing_is_sampled(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    trace_context_t *trace = tracing_start_trace("test");
    ASSERT(tracing_is_sampled(trace) == true, "Trace сэмплируется");
    
    tracing_end_trace(trace);
    
    TEST_END();
}

static int test_tracing_get_trace_id_hex(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    trace_context_t *trace = tracing_start_trace("test");
    
    char buffer[64] = {0};
    tracing_get_trace_id_hex(trace, buffer, sizeof(buffer));
    
    ASSERT(strlen(buffer) == 32, "Trace ID hex имеет правильную длину");
    
    tracing_end_trace(trace);
    
    TEST_END();
}

// ============================================================================
// Тесты span управления
// ============================================================================

static int test_tracing_start_end_span(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    trace_context_t *trace = tracing_start_trace("test-trace");
    
    span_t *span = tracing_start_span(trace, "test-span", TRACING_SPAN_SERVER);
    ASSERT(span != NULL, "Span создан");
    ASSERT(strcmp(span->name, "test-span") == 0, "Имя span совпадает");
    ASSERT(span->kind == TRACING_SPAN_SERVER, "Тип span совпадает");
    
    int result = tracing_end_span(span);
    ASSERT(result == 0, "Span завершён");
    ASSERT(span->duration_ns > 0, "Длительность span > 0");
    
    tracing_end_trace(trace);
    
    TEST_END();
}

static int test_tracing_set_span_status(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    trace_context_t *trace = tracing_start_trace("test");
    span_t *span = tracing_start_span(trace, "test", TRACING_SPAN_INTERNAL);
    
    int result = tracing_set_span_status(span, TRACING_STATUS_OK);
    ASSERT(result == 0, "Статус установлен");
    ASSERT(span->status == TRACING_STATUS_OK, "Статус OK");
    
    result = tracing_set_span_status(span, TRACING_STATUS_ERROR);
    ASSERT(result == 0, "Статус изменён");
    ASSERT(span->status == TRACING_STATUS_ERROR, "Статус Error");
    
    tracing_end_span(span);
    tracing_end_trace(trace);
    
    TEST_END();
}

static int test_tracing_add_span_attribute(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    trace_context_t *trace = tracing_start_trace("test");
    span_t *span = tracing_start_span(trace, "test", TRACING_SPAN_INTERNAL);
    
    int result = tracing_add_span_attribute(span, "client_ip", "192.168.1.100");
    ASSERT(result == 0, "Атрибут добавлен");
    ASSERT(span->attribute_count == 1, "Количество атрибутов = 1");
    
    result = tracing_add_span_attribute(span, "method", "GET");
    ASSERT(result == 0, "Второй атрибут добавлен");
    ASSERT(span->attribute_count == 2, "Количество атрибутов = 2");
    
    tracing_end_span(span);
    tracing_end_trace(trace);
    
    TEST_END();
}

static int test_tracing_add_span_event(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    trace_context_t *trace = tracing_start_trace("test");
    span_t *span = tracing_start_span(trace, "test", TRACING_SPAN_INTERNAL);
    
    int result = tracing_add_span_event(span, "request_received");
    ASSERT(result == 0, "Событие добавлено");
    ASSERT(span->event_count == 1, "Количество событий = 1");
    
    result = tracing_add_span_event(span, "request_processed");
    ASSERT(result == 0, "Второе событие добавлено");
    ASSERT(span->event_count == 2, "Количество событий = 2");
    
    tracing_end_span(span);
    tracing_end_trace(trace);
    
    TEST_END();
}

static int test_tracing_get_span_id_hex(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    trace_context_t *trace = tracing_start_trace("test");
    span_t *span = tracing_start_span(trace, "test", TRACING_SPAN_INTERNAL);
    
    char buffer[64] = {0};
    tracing_get_span_id_hex(span, buffer, sizeof(buffer));
    
    ASSERT(strlen(buffer) == 16, "Span ID hex имеет правильную длину");
    
    tracing_end_span(span);
    tracing_end_trace(trace);
    
    TEST_END();
}

// ============================================================================
// Тесты propagation
// ============================================================================

static int test_tracing_create_traceparent(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    trace_context_t *trace = tracing_start_trace("test");
    span_t *span = tracing_start_span(trace, "test", TRACING_SPAN_INTERNAL);
    
    char buffer[64] = {0};
    int result = tracing_create_traceparent(span, buffer, sizeof(buffer));
    
    ASSERT(result == 0, "Traceparent создан");
    ASSERT(strlen(buffer) == 55, "Traceparent имеет правильную длину");
    ASSERT(strncmp(buffer, "00-", 3) == 0, "Traceparent начинается с 00-");
    
    tracing_end_span(span);
    tracing_end_trace(trace);
    
    TEST_END();
}

static int test_tracing_inject_context(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    trace_context_t *trace = tracing_start_trace("test");
    span_t *span = tracing_start_span(trace, "test", TRACING_SPAN_INTERNAL);
    
    char traceparent[64] = {0};
    char tracestate[64] = {0};
    
    int result = tracing_inject_context(trace, span, traceparent, sizeof(traceparent),
                                         tracestate, sizeof(tracestate));
    
    ASSERT(result == 0, "Контекст инжектирован");
    ASSERT(strlen(traceparent) == 55, "Traceparent имеет правильную длину");
    
    tracing_end_span(span);
    tracing_end_trace(trace);
    
    TEST_END();
}

static int test_tracing_extract_context(void) {
    TEST_START();
    
    tracing_init("test-service");
    
    const char *traceparent = "00-1234567890abcdef1234567890abcdef-1234567890abcdef-01";
    trace_context_t *trace = tracing_extract_context(traceparent, NULL);
    
    ASSERT(trace != NULL, "Контекст извлечён");
    ASSERT(strcmp(trace->id, "1234567890abcdef1234567890abcdef") == 0, "Trace ID совпадает");
    
    tracing_end_trace(trace);
    
    TEST_END();
}

// ============================================================================
// Тесты статистики
// ============================================================================

static int test_tracing_get_stats(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    trace_context_t *trace = tracing_start_trace("test");
    span_t *span = tracing_start_span(trace, "test", TRACING_SPAN_INTERNAL);
    tracing_end_span(span);
    tracing_end_trace(trace);
    
    tracing_stats_t stats;
    int result = tracing_get_stats(&stats);
    
    ASSERT(result == 0, "Статистика получена");
    ASSERT(stats.total_traces == 1, "Всего trace: 1");
    ASSERT(stats.total_spans == 1, "Всего span: 1");
    ASSERT(stats.completed_traces == 1, "Завершено trace: 1");
    
    TEST_END();
}

static int test_tracing_get_stats_string(void) {
    TEST_START();
    
    tracing_init("test-service");
    
    char buffer[512] = {0};
    int result = tracing_get_stats_string(buffer, sizeof(buffer));
    
    ASSERT(result == 0, "Строка статистики получена");
    ASSERT(strlen(buffer) > 0, "Строка не пустая");
    ASSERT(strstr(buffer, "Distributed Tracing") != NULL, "Содержит заголовок");
    
    TEST_END();
}

static int test_tracing_reset_stats(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    trace_context_t *trace = tracing_start_trace("test");
    span_t *span = tracing_start_span(trace, "test", TRACING_SPAN_INTERNAL);
    tracing_end_span(span);
    tracing_end_trace(trace);
    
    tracing_reset_stats();
    
    tracing_stats_t stats;
    tracing_get_stats(&stats);
    
    ASSERT(stats.total_traces == 0, "total_traces сброшен");
    ASSERT(stats.total_spans == 0, "total_spans сброшен");
    
    TEST_END();
}

static int test_tracing_get_active_counts(void) {
    TEST_START();
    
    tracing_init("test-service");
    tracing_set_sampling_rate(1.0);
    
    int trace_count = tracing_get_active_trace_count();
    ASSERT(trace_count == 0, "Активных trace: 0");
    
    trace_context_t *trace = tracing_start_trace("test");
    span_t *span = tracing_start_span(trace, "test", TRACING_SPAN_INTERNAL);
    
    trace_count = tracing_get_active_trace_count();
    ASSERT(trace_count == 1, "Активных trace: 1");
    
    int span_count = tracing_get_active_span_count();
    ASSERT(span_count == 1, "Активных span: 1");
    
    tracing_end_span(span);
    tracing_end_trace(trace);
    
    TEST_END();
}

// ============================================================================
// Тесты утилит
// ============================================================================

static int test_tracing_generate_trace_id(void) {
    TEST_START();
    
    char buffer[64] = {0};
    tracing_generate_trace_id(buffer, sizeof(buffer));
    
    ASSERT(strlen(buffer) == 32, "Trace ID имеет правильную длину");
    
    // Проверка на уникальность
    char buffer2[64] = {0};
    tracing_generate_trace_id(buffer2, sizeof(buffer2));
    
    ASSERT(strcmp(buffer, buffer2) != 0, "Trace ID уникальны");
    
    TEST_END();
}

static int test_tracing_generate_span_id(void) {
    TEST_START();
    
    char buffer[64] = {0};
    tracing_generate_span_id(buffer, sizeof(buffer));
    
    ASSERT(strlen(buffer) == 16, "Span ID имеет правильную длину");
    
    TEST_END();
}

static int test_tracing_status_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(tracing_status_to_string(TRACING_STATUS_UNSET), "Unset") == 0, "Unset");
    ASSERT(strcmp(tracing_status_to_string(TRACING_STATUS_OK), "OK") == 0, "OK");
    ASSERT(strcmp(tracing_status_to_string(TRACING_STATUS_ERROR), "Error") == 0, "Error");
    
    TEST_END();
}

static int test_tracing_span_kind_to_string(void) {
    TEST_START();
    
    ASSERT(strcmp(tracing_span_kind_to_string(TRACING_SPAN_SERVER), "Server") == 0, "Server");
    ASSERT(strcmp(tracing_span_kind_to_string(TRACING_SPAN_CLIENT), "Client") == 0, "Client");
    ASSERT(strcmp(tracing_span_kind_to_string(TRACING_SPAN_INTERNAL), "Internal") == 0, "Internal");
    
    TEST_END();
}

static int test_tracing_get_time_ns(void) {
    TEST_START();
    
    int64_t time1 = tracing_get_time_ns();
    
#ifdef _WIN32
    Sleep(10);
#else
    usleep(10000);
#endif
    
    int64_t time2 = tracing_get_time_ns();
    
    ASSERT(time2 > time1, "Время увеличивается");
    
    TEST_END();
}

// ============================================================================
// Main
// ============================================================================

int main(void) {
    test_init("MTProxy Distributed Tracing Tests");
    
    // Инициализация
    test_run(test_tracing_init);
    test_run(test_tracing_init_null);
    test_run(test_tracing_cleanup);
    
    // Конфигурация
    test_run(test_tracing_set_sampling_rate);
    test_run(test_tracing_set_enabled);
    
    // Trace управление
    test_run(test_tracing_start_end_trace);
    test_run(test_tracing_start_trace_with_context);
    test_run(test_tracing_is_sampled);
    test_run(test_tracing_get_trace_id_hex);
    
    // Span управление
    test_run(test_tracing_start_end_span);
    test_run(test_tracing_set_span_status);
    test_run(test_tracing_add_span_attribute);
    test_run(test_tracing_add_span_event);
    test_run(test_tracing_get_span_id_hex);
    
    // Propagation
    test_run(test_tracing_create_traceparent);
    test_run(test_tracing_inject_context);
    test_run(test_tracing_extract_context);
    
    // Статистика
    test_run(test_tracing_get_stats);
    test_run(test_tracing_get_stats_string);
    test_run(test_tracing_reset_stats);
    test_run(test_tracing_get_active_counts);
    
    // Утилиты
    test_run(test_tracing_generate_trace_id);
    test_run(test_tracing_generate_span_id);
    test_run(test_tracing_status_to_string);
    test_run(test_tracing_span_kind_to_string);
    test_run(test_tracing_get_time_ns);
    
    return test_finish();
}
