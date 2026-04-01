/*
    Реализация системы примеров использования MTProxy
*/

#include "mtproxy-examples.h"

// Объявления функций
size_t strlen(const char *s);

// Глобальная система примеров
static mtproxy_examples_t *g_examples = 0;
static uint64_t g_example_id_counter = 1;

// Вспомогательные функции
static void initialize_basic_examples(mtproxy_examples_t *examples);
static void initialize_advanced_examples(mtproxy_examples_t *examples);
static void initialize_security_examples(mtproxy_examples_t *examples);
static void initialize_performance_examples(mtproxy_examples_t *examples);
static int validate_example_code(const char *code);
static int validate_example_config(const char *config);

// Инициализация
mtproxy_examples_t* examples_init(int max_collections, int max_examples_per_collection) {
    mtproxy_examples_t *examples = (mtproxy_examples_t*)0x200000000;
    if (!examples) {
        return 0;
    }
    
    // Обнуление структуры
    for (int i = 0; i < sizeof(mtproxy_examples_t); i++) {
        ((char*)examples)[i] = 0;
    }
    
    // Конфигурация по умолчанию
    examples->config.enable_compilation_examples = 1;
    examples->config.enable_runtime_examples = 1;
    examples->config.enable_security_examples = 1;
    examples->config.enable_performance_examples = 1;
    examples->config.show_detailed_output = 1;
    
    const char *default_dir = "./examples_output";
    for (int i = 0; i < 255 && default_dir[i] != '\0'; i++) {
        examples->config.output_directory[i] = default_dir[i];
    }
    examples->config.output_directory[255] = '\0';
    
    // Выделение памяти для коллекций
    examples->max_collections = max_collections > 0 ? max_collections : 8;
    examples->collections = (example_collection_t*)0x210000000;
    if (examples->collections) {
        for (int i = 0; i < sizeof(example_collection_t) * examples->max_collections; i++) {
            ((char*)examples->collections)[i] = 0;
        }
    }
    
    examples->is_initialized = 1;
    examples->creation_time = 0; // Будет реальное время
    
    // Инициализация стандартных примеров
    initialize_basic_examples(examples);
    initialize_advanced_examples(examples);
    initialize_security_examples(examples);
    initialize_performance_examples(examples);
    
    g_examples = examples;
    return examples;
}

// Конфигурация
int examples_configure(mtproxy_examples_t *examples, const examples_config_t *config) {
    if (!examples || !config) return -1;
    
    examples->config = *config;
    return 0;
}

// Очистка
void examples_cleanup(mtproxy_examples_t *examples) {
    if (!examples) return;
    
    examples->is_initialized = 0;
    
    if (g_examples == examples) {
        g_examples = 0;
    }
}

// Создание коллекции
int examples_create_collection(mtproxy_examples_t *examples, 
                             const char *name, const char *description) {
    if (!examples || !name || !description || examples->collection_count >= examples->max_collections) {
        return -1;
    }
    
    example_collection_t *collection = &examples->collections[examples->collection_count];
    
    collection->example_count = 0;
    collection->max_examples = 32; // Фиксированный размер
    
    // Выделение памяти для примеров
    collection->examples = (usage_example_t*)0x220000000 + (examples->collection_count * sizeof(usage_example_t) * 32);
    if (collection->examples) {
        for (int i = 0; i < sizeof(usage_example_t) * collection->max_examples; i++) {
            ((char*)collection->examples)[i] = 0;
        }
    }
    
    // Копирование имени
    for (int i = 0; i < 63 && name[i] != '\0'; i++) {
        collection->collection_name[i] = name[i];
    }
    collection->collection_name[63] = '\0';
    
    // Копирование описания
    for (int i = 0; i < 127 && description[i] != '\0'; i++) {
        collection->collection_description[i] = description[i];
    }
    collection->collection_description[127] = '\0';
    
    examples->collection_count++;
    return 0;
}

