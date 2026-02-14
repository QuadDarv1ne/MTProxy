# Расширенная система логирования MTProxy

## Обзор

Система расширенного логирования предоставляет мощные возможности для отладки, мониторинга и анализа работы MTProxy с поддержкой различных форматов вывода, уровней детализации и асинхронной обработки.

## Архитектура

### Основные компоненты

1. **Advanced Logger** (`advanced-logger.h/.c`) - основной логгер с расширенными возможностями
2. **Structured Logger** (`structured-logger.h/.c`) - структурированное логирование
3. **Log Aggregator** (`log-aggregator.h/.c`) - агрегация и фильтрация логов
4. **Kprintf** (`kprintf.h/.c`) - совместимость с существующими системами

### Уровни логирования

```c
typedef enum {
    LOG_LEVEL_OFF = 0,    // Логирование отключено
    LOG_LEVEL_FATAL,      // Критические ошибки
    LOG_LEVEL_ERROR,      // Ошибки
    LOG_LEVEL_WARN,       // Предупреждения
    LOG_LEVEL_INFO,       // Информационные сообщения
    LOG_LEVEL_DEBUG,      // Отладочная информация
    LOG_LEVEL_TRACE       // Трассировка выполнения
} log_level_t;
```

## Основные функции

### Инициализация
```c
// Конфигурация по умолчанию
advanced_logger_t* logger = logger_init(NULL);

// С пользовательской конфигурацией
logger_config_t config = {
    .min_level = LOG_LEVEL_DEBUG,
    .format = LOG_FORMAT_JSON,
    .output_type = LOG_OUTPUT_FILE,
    .log_file_path = "/var/log/mtproxy.log",
    .enable_rotation = 1,
    .max_file_size_bytes = 50 * 1024 * 1024, // 50MB
    .enable_async_logging = 1
};
advanced_logger_t* logger = logger_init(&config);
```

### Логирование сообщений
```c
// Базовое логирование
logger_info(logger, "network", "Connection established from %s", client_ip);

// Логирование с уровнем ошибки
logger_error(logger, "crypto", "Failed to decrypt packet: %s", error_msg);

// Логирование с информацией о вызове
logger_log_caller(logger, LOG_LEVEL_DEBUG, "performance", 
                  "Processing took %f ms", processing_time);
```

### Макросы для удобства
```c
// Удобные макросы
logger_fatal(logger, "system", "Critical system error occurred");
logger_warn(logger, "security", "Suspicious activity detected from %s", ip);
logger_debug(logger, "crypto", "AES key generated successfully");
logger_trace(logger, "network", "Packet received: size=%d", packet_size);
```

## Форматы вывода

### Простой формат
```
[INFO] network: Connection established
```

### Детализированный формат
```
[2024-01-15 14:30:25] [INFO] [network] Connection established from 192.168.1.100
```

### JSON формат
```json
{"timestamp":"2024-01-15 14:30:25","level":"INFO","component":"network","message":"Connection established"}
```

### Syslog формат
```
<14>Jan 15 14:30:25 network: Connection established
```

## Расширенные возможности

### Ротация логов
```c
// Автоматическая ротация при достижении размера
logger_config_t config = {
    .enable_rotation = 1,
    .max_file_size_bytes = 10 * 1024 * 1024,  // 10MB
    .max_backup_files = 5
};
```

### Асинхронное логирование
```c
// Асинхронная обработка для высокой производительности
logger_config_t config = {
    .enable_async_logging = 1,
    .queue_size = 10000
};
```

### Буферизация
```c
// Буферизованный вывод для снижения I/O операций
logger_config_t config = {
    .enable_buffering = 1,
    .buffer_size = 16384,  // 16KB буфер
    .flush_interval_seconds = 30
};
```

## Специализированные функции логирования

### Hex dump
```c
unsigned char data[32] = {0x01, 0x02, 0x03, /* ... */};
logger_log_hex_dump(logger, LOG_LEVEL_DEBUG, "crypto", data, 32, "Encrypted packet");
```

