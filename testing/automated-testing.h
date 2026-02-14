/*
    Система автоматического тестирования MTProxy
    Комплексное тестирование всех компонентов и интеграций
*/

#ifndef AUTOMATED_TESTING_H
#define AUTOMATED_TESTING_H

#include <stdint.h>
#include <stddef.h>

// Типы тестов
typedef enum {
    TEST_TYPE_UNIT = 0,        // Модульные тесты
    TEST_TYPE_INTEGRATION = 1, // Интеграционные тесты
    TEST_TYPE_PERFORMANCE = 2, // Тесты производительности
    TEST_TYPE_SECURITY = 3,    // Тесты безопасности
    TEST_TYPE_STRESS = 4,      // Стресс-тесты
    TEST_TYPE_REGRESSION = 5   // Регрессионные тесты
} test_type_t;

// Статусы тестов
typedef enum {
    TEST_STATUS_PENDING = 0,
    TEST_STATUS_RUNNING = 1,
    TEST_STATUS_PASSED = 2,
    TEST_STATUS_FAILED = 3,
    TEST_STATUS_SKIPPED = 4,
    TEST_STATUS_ERROR = 5
} test_status_t;

// Уровни критичности
typedef enum {
    TEST_CRITICALITY_LOW = 0,
    TEST_CRITICALITY_MEDIUM = 1,
    TEST_CRITICALITY_HIGH = 2,
    TEST_CRITICALITY_CRITICAL = 3
} test_criticality_t;

// Тестовый сценарий
typedef struct test_case {
    uint64_t test_id;
    char name[128];
    char description[256];
    test_type_t type;
    test_criticality_t criticality;
    test_status_t status;
    void (*test_function)(struct test_case *test);
    long long start_time;
    long long end_time;
    long long duration_ms;
    int assertion_count;
    int passed_assertions;
    int failed_assertions;
    char error_message[512];
    void *test_data;
    int is_enabled;
} test_case_t;

// Тестовый набор
typedef struct test_suite {
    uint64_t suite_id;
    char name[64];
    char description[128];
    test_case_t *test_cases;
    int test_count;
    int max_tests;
    test_status_t overall_status;
    long long total_duration_ms;
    int total_passed;
    int total_failed;
    int total_skipped;
} test_suite_t;

// Конфигурация тестирования
typedef struct testing_config {
    int enable_parallel_execution;
    int max_parallel_tests;
    int enable_timeout_protection;
    int test_timeout_ms;
    int enable_memory_checking;
    int enable_coverage_analysis;
    int verbose_output;
    char output_format[16]; // "text", "json", "xml"
    char report_file[256];
} testing_config_t;

// Статистика тестирования
typedef struct testing_stats {
    long long total_tests;
    long long passed_tests;
    long long failed_tests;
    long long skipped_tests;
    long long error_tests;
    double pass_rate_percentage;
    long long total_duration_ms;
    double avg_test_duration_ms;
    long long memory_allocated_bytes;
    long long memory_leaks_detected;
} testing_stats_t;

// Система тестирования
typedef struct automated_testing {
    // Конфигурация
    testing_config_t config;
    
    // Тестовые наборы
    test_suite_t *test_suites;
    int suite_count;
    int max_suites;
    
    // Статистика
    testing_stats_t stats;
    
    // Состояние
    int is_initialized;
    int is_running;
    long long start_time;
    
    // Callback функции
    void (*on_test_start)(test_case_t *test);
    void (*on_test_complete)(test_case_t *test);
    void (*on_suite_complete)(test_suite_t *suite);
    void (*on_testing_complete)(struct automated_testing *testing);
} automated_testing_t;

// Результаты теста
typedef struct test_result {
    uint64_t test_id;
    test_status_t status;
    long long duration_ms;
    int assertion_count;
    int passed_assertions;
    int failed_assertions;
    char error_message[512];
} test_result_t;

// Инициализация
automated_testing_t* testing_init(int max_suites, int max_tests_per_suite);
int testing_configure(automated_testing_t *testing, const testing_config_t *config);
void testing_cleanup(automated_testing_t *testing);

// Управление тестами
int testing_create_suite(automated_testing_t *testing, const char *name, const char *description);
int testing_add_test(automated_testing_t *testing, uint64_t suite_id, 
                    const char *name, const char *description,
                    test_type_t type, test_criticality_t criticality,
                    void (*test_function)(test_case_t*));
int testing_remove_test(automated_testing_t *testing, uint64_t test_id);
int testing_enable_test(automated_testing_t *testing, uint64_t test_id);
int testing_disable_test(automated_testing_t *testing, uint64_t test_id);

// Запуск тестов
int testing_run_single_test(automated_testing_t *testing, uint64_t test_id);
int testing_run_suite(automated_testing_t *testing, uint64_t suite_id);
int testing_run_all_tests(automated_testing_t *testing);
int testing_run_tests_by_type(automated_testing_t *testing, test_type_t type);
int testing_run_tests_by_criticality(automated_testing_t *testing, test_criticality_t criticality);

// Управление результатами
test_result_t* testing_get_test_result(automated_testing_t *testing, uint64_t test_id);
test_suite_t* testing_get_suite(automated_testing_t *testing, uint64_t suite_id);
test_case_t* testing_get_test(automated_testing_t *testing, uint64_t test_id);
void testing_get_stats(automated_testing_t *testing, testing_stats_t *stats);

// Утилиты для тестов
void testing_assert_true(test_case_t *test, int condition, const char *message);
void testing_assert_false(test_case_t *test, int condition, const char *message);
void testing_assert_equal(test_case_t *test, long long expected, long long actual, const char *message);
void testing_assert_not_equal(test_case_t *test, long long expected, long long actual, const char *message);
void testing_assert_null(test_case_t *test, void *ptr, const char *message);
void testing_assert_not_null(test_case_t *test, void *ptr, const char *message);
void testing_assert_string_equal(test_case_t *test, const char *expected, const char *actual, const char *message);
void testing_fail(test_case_t *test, const char *message);

// Тестовые функции
void testing_run_security_tests(automated_testing_t *testing);
void testing_run_performance_tests(automated_testing_t *testing);
void testing_run_network_tests(automated_testing_t *testing);
void testing_run_crypto_tests(automated_testing_t *testing);
void testing_run_integration_tests(automated_testing_t *testing);

// Генерация отчетов
int testing_generate_report(automated_testing_t *testing, const char *filename);
int testing_export_results(automated_testing_t *testing, const char *filename, const char *format);
void testing_print_summary(automated_testing_t *testing);

// Callback регистрации
void testing_set_test_start_callback(automated_testing_t *testing,
                                   void (*callback)(test_case_t*));
void testing_set_test_complete_callback(automated_testing_t *testing,
                                       void (*callback)(test_case_t*));
void testing_set_suite_complete_callback(automated_testing_t *testing,
                                        void (*callback)(test_suite_t*));
void testing_set_testing_complete_callback(automated_testing_t *testing,
                                          void (*callback)(automated_testing_t*));

// Вспомогательные функции
const char* testing_status_to_string(test_status_t status);
const char* testing_type_to_string(test_type_t type);
const char* testing_criticality_to_string(test_criticality_t criticality);
uint64_t testing_generate_test_id(void);
int testing_is_test_enabled(automated_testing_t *testing, uint64_t test_id);

#endif // AUTOMATED_TESTING_H