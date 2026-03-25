# Plugin System Documentation

## Обзор

Система плагинов MTProxy позволяет расширять функциональность прокси-сервера без изменения основного кода. Плагины загружаются динамически и могут вмешиваться в различные этапы обработки соединений через систему хуков.

## Возможности

- **Динамическая загрузка** — загрузка .so файлов во время выполнения
- **Горячая перезагрузка** — обновление плагинов без остановки сервера
- **Система хуков** — 20+ точек расширения
- **Приоритеты** — контроль порядка выполнения хуков
- **Безопасность** — изоляция плагинов и проверка версий
- **Статистика** — мониторинг производительности плагинов

## Быстрый старт

### 1. Создание плагина

```c
#include <plugin-system.h>

// Данные плагина
typedef struct {
    int connections_count;
} my_plugin_data_t;

// Хук на подключение клиента
static plugin_result_t on_connection_accept(plugin_hook_context_t *ctx, 
                                            void *plugin_data) {
    my_plugin_data_t *data = (my_plugin_data_t *)plugin_data;
    data->connections_count++;
    
    printf("New connection from %s:%d\n", ctx->client_ip, ctx->client_port);
    
    return PLUGIN_OK;  // Продолжить цепочку
}

// Информация о плагине
PLUGIN_DECLARE_INFO(
    "my-plugin",
    "My custom plugin",
    "1.0.0",
    "Your Name",
    "MIT"
)

// Инициализация
int plugin_init(const plugin_config_t *config, void **plugin_data) {
    my_plugin_data_t *data = calloc(1, sizeof(my_plugin_data_t));
    *plugin_data = data;
    
    // Регистрация хуков
    plugin_register_hook(HOOK_CONNECTION_ACCEPT, on_connection_accept, 0, data);
    
    return PLUGIN_OK;
}

// Завершение
void plugin_shutdown(void *plugin_data) {
    my_plugin_data_t *data = (my_plugin_data_t *)plugin_data;
    printf("Total connections: %d\n", data->connections_count);
    free(data);
}
```

### 2. Компиляция

```bash
gcc -shared -fPIC -o my-plugin.so my-plugin.c
```

### 3. Установка

```bash
cp my-plugin.so /usr/lib/mtproxy/plugins/
```

### 4. Конфигурация

Создайте файл `/etc/mtproxy/plugins.conf`:

```ini
[plugins]
load = my-plugin.so
load = example-logger.so

[my-plugin]
enabled = true
priority = 10

[example-logger]
enabled = true
log_file = /var/log/mtproxy/plugin.log
verbose = true
```

### 5. Запуск

```bash
./mtproto-proxy --plugins /usr/lib/mtproxy/plugins/ \
                --plugin-config /etc/mtproxy/plugins.conf \
                ...остальные параметры...
```

## Точки расширения (Хуки)

### Инициализация

| Хук | Описание | Когда вызывается |
|-----|----------|------------------|
| `HOOK_PLUGIN_INIT` | Инициализация плагина | При загрузке плагина |
| `HOOK_PLUGIN_SHUTDOWN` | Завершение плагина | При выгрузке плагина |

### Сетевые хуки

| Хук | Описание | Когда вызывается |
|-----|----------|------------------|
| `HOOK_CONNECTION_ACCEPT` | Новое подключение | При принятии соединения |
| `HOOK_CONNECTION_CLOSE` | Закрытие подключения | При закрытии соединения |
| `HOOK_DATA_RECEIVED` | Получение данных | При получении данных от клиента |
| `HOOK_DATA_SENT` | Отправка данных | После отправки данных клиенту |

### MTProto хуки

| Хук | Описание | Когда вызывается |
|-----|----------|------------------|
| `HOOK_MTPROTO_HANDSHAKE` | Handshake | Во время рукопожатия MTProto |
| `HOOK_MTPROTO_ENCRYPT` | Шифрование | Перед шифрованием пакета |
| `HOOK_MTPROTO_DECRYPT` | Расшифровка | После расшифровки пакета |
| `HOOK_MTPROTO_VALIDATE` | Валидация | При валидации пакета |

### Безопасность

| Хук | Описание | Когда вызывается |
|-----|----------|------------------|
| `HOOK_SECURITY_CHECK` | Проверка безопасности | Перед обработкой запроса |
| `HOOK_RATE_LIMIT_CHECK` | Rate limiting | При проверке лимитов |
| `HOOK_AUTH_CHECK` | Аутентификация | При проверке токена |

### Конфигурация

| Хук | Описание | Когда вызывается |
|-----|----------|------------------|
| `HOOK_CONFIG_LOAD` | Загрузка конфигурации | При загрузке конфига |
| `HOOK_CONFIG_RELOAD` | Перезагрузка конфигурации | При hot-reload |

