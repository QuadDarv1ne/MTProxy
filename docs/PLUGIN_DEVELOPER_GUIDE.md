# Plugin Developer Guide

Руководство разработчика плагинов для MTProxy.

## Содержание

1. [Архитектура плагинов](#архитектура-плагинов)
2. [Создание плагина](#создание-плагина)
3. [Система хуков](#система-хуков)
4. [Примеры плагинов](#примеры-плагинов)
5. [Отладка плагинов](#отладка-плагинов)
6. [Публикация плагинов](#публикация-плагинов)

---

## Архитектура плагинов

### Обзор

Система плагинов MTProxy позволяет расширять функциональность прокси-сервера без изменения основного кода.

**Компоненты:**
```
┌─────────────────────────────────────────────────┐
│              MTProxy Server                     │
│  ┌─────────────────────────────────────────┐   │
│  │         Plugin Manager                  │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐   │   │
│  │  │ Plugin  │ │ Plugin  │ │ Plugin  │   │   │
│  │  │    1    │ │    2    │ │    3    │   │   │
│  │  └─────────┘ └─────────┘ └─────────┘   │   │
│  └─────────────────────────────────────────┘   │
└─────────────────────────────────────────────────┘
```

### Точки расширения (Hooks)

| Хук | Описание | Когда вызывается |
|-----|----------|------------------|
| `HOOK_CONNECTION_ACCEPT` | Новое подключение | При принятии нового соединения |
| `HOOK_CONNECTION_CLOSE` | Закрытие подключения | При закрытии соединения |
| `HOOK_DATA_RECEIVED` | Получение данных | При получении данных от клиента |
| `HOOK_DATA_SENT` | Отправка данных | При отправке данных клиенту |
| `HOOK_MTPROTO_HANDSHAKE` | Handshake MTProto | Во время рукопожатия MTProto |
| `HOOK_MTPROTO_ENCRYPT` | Шифрование | Перед шифрованием данных |
| `HOOK_MTPROTO_DECRYPT` | Расшифровка | После расшифровки данных |
| `HOOK_SECURITY_CHECK` | Проверка безопасности | Перед проверкой безопасности |
| `HOOK_RATE_LIMIT_CHECK` | Rate limiting | Перед проверкой rate limit |
| `HOOK_AUTH_CHECK` | Аутентификация | Перед проверкой аутентификации |
| `HOOK_CONFIG_LOAD` | Загрузка конфигурации | При загрузке конфигурации |
| `HOOK_CONFIG_RELOAD` | Перезагрузка конфигурации | При перезагрузке конфигурации |
| `HOOK_STATS_COLLECT` | Сбор статистики | При сборе статистики |
| `HOOK_LOG_MESSAGE` | Логирование | При логировании сообщений |

### Коды возврата

| Код | Значение | Описание |
|-----|----------|----------|
| `PLUGIN_OK` | 0 | Успех, продолжить цепочку |
| `PLUGIN_ERROR` | -1 | Ошибка |
| `PLUGIN_SKIP` | 1 | Пропустить следующий плагин |
| `PLUGIN_STOP` | 2 | Остановить цепочку |
| `PLUGIN_REJECT` | 3 | Отклонить (для accept хуков) |

---

## Создание плагина

### Шаг 1: Структура плагина

```c
#include <plugin-system.h>

// 1. Данные плагина
typedef struct {
    // Ваши данные
} my_plugin_data_t;

// 2. Хуки
static plugin_result_t on_connection_accept(
    plugin_hook_context_t *ctx,
    void *plugin_data
) {
    // Ваш код
    return PLUGIN_OK;
}

// 3. Информация о плагине
PLUGIN_DECLARE_INFO(
    "my-plugin",
    "Description",
    "1.0.0",
    "Author",
    "License"
)

// 4. Инициализация
int plugin_init(const plugin_config_t *config, void **plugin_data) {
    // Выделение памяти, настройка
    return PLUGIN_OK;
}

// 5. Завершение
void plugin_shutdown(void *plugin_data) {
    // Освобождение ресурсов
}
```

### Шаг 2: Реализация хуков

**Пример: Логирование подключений**

```c
static plugin_result_t on_connection_accept(
    plugin_hook_context_t *ctx,
    void *plugin_data
) {
    printf("New connection from %s:%d\n", 
           ctx->client_ip, ctx->client_port);
    
    return PLUGIN_OK;  // Продолжить цепочку
}

static plugin_result_t on_connection_close(
    plugin_hook_context_t *ctx,
    void *plugin_data
) {
    printf("Connection closed: %s (reason: %s)\n",
           ctx->client_ip, ctx->result_data);
    
    return PLUGIN_OK;
}
```

**Пример: Блокировка по IP**

```c
static plugin_result_t on_security_check(
    plugin_hook_context_t *ctx,
    void *plugin_data
) {
    // Блокировка конкретного IP
    if (strcmp(ctx->client_ip, "192.168.1.100") == 0) {
        strncpy(ctx->result_data, "IP blocked", 
                sizeof(ctx->result_data) - 1);
        ctx->result_code = 403;
        return PLUGIN_REJECT;  // Блокировать
    }
    
    return PLUGIN_OK;  // Разрешить
}
```

### Шаг 3: Компиляция

**Linux/macOS:**
```bash
gcc -shared -fPIC -o my-plugin.so my-plugin.c
```

**Windows (MinGW):**
```bash
gcc -shared -o my-plugin.dll my-plugin.c
```

**Windows (MSVC):**
```batch
cl /LD my-plugin.c /Fe:my-plugin.dll
```

### Шаг 4: Установка

**Linux:**
```bash
sudo cp my-plugin.so /usr/lib/mtproxy/plugins/
```

**Windows:**
```batch
copy my-plugin.dll C:\Program Files\MTProxy\plugins\
```

### Шаг 5: Конфигурация

Создайте `/etc/mtproxy/plugins/my-plugin.conf`:

```ini
[my-plugin]
enabled = true
priority = 10

# Ваши параметры
param1 = value1
param2 = value2
```

---

## Система хуков

### Контекст хука

```c
typedef struct {
    // Информация о клиенте
    char client_ip[64];
    int client_port;
    int connection_fd;
    
    // Данные
    void *data;
    size_t data_size;
    
    // Результат
    int result_code;
    char result_data[256];
    
    // Контекст
    void *user_data;
    uint64_t timestamp;
} plugin_hook_context_t;
```

### Приоритеты хуков

| Приоритет | Значение | Описание |
|-----------|----------|----------|
| Высокий | 0-10 | Выполняются первыми |
| Средний | 11-50 | Обычные плагины |
| Низкий | 51-100 | Выполняются последними |

**Пример:**
```c
// Высокий приоритет (блокировка IP)
plugin_register_hook(HOOK_SECURITY_CHECK, on_security, 5, data);

// Средний приоритет (логирование)
plugin_register_hook(HOOK_CONNECTION_ACCEPT, on_accept, 50, data);

// Низкий приоритет (статистика)
plugin_register_hook(HOOK_CONNECTION_CLOSE, on_close, 100, data);
```

### Цепочка выполнения

```
Connection Accept
    ↓
Plugin 1 (priority=5) → PLUGIN_OK
    ↓
Plugin 2 (priority=10) → PLUGIN_OK
    ↓
Plugin 3 (priority=50) → PLUGIN_REJECT
    ↓
Connection Blocked
```

---

## Примеры плагинов

### 1. Logger Plugin

Логирование подключений в файл.

**Файл:** `plugins/example-logger.c`

**Возможности:**
- Логирование подключений/отключений
- Логирование объёма данных
- Конфигурируемый файл лога

**Использование:**
```ini
[example-logger]
enabled = true
log_file = /var/log/mtproxy/plugin.log
verbose = true
```

### 2. Rate Limit Plugin

Ограничение количества подключений с одного IP.

**Файл:** `plugins/ratelimit-plugin.c`

**Возможности:**
- Лимит подключений за окно времени
- Скользящее окно
- Статистика блокировок

**Использование:**
```ini
[ratelimit-plugin]
enabled = true
max_connections = 10
window_seconds = 60
```

### 3. Blacklist Plugin

Блокировка по чёрному/белому списку IP.

**Файл:** `plugins/blacklist-plugin.c`

**Возможности:**
- Чёрный список IP
- Белый список IP
- Поддержка масок (192.168.1.*)
- Режим whitelist (только разрешённые)

**Использование:**
```ini
[blacklist-plugin]
enabled = true
whitelist_mode = false

blacklist = 10.0.0.1
blacklist = 192.168.1.*
whitelist = 192.168.1.100
```

---

## Отладка плагинов

### Логирование

**Включите подробное логирование:**
```ini
[plugins]
debug = true
log_file = /var/log/mtproxy/plugins.log
```

**Пример логирования в плагине:**
```c
printf("[my-plugin] Debug: value=%d\n", value);
fprintf(stderr, "[my-plugin] Error: %s\n", error_msg);
```

### Отладка с GDB

```bash
# Запуск с GDB
gdb --args mtproto-proxy.exe -p 8888

# Установка точки останова в плагине
(gdb) break plugin_init
(gdb) break on_connection_accept

# Запуск
(gdb) run
```

### Проверка памяти

```bash
# Valgrind
valgrind --leak-check=full \
  --show-leak-kinds=all \
  ./mtproto-proxy.exe -p 8888

# AddressSanitizer
export ASAN_OPTIONS=detect_leaks=1
./mtproto-proxy.exe -p 8888
```

### Тестирование

**Создайте тестовый плагин:**
```c
// test-plugin.c
#include <assert.h>
#include <plugin-system.h>

void test_on_connection_accept() {
    plugin_hook_context_t ctx = {0};
    strcpy(ctx.client_ip, "127.0.0.1");
    ctx.client_port = 12345;
    
    plugin_result_t result = on_connection_accept(&ctx, NULL);
    assert(result == PLUGIN_OK);
}

int main() {
    test_on_connection_accept();
    printf("All tests passed!\n");
    return 0;
}
```

---

## Публикация плагинов

### Структура репозитория

```
my-plugin/
├── README.md
├── LICENSE
├── my-plugin.c
├── my-plugin.h
├── Makefile
├── example.conf
└── tests/
    └── test_my_plugin.c
```

### README.md шаблон

```markdown
# My Plugin for MTProxy

Описание плагина.

## Возможности

- Feature 1
- Feature 2

## Установка

```bash
git clone https://github.com/user/my-plugin
cd my-plugin
make
sudo make install
```

## Конфигурация

```ini
[my-plugin]
enabled = true
```

## Использование

Примеры использования.

## Лицензия

MIT
```

### Публикация

1. **GitHub:**
   ```bash
   git init
   git add .
   git commit -m "Initial commit"
   git remote add origin https://github.com/user/my-plugin
   git push -u origin master
   ```

2. **Добавьте тему:**
   - Добавьте тему `mtproxy` на GitHub
   - Добавьте в README ссылку на основной проект

---

## API Reference

### Макросы

```c
// Объявление информации о плагине
PLUGIN_DECLARE_INFO(name, description, version, author, license)

// Регистрация хука
plugin_register_hook(hook_type, callback, priority, plugin_data)

// Отмена регистрации хука
plugin_unregister_hook(hook_type, callback)
```

### Функции

```c
// Инициализация
int plugin_init(const plugin_config_t *config, void **plugin_data);

// Завершение
void plugin_shutdown(void *plugin_data);

// Получение статистики
void plugin_get_stats(plugin_stats_t *stats);
```

### Типы данных

```c
// Результат выполнения
typedef enum {
    PLUGIN_OK = 0,
    PLUGIN_ERROR = -1,
    PLUGIN_SKIP = 1,
    PLUGIN_STOP = 2,
    PLUGIN_REJECT = 3
} plugin_result_t;

// Тип хука
typedef enum {
    HOOK_CONNECTION_ACCEPT,
    HOOK_CONNECTION_CLOSE,
    HOOK_DATA_RECEIVED,
    HOOK_DATA_SENT,
    // ...
} plugin_hook_type_t;

// Контекст хука
typedef struct {
    char client_ip[64];
    int client_port;
    int connection_fd;
    void *data;
    size_t data_size;
    int result_code;
    char result_data[256];
} plugin_hook_context_t;
```

---

## Лучшие практики

### Безопасность

1. **Проверяйте входные данные:**
   ```c
   if (!ctx || !ctx->client_ip) {
       return PLUGIN_ERROR;
   }
   ```

2. **Не используйте неинициализированную память:**
   ```c
   char buffer[256] = {0};  // Инициализация нулём
   ```

3. **Освобождайте ресурсы:**
   ```c
   void plugin_shutdown(void *plugin_data) {
       my_data_t *data = (my_data_t *)plugin_data;
       if (data->file) fclose(data->file);
       free(data);
   }
   ```

### Производительность

1. **Минимизируйте работу в хуках:**
   ```c
   // Плохо: долгие операции в хуке
   static plugin_result_t on_accept(...) {
       sleep(1);  // Блокировка!
       return PLUGIN_OK;
   }
   
   // Хорошо: быстрая проверка
   static plugin_result_t on_accept(...) {
       if (is_blocked(ip)) return PLUGIN_REJECT;
       return PLUGIN_OK;
   }
   ```

2. **Используйте кэширование:**
   ```c
   static time_t last_check = 0;
   static bool is_blocked = false;
   
   if (time(NULL) - last_check > 60) {
       is_blocked = check_ip_list();
       last_check = time(NULL);
   }
   ```

### Надёжность

1. **Обрабатывайте ошибки:**
   ```c
   FILE *fp = fopen(path, "r");
   if (!fp) {
       fprintf(stderr, "Cannot open file: %s\n", path);
       return PLUGIN_ERROR;
   }
   ```

2. **Проверяйте размеры буферов:**
   ```c
   strncpy(dest, src, sizeof(dest) - 1);
   dest[sizeof(dest) - 1] = '\0';
   ```

---

## Дополнительные ресурсы

- [PLUGIN_SYSTEM.md](PLUGIN_SYSTEM.md) — Документация системы плагинов
- [API_REFERENCE.md](../API_REFERENCE.md) — REST API документация
- [DEBUGGING.md](DEBUGGING.md) — Руководство по отладке
