/*
 * Адаптер для интеграции с протоколом Shadowsocks
 *
 * Реализация интерфейсов для поддержки протокола Shadowsocks
 * в составе MTProxy.
 */

#ifndef __SIZE_TYPE__
#define __SIZE_TYPE__ unsigned long
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef __SIZE_TYPE__ size_t;
#endif

#ifndef _TIME_T
#define _TIME_T
typedef long time_t;
#endif

// Define basic functions if not available
#ifndef __STRING_H_
#define __STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);

#ifdef __cplusplus
}
#endif

#endif

#include "shadowsocks-adapter.h"

/* Глобальная конфигурация */
static shadowsocks_config_t g_ss_config = {
    .default_method = SS_CIPHER_CHACHA20_POLY1305,
    .support_all_methods = 1,
    .enable_udp_forwarding = 1,
    .tcp_no_delay = 1,
    .timeout_seconds = 300
};

/* Размеры ключей для разных методов */
static const int key_sizes[] = {
    0,  // SS_CIPHER_NONE
    32, // SS_CIPHER_CHACHA20_POLY1305
    32, // SS_CIPHER_AES_256_GCM
    24, // SS_CIPHER_AES_192_GCM
    16, // SS_CIPHER_AES_128_GCM
    32  // SS_CIPHER_XCHACHA20_POLY1305
};

/* Размеры IV для разных методов */
static const int iv_sizes[] = {
    0,  // SS_CIPHER_NONE
    16, // SS_CIPHER_CHACHA20_POLY1305
    12, // SS_CIPHER_AES_256_GCM
    12, // SS_CIPHER_AES_192_GCM
    12, // SS_CIPHER_AES_128_GCM
    24  // SS_CIPHER_XCHACHA20_POLY1305
};

/* Проверка, поддерживается ли метод шифрования */
int shadowsocks_is_cipher_supported(ss_cipher_method_t method) {
    return (method > SS_CIPHER_NONE && method < SS_CIPHER_MAX);
}

/* Инициализация соединения Shadowsocks */
int shadowsocks_init_connection(shadowsocks_connection_info_t *conn, ss_cipher_method_t method, const char *password, int pwd_len) {
    if (!conn || !password || pwd_len <= 0 || !shadowsocks_is_cipher_supported(method)) {
        return -1;
    }

    // Инициализируем поля структуры
    memset(conn, 0, sizeof(shadowsocks_connection_info_t));

    conn->method = method;
    conn->password_len = (pwd_len > 256) ? 256 : pwd_len;
    memcpy(conn->password, password, conn->password_len);
    conn->iv_len = shadowsocks_get_iv_size(method);
    conn->connection_id = 0;
    conn->seq_no = 0;
    conn->bytes_sent = 0;
    conn->bytes_received = 0;

    // Генерируем случайную соль
    shadowsocks_generate_salt(conn->salt, sizeof(conn->salt));

    return 0;
}

/* Определение, является ли трафик Shadowsocks */
proto_detect_result_t shadowsocks_detect_protocol(const unsigned char *data, int len) {
    if (!data || len < 1) {
        return PROTO_DETECT_ERROR;
    }

    // Простая эвристика для определения протокола Shadowsocks
    // В реальности потребуется более сложная логика распознавания
    
    // Shadowsocks начинается с длины адреса (1 байт), затем адрес и порт
    // Но на начальном этапе мы просто проверим, есть ли какие-то признаки
    
    if (len < 2) {
        return PROTO_DETECT_NOT_SS;
    }

    // Первый байт содержит длину адреса (обычно 0x01, 0x04 или 0x08 для IPv4/IPv6)
    unsigned char addr_len = data[0];
    
    // Проверяем, находится ли длина адреса в разумных пределах
    if ((addr_len == 1 || addr_len == 4 || addr_len == 8 || addr_len == 16) && len >= (int)(1 + addr_len + 2)) {
        return PROTO_DETECT_IS_SS;
    }

    return PROTO_DETECT_NOT_SS;
}

