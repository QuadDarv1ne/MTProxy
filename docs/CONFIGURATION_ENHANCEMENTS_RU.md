# Улучшения системы конфигурации MTProxy

## Обзор

Система управления конфигурацией MTProxy была значительно улучшена для поддержки современных требований к управлению конфигурацией высоконагруженных систем.

## Новые возможности

### 1. Расширенные типы параметров

Добавлены новые типы параметров:
- `CONFIG_TYPE_ARRAY` - массивы значений
- `CONFIG_TYPE_OBJECT` - вложенные объекты конфигурации

```c
// Регистрация параметра типа array
config_manager_register_parameter("network", "allowed_ports", 
                                  CONFIG_TYPE_ARRAY, &ports, 
                                  sizeof(ports), 1, "80,443,8080",
                                  "Разрешенные порты");
```

### 2. Система callback'ов

Подписка на изменения конфигурации:

```c
void on_config_change(const char *section, const char *param_name,
                     enum config_change_event event, void *user_data) {
    printf("Config changed: %s.%s, event: %d\n", section, param_name, event);
}

// Регистрация callback
config_manager_register_callback("network", "max_connections",
                                on_config_change, NULL);
```

### 3. История изменений

Автоматическое отслеживание всех изменений конфигурации:

```c
struct config_change_entry entries[100];
int count = config_manager_get_change_history(entries, 100, 
                                              start_time, end_time);

for (int i = 0; i < count; i++) {
    printf("[%ld] %s.%s: %s -> %s\n", 
           entries[i].timestamp,
           entries[i].section,
           entries[i].parameter,
           entries[i].old_value,
           entries[i].new_value);
}
```

### 4. JSON экспорт/импорт

Сохранение и загрузка конфигурации в формате JSON:

```c
// Экспорт в JSON
config_manager_export_to_json("/etc/mtproxy/config.json", 0);

// Импорт из JSON
config_manager_import_from_json("/etc/mtproxy/config.json");

// Экспорт в строку
char *json = config_manager_export_to_json_string(0);
printf("%s\n", json);
free(json);
```

### 5. Горячая перезагрузка

Изменение конфигурации без перезапуска:

```c
// Проверка необходимости перезагрузки
if (config_manager_is_reload_needed()) {
    // Горячая перезагрузка
    config_manager_hot_reload("/etc/mtproxy/new.conf");
}

// Отложенная перезагрузка
config_manager_schedule_reload(30); // Через 30 секунд
```

### 6. Batch режим

Группировка изменений для атомарного применения:

```c
config_manager_begin_batch();

// Множественные изменения
config_manager_set_parameter_string("network", "max_connections", "10000");
config_manager_set_parameter_string("network", "timeout", "30");
config_manager_set_parameter_string("security", "ddos_threshold", "1000");

// Применение всех изменений
config_manager_commit_batch();

// Или откат
// config_manager_rollback_batch();
```

### 7. Валидация параметров

Автоматическая проверка значений:

```c
// Пользовательский валидатор
int validate_port(const void *value) {
    int port = *(int*)value;
    return (port > 0 && port < 65536) ? 0 : -1;
}

// Установка валидатора
config_manager_set_validator("network", "port", validate_port, NULL);

// Проверка перед установкой
if (config_manager_validate_parameter("network", "port", &new_port) == 0) {
    config_manager_set_parameter("network", "port", &new_port, sizeof(int));
}
```

### 8. Версионирование

Отслеживание версий конфигурации:

```c
int version = config_manager_get_config_version();
printf("Current config version: %d\n", version);

// После hot-reload версия увеличивается
config_manager_hot_reload("new.conf");
int new_version = config_manager_get_config_version(); // version + 1
```

## Структуры данных

### config_parameter

```c
struct config_parameter {
    char name[128];              // Имя параметра
    char description[256];       // Описание
    enum config_param_type type; // Тип
    void *value_ptr;             // Указатель на значение
    size_t value_size;           // Размер
    int is_runtime_modifiable;   // Можно ли изменить runtime
    int is_sensitive;            // Чувствительные данные
    char default_value[256];     // Значение по умолчанию
    char min_value[64];          // Минимальное значение
    char max_value[64];          // Максимальное значение
    time_t last_modified;        // Время последнего изменения
    int version;                 // Версия параметра
    
    // Расширенные поля
    char unit[32];                          // Единица измерения
    int (*validator_func)(const void *val); // Валидатор
    void *validator_data;                   // Данные валидатора
    char depends_on[128];                   // Зависимый параметр
    int is_deprecated;                      // Устаревший
    char replacement[128];                  // Замена
};
```

### config_change_history

```c
struct config_change_history {
    struct config_change_entry *entries;
    int entry_count;
    int max_entries;      // До 1000 записей
    int current_index;
};
```

## API функции

### Инициализация

```c
int config_manager_init(const char *config_file_path);
void config_manager_cleanup(void);
```

### Управление параметрами

```c
int config_manager_register_parameter(...);
int config_manager_set_parameter(...);
int config_manager_get_parameter(...);
int config_manager_set_parameter_string(...);
```