// Добавление примера
int examples_add_example(mtproxy_examples_t *examples, uint64_t collection_id,
                        const char *name, const char *description,
                        example_type_t type, usage_scenario_t scenario,
                        const char *code_sample, const char *configuration) {
    if (!examples || !name || !description) return -1;
    
    example_collection_t *collection = examples_get_collection(examples, collection_id);
    if (!collection || collection->example_count >= collection->max_examples) return -1;
    
    usage_example_t *example = &collection->examples[collection->example_count];
    
    example->example_id = g_example_id_counter++;
    example->type = type;
    example->scenario = scenario;
    example->difficulty_level = 1;
    example->estimated_time_minutes = 5;
    
    // Копирование данных
    for (int i = 0; i < 63 && name[i] != '\0'; i++) {
        example->name[i] = name[i];
    }
    example->name[63] = '\0';
    
    for (int i = 0; i < 255 && description[i] != '\0'; i++) {
        example->description[i] = description[i];
    }
    example->description[255] = '\0';
    
    if (code_sample) {
        for (int i = 0; i < 2047 && code_sample[i] != '\0'; i++) {
            example->code_sample[i] = code_sample[i];
        }
        example->code_sample[2047] = '\0';
    }
    
    if (configuration) {
        for (int i = 0; i < 1023 && configuration[i] != '\0'; i++) {
            example->configuration[i] = configuration[i];
        }
        example->configuration[1023] = '\0';
    }
    
    const char *expected = "Example executed successfully";
    for (int i = 0; i < 511 && expected[i] != '\0'; i++) {
        example->expected_output[i] = expected[i];
    }
    example->expected_output[511] = '\0';
    
    collection->example_count++;
    return 0;
}

// Удаление примера
int examples_remove_example(mtproxy_examples_t *examples, uint64_t example_id) {
    if (!examples) return -1;
    
    for (int i = 0; i < examples->collection_count; i++) {
        example_collection_t *collection = &examples->collections[i];
        for (int j = 0; j < collection->example_count; j++) {
            if (collection->examples[j].example_id == example_id) {
                // Сдвиг остальных примеров
                for (int k = j; k < collection->example_count - 1; k++) {
                    collection->examples[k] = collection->examples[k + 1];
                }
                collection->example_count--;
                return 0;
            }
        }
    }
    
    return -1;
}

// Обновление примера
int examples_update_example(mtproxy_examples_t *examples, uint64_t example_id,
                           const char *code_sample, const char *configuration) {
    if (!examples) return -1;
    
    usage_example_t *example = examples_get_example(examples, example_id);
    if (!example) return -1;
    
    if (code_sample) {
        for (int i = 0; i < 2047 && code_sample[i] != '\0'; i++) {
            example->code_sample[i] = code_sample[i];
        }
        example->code_sample[2047] = '\0';
    }
    
    if (configuration) {
        for (int i = 0; i < 1023 && configuration[i] != '\0'; i++) {
            example->configuration[i] = configuration[i];
        }
        example->configuration[1023] = '\0';
    }
    
    return 0;
}

// Запуск примеров
int examples_run_single_example(mtproxy_examples_t *examples, uint64_t example_id) {
    if (!examples) return -1;
    
    usage_example_t *example = examples_get_example(examples, example_id);
    if (!example) return -1;
    
    // В реальной реализации:
    // 1. Компиляция примера
    // 2. Запуск с тестовыми данными
    // 3. Проверка результата
    // 4. Сравнение с ожидаемым выводом
    
    int success = 1; // Предполагаем успех
    
    if (examples->on_example_run) {
        examples->on_example_run(example, success);
    }
    
    if (examples->on_example_complete) {
        const char *output = "Example completed successfully";
        examples->on_example_complete(example, output);
    }
    
    return success ? 0 : -1;
}

int examples_run_collection_examples(mtproxy_examples_t *examples, uint64_t collection_id) {
    if (!examples) return -1;
    
    example_collection_t *collection = examples_get_collection(examples, collection_id);
    if (!collection) return -1;
    
    int failed_count = 0;
    for (int i = 0; i < collection->example_count; i++) {
        if (examples_run_single_example(examples, collection->examples[i].example_id) != 0) {
            failed_count++;
        }
    }
    
    return failed_count;
}

int examples_run_all_examples(mtproxy_examples_t *examples) {
    if (!examples) return -1;
    
    int total_failed = 0;
    for (int i = 0; i < examples->collection_count; i++) {
        total_failed += examples_run_collection_examples(examples, i + 1);
    }
    
    return total_failed;
}

int examples_run_examples_by_type(mtproxy_examples_t *examples, example_type_t type) {
    if (!examples) return -1;
    
    int failed_count = 0;
    for (int i = 0; i < examples->collection_count; i++) {
        example_collection_t *collection = &examples->collections[i];
        for (int j = 0; j < collection->example_count; j++) {
            if (collection->examples[j].type == type) {
                if (examples_run_single_example(examples, collection->examples[j].example_id) != 0) {
                    failed_count++;
                }
            }
        }
    }
    
    return failed_count;
}

