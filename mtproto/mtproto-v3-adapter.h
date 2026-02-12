/*
 * Адаптер для работы с новыми версиями MTProto (v3.0+)
 *
 * Этот файл содержит интерфейсы для поддержки новых версий протокола MTProto
 * с обеспечением обратной совместимости.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Версии MTProto */
typedef enum {
    MTPROTO_VERSION_2_0 = 0x00000002,
    MTPROTO_VERSION_3_0 = 0x00000003,
    MTPROTO_VERSION_4_0 = 0x00000004,
    MTPROTO_VERSION_LATEST = MTPROTO_VERSION_3_0
} mtproto_version_t;

/* Состояние соединения MTProto */
typedef struct {
    mtproto_version_t version;      /* Версия протокола */
    int features_mask;              /* Маска поддерживаемых возможностей */
    int64_t auth_key_id;            /* ID ключа аутентификации */
    unsigned char auth_key[256];    /* Ключ аутентификации */
    unsigned char tmp_aes_key[32];  /* Временный AES ключ */
    unsigned char server_nonce[16]; /* Случайное число сервера */
    unsigned char client_nonce[16]; /* Случайное число клиента */
    int use_pfs;                    /* Использовать ли PFS */
    int_fast32_t seq_no;            /* Номер последовательности */
    long long session_id;           /* ID сессии */
    long long salt;                 /* Соль */
    uint32_t extra_flags;           /* Дополнительные флаги для новых версий */
    uint64_t connection_id;         /* Уникальный ID соединения */
} mtproto_connection_info_t;

/* Результат определения версии */
typedef enum {
    VERSION_DETECT_ERROR = -1,
    VERSION_DETECT_PENDING = 0,
    VERSION_DETECT_SUCCESS = 1
} version_detect_result_t;

/* Определение версии протокола по начальным байтам */
version_detect_result_t mtproto_detect_version(const unsigned char *data, int len, mtproto_version_t *detected_version);

/* Инициализация соединения с указанной версией протокола */
int mtproto_init_connection(mtproto_connection_info_t *conn, mtproto_version_t version);

/* Обновление версии протокола для существующего соединения */
int mtproto_upgrade_connection_version(mtproto_connection_info_t *conn, mtproto_version_t new_version);

/* Шифрование пакета для MTProto v3 */
int mtproto_encrypt_packet_v3(void *in, int in_len, void *out, mtproto_connection_info_t *conn);

/* Расшифровка пакета для MTProto v3 */
int mtproto_decrypt_packet_v3(void *in, int in_len, void *out, mtproto_connection_info_t *conn);

/* Обработка рукопожатия для MTProto v3 */
int mtproto_handshake_v3(mtproto_connection_info_t *conn, const void *handshake_data, int data_len);

/* Проверка аутентификации для MTProto v3 */
int mtproto_validate_auth_v3(const mtproto_connection_info_t *conn);

/* Получение маски возможностей для версии */
uint32_t mtproto_get_features_mask(mtproto_version_t version);

/* Проверка совместимости версий */
int mtproto_versions_compatible(mtproto_version_t client_version, mtproto_version_t server_version);

/* Обновление информации о соединении */
int mtproto_update_connection_info(mtproto_connection_info_t *conn, const void *update_data, int data_len);

/* Освобождение ресурсов соединения */
void mtproto_free_connection(mtproto_connection_info_t *conn);

#ifdef __cplusplus
}
#endif