### Статистика и логи

| Хук | Описание | Когда вызывается |
|-----|----------|------------------|
| `HOOK_STATS_COLLECT` | Сбор статистики | Периодически |
| `HOOK_STATS_REPORT` | Отчет статистики | При запросе статистики |
| `HOOK_LOG_MESSAGE` | Логирование | При записи в лог |

## Коды возврата

| Код | Значение | Описание |
|-----|----------|----------|
| `PLUGIN_OK` (0) | Успех | Продолжить цепочку |
| `PLUGIN_ERROR` (-1) | Ошибка | Прервать выполнение |
| `PLUGIN_SKIP` (1) | Пропустить | Пропустить следующий плагин |
| `PLUGIN_STOP` (2) | Остановить | Остановить цепочку хуков |
| `PLUGIN_REJECT` (3) | Отклонить | Отклонить запрос (для accept хуков) |

## Контекст хука

```c
typedef struct {
    plugin_hook_type_t hook_type;   // Тип хука
    void *data;                     // Данные (зависит от типа хука)
    size_t data_size;               // Размер данных
    void *user_data;                // Пользовательские данные
    void *plugin_data;              // Данные плагина
    int connection_fd;              // FD подключения
    char client_ip[64];             // IP клиента
    uint16_t client_port;           // Порт клиента
    uint64_t timestamp;             // Временная метка
    uint64_t sequence;              // Порядковый номер
    int priority;                   // Приоритет
    char result_data[1024];         // Результаты обработки
    int result_code;                // Код результата
} plugin_hook_context_t;
```

## Примеры использования

### 1. Логирование подключений

```c
static plugin_result_t on_connection_accept(plugin_hook_context_t *ctx, 
                                            void *plugin_data) {
    FILE *log = (FILE *)plugin_data;
    
    fprintf(log, "[%lu] CONNECT %s:%d fd=%d\n",
            (unsigned long)ctx->timestamp,
            ctx->client_ip,
            ctx->client_port,
            ctx->connection_fd);
    fflush(log);
    
    return PLUGIN_OK;
}
```

### 2. Блокировка по IP

```c
static plugin_result_t on_security_check(plugin_hook_context_t *ctx, 
                                         void *plugin_data) {
    // Блокировка определенного IP
    if (strcmp(ctx->client_ip, "192.168.1.100") == 0) {
        ctx->result_code = 403;
        strncpy(ctx->result_data, "IP blocked", sizeof(ctx->result_data) - 1);
        return PLUGIN_REJECT;
    }
    
    return PLUGIN_OK;
}
```

### 3. Rate Limiting

```c
static plugin_result_t on_rate_limit_check(plugin_hook_context_t *ctx, 
                                           void *plugin_data) {
    rate_limiter_t *limiter = (rate_limiter_t *)plugin_data;
    
    if (!rate_limiter_allow(limiter, ctx->client_ip)) {
        ctx->result_code = 429;
        strncpy(ctx->result_data, "Rate limit exceeded", 
                sizeof(ctx->result_data) - 1);
        return PLUGIN_REJECT;
    }
    
    return PLUGIN_OK;
}
```

### 4. Модификация данных

```c
static plugin_result_t on_data_received(plugin_hook_context_t *ctx, 
                                        void *plugin_data) {
    // Добавление префикса к данным
    if (ctx->data && ctx->data_size > 0) {
        unsigned char *new_data = malloc(ctx->data_size + 4);
        memcpy(new_data, "\x00\x00\x00\x00", 4);  // Префикс
        memcpy(new_data + 4, ctx->data, ctx->data_size);
        
        ctx->data = new_data;
        ctx->data_size += 4;
    }
    
    return PLUGIN_OK;
}
```

### 5. Шифрование MTProto

```c
static plugin_result_t on_mtpROTO_encrypt(plugin_hook_context_t *ctx, 
                                          void *plugin_data) {
    // Дополнительное шифрование данных
    unsigned char *data = (unsigned char *)ctx->data;
    size_t size = ctx->data_size;
    
    // Применить дополнительное шифрование
    custom_encrypt(data, size, ctx->plugin_data);
    
    return PLUGIN_OK;
}
```

## API Менеджера плагинов

### Инициализация

```c
plugin_manager_t manager;
plugin_manager_config_t config = {0};

strncpy(config.plugin_dir, "/usr/lib/mtproxy/plugins/", sizeof(config.plugin_dir) - 1);
config.auto_load = true;
config.hot_reload = true;
config.max_plugins = 32;
config.enable_stats = true;
config.enable_logging = true;

plugin_manager_init(&manager, &config);
plugin_manager_start(&manager);
```

