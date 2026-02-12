/*
 * Менеджер мультипротокола для MTProxy
 *
 * Центральный компонент для управления несколькими протоколами
 * (MTProto, Shadowsocks и другими) в одном экземпляре MTProxy.
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

#include "multiproto-manager.h"
#include "shadowsocks-adapter.h"
#include "../mtproto/mtproto-version-manager.h"

/* Глобальная конфигурация */
static multiproto_config_t g_multiproto_config = {
    .enable_mtproto = 1,
    .enable_shadowsocks = 1,
    .enable_ohttp = 0,
};

/* Инициализация мультипротокольного менеджера */
multiproto_init_result_t multiproto_manager_init(const multiproto_config_t *config) {
    if (!config) {
        return MULTIPROTO_INIT_ERROR;
    }

    // Копируем конфигурацию
    memcpy(&g_multiproto_config, config, sizeof(multiproto_config_t));

    // Инициализируем внутренние компоненты
    if (g_multiproto_config.enable_mtproto) {
        mtproto_init_result_t mtproto_res = mtproto_version_manager_init(&g_multiproto_config.mtproto_cfg);
        if (mtproto_res != MTPROTO_INIT_OK) {
            return MULTIPROTO_INIT_ERROR;
        }
    }

    if (g_multiproto_config.enable_shadowsocks) {
        if (shadowsocks_set_config(&g_multiproto_config.shadowsocks_cfg) != 0) {
            return MULTIPROTO_INIT_ERROR;
        }
    }

    return MULTIPROTO_INIT_OK;
}

/* Деинициализация мультипротокольного менеджера */
void multiproto_manager_deinit(void) {
    // Деинициализируем внутренние компоненты
    if (g_multiproto_config.enable_mtproto) {
        mtproto_version_manager_deinit();
    }

    // Сбрасываем конфигурацию
    memset(&g_multiproto_config, 0, sizeof(multiproto_config_t));
    g_multiproto_config.enable_mtproto = 1;
    g_multiproto_config.enable_shadowsocks = 1;
    g_multiproto_config.enable_ohttp = 0;
}

/* Определение типа протокола по начальным байтам данных */
multiproto_type_t multiproto_detect_protocol(const unsigned char *data, int len) {
    if (!data || len < 1) {
        return MULTIPROTO_MAX; // Неопределен
    }

    // Проверяем, может ли это быть MTProto
    // MTProto обычно начинается с 4 байт длины сообщения
    if (len >= 8) {
        uint32_t len_field = *((uint32_t*)data);
        // Если длина в пределах разумных значений и после нее идут данные
        if (len_field > 0 && len_field <= 0x1000000 && (len_field + 4) <= len) {
            // Проверяем следующие 4 байта на признаки MTProto
            uint32_t msg_id = *((uint32_t*)(data + 4));
            // В MTProto ID сообщений обычно большие числа
            if (msg_id > 0x10000000) {
                return MULTIPROTO_MTProto;
            }
        }
    }

    // Проверяем, может ли это быть Shadowsocks
    proto_detect_result_t ss_result = shadowsocks_detect_protocol(data, len);
    if (ss_result == PROTO_DETECT_IS_SS) {
        return MULTIPROTO_SHADOWSOCKS;
    }

    // По умолчанию возвращаем MTProto как основной протокол
    return MULTIPROTO_MTProto;
}

/* Инициализация соединения для определенного протокола */
int multiproto_init_connection(multiproto_connection_info_t *conn, multiproto_type_t proto_type) {
    if (!conn) {
        return -1;
    }

    // Инициализируем структуру
    memset(conn, 0, sizeof(multiproto_connection_info_t));
    conn->protocol_type = proto_type;

    // Инициализируем специфичные для протокола данные
    switch(proto_type) {
        case MULTIPROTO_MTProto:
            if (!multiproto_is_protocol_enabled(proto_type)) {
                return -1;
            }
            return mtproto_init_connection(&conn->proto_data.mtproto_conn, g_multiproto_config.mtproto_cfg.default_version);

        case MULTIPROTO_SHADOWSOCKS:
            if (!multiproto_is_protocol_enabled(proto_type)) {
                return -1;
            }
            // Инициализируем с настройками по умолчанию
            return shadowsocks_init_connection(
                &conn->proto_data.shadowsocks_conn,
                g_multiproto_config.shadowsocks_cfg.default_method,
                "default_password", // будет заменено на реальный пароль
                16
            );

        default:
            return -1;
    }
}

