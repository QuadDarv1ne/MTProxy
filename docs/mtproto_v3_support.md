# Поддержка новых версий MTProto (v3.0+)

## Обзор
Данный документ описывает реализацию поддержки новых версий протокола MTProto (версии 3.0 и выше) в MTProxy.

## Особенности новых версий MTProto

### MTProto 3.0
- Улучшенная схема шифрования
- Новые методы аутентификации
- Поддержка более длинных сообщений
- Улучшенная защита от анализа трафика

### MTProto 4.0 (планируется)
- Квантово-устойчивое шифрование
- Дополнительные методы обфускации
- Улучшенная производительность

## Требования к реализации

### Архитектурные изменения
- Модульная система поддержки нескольких версий протокола
- Обратная совместимость с существующими версиями
- Автоматическое определение версии протокола
- Возможность настройки предпочитаемой версии

### Криптографические изменения
- Поддержка новых алгоритмов шифрования
- Обновленные схемы генерации ключей
- Современные методы аутентификации

## Реализация

### Структуры данных
```c
// Версия протокола
typedef enum {
    MTPROTO_VERSION_2_0 = 0x00000002,
    MTPROTO_VERSION_3_0 = 0x00000003,
    MTPROTO_VERSION_4_0 = 0x00000004
} mtproto_version_t;

// Информация о соединении с учетом версии
typedef struct {
    mtproto_version_t version;
    int features_mask;
    int64_t auth_key_id;
    unsigned char auth_key[256];
    unsigned char tmp_aes_key[32];
    unsigned char server_nonce[16];
    unsigned char client_nonce[16];
    int use_pfs;
    int_fast32_t seq_no;
    long long session_id;
    long long salt;
} mtproto_connection_info_t;
```

### Интерфейсы

#### Функции для работы с версиями
- `int mtproto_detect_version(unsigned char *data, int len)` - Определение версии протокола
- `int mtproto_init_connection(mtproto_connection_info_t *conn, mtproto_version_t version)` - Инициализация соединения
- `int mtproto_encrypt_packet_v3(void *in, int in_len, void *out, mtproto_connection_info_t *conn)` - Шифрование для v3
- `int mtproto_decrypt_packet_v3(void *in, int in_len, void *out, mtproto_connection_info_t *conn)` - Расшифровка для v3

#### Модуль аутентификации
- `int mtproto_handshake_v3(mtproto_connection_info_t *conn, void *handshake_data)` - Согласование соединения для v3
- `int mtproto_validate_auth_v3(mtproto_connection_info_t *conn)` - Проверка аутентификации для v3

## Параметры конфигурации

### Поддержка нескольких версий
- `--mtproto-version-min` - Минимальная поддерживаемая версия
- `--mtproto-version-max` - Максимальная поддерживаемая версия
- `--mtproto-default-version` - Версия по умолчанию

## Совместимость
- Полная обратная совместимость с MTProto 2.0
- Возможность одновременной работы разных версий
- Автоматическое обновление соединений при возможности

## Тестирование
- Модульные тесты для каждой версии протокола
- Тесты совместимости
- Тесты производительности