### Загрузка плагина

```c
// Загрузка одного плагина
plugin_manager_load(&manager, "/path/to/plugin.so");

// Загрузка всех плагинов из директории
plugin_manager_load_all(&manager);
```

### Выполнение хука

```c
plugin_hook_context_t ctx = {0};
ctx.hook_type = HOOK_CONNECTION_ACCEPT;
ctx.connection_fd = client_fd;
ctx.timestamp = get_timestamp_ms();
strncpy(ctx.client_ip, client_ip, sizeof(ctx.client_ip) - 1);
ctx.client_port = client_port;

plugin_manager_execute_hook(&manager, HOOK_CONNECTION_ACCEPT, &ctx);
```

### Статистика

```c
plugin_manager_stats_t stats;
plugin_manager_get_stats(&manager, &stats);

printf("Loaded plugins: %d\n", stats.loaded_plugins);
printf("Total hook calls: %lu\n", stats.total_hook_calls);
```

## Отладка плагинов

### Логирование

```c
void plugin_log(plugin_handle_t *plugin, int level, const char *format, ...) {
    va_list args;
    va_start(args, level);
    
    fprintf(stderr, "[%s] ", plugin->info.name);
    
    switch (level) {
        case 0: fprintf(stderr, "[ERROR] "); break;
        case 1: fprintf(stderr, "[WARN] "); break;
        case 2: fprintf(stderr, "[INFO] "); break;
        case 3: fprintf(stderr, "[DEBUG] "); break;
    }
    
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    
    va_end(args);
}

// Использование
plugin_log(plugin, 2, "Connection from %s:%d", ip, port);
```

### Проверка памяти

```bash
# Запуск с valgrind
valgrind --leak-check=full --show-leak-kinds=all \
         ./mtproto-proxy --plugins /path/to/plugins
```

## Best Practices

### 1. Обработка ошибок

```c
int plugin_init(const plugin_config_t *config, void **plugin_data) {
    if (!plugin_data) {
        return PLUGIN_ERROR;
    }
    
    void *data = calloc(1, sizeof(my_plugin_data_t));
    if (!data) {
        fprintf(stderr, "Failed to allocate memory\n");
        return PLUGIN_ERROR;
    }
    
    *plugin_data = data;
    return PLUGIN_OK;
}
```

### 2. Очистка ресурсов

```c
void plugin_shutdown(void *plugin_data) {
    my_plugin_data_t *data = (my_plugin_data_t *)plugin_data;
    
    if (data->file) {
        fclose(data->file);
    }
    if (data->buffer) {
        free(data->buffer);
    }
    
    free(data);
}
```

### 3. Потокобезопасность

```c
typedef struct {
    pthread_mutex_t mutex;
    int counter;
} my_plugin_data_t;

static plugin_result_t on_hook(plugin_hook_context_t *ctx, void *plugin_data) {
    my_plugin_data_t *data = (my_plugin_data_t *)plugin_data;
    
    pthread_mutex_lock(&data->mutex);
    data->counter++;
    pthread_mutex_unlock(&data->mutex);
    
    return PLUGIN_OK;
}

int plugin_init(...) {
    // ...
    pthread_mutex_init(&data->mutex, NULL);
    // ...
}

void plugin_shutdown(...) {
    my_plugin_data_t *data = (my_plugin_data_t *)plugin_data;
    pthread_mutex_destroy(&data->mutex);
    // ...
}
```

### 4. Версионирование

```c
// Проверка версии API
if (!plugin_check_api_version(MTPLUGIN_API_VERSION)) {
    fprintf(stderr, "API version mismatch\n");
    return PLUGIN_ERROR;
}
```

## Переменные окружения

| Переменная | Описание | По умолчанию |
|------------|----------|--------------|
| `MTPROXY_PLUGIN_DIR` | Директория плагинов | `/usr/lib/mtproxy/plugins/` |
| `MTPROXY_PLUGIN_CONFIG` | Файл конфигурации | `/etc/mtproxy/plugins.conf` |
| `MTPROXY_PLUGIN_LOG` | Файл логов плагинов | `/var/log/mtproxy/plugins.log` |
| `MTPROXY_PLUGIN_DEBUG` | Режим отладки | `0` |

## Известные ограничения

1. **Один процесс** — плагины работают в том же процессе что и сервер
2. **Нет песочницы** — плагины имеют полный доступ к памяти
3. **Совместимость ABI** — плагины должны быть скомпилированы с той же версией API
4. **Сигналы** — плагины не должны перехватывать сигналы

## Поддержка

- Документация: `docs/PLUGIN_API.md`
- Примеры: `plugins/`
- Issues: https://github.com/QuadDarv1ne/MTProxy/issues
