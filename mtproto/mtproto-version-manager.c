/*
 * Менеджер версий MTProto
 *
 * Центральный компонент для управления различными версиями протокола MTProto
 * и обеспечения совместимости между ними.
 */

#include "mtproto-version-manager.h"
#include "mtproto-v3-adapter.h"

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

#ifdef __cplusplus
}
#endif

#endif

/* Проверка, поддерживается ли данная версия */
static int is_supported_version(mtproto_version_t version) {
    return (version == MTPROTO_VERSION_2_0 || 
            version == MTPROTO_VERSION_3_0 || 
            version == MTPROTO_VERSION_4_0);
}

/* Глобальная конфигурация */
static mtproto_version_config_t g_config = {
    .min_version = MTPROTO_VERSION_2_0,
    .max_version = MTPROTO_VERSION_3_0,
    .default_version = MTPROTO_VERSION_2_0,
    .enable_autoupgrade = 0,
    .supported_features = 0x00000007
};

/* Инициализация системы управления версиями */
mtproto_init_result_t mtproto_version_manager_init(const mtproto_version_config_t *config) {
    if (!config) {
        return MTPROTO_INIT_CONFIG_ERROR;
    }
    
    // Проверяем корректность конфигурации
    if (config->min_version > config->max_version) {
        return MTPROTO_INIT_CONFIG_ERROR;
    }
    
    if (!is_supported_version(config->min_version) || !is_supported_version(config->max_version)) {
        return MTPROTO_INIT_UNSUPPORTED_VERSION;
    }
    
    // Копируем конфигурацию
    memcpy(&g_config, config, sizeof(mtproto_version_config_t));
    
    return MTPROTO_INIT_OK;
}

/* Деинициализация системы управления версиями */
void mtproto_version_manager_deinit(void) {
    // Сбросим конфигурацию к значениям по умолчанию
    g_config.min_version = MTPROTO_VERSION_2_0;
    g_config.max_version = MTPROTO_VERSION_3_0;
    g_config.default_version = MTPROTO_VERSION_2_0;
    g_config.enable_autoupgrade = 0;
    g_config.supported_features = 0x00000007;
}

/* Установка конфигурации поддержки версий */
int mtproto_set_version_config(const mtproto_version_config_t *config) {
    if (!config) {
        return -1;
    }
    
    // Проверяем корректность конфигурации
    if (config->min_version > config->max_version) {
        return -1;
    }
    
    if (!is_supported_version(config->min_version) || !is_supported_version(config->max_version)) {
        return -1;
    }
    
    // Копируем конфигурацию
    memcpy(&g_config, config, sizeof(mtproto_version_config_t));
    
    return 0;
}

/* Получение текущей конфигурации */
const mtproto_version_config_t* mtproto_get_version_config(void) {
    return &g_config;
}

/* Определение наилучшей версии для соединения */
mtproto_version_t mtproto_select_best_version(mtproto_version_t client_version) {
    if (!is_supported_version(client_version)) {
        return g_config.default_version;
    }
    
    // Если версия клиента поддерживается, проверяем, входит ли она в диапазон
    if (client_version >= g_config.min_version && client_version <= g_config.max_version) {
        // Возвращаем максимальную из возможных версий
        if (g_config.enable_autoupgrade && client_version < g_config.max_version) {
            return g_config.max_version;
        }
        return client_version;
    }
    
    // Если версия вне диапазона, выбираем ближайшую подходящую
    if (client_version < g_config.min_version) {
        return g_config.min_version;
    }
    
    return g_config.max_version;
}

/* Проверка, поддерживается ли версия */
int mtproto_is_version_supported(mtproto_version_t version) {
    return (version >= g_config.min_version && version <= g_config.max_version);
}