### Производительность
```c
logger_log_performance(logger, "network", "packet_processing", 
                      2.5, 1024); // 2.5ms, 1024 bytes
```

### События безопасности
```c
logger_log_security_event(logger, "DDOS_ATTEMPT", 
                         "192.168.1.100", "Rate limit exceeded");
```

## Интеграция с существующими системами

### С kprintf
```c
int logger_integrate_with_kprintf(advanced_logger_t *logger);
```

### Со структурированным логгером
```c
int logger_integrate_with_structured_logger(advanced_logger_t *logger);
```

## Конфигурация через переменные окружения

```bash
# Установка уровня логирования
export MT_LOG_LEVEL=DEBUG

# Установка файла логов
export MT_LOG_FILE=/var/log/mtproxy.log

# Включение JSON формата
export MT_LOG_FORMAT=JSON

# Включение асинхронного логирования
export MT_LOG_ASYNC=1
```

## Пример использования

```c
#include "common/advanced-logger.h"

int main() {
    // Инициализация логгера
    logger_config_t config = {
        .min_level = LOG_LEVEL_DEBUG,
        .format = LOG_FORMAT_DETAILED,
        .output_type = LOG_OUTPUT_FILE,
        .log_file_path = "mtproxy.log",
        .enable_rotation = 1,
        .enable_async_logging = 1
    };
    
    advanced_logger_t *logger = logger_init(&config);
    if (!logger) {
        fprintf(stderr, "Failed to initialize logger\n");
        return -1;
    }
    
    // Установка глобального логгера
    logger_set_global_logger(logger);
    
    // Использование логгера
    logger_info(logger, "main", "MTProxy started successfully");
    logger_debug(logger, "config", "Loaded configuration: port=%d", 8080);
    
    // Специализированное логирование
    unsigned char test_data[16] = {0};
    logger_log_hex_dump(logger, LOG_LEVEL_TRACE, "crypto", 
                       test_data, 16, "Test data");
    
    // Печать статистики
    const logger_stats_t *stats = logger_get_stats(logger);
    printf("Total log entries: %lld\n", stats->total_entries);
    
    // Очистка
    logger_cleanup(logger);
    return 0;
}
```

## Производительность

### Ожидаемые характеристики:
- **Асинхронное логирование**: до 100,000 записей/сек
- **Синхронное логирование**: до 10,000 записей/сек
- **Буферизованный вывод**: снижение I/O операций на 80%
- **Ротация логов**: автоматическое управление размером

### Тестирование производительности:
```bash
# Компиляция тестов
make logger-benchmark

# Запуск бенчмарка
./bin/logger-benchmark --entries=100000 --async=1 --buffer-size=16384
```

## Мониторинг и метрики

Система собирает статистику:
- Количество записей по уровням
- Количество отброшенных записей
- Средний размер записи
- Общее время логирования
- Эффективность буферизации

```c
const logger_stats_t* stats = logger_get_stats(logger);
printf("Log entries by level:\n");
for (int i = 0; i < 7; i++) {
    printf("  %s: %lld\n", logger_level_to_string(i), stats->entries_by_level[i]);
}
```

## Отладка и диагностика

### Включение трассировки:
```c
logger_config_t debug_config = {
    .min_level = LOG_LEVEL_TRACE,
    .enable_caller_info = 1,
    .enable_thread_ids = 1
};
```

### Анализ проблем:
```c
// Проверка конфигурации
const logger_config_t *config = logger_get_config(logger);
printf("Current log level: %s\n", logger_level_to_string(config->min_level));

// Сброс статистики для нового анализа
logger_reset_stats(logger);
```

## Совместимость

- **Windows**: Поддержка через MinGW/MSVC
- **Linux**: Полная поддержка syslog
- **macOS**: Поддержка через системный лог
- **FreeBSD**: Совместимость с syslog

## Безопасность

- Защита от переполнения буфера
- Безопасное форматирование строк
- Защита от race condition через мьютексы
- Очистка чувствительных данных