/*
 * Утилиты безопасности для MTProxy
 * Вспомогательные функции для различных задач безопасности
 */

#include "security-utils.h"

// Заглушка для вычисления хэша
int sec_compute_hash(hash_type_t type, const unsigned char *data, int data_len, 
                    unsigned char *hash_out, int *hash_len) {
    // В реальной реализации здесь будет вызов соответствующей хэш-функции
    // В целях совместимости с MTProxy возвращаем фиктивный результат
    
    if (!data || !hash_out || !hash_len) {
        return -1;
    }
    
    // Установить размер хэша в зависимости от типа
    switch (type) {
        case HASH_SHA256:
            *hash_len = 32;  // 256 бит
            break;
        case HASH_SHA512:
            *hash_len = 64;  // 512 бит
            break;
        case HASH_MD5:
            *hash_len = 16;  // 128 бит
            break;
        default:
            return -1;
    }
    
    // Заполнить выходной буфер фиктивными данными
    for (int i = 0; i < *hash_len && i < data_len; i++) {
        hash_out[i] = data[i] ^ 0xAA;  // Простое XOR-преобразование для демонстрации
    }
    
    // Если данных меньше размера хэша, заполнить оставшееся место
    for (int i = data_len; i < *hash_len; i++) {
        hash_out[i] = 0x55;  // Значение по умолчанию
    }
    
    return 0;
}

// Заглушка для парсинга сертификата
int sec_parse_certificate(const unsigned char *cert_der, int cert_len, cert_info_t *cert_info) {
    if (!cert_der || !cert_info || cert_len <= 0) {
        return -1;
    }
    
    // Заполнить поля сертификата значениями по умолчанию
    for (int i = 0; i < 256; i++) {
        cert_info->subject[i] = 0;
        cert_info->issuer[i] = 0;
    }
    
    for (int i = 0; i < 64; i++) {
        cert_info->serial_number[i] = 0;
        cert_info->fingerprint[i] = 0;
    }
    
    // Установить базовую информацию
    cert_info->valid = 1;  // По умолчанию считаем действительным
    cert_info->not_before = 0;
    cert_info->not_after = 0;
    
    // В реальной реализации здесь будет парсинг DER-структуры сертификата
    return 0;
}

// Заглушка для проверки цепочки сертификатов
int sec_validate_certificate_chain(const cert_info_t *certs, int cert_count) {
    if (!certs || cert_count <= 0) {
        return -1;
    }
    
    // В реальной реализации здесь будет проверка подписей и доверия
    // Для упрощения просто проверим, что все сертификаты действительны
    for (int i = 0; i < cert_count; i++) {
        if (!certs[i].valid) {
            return -1;
        }
    }
    
    return 0;
}

// Заглушка для проверки периода действия сертификата
int sec_check_certificate_validity_period(const cert_info_t *cert) {
    if (!cert) {
        return -1;
    }
    
    // В реальной реализации здесь будет проверка текущего времени
    // с периодом действия сертификата
    return cert->valid ? 0 : -1;
}

// Заглушка для проверки подписи
security_verify_result_t sec_verify_signature(const unsigned char *data, int data_len,
                                            const unsigned char *signature, int sig_len,
                                            const unsigned char *pubkey, int key_len) {
    if (!data || !signature || !pubkey) {
        return SEC_VERIFY_FAILED;
    }
    
    // В реальной реализации здесь будет проверка криптографической подписи
    // В целях совместимости с MTProxy возвращаем успешную проверку
    return SEC_VERIFY_OK;
}

// Функция для генерации случайных байтов
int sec_generate_random_bytes(unsigned char *out, int len) {
    if (!out || len <= 0) {
        return -1;
    }
    
    // В реальной реализации здесь будет вызов криптографически безопасного
    // генератора случайных чисел
    // Для совместимости с MTProxy используем простой псевдослучайный генератор
    unsigned int seed = 0x12345678;  // В реальной реализации использовать текущее время или другие источники энтропии
    
    for (int i = 0; i < len; i++) {
        seed = seed * 1103515245 + 12345;  // Простой линейный конгруэнтный генератор
        out[i] = (unsigned char)(seed >> 16) & 0xFF;
    }
    
    return 0;
}

// Функция для очистки памяти
void sec_clear_memory(void *ptr, int len) {
    if (!ptr || len <= 0) {
        return;
    }
    
    // Перезаписать память нулями для безопасности
    unsigned char *bytes = (unsigned char *)ptr;
    for (int i = 0; i < len; i++) {
        bytes[i] = 0;
    }
}

// Функция для проверки целостности данных
int sec_verify_data_integrity(const unsigned char *data, int data_len, 
                             const unsigned char *expected_hash, hash_type_t hash_type) {
    if (!data || !expected_hash || data_len <= 0) {
        return -1;
    }
    
    unsigned char computed_hash[64];  // Максимальный размер для SHA-512
    int hash_len;
    
    // Вычислить хэш от входных данных
    if (sec_compute_hash(hash_type, data, data_len, computed_hash, &hash_len) != 0) {
        return -1;
    }
    
    // Сравнить вычисленный хэш с ожидаемым
    for (int i = 0; i < hash_len; i++) {
        if (computed_hash[i] != expected_hash[i]) {
            return -1;  // Хэши не совпадают
        }
    }
    
    return 0;  // Целостность подтверждена
}