int examples_run_examples_by_scenario(mtproxy_examples_t *examples, usage_scenario_t scenario) {
    if (!examples) return -1;
    
    int failed_count = 0;
    for (int i = 0; i < examples->collection_count; i++) {
        example_collection_t *collection = &examples->collections[i];
        for (int j = 0; j < collection->example_count; j++) {
            if (collection->examples[j].scenario == scenario) {
                if (examples_run_single_example(examples, collection->examples[j].example_id) != 0) {
                    failed_count++;
                }
            }
        }
    }
    
    return failed_count;
}

// Получение информации
usage_example_t* examples_get_example(mtproxy_examples_t *examples, uint64_t example_id) {
    if (!examples) return 0;
    
    for (int i = 0; i < examples->collection_count; i++) {
        example_collection_t *collection = &examples->collections[i];
        for (int j = 0; j < collection->example_count; j++) {
            if (collection->examples[j].example_id == example_id) {
                return &collection->examples[j];
            }
        }
    }
    
    return 0;
}

example_collection_t* examples_get_collection(mtproxy_examples_t *examples, uint64_t collection_id) {
    if (!examples || collection_id == 0 || collection_id > examples->collection_count) {
        return 0;
    }
    
    return &examples->collections[collection_id - 1];
}

void examples_list_collections(mtproxy_examples_t *examples, char *buffer, size_t buffer_size) {
    if (!examples || !buffer || buffer_size < 100) return;
    
    size_t pos = 0;
    const char *header = "Available collections:\n";
    for (size_t i = 0; header[i] && pos < buffer_size - 1; i++, pos++) {
        buffer[pos] = header[i];
    }
    
    for (int i = 0; i < examples->collection_count && pos < buffer_size - 50; i++) {
        const char *format = "- %s (%d examples)\n";
        // Упрощенное форматирование
        const char *name = examples->collections[i].collection_name;
        for (size_t j = 0; name[j] && pos < buffer_size - 20; j++, pos++) {
            buffer[pos] = name[j];
        }
        buffer[pos++] = '\n';
    }
    
    buffer[pos < buffer_size ? pos : buffer_size - 1] = '\0';
}

void examples_list_examples(mtproxy_examples_t *examples, uint64_t collection_id, 
                           char *buffer, size_t buffer_size) {
    if (!examples || !buffer || buffer_size < 100) return;
    
    example_collection_t *collection = examples_get_collection(examples, collection_id);
    if (!collection) {
        const char *error = "Collection not found";
        for (size_t i = 0; error[i] && i < buffer_size - 1; i++) {
            buffer[i] = error[i];
        }
        buffer[buffer_size - 1] = '\0';
        return;
    }
    
    size_t pos = 0;
    const char *header = "Examples in collection:\n";
    for (size_t i = 0; header[i] && pos < buffer_size - 1; i++, pos++) {
        buffer[pos] = header[i];
    }
    
    for (int i = 0; i < collection->example_count && pos < buffer_size - 100; i++) {
        const char *name = collection->examples[i].name;
        for (size_t j = 0; name[j] && pos < buffer_size - 10; j++, pos++) {
            buffer[pos] = name[j];
        }
        buffer[pos++] = '\n';
    }
    
    buffer[pos < buffer_size ? pos : buffer_size - 1] = '\0';
}

// Генерация кода
int examples_generate_compilation_script(mtproxy_examples_t *examples, uint64_t example_id,
                                        const char *output_file) {
    if (!examples || !output_file) return -1;
    
    usage_example_t *example = examples_get_example(examples, example_id);
    if (!example) return -1;
    
    // В реальной реализации генерация скрипта компиляции
    return 0;
}

int examples_generate_configuration_file(mtproxy_examples_t *examples, uint64_t example_id,
                                        const char *output_file) {
    if (!examples || !output_file) return -1;
    
    usage_example_t *example = examples_get_example(examples, example_id);
    if (!example) return -1;
    
    // В реальной реализации генерация конфигурационного файла
    return 0;
}

int examples_generate_docker_compose(mtproxy_examples_t *examples, uint64_t example_id,
                                    const char *output_file) {
    if (!examples || !output_file) return -1;
    
    usage_example_t *example = examples_get_example(examples, example_id);
    if (!example) return -1;
    
    // В реальной реализации генерация docker-compose.yml
    return 0;
}

