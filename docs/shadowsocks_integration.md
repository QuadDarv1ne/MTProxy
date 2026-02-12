# Интеграция с протоколом Shadowsocks

## Обзор
Данный документ описывает реализацию интеграции MTProxy с протоколом Shadowsocks для расширения возможностей прокси-сервера и обеспечения совместимости с большей экосистемой прокси-решений.

## Особенности протокола Shadowsocks

### Основные характеристики
- Асинхронный шифрование на основе симметричных ключей
- Поддержка различных методов шифрования (AES, Chacha20, и др.)
- Возможность обфускации трафика
- Легковесный протокол с низкой задержкой

### Поддерживаемые методы шифрования
- chacha20-ietf-poly1305
- aes-256-gcm
- aes-192-gcm
- aes-128-gcm
- xchacha20-ietf-poly1305
- и другие современные алгоритмы

## Требования к реализации

### Архитектурные изменения
- Модульная система поддержки нескольких протоколов
- Возможность одновременной работы MTProto и Shadowsocks
- Механизм распознавания протокола по входящему трафику
- Общие компоненты для управления соединениями

### Криптографические изменения
- Интеграция библиотек для работы с методами шифрования Shadowsocks
- Поддержка AEAD (Authenticated Encryption with Associated Data) шифрования
- Генерация и управление ключами для шифрования

## Реализация

### Структуры данных
```c
// Методы шифрования Shadowsocks
typedef enum {
    SS_CIPHER_NONE = 0,
    SS_CIPHER_CHACHA20_POLY1305,
    SS_CIPHER_AES_256_GCM,
    SS_CIPHER_AES_192_GCM,
    SS_CIPHER_AES_128_GCM,
    SS_CIPHER_XCHACHA20_POLY1305,
    SS_CIPHER_MAX
} ss_cipher_method_t;

// Информация о соединении Shadowsocks
typedef struct {
    ss_cipher_method_t method;      /* Метод шифрования */
    unsigned char password[256];    /* Пароль/ключ шифрования */
    int password_len;               /* Длина пароля */
    unsigned char salt[32];         /* Соль для генерации ключа */
    unsigned char iv[32];           /* Вектор инициализации */
    int iv_len;                     /* Длина IV */
    long long connection_id;        /* ID соединения */
    int_fast32_t seq_no;            /* Номер последовательности */
} shadowsocks_connection_info_t;

// Конфигурация Shadowsocks
typedef struct {
    ss_cipher_method_t default_method;  /* Метод шифрования по умолчанию */
    int support_all_methods;            /* Поддержка всех методов шифрования */
    int enable_udp_forwarding;          /* Включить UDP пересылку */
    int tcp_no_delay;                   /* Включить TCP_NODELAY */
} shadowsocks_config_t;
```

### Интерфейсы

#### Функции для работы с соединениями
- `int shadowsocks_init_connection(shadowsocks_connection_info_t *conn, ss_cipher_method_t method, const char *password, int pwd_len)` - Инициализация соединения
- `int shadowsocks_detect_protocol(const unsigned char *data, int len)` - Определение, является ли трафик Shadowsocks
- `int shadowsocks_encrypt_data(void *in, int in_len, void *out, shadowsocks_connection_info_t *conn)` - Шифрование данных
- `int shadowsocks_decrypt_data(void *in, int in_len, void *out, shadowsocks_connection_info_t *conn)` - Расшифровка данных

#### Модуль аутентификации
- `int shadowsocks_setup_cipher(shadowsocks_connection_info_t *conn, const char *password, ss_cipher_method_t method)` - Настройка шифрования
- `int shadowsocks_generate_salt(unsigned char *salt, int salt_len)` - Генерация соли

## Параметры конфигурации

### Поддержка Shadowsocks
- `--shadowsocks-enable` - Включить поддержку Shadowsocks
- `--shadowsocks-password` - Пароль для шифрования
- `--shadowsocks-method` - Метод шифрования
- `--shadowsocks-port` - Порт для приема Shadowsocks соединений

## Совместимость
- Возможность одновременной работы MTProto и Shadowsocks
- Автоматическое определение протокола
- Общая система управления соединениями
- Совместное использование сетевых ресурсов

## Тестирование
- Модульные тесты для каждого метода шифрования
- Тесты совместимости с существующими клиентами Shadowsocks
- Тесты производительности