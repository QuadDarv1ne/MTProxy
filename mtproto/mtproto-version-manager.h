/*
 * Менеджер версий MTProto
 *
 * Центральный компонент для управления различными версиями протокола MTProto
 * и обеспечения совместимости между ними.
 */

#pragma once

#include "mtproto-v3-adapter.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Конфигурация поддержки версий */
typedef struct {
    mtproto_version_t min_version;      /* Минимальная поддерживаемая версия */
    mtproto_version_t max_version;      /* Максимальная поддерживаемая версия */
    mtproto_version_t default_version;  /* Версия по умолчанию */
    int enable_autoupgrade;             /* Разрешить автоповышение версии */
    uint32_t supported_features;        /* Поддерживаемые глобальные возможности */
} mtproto_version_config_t;

/* Результат инициализации версии */
typedef enum {
    MTPROTO_INIT_OK = 0,
    MTPROTO_INIT_UNSUPPORTED_VERSION = -1,
    MTPROTO_INIT_CONFIG_ERROR = -2,
    MTPROTO_INIT_RESOURCE_ERROR = -3
} mtproto_init_result_t;

/* Инициализация системы управления версиями */
mtproto_init_result_t mtproto_version_manager_init(const mtproto_version_config_t *config);

/* Деинициализация системы управления версиями */
void mtproto_version_manager_deinit(void);

/* Установка конфигурации поддержки версий */
int mtproto_set_version_config(const mtproto_version_config_t *config);

/* Получение текущей конфигурации */
const mtproto_version_config_t* mtproto_get_version_config(void);

/* Определение наилучшей версии для соединения */
mtproto_version_t mtproto_select_best_version(mtproto_version_t client_version);

/* Проверка, поддерживается ли версия */
int mtproto_is_version_supported(mtproto_version_t version);

/* Обновление версии соединения с учетом конфигурации */
int mtproto_connection_update_version(mtproto_connection_info_t *conn);

/* Получение строки версии для отображения */
const char* mtproto_version_to_string(mtproto_version_t version);

/* Парсинг строки версии */
mtproto_version_t mtproto_parse_version_string(const char *version_str);

/* Проверка, является ли версия новее указанной */
int mtproto_is_newer_version(mtproto_version_t version1, mtproto_version_t version2);

/* Функции для работы с криптографией на основе версии */
int mtproto_encrypt_packet_versioned(void *in, int in_len, void *out, mtproto_connection_info_t *conn);
int mtproto_decrypt_packet_versioned(void *in, int in_len, void *out, mtproto_connection_info_t *conn);
int mtproto_handshake_versioned(mtproto_connection_info_t *conn, const void *handshake_data, int data_len);

#ifdef __cplusplus
}
#endif