// Экспорт и импорт
int examples_export_collection(mtproxy_examples_t *examples, uint64_t collection_id,
                              const char *filename) {
    if (!examples || !filename) return -1;
    
    // В реальной реализации экспорт в файл
    return 0;
}

int examples_import_collection(mtproxy_examples_t *examples, const char *filename) {
    if (!examples || !filename) return -1;
    
    // В реальной реализации импорт из файла
    return 0;
}

int examples_export_all_examples(mtproxy_examples_t *examples, const char *filename) {
    if (!examples || !filename) return -1;
    
    // В реальной реализации экспорт всех примеров
    return 0;
}

// Вспомогательные функции
const char* examples_type_to_string(example_type_t type) {
    switch (type) {
        case EXAMPLE_TYPE_BASIC: return "BASIC";
        case EXAMPLE_TYPE_ADVANCED: return "ADVANCED";
        case EXAMPLE_TYPE_SECURITY: return "SECURITY";
        case EXAMPLE_TYPE_PERFORMANCE: return "PERFORMANCE";
        case EXAMPLE_TYPE_INTEGRATION: return "INTEGRATION";
        default: return "UNKNOWN";
    }
}

const char* examples_scenario_to_string(usage_scenario_t scenario) {
    switch (scenario) {
        case SCENARIO_SIMPLE_PROXY: return "SIMPLE_PROXY";
        case SCENARIO_LOAD_BALANCING: return "LOAD_BALANCING";
        case SCENARIO_HIGH_AVAILABILITY: return "HIGH_AVAILABILITY";
        case SCENARIO_SECURITY_HARDENED: return "SECURITY_HARDENED";
        case SCENARIO_PERFORMANCE_TUNING: return "PERFORMANCE_TUNING";
        default: return "UNKNOWN";
    }
}

const char* examples_difficulty_to_string(int difficulty) {
    switch (difficulty) {
        case 1: return "BEGINNER";
        case 2: return "EASY";
        case 3: return "MEDIUM";
        case 4: return "HARD";
        case 5: return "EXPERT";
        default: return "UNKNOWN";
    }
}

uint64_t examples_generate_example_id(void) {
    return g_example_id_counter++;
}

int examples_validate_example(usage_example_t *example) {
    if (!example) return 0;
    
    if (strlen(example->name) == 0) return 0;
    if (strlen(example->description) == 0) return 0;
    if (strlen(example->code_sample) == 0) return 0;
    
    return 1;
}

// Примеры для разных сценариев

void example_basic_proxy_setup(mtproxy_examples_t *examples) {
    const char *code = 
        "#include \"mtproxy-enhanced.h\"\n\n"
        "int main() {\n"
        "    // Инициализация MTProxy\n"
        "    mtproxy_config_t config = {0};\n"
        "    config.port = 8080;\n"
        "    strcpy(config.proxy_secret, \"your_secret_here\");\n\n"
        "    mtproxy_init(&config);\n"
        "    mtproxy_start();\n\n"
        "    return 0;\n"
        "}";
    
    const char *config = 
        "{\n"
        "    \"port\": 8080,\n"
        "    \"proxy_secret\": \"your_secret_here\",\n"
        "    \"workers\": 4\n"
        "}";
    
    examples_add_example(examples, 1, "Basic Proxy Setup", 
                        "Simple MTProxy configuration and startup",
                        EXAMPLE_TYPE_BASIC, SCENARIO_SIMPLE_PROXY,
                        code, config);
}

void example_load_balancing_setup(mtproxy_examples_t *examples) {
    const char *code = 
        "#include \"load-balancer.h\"\n\n"
        "int main() {\n"
        "    // Настройка балансировки нагрузки\n"
        "    load_balancer_config_t lb_config = {0};\n"
        "    lb_config.algorithm = LB_ALGORITHM_ROUND_ROBIN;\n"
        "    lb_config.servers_count = 3;\n\n"
        "    // Добавление серверов\n"
        "    load_balancer_add_server(\"192.168.1.10\", 8080);\n"
        "    load_balancer_add_server(\"192.168.1.11\", 8080);\n"
        "    load_balancer_add_server(\"192.168.1.12\", 8080);\n\n"
        "    load_balancer_init(&lb_config);\n"
        "    load_balancer_start();\n\n"
        "    return 0;\n"
        "}";
    
    examples_add_example(examples, 2, "Load Balancing Setup",
                        "Configure load balancing with multiple backend servers",
                        EXAMPLE_TYPE_ADVANCED, SCENARIO_LOAD_BALANCING,
                        code, NULL);
}

