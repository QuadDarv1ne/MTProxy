/*
    Система примеров использования MTProxy
    Практические примеры для различных сценариев
*/

#ifndef MTProxy_EXAMPLES_H
#define MTProxy_EXAMPLES_H

#include <stdint.h>
#include <stddef.h>

// Типы примеров
typedef enum {
    EXAMPLE_TYPE_BASIC = 0,      // Базовые примеры
    EXAMPLE_TYPE_ADVANCED = 1,   // Продвинутые примеры
    EXAMPLE_TYPE_SECURITY = 2,   // Примеры безопасности
    EXAMPLE_TYPE_PERFORMANCE = 3, // Примеры оптимизации
    EXAMPLE_TYPE_INTEGRATION = 4  // Примеры интеграции
} example_type_t;

// Сценарии использования
typedef enum {
    SCENARIO_SIMPLE_PROXY = 0,     // Простой прокси
    SCENARIO_LOAD_BALANCING = 1,   // Балансировка нагрузки
    SCENARIO_HIGH_AVAILABILITY = 2, // Высокая доступность
    SCENARIO_SECURITY_HARDENED = 3, // Усиленная безопасность
    SCENARIO_PERFORMANCE_TUNING = 4 // Настройка производительности
} usage_scenario_t;

// Пример использования
typedef struct usage_example {
    uint64_t example_id;
    char name[64];
    char description[256];
    example_type_t type;
    usage_scenario_t scenario;
    char code_sample[2048];
    char configuration[1024];
    char expected_output[512];
    int difficulty_level; // 1-5
    int estimated_time_minutes;
} usage_example_t;

// Коллекция примеров
typedef struct example_collection {
    usage_example_t *examples;
    int example_count;
    int max_examples;
    char collection_name[64];
    char collection_description[128];
} example_collection_t;

// Конфигурация примеров
typedef struct examples_config {
    int enable_compilation_examples;
    int enable_runtime_examples;
    int enable_security_examples;
    int enable_performance_examples;
    int show_detailed_output;
    char output_directory[256];
} examples_config_t;

// Система примеров
typedef struct mtproxy_examples {
    // Конфигурация
    examples_config_t config;
    
    // Коллекции примеров
    example_collection_t *collections;
    int collection_count;
    int max_collections;
    
    // Состояние
    int is_initialized;
    long long creation_time;
    
    // Callback функции
    void (*on_example_run)(usage_example_t *example, int success);
    void (*on_example_complete)(usage_example_t *example, const char *output);
} mtproxy_examples_t;

// Инициализация
mtproxy_examples_t* examples_init(int max_collections, int max_examples_per_collection);
int examples_configure(mtproxy_examples_t *examples, const examples_config_t *config);
void examples_cleanup(mtproxy_examples_t *examples);

// Управление примерами
int examples_create_collection(mtproxy_examples_t *examples, 
                             const char *name, const char *description);
int examples_add_example(mtproxy_examples_t *examples, uint64_t collection_id,
                        const char *name, const char *description,
                        example_type_t type, usage_scenario_t scenario,
                        const char *code_sample, const char *configuration);
int examples_remove_example(mtproxy_examples_t *examples, uint64_t example_id);
int examples_update_example(mtproxy_examples_t *examples, uint64_t example_id,
                           const char *code_sample, const char *configuration);

// Запуск примеров
int examples_run_single_example(mtproxy_examples_t *examples, uint64_t example_id);
int examples_run_collection_examples(mtproxy_examples_t *examples, uint64_t collection_id);
int examples_run_all_examples(mtproxy_examples_t *examples);
int examples_run_examples_by_type(mtproxy_examples_t *examples, example_type_t type);
int examples_run_examples_by_scenario(mtproxy_examples_t *examples, usage_scenario_t scenario);

// Получение информации
usage_example_t* examples_get_example(mtproxy_examples_t *examples, uint64_t example_id);
example_collection_t* examples_get_collection(mtproxy_examples_t *examples, uint64_t collection_id);
void examples_list_collections(mtproxy_examples_t *examples, char *buffer, size_t buffer_size);
void examples_list_examples(mtproxy_examples_t *examples, uint64_t collection_id, 
                           char *buffer, size_t buffer_size);

// Генерация кода
int examples_generate_compilation_script(mtproxy_examples_t *examples, uint64_t example_id,
                                        const char *output_file);
int examples_generate_configuration_file(mtproxy_examples_t *examples, uint64_t example_id,
                                        const char *output_file);
int examples_generate_docker_compose(mtproxy_examples_t *examples, uint64_t example_id,
                                    const char *output_file);

// Экспорт и импорт
int examples_export_collection(mtproxy_examples_t *examples, uint64_t collection_id,
                              const char *filename);
int examples_import_collection(mtproxy_examples_t *examples, const char *filename);
int examples_export_all_examples(mtproxy_examples_t *examples, const char *filename);

// Вспомогательные функции
const char* examples_type_to_string(example_type_t type);
const char* examples_scenario_to_string(usage_scenario_t scenario);
const char* examples_difficulty_to_string(int difficulty);
uint64_t examples_generate_example_id(void);
int examples_validate_example(usage_example_t *example);

// Примеры для разных сценариев

// Базовый пример
void example_basic_proxy_setup(mtproxy_examples_t *examples);

// Пример балансировки нагрузки
void example_load_balancing_setup(mtproxy_examples_t *examples);

// Пример высокой доступности
void example_high_availability_setup(mtproxy_examples_t *examples);

// Пример усиленной безопасности
void example_security_hardened_setup(mtproxy_examples_t *examples);

// Пример оптимизации производительности
void example_performance_tuning_setup(mtproxy_examples_t *examples);

// Callback регистрации
void examples_set_example_run_callback(mtproxy_examples_t *examples,
                                      void (*callback)(usage_example_t*, int));
void examples_set_example_complete_callback(mtproxy_examples_t *examples,
                                           void (*callback)(usage_example_t*, const char*));

#endif // MTProxy_EXAMPLES_H