/* Шифрование данных для Shadowsocks */
int shadowsocks_encrypt_data(void *in, int in_len, void *out, shadowsocks_connection_info_t *conn) {
    if (!in || !out || !conn || in_len <= 0 || !shadowsocks_is_cipher_supported(conn->method)) {
        return -1;
    }

    // В реальной реализации здесь будет шифрование с использованием выбранного метода
    // Пока реализуем заглушку, указывающую на необходимость реализации
    
    unsigned char *input = (unsigned char*)in;
    unsigned char *output = (unsigned char*)out;

    // Для демонстрации просто копируем данные
    // В реальной реализации нужно будет использовать соответствующий шифр
    memcpy(output, input, in_len);
    
    conn->bytes_sent += in_len;

    return in_len; // Возвращаем длину зашифрованных данных
}

/* Расшифровка данных для Shadowsocks */
int shadowsocks_decrypt_data(void *in, int in_len, void *out, shadowsocks_connection_info_t *conn) {
    if (!in || !out || !conn || in_len <= 0 || !shadowsocks_is_cipher_supported(conn->method)) {
        return -1;
    }

    // В реальной реализации здесь будет расшифровка с использованием выбранного метода
    // Пока реализуем заглушку, указывающую на необходимость реализации
    
    unsigned char *input = (unsigned char*)in;
    unsigned char *output = (unsigned char*)out;

    // Для демонстрации просто копируем данные
    // В реальной реализации нужно будет использовать соответствующий дешифр
    memcpy(output, input, in_len);
    
    conn->bytes_received += in_len;

    return in_len; // Возвращаем длину расшифрованных данных
}

/* Настройка шифрования */
int shadowsocks_setup_cipher(shadowsocks_connection_info_t *conn, const char *password, ss_cipher_method_t method) {
    if (!conn || !password || !shadowsocks_is_cipher_supported(method)) {
        return -1;
    }

    // Устанавливаем метод шифрования и пароль
    conn->method = method;
    int pwd_len = strlen(password);
    conn->password_len = (pwd_len > 256) ? 256 : pwd_len;
    memcpy(conn->password, password, conn->password_len);

    // Генерируем соль
    shadowsocks_generate_salt(conn->salt, sizeof(conn->salt));

    return 0;
}

/* Генерация соли */
int shadowsocks_generate_salt(unsigned char *salt, int salt_len) {
    if (!salt || salt_len <= 0) {
        return -1;
    }

    // В реальной реализации здесь будет криптографически безопасная генерация соли
    // Пока используем простую реализацию для демонстрации
    
    for (int i = 0; i < salt_len; i++) {
        salt[i] = (unsigned char)(i % 256); // Заменить на真正的随机生成
    }

    return 0;
}

/* Получение размера ключа для метода */
int shadowsocks_get_key_size(ss_cipher_method_t method) {
    if (method <= SS_CIPHER_NONE || method >= SS_CIPHER_MAX) {
        return 0;
    }
    
    return key_sizes[method];
}

/* Получение размера IV для метода */
int shadowsocks_get_iv_size(ss_cipher_method_t method) {
    if (method <= SS_CIPHER_NONE || method >= SS_CIPHER_MAX) {
        return 0;
    }
    
    return iv_sizes[method];
}

/* Установка конфигурации Shadowsocks */
int shadowsocks_set_config(const shadowsocks_config_t *config) {
    if (!config) {
        return -1;
    }

    if (!shadowsocks_is_cipher_supported(config->default_method)) {
        return -1;
    }

    memcpy(&g_ss_config, config, sizeof(shadowsocks_config_t));

    return 0;
}

/* Получение текущей конфигурации */
const shadowsocks_config_t* shadowsocks_get_config(void) {
    return &g_ss_config;
}

/* Освобождение ресурсов соединения */
void shadowsocks_free_connection(shadowsocks_connection_info_t *conn) {
    if (conn) {
        // Обнуляем чувствительные данные
        memset(conn->password, 0, sizeof(conn->password));
        memset(conn->salt, 0, sizeof(conn->salt));
        memset(conn->iv, 0, sizeof(conn->iv));

        // Сбрасываем остальные поля
        memset(conn, 0, sizeof(shadowsocks_connection_info_t));
    }
}