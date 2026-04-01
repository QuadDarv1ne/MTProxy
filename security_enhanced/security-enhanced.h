#ifndef SECURITY_ENHANCED_H
#define SECURITY_ENHANCED_H

#include <stdint.h>

/*
 * Типы поддерживаемых шифров
 */
typedef enum {
    CIPHER_AES_128_GCM,
    CIPHER_AES_256_GCM,
    CIPHER_CHACHA20_POLY1305,
    CIPHER_XCHACHA20_POLY1305,
    CIPHER_POST_QUANTUM_TEST  // Для будущей интеграции пост-квантового шифрования
} cipher_type_t;

/*
 * Структура для хранения информации о шифровании
 */
typedef struct cipher_context {
    cipher_type_t cipher_type;
    uint8_t *key;
    uint8_t *iv;  // Синхропосылка
    size_t key_length;
    size_t iv_length;
    void *internal_state;  // Внутреннее состояние шифра
} cipher_context_t;

/*
 * Структура для хранения информации о сертификатах
 */
typedef struct cert_info {
    char issuer[256];      // Издатель сертификата
    char subject[256];     // Владелец сертификата
    char fingerprint[64];  // Отпечаток сертификата
    long valid_from;       // Дата начала действия (Unix timestamp)
    long valid_until;      // Дата окончания действия (Unix timestamp)
    int pinned;           // Флаг прикрепления сертификата
} cert_info_t;

/*
 * Инициализирует контекст шифрования
 * @param ctx: указатель на контекст шифрования
 * @param cipher_type: тип шифра
 * @param key: ключ шифрования
 * @param key_length: длина ключа
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int init_cipher_context(cipher_context_t *ctx, cipher_type_t cipher_type, 
                        uint8_t *key, size_t key_length);

/*
 * Шифрует данные с помощью заданного контекста
 * @param ctx: указатель на контекст шифрования
 * @param plaintext: открытый текст
 * @param plaintext_len: длина открытого текста
 * @param ciphertext: зашифрованный текст
 * @param auth_tag: тег аутентификации (для AEAD шифров)
 * @return: длина зашифрованных данных или -1 в случае ошибки
 */
int encrypt_data(cipher_context_t *ctx, uint8_t *plaintext, size_t plaintext_len,
                 uint8_t *ciphertext, uint8_t *auth_tag);

/*
 * Расшифровывает данные с помощью заданного контекста
 * @param ctx: указатель на контекст шифрования
 * @param ciphertext: зашифрованный текст
 * @param ciphertext_len: длина зашифрованного текста
 * @param plaintext: открытый текст
 * @param auth_tag: тег аутентификации (для AEAD шифров)
 * @return: длина расшифрованных данных или -1 в случае ошибки
 */
int decrypt_data(cipher_context_t *ctx, uint8_t *ciphertext, size_t ciphertext_len,
                 uint8_t *plaintext, uint8_t *auth_tag);

/*
 * Обновляет ключ шифрования
 * @param ctx: указатель на контекст шифрования
 * @param new_key: новый ключ
 * @param key_length: длина нового ключа
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int update_cipher_key(cipher_context_t *ctx, uint8_t *new_key, size_t key_length);

/*
 * Освобождает ресурсы, связанные с контекстом шифрования
 * @param ctx: указатель на контекст шифрования
 */
void free_cipher_context(cipher_context_t *ctx);

/*
 * Проверяет действительность сертификата
 * @param cert: информация о сертификате
 * @return: 1 если действителен, 0 если недействителен
 */
int is_certificate_valid(cert_info_t *cert);

/*
 * Прикрепляет сертификат (certificate pinning)
 * @param cert: информация о сертификате для прикрепления
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int pin_certificate(cert_info_t *cert);

/*
 * Проверяет прикрепленный сертификат
 * @param cert: информация о сертификате для проверки
 * @return: 1 если сертификат прикреплен и действителен, 0 в противном случае
 */
int verify_pinned_certificate(cert_info_t *cert);

#endif