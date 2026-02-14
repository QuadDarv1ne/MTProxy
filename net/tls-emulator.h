/*
 * Расширенная система эмуляции TLS-соединений для MTProxy
 * Поддержка различных версий протокола и адаптивная обфускация
 */

#ifndef _TLS_EMULATOR_H_
#define _TLS_EMULATOR_H_

#include <stdint.h>

// Версии TLS
typedef enum {
    TLS_VERSION_SSL3 = 0x0300,
    TLS_VERSION_TLS10 = 0x0301,
    TLS_VERSION_TLS11 = 0x0302,
    TLS_VERSION_TLS12 = 0x0303,
    TLS_VERSION_TLS13 = 0x0304
} tls_version_t;

// Типы обфускации TLS
typedef enum {
    TLS_OBFUSCATION_NONE = 0,
    TLS_OBFUSCATION_BASIC = 1,
    TLS_OBFUSCATION_EXTENDED = 2,
    TLS_OBFUSCATION_FULL = 3
} tls_obfuscation_level_t;

// Статус TLS-эмулятора
typedef enum {
    TLS_EMULATOR_STATUS_UNINITIALIZED = 0,
    TLS_EMULATOR_STATUS_INITIALIZED = 1,
    TLS_EMULATOR_STATUS_ACTIVE = 2,
    TLS_EMULATOR_STATUS_ERROR = 3
} tls_emulator_status_t;

// Статистика TLS-эмулятора
typedef struct {
    long long total_connections;
    long long successful_emulations;
    long long failed_emulations;
    long long version_mismatches;
    long long protocol_violations;
    long long adaptive_changes;
    tls_emulator_status_t current_status;
    int emulation_success_rate;
    int current_tls_version;
} tls_emulator_stats_t;

// Конфигурация TLS-эмулятора
typedef struct {
    int enable_tls_emulation;
    tls_version_t preferred_version;
    tls_version_t min_supported_version;
    tls_version_t max_supported_version;
    int enable_version_randomization;
    int enable_cipher_randomization;
    int enable_session_resumption;
    int enable_extended_obfuscation;
    int adaptive_emulation;
    int mimic_browser_behavior;
    int randomize_handshake_timing;
    int max_handshake_delay_ms;
} tls_emulator_config_t;

// Контекст TLS-эмулятора
typedef struct {
    tls_emulator_config_t config;
    tls_emulator_stats_t stats;
    tls_emulator_status_t status;
    void *tls_state;
    void *cipher_suite_list;
    void *session_cache;
    int current_version;
    int handshake_completed;
    int session_resumed;
    long long last_handshake_time;
    int handshake_delay_ms;
} tls_emulator_context_t;

// Структура для данных TLS-рукопожатия
typedef struct {
    tls_version_t client_version;
    tls_version_t server_version;
    int cipher_suite;
    int compression_method;
    int extensions_length;
    unsigned char *extensions_data;
    int session_id_length;
    unsigned char *session_id;
    int random_length;
    unsigned char *random_data;
} tls_handshake_data_t;

// Структура для эмуляции браузера
typedef struct {
    char user_agent[256];
    char client_hello_extensions[1024];
    int tls_extensions_count;
    int supported_groups[32];
    int signature_algorithms[32];
    int application_layer_protocols[16];
    int grease_values[8];
    int padding_length;
} browser_emulation_profile_t;

// Функции инициализации
int tls_emulator_init(tls_emulator_context_t *ctx);
int tls_emulator_init_with_config(tls_emulator_context_t *ctx, 
                                 const tls_emulator_config_t *config);
void tls_emulator_cleanup(tls_emulator_context_t *ctx);

// Функции управления версиями
int tls_emulator_set_version(tls_emulator_context_t *ctx, tls_version_t version);
tls_version_t tls_emulator_get_current_version(tls_emulator_context_t *ctx);
int tls_emulator_get_supported_versions(tls_emulator_context_t *ctx, 
                                       tls_version_t *versions, int max_versions);
int tls_emulator_select_optimal_version(tls_emulator_context_t *ctx, 
                                       const tls_version_t *client_versions, 
                                       int version_count);

// Функции эмуляции рукопожатия
int tls_emulator_generate_client_hello(tls_emulator_context_t *ctx, 
                                      tls_handshake_data_t *handshake_data,
                                      const browser_emulation_profile_t *profile);
int tls_emulator_process_server_hello(tls_emulator_context_t *ctx, 
                                     const tls_handshake_data_t *server_hello);
int tls_emulator_complete_handshake(tls_emulator_context_t *ctx);
int tls_emulator_reset_handshake(tls_emulator_context_t *ctx);

// Функции обфускации
int tls_emulator_apply_obfuscation(tls_emulator_context_t *ctx, 
                                  tls_obfuscation_level_t level);
int tls_emulator_get_adaptive_obfuscation_level(tls_emulator_context_t *ctx);
int tls_emulator_modify_handshake_timing(tls_emulator_context_t *ctx, 
                                        int delay_ms);

// Функции эмуляции браузера
int tls_emulator_load_browser_profile(tls_emulator_context_t *ctx, 
                                     const browser_emulation_profile_t *profile);
int tls_emulator_generate_browser_fingerprint(tls_emulator_context_t *ctx, 
                                             browser_emulation_profile_t *profile);
int tls_emulator_mimic_browser_behavior(tls_emulator_context_t *ctx);

// Функции управления сессиями
int tls_emulator_enable_session_resumption(tls_emulator_context_t *ctx);
int tls_emulator_disable_session_resumption(tls_emulator_context_t *ctx);
int tls_emulator_store_session(tls_emulator_context_t *ctx, 
                              const unsigned char *session_data, int data_length);
int tls_emulator_restore_session(tls_emulator_context_t *ctx, 
                                unsigned char *session_data, int *data_length);

// Функции статистики
tls_emulator_stats_t tls_emulator_get_stats(tls_emulator_context_t *ctx);
void tls_emulator_reset_stats(tls_emulator_context_t *ctx);

// Функции конфигурации
void tls_emulator_get_config(tls_emulator_context_t *ctx, 
                            tls_emulator_config_t *config);
int tls_emulator_update_config(tls_emulator_context_t *ctx, 
                              const tls_emulator_config_t *new_config);

// Вспомогательные функции
int tls_emulator_is_available(void);
const char* tls_emulator_get_version_string(tls_version_t version);
int tls_emulator_validate_handshake_data(const tls_handshake_data_t *data);
int tls_emulator_get_handshake_delay(tls_emulator_context_t *ctx);
int tls_emulator_set_custom_extensions(tls_emulator_context_t *ctx, 
                                      const unsigned char *extensions, 
                                      int extensions_length);

#endif