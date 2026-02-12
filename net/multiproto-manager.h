/*
 * Менеджер мультипротокола для MTProxy
 *
 * Центральный компонент для управления несколькими протоколами
 * (MTProto, Shadowsocks и другими) в одном экземпляре MTProxy.
 */

#pragma once

#include "shadowsocks-adapter.h"
#include "../mtproto/mtproto-version-manager.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Поддерживаемые протоколы */
typedef enum {
    MULTIPROTO_MTProto = 0,
    MULTIPROTO_SHADOWSOCKS,
    MULTIPROTO_OHTTP,  // Oblivious HTTP (будет реализовано позже)
    MULTIPROTO_MAX
} multiproto_type_t;

/* Информация о соединении с поддержкой нескольких протоколов */
typedef struct {
    multiproto_type_t protocol_type;  /* Тип протокола */
    
    /* Объединение для данных разных протоколов */
    union {
        mtproto_connection_info_t mtproto_conn;
        shadowsocks_connection_info_t shadowsocks_conn;
    } proto_data;
    
    uint64_t connection_id;           /* Уникальный ID соединения */
    int_fast32_t seq_no;              /* Номер последовательности */
    double connect_time;              /* Время подключения */
    double last_activity;             /* Время последней активности */
    uint64_t bytes_sent;              /* Отправленные байты */
    uint64_t bytes_received;          /* Полученные байты */
} multiproto_connection_info_t;

/* Конфигурация мультипротокола */
typedef struct {
    int enable_mtproto;               /* Включить поддержку MTProto */
    int enable_shadowsocks;           /* Включить поддержку Shadowsocks */
    int enable_ohttp;                 /* Включить поддержку Oblivious HTTP */
    mtproto_version_config_t mtproto_cfg;  /* Конфигурация MTProto */
    shadowsocks_config_t shadowsocks_cfg;  /* Конфигурация Shadowsocks */
} multiproto_config_t;

/* Результат инициализации */
typedef enum {
    MULTIPROTO_INIT_OK = 0,
    MULTIPROTO_INIT_ERROR = -1,
    MULTIPROTO_INIT_UNSUPPORTED_PROTO = -2
} multiproto_init_result_t;

/* Инициализация мультипротокольного менеджера */
multiproto_init_result_t multiproto_manager_init(const multiproto_config_t *config);

/* Деинициализация мультипротокольного менеджера */
void multiproto_manager_deinit(void);

/* Определение типа протокола по начальным байтам данных */
multiproto_type_t multiproto_detect_protocol(const unsigned char *data, int len);

/* Инициализация соединения для определенного протокола */
int multiproto_init_connection(multiproto_connection_info_t *conn, multiproto_type_t proto_type);

/* Шифрование данных с учетом типа протокола */
int multiproto_encrypt_data(void *in, int in_len, void *out, multiproto_connection_info_t *conn);

/* Расшифровка данных с учетом типа протокола */
int multiproto_decrypt_data(void *in, int in_len, void *out, multiproto_connection_info_t *conn);

/* Обработка рукопожатия с учетом типа протокола */
int multiproto_handshake(void *conn, const void *handshake_data, int data_len);

/* Установка конфигурации мультипротокола */
int multiproto_set_config(const multiproto_config_t *config);

/* Получение текущей конфигурации */
const multiproto_config_t* multiproto_get_config(void);

/* Проверка, включен ли протокол */
int multiproto_is_protocol_enabled(multiproto_type_t proto_type);

/* Получение строкового представления типа протокола */
const char* multiproto_type_to_string(multiproto_type_t proto_type);

/* Обновление статистики соединения */
void multiproto_update_stats(multiproto_connection_info_t *conn, int sent_bytes, int received_bytes);

#ifdef __cplusplus
}
#endif