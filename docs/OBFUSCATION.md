# 🔒 Методы обфускации трафика для MTProxy

**Обфускация трафика** — это набор техник для маскировки MTProxy трафика с целью обхода DPI (Deep Packet Inspection) и блокировок провайдеров.

## 📖 Оглавление

- [Обзор методов обфускации](#обзор-методов-обфускации)
- [Реализация в MTProxy](#реализация-в-mtproxy)
- [Shadowsocks обфускация](#shadowsocks-обфускация)
- [Планы развития](#планы-развития)
- [Примеры использования](#примеры-использования)
- [Сравнение методов](#сравнение-методов)
- [API для разработчиков](#api-для-разработчиков)

---

## 📊 Обзор методов обфускации

### 1️⃣ Обфускация на транспортном уровне

| Метод | Описание | Сложность | Статус |
|-------|----------|-----------|--------|
| **Obfsproxy** | Маскировка под случайный шум | Средняя | 📋 Planned |
| **Scramble** | XOR-шифрование сигнатур | Низкая | 📋 Planned |
| **Packet Size Padding** | Выравнивание размеров пакетов | Низкая | ✅ Available |
| **Random Fill** | Случайное заполнение пакетов | Низкая | ✅ Available |

#### Obfsproxy

Маскирует трафик под случайный шум, делая невозможным определение протокола по сигнатурам.

**Принцип работы:**
```
MTProto пакет → Obfuscation Layer → Случайные данные → Сеть
```

**Преимущества:**
- ✅ Скрывает факт использования MTProxy
- ✅ Обходит простые DPI системы
- ✅ Минимальные накладные расходы

**Недостатки:**
- ❌ Увеличивает размер пакетов
- ❌ Может определяться продвинутыми DPI

#### Scramble (XOR)

Простое XOR-шифрование заголовков пакетов для скрытия сигнатур протоколов.

**Пример реализации на C:**
```c
// crypto/obfuscate.h
#ifndef OBFUSCATE_H
#define OBFUSCATE_H

#include <stdint.h>
#include <stddef.h>

#define OBFUSCATE_KEY_SIZE 16

// Структура контекста обфускации
typedef struct {
    unsigned char key[OBFUSCATE_KEY_SIZE];
    size_t key_pos;
} obfuscate_ctx_t;

// Инициализация контекста
int obfuscate_init(obfuscate_ctx_t *ctx, const unsigned char *key, size_t key_len);

// Обфускация пакета
int obfuscate_packet(obfuscate_ctx_t *ctx, unsigned char *packet, size_t len);

// Деобфускация пакета (XOR обратим)
int deobfuscate_packet(obfuscate_ctx_t *ctx, unsigned char *packet, size_t len);

// Очистка контекста
void obfuscate_cleanup(obfuscate_ctx_t *ctx);

#endif // OBFUSCATE_H
```

```c
// crypto/obfuscate.c
#include "obfuscate.h"
#include <string.h>
#include <stdlib.h>

int obfuscate_init(obfuscate_ctx_t *ctx, const unsigned char *key, size_t key_len) {
    if (!ctx || !key || key_len == 0) {
        return -1;
    }
    
    // Копируем ключ или хешируем если слишком длинный
    if (key_len <= OBFUSCATE_KEY_SIZE) {
        memcpy(ctx->key, key, key_len);
        memset(ctx->key + key_len, 0, OBFUSCATE_KEY_SIZE - key_len);
    } else {
        // Простой хеш для длинных ключей
        for (size_t i = 0; i < OBFUSCATE_KEY_SIZE; i++) {
            ctx->key[i] = key[i % key_len] ^ key[(i * 7) % key_len];
        }
    }
    
    ctx->key_pos = 0;
    return 0;
}

int obfuscate_packet(obfuscate_ctx_t *ctx, unsigned char *packet, size_t len) {
    if (!ctx || !packet || len == 0) {
        return -1;
    }
    
    for (size_t i = 0; i < len; i++) {
        packet[i] ^= ctx->key[ctx->key_pos];
        ctx->key_pos = (ctx->key_pos + 1) % OBFUSCATE_KEY_SIZE;
    }
    
    return 0;
}

int deobfuscate_packet(obfuscate_ctx_t *ctx, unsigned char *packet, size_t len) {
    // XOR обратим, используем ту же функцию
    return obfuscate_packet(ctx, packet, len);
}

void obfuscate_cleanup(obfuscate_ctx_t *ctx) {
    if (ctx) {
        memset(ctx, 0, sizeof(obfuscate_ctx_t));
    }
}
```

#### Packet Size Padding

Выравнивание размеров пакетов до фиксированных значений для скрытия паттернов трафика.

**Пример:**
```c
// net/padding.h
#ifndef PADDING_H
#define PADDING_H

#include <stdint.h>
#include <stddef.h>

#define PADDING_BLOCK_SIZE 64
#define PADDING_MAX_SIZE 256

// Добавляет padding до ближайшего блока
int add_padding(unsigned char *packet, size_t *len, size_t block_size);

// Удаляет padding
int remove_padding(const unsigned char *packet, size_t *len);

// Добавляет случайное заполнение
int add_random_fill(unsigned char *packet, size_t *len, size_t max_fill);

#endif // PADDING_H
```

```c
// net/padding.c
#include "padding.h"
#include <string.h>
#include <stdlib.h>

int add_padding(unsigned char *packet, size_t *len, size_t block_size) {
    if (!packet || !len || block_size == 0) {
        return -1;
    }
    
    size_t remainder = *len % block_size;
    if (remainder == 0) {
        return 0; // Уже выровнено
    }
    
    size_t padding_len = block_size - remainder;
    
    // Проверяем, достаточно ли места
    if (*len + padding_len > PADDING_MAX_SIZE) {
        return -1;
    }
    
    // Заполняем padding случайными данными
    for (size_t i = 0; i < padding_len; i++) {
        packet[*len + i] = (unsigned char)(i & 0xFF);
    }
    
    // Последний байт содержит длину padding
    packet[*len + padding_len - 1] = (unsigned char)padding_len;
    *len += padding_len;
    
    return 0;
}

int remove_padding(const unsigned char *packet, size_t *len) {
    if (!packet || !len || *len == 0) {
        return -1;
    }
    
    // Получаем длину padding из последнего байта
    size_t padding_len = packet[*len - 1];
    
    if (padding_len == 0 || padding_len > *len) {
        return -1; // Неверный padding
    }
    
    *len -= padding_len;
    return 0;
}

int add_random_fill(unsigned char *packet, size_t *len, size_t max_fill) {
    if (!packet || !len || max_fill == 0) {
        return -1;
    }
    
    // Генерируем случайную длину заполнения
    size_t fill_len = rand() % max_fill;
    
    if (*len + fill_len > PADDING_MAX_SIZE) {
        fill_len = PADDING_MAX_SIZE - *len;
    }
    
    // Заполняем случайными данными
    for (size_t i = 0; i < fill_len; i++) {
        packet[*len + i] = (unsigned char)(rand() & 0xFF);
    }
    
    *len += fill_len;
    return 0;
}
```

---

### 2️⃣ Маскировка под легитимные протоколы

| Метод | Описание | Сложность | Статус |
|-------|----------|-----------|--------|
| **VLESS + Reality** | Маскировка под HTTPS (TLS 1.3) | Высокая | 🔮 Future |
| **Trojan** | Трафик как обычный HTTPS | Средняя | 📋 Planned |
| **ShadowTLS** | TLS-обёртка для Shadowsocks | Средняя | 📋 Planned |
| **WebSocket (WS)** | Трафик как WebSocket | Низкая | 📋 Planned |
| **gRPC / HTTP/2** | Маскировка под gRPC | Средняя | 🔮 Future |
| **QUIC** | Трафик как QUIC/HTTP3 | Высокая | 🔮 Future |

#### Trojan Protocol

Трафик маскируется под обычный HTTPS, что делает его неотличимым от легитимного веб-трафика.

**Структура пакета Trojan:**
```
+-------------------+------------------+------------------+
|    HMAC-SHA256    |    Длина данных  |     Данные       |
|    (32 байта)     |    (2 байта)     |    (переменная)  |
+-------------------+------------------+------------------+
```

**Пример реализации:**
```c
// net/trojan.h
#ifndef TROJAN_H
#define TROJAN_H

#include <stdint.h>
#include <stddef.h>

#define TROJAN_HMAC_SIZE 32
#define TROJAN_LENGTH_SIZE 2
#define TROJAN_CRLF_SIZE 2

// Структура заголовка Trojan
typedef struct {
    unsigned char hmac[TROJAN_HMAC_SIZE];
    unsigned char length[TROJAN_LENGTH_SIZE];
    unsigned char crlf[TROJAN_CRLF_SIZE];
} trojan_header_t;

// Инициализация Trojan
int trojan_init(const unsigned char *password, size_t password_len);

// Создание заголовка
int trojan_create_header(const unsigned char *data, size_t data_len,
                        trojan_header_t *header);

// Проверка заголовка
int trojan_verify_header(const trojan_header_t *header, 
                        const unsigned char *data, size_t data_len);

#endif // TROJAN_H
```

#### ShadowTLS

TLS-обёртка для Shadowsocks, маскирующая трафик под HTTPS соединение.

**Схема работы:**
```
Клиент → TLS Handshake → Shadowsocks данные → Сервер
         (выглядит как HTTPS)
```

---

### 3️⃣ Обход DPI (Deep Packet Inspection)

| Метод | Описание | Сложность | Статус |
|-------|----------|-----------|--------|
| **AmneziaWG** | Модифицированный WireGuard | Высокая | 🔮 Future |
| **Cloak** | Скрытие факта соединения | Высокая | 📋 Planned |
| **Fragmentation** | Фрагментация TLS handshake | Низкая | 📋 Planned |
| **Domain Fronting** | Использование CDN | Средняя | 📋 Planned |
| **MTProto Fake TLS** | Маскировка под TLS handshake | Средняя | ✅ Available |

#### Fragmentation

Разбиение TLS handshake на мелкие фрагменты для обхода DPI.

**Пример:**
```c
// net/fragmentation.h
#ifndef FRAGMENTATION_H
#define FRAGMENTATION_H

#include <stdint.h>
#include <stddef.h>

#define FRAGMENT_MIN_SIZE 64
#define FRAGMENT_MAX_SIZE 512
#define FRAGMENT_MAX_COUNT 16

// Структура фрагмента
typedef struct {
    unsigned char data[FRAGMENT_MAX_SIZE];
    size_t len;
} fragment_t;

// Разбиение данных на фрагменты
int fragment_data(const unsigned char *data, size_t data_len,
                 fragment_t *fragments, size_t *fragment_count,
                 size_t fragment_size);

// Сборка данных из фрагментов
int assemble_fragments(const fragment_t *fragments, size_t fragment_count,
                      unsigned char *data, size_t *data_len);

#endif // FRAGMENTATION_H
```

```c
// net/fragmentation.c
#include "fragmentation.h"
#include <string.h>

int fragment_data(const unsigned char *data, size_t data_len,
                 fragment_t *fragments, size_t *fragment_count,
                 size_t fragment_size) {
    if (!data || !fragments || !fragment_count || fragment_size == 0) {
        return -1;
    }
    
    if (fragment_size < FRAGMENT_MIN_SIZE) {
        fragment_size = FRAGMENT_MIN_SIZE;
    }
    
    if (fragment_size > FRAGMENT_MAX_SIZE) {
        fragment_size = FRAGMENT_MAX_SIZE;
    }
    
    size_t count = 0;
    size_t offset = 0;
    
    while (offset < data_len && count < FRAGMENT_MAX_COUNT) {
        size_t chunk_size = fragment_size;
        if (offset + chunk_size > data_len) {
            chunk_size = data_len - offset;
        }
        
        memcpy(fragments[count].data, data + offset, chunk_size);
        fragments[count].len = chunk_size;
        
        offset += chunk_size;
        count++;
    }
    
    *fragment_count = count;
    return 0;
}

int assemble_fragments(const fragment_t *fragments, size_t fragment_count,
                      unsigned char *data, size_t *data_len) {
    if (!fragments || !data || !data_len) {
        return -1;
    }
    
    size_t total_len = 0;
    for (size_t i = 0; i < fragment_count; i++) {
        total_len += fragments[i].len;
    }
    
    if (total_len > *data_len) {
        return -1; // Недостаточно места
    }
    
    size_t offset = 0;
    for (size_t i = 0; i < fragment_count; i++) {
        memcpy(data + offset, fragments[i].data, fragments[i].len);
        offset += fragments[i].len;
    }
    
    *data_len = total_len;
    return 0;
}
```

#### Domain Fronting

Использование CDN (Cloudflare, Google) для скрытия реального сервера.

**Схема работы:**
```
Клиент → CDN (SNI: cdn.example.com) → Ваш сервер
         DPI видит только CDN
```

**Конфигурация:**
```json
{
  "domain_fronting": {
    "enabled": true,
    "front_domain": "cdn.cloudflare.com",
    "real_domain": "mtproxy.example.com",
    "port": 443
  }
}
```

#### MTProto Fake TLS

Специальная реализация для MTProto, маскирующая handshake под TLS 1.3.

**Пример:**
```c
// mtproto/fake_tls.h
#ifndef FAKE_TLS_H
#define FAKE_TLS_H

#include <stdint.h>
#include <stddef.h>

#define FAKE_TLS_RECORD_SIZE 512
#define FAKE_TLS_HANDSHAKE_TYPES 10

// Типы TLS handshake
typedef enum {
    TLS_CLIENT_HELLO = 1,
    TLS_SERVER_HELLO = 2,
    TLS_CERTIFICATE = 11,
    TLS_SERVER_KEY_EXCHANGE = 12,
    TLS_CLIENT_KEY_EXCHANGE = 16,
    TLS_FINISHED = 20
} tls_handshake_type_t;

// Создание поддельного Client Hello
int fake_tls_create_client_hello(unsigned char *buffer, size_t *len,
                                 const char *sni_domain);

// Парсинг TLS записей
int fake_tls_parse_record(const unsigned char *buffer, size_t len,
                         tls_handshake_type_t *type, 
                         unsigned char **payload, size_t *payload_len);

#endif // FAKE_TLS_H
```

---

## 🔧 Реализация в MTProxy

### Архитектура системы обфускации

```
┌─────────────────────────────────────────────────────────┐
│                      MTProxy                            │
├─────────────────────────────────────────────────────────┤
│  Socket → MTProto Parser → Obfuscation Layer           │
│                                    ↓                    │
│  ┌─────────────────────────────────────────────────┐   │
│  │           Obfuscation Methods                   │   │
│  │  ┌──────────┬──────────┬──────────┬──────────┐  │   │
│  │  │ Padding  │ Scramble │  Trojan  │  Cloak   │  │   │
│  │  ├──────────┼──────────┼──────────┼──────────┤  │   │
│  │  │Fragment. │ Fake TLS │ Domain   │ Shadow-  │  │   │
│  │  │          │          │ Fronting │  TLS     │  │   │
│  │  └──────────┴──────────┴──────────┴──────────┘  │   │
│  └─────────────────────────────────────────────────┘   │
│                                    ↓                    │
│                    Network → Internet                  │
└─────────────────────────────────────────────────────────┘
```

### Структура конфигурации

```c
// common/obfuscation_config.h
#ifndef OBFUSCATION_CONFIG_H
#define OBFUSCATION_CONFIG_H

#include <stdint.h>
#include <stddef.h>

#define MAX_OBFUSCATION_METHOD_LEN 32
#define MAX_OBFUSCATION_KEY_LEN 256
#define MAX_DOMAIN_LEN 256

// Методы обфускации
typedef enum {
    OBFUSCATE_NONE = 0,
    OBFUSCATE_SCRAMBLE = 1,
    OBFUSCATE_PADDING = 2,
    OBFUSCATE_TROJAN = 3,
    OBFUSCATE_SHADOWTLS = 4,
    OBFUSCATE_FAKE_TLS = 5,
    OBFUSCATE_DOMAIN_FRONTING = 6,
    OBFUSCATE_CLOAK = 7
} obfuscation_method_t;

// Конфигурация обфускации
typedef struct {
    obfuscation_method_t method;
    char method_name[MAX_OBFUSCATION_METHOD_LEN];
    
    // Ключ для Scramble/XOR
    unsigned char key[MAX_OBFUSCATION_KEY_LEN];
    size_t key_len;
    
    // Настройки Padding
    size_t padding_block_size;
    int padding_random_fill;
    
    // Настройки Trojan
    unsigned char trojan_password[64];
    size_t trojan_password_len;
    
    // Настройки Domain Fronting
    char front_domain[MAX_DOMAIN_LEN];
    char real_domain[MAX_DOMAIN_LEN];
    int front_port;
    
    // Настройки Fragmentation
    size_t fragment_size;
    int fragment_random_order;
    
    // Включено/выключено
    int enabled;
} obfuscation_config_t;

// Парсинг конфигурации из JSON
int obfuscation_config_parse(obfuscation_config_t *config, 
                            const char *json_str);

// Валидация конфигурации
int obfuscation_config_validate(const obfuscation_config_t *config);

// Сериализация в JSON
int obfuscation_config_to_json(const obfuscation_config_t *config,
                              char *buffer, size_t buffer_len);

#endif // OBFUSCATION_CONFIG_H
```

### Контекст обфускации

```c
// common/obfuscation_context.h
#ifndef OBFUSCATION_CONTEXT_H
#define OBFUSCATION_CONTEXT_H

#include "obfuscation_config.h"

// Контекст обфускации
typedef struct {
    obfuscation_config_t config;
    
    // Состояние для Scramble
    unsigned char scramble_state[16];
    
    // Состояние для Trojan
    void *trojan_ctx;
    
    // Состояние для Fragmentation
    void *fragment_ctx;
    
    // Статистика
    uint64_t packets_obfuscated;
    uint64_t packets_deobfuscated;
    uint64_t bytes_obfuscated;
    uint64_t bytes_deobfuscated;
    uint64_t errors;
} obfuscation_ctx_t;

// Инициализация контекста
int obfuscation_ctx_init(obfuscation_ctx_t *ctx, 
                        const obfuscation_config_t *config);

// Обфускация пакета
int obfuscation_obfuscate(obfuscation_ctx_t *ctx, 
                         unsigned char *packet, size_t *len);

// Деобфускация пакета
int obfuscation_deobfuscate(obfuscation_ctx_t *ctx, 
                           unsigned char *packet, size_t *len);

// Получение статистики
int obfuscation_get_stats(const obfuscation_ctx_t *ctx, char *buffer, size_t len);

// Очистка контекста
void obfuscation_ctx_cleanup(obfuscation_ctx_t *ctx);

#endif // OBFUSCATION_CONTEXT_H
```

---

## 📋 Планы развития

### Q2 2026

- [ ] **Scramble (XOR)** — базовая реализация
- [ ] **Packet Padding** — улучшенная система padding
- [ ] **TLS Fragmentation** — фрагментация handshake

### Q3 2026

- [ ] **Trojan Protocol** — полная поддержка
- [ ] **ShadowTLS** — TLS-обёртка
- [ ] **Fake TLS** — маскировка под TLS 1.3

### Q4 2026

- [ ] **Cloak Integration** — плагиновая система
- [ ] **Domain Fronting** — CDN интеграция
- [ ] **VLESS + Reality** — продвинутая маскировка

---

## 📊 Сравнение методов

| Метод | Обход DPI | Скорость | Сложность | Надёжность |
|-------|-----------|----------|-----------|------------|
| **None** | ❌ | ⚡⚡⚡ | Низкая | Низкая |
| **Padding** | ⚠️ Частично | ⚡⚡⚡ | Низкая | Средняя |
| **Scramble** | ⚠️ Частично | ⚡⚡⚡ | Низкая | Средняя |
| **Fragmentation** | ✅ Да | ⚡⚡ | Низкая | Высокая |
| **Fake TLS** | ✅ Да | ⚡⚡ | Средняя | Высокая |
| **Trojan** | ✅ Да | ⚡⚡ | Средняя | Очень высокая |
| **ShadowTLS** | ✅ Да | ⚡⚡ | Средняя | Очень высокая |
| **Domain Fronting** | ✅ Да | ⚡ | Средняя | Высокая |
| **Cloak** | ✅ Да | ⚡ | Высокая | Максимальная |

---

## 🔍 Примеры использования

### Пример 1: Базовая обфускация (Padding + Random Fill)

**Конфигурация:**
```json
{
  "obfuscation": {
    "enabled": true,
    "method": "padding",
    "settings": {
      "padding_block_size": 64,
      "padding_random_fill": true,
      "max_random_fill_size": 128
    }
  }
}
```

**Запуск:**
```bash
./mtproto-proxy -S secret123 -p 8888 \
  --obfuscation padding \
  --obfuscation-config obfuscation.json
```

### Пример 2: Scramble (XOR) обфускация

**Конфигурация:**
```json
{
  "obfuscation": {
    "enabled": true,
    "method": "scramble",
    "settings": {
      "key": "my-secret-obfuscation-key",
      "key_length": 16
    }
  }
}
```

**Использование в коде:**
```c
#include "obfuscation_context.h"

obfuscation_config_t config = {0};
config.method = OBFUSCATE_SCRAMBLE;
config.enabled = 1;
strcpy(config.key, "my-secret-obfuscation-key");
config.key_len = 16;

obfuscation_ctx_t ctx = {0};
obfuscation_ctx_init(&ctx, &config);

// Обфускация пакета
unsigned char packet[1024];
size_t packet_len = 512;
obfuscation_obfuscate(&ctx, packet, &packet_len);

// Отправка пакета...

// Очистка
obfuscation_ctx_cleanup(&ctx);
```

### Пример 3: Fragmentation для обхода DPI

**Конфигурация:**
```json
{
  "obfuscation": {
    "enabled": true,
    "method": "fragmentation",
    "settings": {
      "fragment_size": 128,
      "fragment_random_order": false
    }
  }
}
```

### Пример 4: Комбинированная обфускация

**Конфигурация:**
```json
{
  "obfuscation": {
    "enabled": true,
    "methods": [
      {
        "type": "scramble",
        "key": "xor-key-12345"
      },
      {
        "type": "padding",
        "block_size": 64,
        "random_fill": true
      },
      {
        "type": "fragmentation",
        "fragment_size": 128
      }
    ]
  }
}
```

---

## 🛠️ API для разработчиков

### Добавление нового метода обфускации

```c
// obfuscation/my_obfuscation.h
#ifndef MY_OBFUSCATION_H
#define MY_OBFUSCATION_H

#include "obfuscation_context.h"

// Структура конфигурации
typedef struct {
    unsigned char key[32];
    size_t key_len;
    int iteration_count;
} my_obfuscation_config_t;

// Структура контекста
typedef struct {
    my_obfuscation_config_t config;
    unsigned char state[64];
} my_obfuscation_ctx_t;

// Инициализация
int my_obfuscation_init(my_obfuscation_ctx_t *ctx, 
                       const my_obfuscation_config_t *config);

// Обфускация
int my_obfuscation_obfuscate(my_obfuscation_ctx_t *ctx,
                            unsigned char *packet, size_t *len);

// Деобфускация
int my_obfuscation_deobfuscate(my_obfuscation_ctx_t *ctx,
                              unsigned char *packet, size_t *len);

// Очистка
void my_obfuscation_cleanup(my_obfuscation_ctx_t *ctx);

#endif // MY_OBFUSCATION_H
```

### Регистрация метода

```c
// obfuscation/registry.c
#include "obfuscation_registry.h"
#include "my_obfuscation.h"

static obfuscation_method_t methods[] = {
    {
        .name = "scramble",
        .init = scramble_init,
        .obfuscate = scramble_obfuscate,
        .deobfuscate = scramble_deobfuscate,
        .cleanup = scramble_cleanup
    },
    {
        .name = "padding",
        .init = padding_init,
        .obfuscate = padding_obfuscate,
        .deobfuscate = padding_deobfuscate,
        .cleanup = padding_cleanup
    },
    {
        .name = "my_method",
        .init = (obfuscation_init_fn)my_obfuscation_init,
        .obfuscate = (obfuscation_fn)my_obfuscation_obfuscate,
        .deobfuscate = (obfuscation_fn)my_obfuscation_deobfuscate,
        .cleanup = (obfuscation_cleanup_fn)my_obfuscation_cleanup
    }
};

int obfuscation_register_method(const char *name, obfuscation_method_t *method) {
    // Регистрация нового метода
    return 0;
}

obfuscation_method_t* obfuscation_get_method(const char *name) {
    // Поиск метода по имени
    for (int i = 0; i < sizeof(methods)/sizeof(methods[0]); i++) {
        if (strcmp(methods[i].name, name) == 0) {
            return &methods[i];
        }
    }
    return NULL;
}
```

### Интеграция с основным движком

```c
// engine/mtproto_engine.c
#include "obfuscation_context.h"

typedef struct {
    // ... другие поля ...
    obfuscation_ctx_t obfuscation_ctx;
    int obfuscation_enabled;
} mtproto_engine_t;

int mtproto_engine_init(mtproto_engine_t *engine, const config_t *config) {
    // ... инициализация ...
    
    // Инициализация обфускации
    if (config->obfuscation.enabled) {
        obfuscation_ctx_init(&engine->obfuscation_ctx, &config->obfuscation);
        engine->obfuscation_enabled = 1;
    }
    
    return 0;
}

int mtproto_send(mtproto_engine_t *engine, const unsigned char *data, size_t len) {
    unsigned char buffer[2048];
    size_t buffer_len = len;
    
    memcpy(buffer, data, len);
    
    // Применяем обфускацию если включена
    if (engine->obfuscation_enabled) {
        obfuscation_obfuscate(&engine->obfuscation_ctx, buffer, &buffer_len);
    }
    
    // Отправка данных
    return send(engine->socket_fd, buffer, buffer_len, 0);
}

int mtproto_receive(mtproto_engine_t *engine, unsigned char *buffer, size_t *len) {
    int received = recv(engine->socket_fd, buffer, *len, 0);
    
    if (received > 0 && engine->obfuscation_enabled) {
        // Применяем деобфускацию
        size_t recv_len = received;
        obfuscation_deobfuscate(&engine->obfuscation_ctx, buffer, &recv_len);
        *len = recv_len;
        return recv_len;
    }
    
    *len = received;
    return received;
}
```

---

## 📈 Мониторинг и статистика

### Получение статистики обфускации

```c
#include "obfuscation_context.h"

obfuscation_ctx_t ctx;
char stats_buffer[1024];

if (obfuscation_get_stats(&ctx, stats_buffer, sizeof(stats_buffer)) == 0) {
    printf("Obfuscation Statistics:\n%s\n", stats_buffer);
}
```

**Пример вывода:**
```
Obfuscation Statistics:
  Method: scramble
  Status: enabled
  Packets obfuscated: 1234567
  Packets deobfuscated: 1234500
  Bytes obfuscated: 987654321
  Bytes deobfuscated: 987600000
  Errors: 0
  Success rate: 99.99%
```

---

## 🔧 Тестирование обфускации

### Модульные тесты

```c
// testing/test_obfuscation.c
#include "obfuscation_context.h"
#include <assert.h>
#include <string.h>

void test_scramble_basic(void) {
    obfuscation_config_t config = {0};
    config.method = OBFUSCATE_SCRAMBLE;
    config.enabled = 1;
    strcpy(config.key, "test-key-12345");
    config.key_len = 14;
    
    obfuscation_ctx_t ctx = {0};
    assert(obfuscation_ctx_init(&ctx, &config) == 0);
    
    unsigned char packet[] = "Hello, World!";
    size_t original_len = strlen(packet);
    size_t obfuscated_len = original_len;
    
    // Сохраняем оригинал
    unsigned char original[sizeof(packet)];
    memcpy(original, packet, original_len);
    
    // Обфускация
    assert(obfuscation_obfuscate(&ctx, packet, &obfuscated_len) == 0);
    
    // Пакет должен измениться
    assert(memcmp(packet, original, original_len) != 0);
    
    // Деобфускация
    assert(obfuscation_deobfuscate(&ctx, packet, &obfuscated_len) == 0);
    
    // Должны получить оригинал
    assert(memcmp(packet, original, original_len) == 0);
    
    obfuscation_ctx_cleanup(&ctx);
    printf("✓ test_scramble_basic passed\n");
}

void test_padding_basic(void) {
    obfuscation_config_t config = {0};
    config.method = OBFUSCATE_PADDING;
    config.enabled = 1;
    config.padding_block_size = 64;
    
    obfuscation_ctx_t ctx = {0};
    assert(obfuscation_ctx_init(&ctx, &config) == 0);
    
    unsigned char packet[100];
    size_t packet_len = 50;
    memset(packet, 'A', packet_len);
    
    size_t original_len = packet_len;
    
    // Обфускация с padding
    assert(obfuscation_obfuscate(&ctx, packet, &packet_len) == 0);
    
    // Длина должна увеличиться до кратной 64
    assert(packet_len % 64 == 0);
    assert(packet_len > original_len);
    
    obfuscation_ctx_cleanup(&ctx);
    printf("✓ test_padding_basic passed\n");
}

int main(void) {
    test_scramble_basic();
    test_padding_basic();
    
    printf("\nAll obfuscation tests passed!\n");
    return 0;
}
```

### Запуск тестов

```bash
cd build
ctest -R obfuscation --output-on-failure
```

---

## 📚 Дополнительные ресурсы

- [MTProto Protocol](https://core.telegram.org/mtproto)
- [RFC 6455 (WebSocket)](https://datatracker.ietf.org/doc/html/rfc6455)
- [RFC 7540 (HTTP/2)](https://datatracker.ietf.org/doc/html/rfc7540)
- [RFC 9000 (QUIC)](https://datatracker.ietf.org/doc/html/rfc9000)
- [Trojan Protocol](https://trojan-gfw.github.io/trojan/)
- [Shadowsocks](https://shadowsocks.org)
- [Cloak Project](https://github.com/cbeuw/Cloak)
- [VLESS & Reality](https://github.com/XTLS/Xray-core)

---

**Версия:** 1.0.0  
**Последнее обновление:** 1 апреля 2026 г.