/* Шифрование данных с учетом типа протокола */
int multiproto_encrypt_data(void *in, int in_len, void *out, multiproto_connection_info_t *conn) {
    if (!in || !out || !conn) {
        return -1;
    }

    switch(conn->protocol_type) {
        case MULTIPROTO_MTProto:
            return mtproto_encrypt_packet_versioned(in, in_len, out, &conn->proto_data.mtproto_conn);

        case MULTIPROTO_SHADOWSOCKS:
            return shadowsocks_encrypt_data(in, in_len, out, &conn->proto_data.shadowsocks_conn);

        default:
            return -1;
    }
}

/* Расшифровка данных с учетом типа протокола */
int multiproto_decrypt_data(void *in, int in_len, void *out, multiproto_connection_info_t *conn) {
    if (!in || !out || !conn) {
        return -1;
    }

    switch(conn->protocol_type) {
        case MULTIPROTO_MTProto:
            return mtproto_decrypt_packet_versioned(in, in_len, out, &conn->proto_data.mtproto_conn);

        case MULTIPROTO_SHADOWSOCKS:
            return shadowsocks_decrypt_data(in, in_len, out, &conn->proto_data.shadowsocks_conn);

        default:
            return -1;
    }
}

/* Обработка рукопожатия с учетом типа протокола */
int multiproto_handshake(void *conn, const void *handshake_data, int data_len) {
    multiproto_connection_info_t *m_conn = (multiproto_connection_info_t*)conn;
    if (!m_conn || !handshake_data) {
        return -1;
    }

    switch(m_conn->protocol_type) {
        case MULTIPROTO_MTProto:
            return mtproto_handshake_versioned(&m_conn->proto_data.mtproto_conn, handshake_data, data_len);

        case MULTIPROTO_SHADOWSOCKS:
            // Для Shadowsocks рукопожатие обычно не требуется
            // или требует специфичной реализации
            return 0;

        default:
            return -1;
    }
}

/* Установка конфигурации мультипротокола */
int multiproto_set_config(const multiproto_config_t *config) {
    if (!config) {
        return -1;
    }

    // Проверяем, можем ли мы поддержать запрашиваемую конфигурацию
    if (config->enable_mtproto) {
        if (mtproto_set_version_config(&config->mtproto_cfg) != 0) {
            return -1;
        }
    }

    if (config->enable_shadowsocks) {
        if (shadowsocks_set_config(&config->shadowsocks_cfg) != 0) {
            return -1;
        }
    }

    // Копируем конфигурацию
    memcpy(&g_multiproto_config, config, sizeof(multiproto_config_t));

    return 0;
}

/* Получение текущей конфигурации */
const multiproto_config_t* multiproto_get_config(void) {
    return &g_multiproto_config;
}

/* Проверка, включен ли протокол */
int multiproto_is_protocol_enabled(multiproto_type_t proto_type) {
    switch(proto_type) {
        case MULTIPROTO_MTProto:
            return g_multiproto_config.enable_mtproto;

        case MULTIPROTO_SHADOWSOCKS:
            return g_multiproto_config.enable_shadowsocks;

        case MULTIPROTO_OHTTP:
            return g_multiproto_config.enable_ohttp;

        default:
            return 0;
    }
}

/* Получение строкового представления типа протокола */
const char* multiproto_type_to_string(multiproto_type_t proto_type) {
    switch(proto_type) {
        case MULTIPROTO_MTProto:
            return "MTProto";

        case MULTIPROTO_SHADOWSOCKS:
            return "Shadowsocks";

        case MULTIPROTO_OHTTP:
            return "Oblivious-HTTP";

        default:
            return "Unknown";
    }
}

/* Обновление статистики соединения */
void multiproto_update_stats(multiproto_connection_info_t *conn, int sent_bytes, int received_bytes) {
    if (!conn) {
        return;
    }

    conn->bytes_sent += sent_bytes;
    conn->bytes_received += received_bytes;
}