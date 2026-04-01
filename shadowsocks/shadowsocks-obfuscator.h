/*
 * Расширенные методы обфускации Shadowsocks для MTProxy
 * Дополнительные алгоритмы обфускации и адаптивная защита
 */

#ifndef _SHADOWSOCKS_OBFUSCATOR_H_
#define _SHADOWSOCKS_OBFUSCATOR_H_

#include <stdint.h>

// Типы обфускации Shadowsocks
typedef enum {
    SS_OBFUSCATION_NONE = 0,
    SS_OBFUSCATION_HTTP_SIMPLE = 1,
    SS_OBFUSCATION_TLS12_TICKET_AUTH = 2,
    SS_OBFUSCATION_RANDOM_HEAD = 3,
    SS_OBFUSCATION_SALTED_SHA256 = 4,
    SS_OBFUSCATION_XOR_MASK = 5,
    SS_OBFUSCATION_BASE64_ENCODE = 6,
    SS_OBFUSCATION_CUSTOM_PATTERN = 7,
    SS_OBFUSCATION_ADAPTIVE = 8,
    SS_OBFUSCATION_HYBRID = 9
} ss_obfuscation_type_t;

// Уровни обфускации
typedef enum {
    SS_OBFUSCATION_LEVEL_LOW = 0,
    SS_OBFUSCATION_LEVEL_MEDIUM = 1,
    SS_OBFUSCATION_LEVEL_HIGH = 2,
    SS_OBFUSCATION_LEVEL_MAXIMUM = 3
} ss_obfuscation_level_t;

// Статус обфускатора
typedef enum {
    SS_OBFUSCATOR_STATUS_UNINITIALIZED = 0,
    SS_OBFUSCATOR_STATUS_INITIALIZED = 1,
    SS_OBFUSCATOR_STATUS_ACTIVE = 2,
    SS_OBFUSCATOR_STATUS_ERROR = 3
} ss_obfuscator_status_t;

// Статистика обфускатора Shadowsocks
typedef struct {
    long long total_packets_processed;
    long long obfuscated_packets;
    long long failed_obfuscations;
    long long adaptive_changes;
    long long pattern_changes;
    long long size_modifications;
    long long timing_adjustments;
    ss_obfuscator_status_t current_status;
    int current_obfuscation_type;
    int obfuscation_success_rate;
} ss_obfuscator_stats_t;

// Конфигурация обфускатора Shadowsocks
typedef struct {
    int enable_obfuscation;
    ss_obfuscation_type_t primary_method;
    ss_obfuscation_type_t fallback_method;
    ss_obfuscation_level_t obfuscation_level;
    int enable_adaptive_obfuscation;
    int enable_size_obfuscation;
    int enable_timing_obfuscation;
    int enable_pattern_obfuscation;
    int min_packet_size;
    int max_packet_size;
    int size_jitter_percent;
    int timing_jitter_ms;
    int enable_replay_protection;
    int max_pattern_history;
} ss_obfuscator_config_t;

// Контекст обфускатора Shadowsocks
typedef struct {
    ss_obfuscator_config_t config;
    ss_obfuscator_stats_t stats;
    ss_obfuscator_status_t status;
    void *pattern_history;
    void *size_distribution;
    void *timing_profile;
    void *encryption_context;
    int current_method;
    int adaptive_mode;
    long long last_adaptation_time;
    int packet_counter;
    int session_id;
} ss_obfuscator_context_t;

// Структура для данных пакета
typedef struct {
    unsigned char *data;
    int data_length;
    int original_length;
    int packet_id;
    long long timestamp;
    int source_port;
    int destination_port;
    char source_ip[46];
    char destination_ip[46];
    int protocol_type;
    int flags;
} ss_packet_data_t;

// Структура для паттерна обфускации
typedef struct {
    unsigned char pattern[256];
    int pattern_length;
    int frequency;
    int last_used_time;
    int effectiveness_score;
} ss_obfuscation_pattern_t;

// Структура для адаптивной конфигурации
typedef struct {
    ss_obfuscation_type_t recommended_method;
    int size_modification;
    int timing_adjustment;
    int pattern_complexity;
    int encryption_strength;
    int recommendation_confidence;
    long long recommendation_time;
} ss_adaptive_config_t;

// Функции инициализации
int ss_obfuscator_init(ss_obfuscator_context_t *ctx);
int ss_obfuscator_init_with_config(ss_obfuscator_context_t *ctx, 
                                  const ss_obfuscator_config_t *config);
void ss_obfuscator_cleanup(ss_obfuscator_context_t *ctx);

