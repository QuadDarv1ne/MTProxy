/*
 * Утилиты безопасности для MTProxy
 * Вспомогательные функции для различных задач безопасности
 */

#ifndef _SECURITY_UTILS_H_
#define _SECURITY_UTILS_H_

#include <stdint.h>

// Типы хэшей
typedef enum {
    HASH_SHA256 = 0,
    HASH_SHA512 = 1,
    HASH_MD5 = 2
} hash_type_t;

// Результаты проверки безопасности
typedef enum {
    SEC_VERIFY_OK = 0,
    SEC_VERIFY_FAILED = 1,
    SEC_VERIFY_CERT_ERROR = 2,
    SEC_VERIFY_SIGNATURE_ERROR = 3,
    SEC_VERIFY_TIMEOUT = 4
} security_verify_result_t;

// Структура для хранения информации о сертификате
typedef struct {
    char subject[256];
    char issuer[256];
    char serial_number[64];
    char fingerprint[64];  // Hex representation
    long long not_before;
    long long not_after;
    int valid;
} cert_info_t;

// Функции для работы с хэшами
int sec_compute_hash(hash_type_t type, const unsigned char *data, int data_len, 
                    unsigned char *hash_out, int *hash_len);
                    
// Функции для проверки сертификатов
int sec_parse_certificate(const unsigned char *cert_der, int cert_len, cert_info_t *cert_info);
int sec_validate_certificate_chain(const cert_info_t *certs, int cert_count);
int sec_check_certificate_validity_period(const cert_info_t *cert);

// Функции для работы с безопасностью
security_verify_result_t sec_verify_signature(const unsigned char *data, int data_len,
                                            const unsigned char *signature, int sig_len,
                                            const unsigned char *pubkey, int key_len);
                                            
// Функции для генерации случайных чисел
int sec_generate_random_bytes(unsigned char *out, int len);

// Функции для очистки памяти
void sec_clear_memory(void *ptr, int len);

// Функции для проверки целостности
int sec_verify_data_integrity(const unsigned char *data, int data_len, 
                             const unsigned char *expected_hash, hash_type_t hash_type);

#endif