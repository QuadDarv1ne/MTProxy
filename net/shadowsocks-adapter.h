/*
 * Адаптер для интеграции с протоколом Shadowsocks
 *
 * Этот файл содержит интерфейсы для поддержки протокола Shadowsocks
 * в составе MTProxy.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Методы шифрования Shadowsocks */
typedef enum {
    SS_CIPHER_NONE = 0,
    SS_CIPHER_CHACHA20_POLY1305,
    SS_CIPHER_AES_256_GCM,
    SS_CIPHER_AES_192_GCM,
    SS_CIPHER_AES_128_GCM,
    SS_CIPHER_XCHACHA20_POLY1305,
    SS_CIPHER_MAX
} ss_cipher_method_t;

/* Информация о соединении Shadowsocks */
typedef struct {
    ss_cipher_method_t method;      /* Метод шифрования */
    unsigned char password[256];    /* Пароль/ключ шифрования */
    int password_len;              /* Длина пароля */
    unsigned char salt[32];        /* Соль для генерации ключа */
    unsigned char iv[32];          /* Вектор инициализации */
    int iv_len;                    /* Длина IV */
    long long connection_id;       /* ID соединения */
    int_fast32_t seq_no;           /* Номер последовательности */
    uint64_t bytes_sent;          /* Отправленные байты */
    uint64_t bytes_received;      /* Полученные байты */
} shadowsocks_connection_info_t;

/* Конфигурация Shadowsocks */
typedef struct {
    ss_cipher_method_t default_method;  /* Метод шифрования по умолчанию */
    int support_all_methods;            /* Поддержка всех методов шифрования */
    int enable_udp_forwarding;          /* Включить UDP пересылку */
    int tcp_no_delay;                   /* Включить TCP_NODELAY */
    int timeout_seconds;                /* Таймаут соединения */
} shadowsocks_config_t;

/* Результат определения протокола */
typedef enum {
    PROTO_DETECT_ERROR = -1,
    PROTO_DETECT_NOT_SS = 0,
    PROTO_DETECT_IS_SS = 1
} proto_detect_result_t;

/* Инициализация соединения Shadowsocks */
int shadowsocks_init_connection(shadowsocks_connection_info_t *conn, ss_cipher_method_t method, const char *password, int pwd_len);

/* Определение, является ли трафик Shadowsocks */
proto_detect_result_t shadowsocks_detect_protocol(const unsigned char *data, int len);

/* Шифрование данных для Shadowsocks */
int shadowsocks_encrypt_data(void *in, int in_len, void *out, shadowsocks_connection_info_t *conn);

/* Расшифровка данных для Shadowsocks */
int shadowsocks_decrypt_data(void *in, int in_len, void *out, shadowsocks_connection_info_t *conn);

/* Настройка шифрования */
int shadowsocks_setup_cipher(shadowsocks_connection_info_t *conn, const char *password, ss_cipher_method_t method);

/* Генерация соли */
int shadowsocks_generate_salt(unsigned char *salt, int salt_len);

/* Получение размера ключа для метода */
int shadowsocks_get_key_size(ss_cipher_method_t method);

/* Получение размера IV для метода */
int shadowsocks_get_iv_size(ss_cipher_method_t method);

/* Установка конфигурации Shadowsocks */
int shadowsocks_set_config(const shadowsocks_config_t *config);

/* Получение текущей конфигурации */
const shadowsocks_config_t* shadowsocks_get_config(void);

/* Проверка поддержки метода шифрования */
int shadowsocks_is_cipher_supported(ss_cipher_method_t method);

/* Освобождение ресурсов соединения */
void shadowsocks_free_connection(shadowsocks_connection_info_t *conn);

#ifdef __cplusplus
}
#endif