// Функции обфускации пакетов
int ss_obfuscator_obfuscate_packet(ss_obfuscator_context_t *ctx, 
                                  ss_packet_data_t *packet);
int ss_obfuscator_deobfuscate_packet(ss_obfuscator_context_t *ctx, 
                                    ss_packet_data_t *packet);
int ss_obfuscator_batch_obfuscate(ss_obfuscator_context_t *ctx, 
                                 ss_packet_data_t *packets, 
                                 int packet_count);

// Функции управления методами
int ss_obfuscator_set_method(ss_obfuscator_context_t *ctx, 
                            ss_obfuscation_type_t method);
ss_obfuscation_type_t ss_obfuscator_get_current_method(ss_obfuscator_context_t *ctx);
int ss_obfuscator_get_available_methods(ss_obfuscator_context_t *ctx, 
                                       ss_obfuscation_type_t *methods, 
                                       int max_methods);

// Функции адаптивной обфускации
int ss_obfuscator_enable_adaptive_mode(ss_obfuscator_context_t *ctx);
int ss_obfuscator_disable_adaptive_mode(ss_obfuscator_context_t *ctx);
int ss_obfuscator_get_adaptive_config(ss_obfuscator_context_t *ctx, 
                                     ss_adaptive_config_t *config);
int ss_obfuscator_apply_adaptive_config(ss_obfuscator_context_t *ctx, 
                                       const ss_adaptive_config_t *config);

// Функции модификации размера
int ss_obfuscator_modify_packet_size(ss_obfuscator_context_t *ctx, 
                                    ss_packet_data_t *packet, 
                                    int target_size);
int ss_obfuscator_get_optimal_size(ss_obfuscator_context_t *ctx, 
                                  const ss_packet_data_t *packet);
int ss_obfuscator_enable_size_obfuscation(ss_obfuscator_context_t *ctx);
int ss_obfuscator_disable_size_obfuscation(ss_obfuscator_context_t *ctx);

// Функции модификации времени
int ss_obfuscator_modify_packet_timing(ss_obfuscator_context_t *ctx, 
                                      ss_packet_data_t *packet, 
                                      int delay_ms);
int ss_obfuscator_get_optimal_timing(ss_obfuscator_context_t *ctx);
int ss_obfuscator_enable_timing_obfuscation(ss_obfuscator_context_t *ctx);
int ss_obfuscator_disable_timing_obfuscation(ss_obfuscator_context_t *ctx);

// Функции паттерн-обфускации
int ss_obfuscator_add_pattern(ss_obfuscator_context_t *ctx, 
                             const unsigned char *pattern, 
                             int pattern_length);
int ss_obfuscator_remove_pattern(ss_obfuscator_context_t *ctx, 
                                const unsigned char *pattern, 
                                int pattern_length);
int ss_obfuscator_generate_random_pattern(ss_obfuscator_context_t *ctx, 
                                         unsigned char *pattern, 
                                         int pattern_length);
int ss_obfuscator_apply_pattern_obfuscation(ss_obfuscator_context_t *ctx, 
                                           ss_packet_data_t *packet);

// Функции защиты от повтора
int ss_obfuscator_enable_replay_protection(ss_obfuscator_context_t *ctx);
int ss_obfuscator_disable_replay_protection(ss_obfuscator_context_t *ctx);
int ss_obfuscator_check_replay_attack(ss_obfuscator_context_t *ctx, 
                                     const ss_packet_data_t *packet);

// Функции статистики
ss_obfuscator_stats_t ss_obfuscator_get_stats(ss_obfuscator_context_t *ctx);
void ss_obfuscator_reset_stats(ss_obfuscator_context_t *ctx);

// Функции конфигурации
void ss_obfuscator_get_config(ss_obfuscator_context_t *ctx, 
                             ss_obfuscator_config_t *config);
int ss_obfuscator_update_config(ss_obfuscator_context_t *ctx, 
                               const ss_obfuscator_config_t *new_config);

// Вспомогательные функции
int ss_obfuscator_is_available(void);
const char* ss_obfuscator_get_method_string(ss_obfuscation_type_t method);
int ss_obfuscator_validate_packet(const ss_packet_data_t *packet);
int ss_obfuscator_get_session_id(ss_obfuscator_context_t *ctx);
int ss_obfuscator_export_patterns(ss_obfuscator_context_t *ctx, 
                                 const char *filename);
int ss_obfuscator_import_patterns(ss_obfuscator_context_t *ctx, 
                                 const char *filename);

#endif