/* Обновление версии соединения с учетом конфигурации */
int mtproto_connection_update_version(mtproto_connection_info_t *conn) {
    if (!conn) {
        return -1;
    }
    
    // Проверяем, поддерживается ли текущая версия
    if (!mtproto_is_version_supported(conn->version)) {
        // Пытаемся обновить до поддерживаемой версии
        mtproto_version_t new_version = mtproto_select_best_version(conn->version);
        
        if (new_version != conn->version) {
            return mtproto_upgrade_connection_version(conn, new_version);
        }
        
        return -1; // Версия не поддерживается и не может быть обновлена
    }
    
    return 0;
}

/* Получение строки версии для отображения */
const char* mtproto_version_to_string(mtproto_version_t version) {
    switch(version) {
        case MTPROTO_VERSION_2_0:
            return "MTProto 2.0";
        case MTPROTO_VERSION_3_0:
            return "MTProto 3.0";
        case MTPROTO_VERSION_4_0:
            return "MTProto 4.0";
        default:
            return "Unknown Version";
    }
}

/* Парсинг строки версии */
mtproto_version_t mtproto_parse_version_string(const char *version_str) {
    if (!version_str) {
        return MTPROTO_VERSION_2_0; // Значение по умолчанию
    }
    
    if (strcmp(version_str, "2.0") == 0 || strcmp(version_str, "MTProto 2.0") == 0) {
        return MTPROTO_VERSION_2_0;
    }
    else if (strcmp(version_str, "3.0") == 0 || strcmp(version_str, "MTProto 3.0") == 0) {
        return MTPROTO_VERSION_3_0;
    }
    else if (strcmp(version_str, "4.0") == 0 || strcmp(version_str, "MTProto 4.0") == 0) {
        return MTPROTO_VERSION_4_0;
    }
    
    // Если строка не распознана, возвращаем значение по умолчанию
    return MTPROTO_VERSION_2_0;
}

/* Проверка, является ли версия новее указанной */
int mtproto_is_newer_version(mtproto_version_t version1, mtproto_version_t version2) {
    return (version1 > version2);
}

/* Функции для работы с криптографией на основе версии */
int mtproto_encrypt_packet_versioned(void *in, int in_len, void *out, mtproto_connection_info_t *conn) {
    if (!conn) {
        return -1;
    }
    
    // Выбираем реализацию в зависимости от версии
    switch(conn->version) {
        case MTPROTO_VERSION_3_0:
            return mtproto_encrypt_packet_v3(in, in_len, out, conn);
        case MTPROTO_VERSION_2_0:
        case MTPROTO_VERSION_4_0:
        default:
            // Для других версий используем существующую реализацию
            // или возвращаем ошибку, если нужна специфичная реализация
            return -2; // Требует реализации
    }
}

int mtproto_decrypt_packet_versioned(void *in, int in_len, void *out, mtproto_connection_info_t *conn) {
    if (!conn) {
        return -1;
    }
    
    // Выбираем реализацию в зависимости от версии
    switch(conn->version) {
        case MTPROTO_VERSION_3_0:
            return mtproto_decrypt_packet_v3(in, in_len, out, conn);
        case MTPROTO_VERSION_2_0:
        case MTPROTO_VERSION_4_0:
        default:
            // Для других версий используем существующую реализацию
            // или возвращаем ошибку, если нужна специфичная реализация
            return -2; // Требует реализации
    }
}

int mtproto_handshake_versioned(mtproto_connection_info_t *conn, const void *handshake_data, int data_len) {
    if (!conn || !handshake_data) {
        return -1;
    }
    
    // Выбираем реализацию в зависимости от версии
    switch(conn->version) {
        case MTPROTO_VERSION_3_0:
            return mtproto_handshake_v3(conn, handshake_data, data_len);
        case MTPROTO_VERSION_2_0:
        case MTPROTO_VERSION_4_0:
        default:
            // Для других версий используем существующую реализацию
            // или возвращаем ошибку, если нужна специфичная реализация
            return -2; // Требует реализации
    }
}