### Callback'и

```c
int config_manager_register_callback(...);
int config_manager_unregister_callback(...);
int config_manager_notify_change(...);
```

### История

```c
int config_manager_get_change_history(...);
int config_manager_clear_change_history(void);
int config_manager_get_change_count(...);
```

### JSON операции

```c
int config_manager_export_to_json(...);
int config_manager_import_from_json(...);
char* config_manager_export_to_json_string(...);
int config_manager_import_from_json_string(...);
```

### Валидация

```c
int config_manager_validate_parameter(...);
int config_manager_set_validator(...);
int config_manager_validate_dependencies(void);
```

### Утилиты

```c
int config_manager_backup_config(...);
int config_manager_restore_config(...);
int config_manager_diff_configs(...);
int config_manager_get_config_version(void);
const char* config_manager_get_last_error(void);
```

### Горячая перезагрузка

```c
int config_manager_hot_reload(...);
int config_manager_schedule_reload(...);
int config_manager_is_reload_needed(void);
```

### Batch операции

```c
int config_manager_begin_batch(void);
int config_manager_commit_batch(void);
int config_manager_rollback_batch(void);
int config_manager_is_batch_mode(void);
```

## Примеры использования

### Базовый пример

```c
#include "common/config-manager.h"

int main() {
    // Инициализация
    config_manager_init("/etc/mtproxy.conf");
    
    // Регистрация параметров
    int max_connections = 1000;
    CONFIG_REGISTER_INT("network", "max_connections", 
                       &max_connections, 1, "1000",
                       "Maximum connections");
    
    // Загрузка из файла
    config_manager_load_from_file("/etc/mtproxy.conf");
    
    // Использование
    int value;
    config_manager_get_parameter("network", "max_connections", 
                                &value, sizeof(int));
    
    // Runtime изменение
    int new_value = 2000;
    config_manager_set_parameter("network", "max_connections",
                                &new_value, sizeof(int));
    
    // Статистика
    config_manager_print_stats();
    
    // Очистка
    config_manager_cleanup();
    
    return 0;
}
```

### Пример с callback'ами

```c
typedef struct {
    int reload_needed;
    int changes_count;
} config_context_t;

void config_change_handler(const char *section, const char *param,
                          enum config_change_event event, void *userdata) {
    config_context_t *ctx = (config_context_t*)userdata;
    ctx->changes_count++;
    
    if (event == CONFIG_EVENT_MODIFIED) {
        ctx->reload_needed = 1;
        printf("Config changed: %s.%s\n", section, param);
    }
}

// Использование
config_context_t ctx = {0};
config_manager_register_callback("network", NULL,
                                config_change_handler, &ctx);
```

### Пример с JSON

```c
// Создание конфигурации
config_manager_init(NULL);

// Регистрация и установка параметров
int port = 8080;
config_manager_register_parameter("server", "port", CONFIG_TYPE_INT,
                                 &port, sizeof(int), 1, "8080", "Server port");

// Экспорт в JSON
config_manager_export_to_json("config.json", 0);

// ... позже ...

// Импорт из JSON
config_manager_import_from_json("config.json");

// Получение версии
int version = config_manager_get_config_version();
```

## Статистика

```c
struct config_manager_stats stats;
config_manager_get_stats(&stats);

printf("Total loads: %lld\n", stats.total_config_loads);
printf("Reloads: %lld\n", stats.config_reload_count);
printf("Validation errors: %lld\n", stats.validation_errors);
printf("Runtime changes: %lld\n", stats.runtime_changes);
printf("Cache hits: %lld\n", stats.config_cache_hits);
```

## Лучшие практики

1. **Всегда регистрируйте параметры перед использованием**
2. **Используйте batch режим для множественных изменений**
3. **Устанавливайте callback'и для критичных параметров**
4. **Проверяйте возвращаемые значения функций**
5. **Используйте JSON для персистентности**
6. **Включайте валидацию для всех параметров**
7. **Мониторьте историю изменений**

## Совместимость

- Полная обратная совместимость с существующим кодом
- Поддержка старых форматов конфигурации
- Кроссплатформенность (Linux, Windows, macOS)

## Производительность

- Кэширование конфигурации в памяти
- Минимальные накладные расходы на валидацию
- Асинхронные callback'и не блокируют основной поток
- Оптимизированная хэш-таблица для быстрого поиска

## Безопасность

- Защита чувствительных параметров (passwords, keys)
- Валидация всех входных данных
- Проверка зависимостей между параметрами
- Логирование всех изменений

## См. также

- [ADVANCED_LOGGING_RU.md](ADVANCED_LOGGING_RU.md) - Система логирования
- [PERFORMANCE_OPTIMIZATIONS_RU.md](PERFORMANCE_OPTIMIZATIONS_RU.md) - Оптимизация производительности
- [MODULAR_ARCHITECTURE_RU.md](MODULAR_ARCHITECTURE_RU.md) - Модульная архитектура