void example_high_availability_setup(mtproxy_examples_t *examples) {
    const char *code = 
        "#include \"auto-scaling.h\"\n\n"
        "int main() {\n"
        "    // Настройка высокой доступности\n"
        "    auto_scaling_config_t as_config = {0};\n"
        "    as_config.min_instances = 2;\n"
        "    as_config.max_instances = 10;\n"
        "    as_config.scale_up_threshold = 80;\n"
        "    as_config.scale_down_threshold = 30;\n\n"
        "    auto_scaling_init(&as_config);\n"
        "    auto_scaling_enable();\n\n"
        "    return 0;\n"
        "}";
    
    examples_add_example(examples, 3, "High Availability Setup",
                        "Configure auto-scaling for high availability",
                        EXAMPLE_TYPE_ADVANCED, SCENARIO_HIGH_AVAILABILITY,
                        code, NULL);
}

void example_security_hardened_setup(mtproxy_examples_t *examples) {
    const char *code = 
        "#include \"security-enhanced.h\"\n\n"
        "int main() {\n"
        "    // Усиленная конфигурация безопасности\n"
        "    security_config_t sec_config = {0};\n"
        "    sec_config.enable_ddos_protection = 1;\n"
        "    sec_config.enable_buffer_overflow_protection = 1;\n"
        "    sec_config.enable_rate_limiting = 1;\n"
        "    sec_config.max_connections_per_ip = 100;\n\n"
        "    security_init(&sec_config);\n"
        "    security_apply_hardening();\n\n"
        "    return 0;\n"
        "}";
    
    examples_add_example(examples, 4, "Security Hardened Setup",
                        "Configure enhanced security features",
                        EXAMPLE_TYPE_SECURITY, SCENARIO_SECURITY_HARDENED,
                        code, NULL);
}

void example_performance_tuning_setup(mtproxy_examples_t *examples) {
    const char *code = 
        "#include \"performance-optimizer.h\"\n\n"
        "int main() {\n"
        "    // Настройка оптимизации производительности\n"
        "    performance_config_t perf_config = {0};\n"
        "    perf_config.enable_numa_optimization = 1;\n"
        "    perf_config.enable_thread_pooling = 1;\n"
        "    perf_config.enable_memory_pooling = 1;\n"
        "    perf_config.workers_count = 8;\n\n"
        "    performance_init(&perf_config);\n"
        "    performance_optimize();\n\n"
        "    return 0;\n"
        "}";
    
    examples_add_example(examples, 5, "Performance Tuning Setup",
                        "Configure performance optimization settings",
                        EXAMPLE_TYPE_PERFORMANCE, SCENARIO_PERFORMANCE_TUNING,
                        code, NULL);
}

// Регистрация callback функций
void examples_set_example_run_callback(mtproxy_examples_t *examples,
                                      void (*callback)(usage_example_t*, int)) {
    if (examples) examples->on_example_run = callback;
}

void examples_set_example_complete_callback(mtproxy_examples_t *examples,
                                           void (*callback)(usage_example_t*, const char*)) {
    if (examples) examples->on_example_complete = callback;
}

// Вспомогательные функции
static void initialize_basic_examples(mtproxy_examples_t *examples) {
    examples_create_collection(examples, "Basic Examples", "Fundamental MTProxy usage examples");
    example_basic_proxy_setup(examples);
}

static void initialize_advanced_examples(mtproxy_examples_t *examples) {
    examples_create_collection(examples, "Advanced Examples", "Advanced configuration and features");
    example_load_balancing_setup(examples);
    example_high_availability_setup(examples);
}

static void initialize_security_examples(mtproxy_examples_t *examples) {
    examples_create_collection(examples, "Security Examples", "Security hardening examples");
    example_security_hardened_setup(examples);
}

static void initialize_performance_examples(mtproxy_examples_t *examples) {
    examples_create_collection(examples, "Performance Examples", "Performance optimization examples");
    example_performance_tuning_setup(examples);
}

static int validate_example_code(const char *code) {
    // В реальной реализации валидация кода
    return code && strlen(code) > 0 ? 1 : 0;
}

static int validate_example_config(const char *config) {
    // В реальной реализации валидация конфигурации
    return config && strlen(config) > 0 ? 1 : 0;
}