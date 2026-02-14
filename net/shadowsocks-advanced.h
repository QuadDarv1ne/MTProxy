/*
    This file is part of Mtproto-proxy Library.

    Mtproto-proxy Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Mtproto-proxy Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Mtproto-proxy Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2024-2026 MTProto Proxy Enhanced Project
              2024-2026 Dupley Maxim Igorevich (Maestro7IT)
*/

#ifndef __SHADOWSOCKS_ADVANCED_H__
#define __SHADOWSOCKS_ADVANCED_H__

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Advanced obfuscation методы
enum obfs_method {
    OBFS_NONE = 0,
    OBFS_HTTP_SIMPLE,
    OBFS_TLS12_ticket_auth,
    OBFS_RANDOM_HEAD,
    OBFS_SALTED_SHA256,
    OBFS_XOR_MASK,
    OBFS_BASE64_ENCODE,
    OBFS_CUSTOM_PATTERN
};

// Pluggable transports
enum transport_type {
    TRANSPORT_TCP = 0,
    TRANSPORT_UDP,
    TRANSPORT_WEBSOCKET,
    TRANSPORT_QUIC,
    TRANSPORT_HTTP2
};

// Forward declaration
struct ss_advanced_context;

// Traffic analysis resistance параметры
struct traffic_analysis_params {
    int enable_timing_obfuscation;
    int enable_size_obfuscation;
    int enable_pattern_obfuscation;
    int min_packet_size;
    int max_packet_size;
    int timing_jitter_ms;
    unsigned char padding_pattern[256];
};

// Статистика для advanced Shadowsocks
struct shadowsocks_advanced_stats {
    long long obfs_encryption_ops;
    long long obfs_decryption_ops;
    long long transport_switches;
    long long traffic_analysis_resistance_activated;
    long long replay_attack_prevented;
    long long total_advanced_connections;
};

// Инициализация advanced Shadowsocks
int shadowsocks_advanced_init(void);

// Создание advanced контекста
struct ss_advanced_context *shadowsocks_advanced_create_context(
    const unsigned char *password, int password_len,
    enum obfs_method obfs, enum transport_type transport);

// Advanced encryption с obfuscation
int shadowsocks_advanced_encrypt(struct ss_advanced_context *ctx,
                                const unsigned char *plaintext, int plaintext_len,
                                unsigned char *ciphertext, int *ciphertext_len);

// Advanced decryption
int shadowsocks_advanced_decrypt(struct ss_advanced_context *ctx,
                                const unsigned char *ciphertext, int ciphertext_len,
                                unsigned char *plaintext, int *plaintext_len);

// Смена transport типа
int shadowsocks_advanced_switch_transport(struct ss_advanced_context *ctx,
                                         enum transport_type new_transport);

// Проверка на replay атаки
int shadowsocks_advanced_check_replay(struct ss_advanced_context *ctx);

// Настройка traffic analysis параметров
int shadowsocks_advanced_set_ta_params(const struct traffic_analysis_params *params);

// Получение статистики
void shadowsocks_advanced_get_stats(struct shadowsocks_advanced_stats *stats);

// Вывод статистики
void shadowsocks_advanced_print_stats(void);

// Очистка контекста
void shadowsocks_advanced_destroy_context(struct ss_advanced_context *ctx);

// Вспомогательные функции для obfuscation
int obfs_http_simple_encrypt(unsigned char *data, int len, 
                            unsigned char *output, int *output_len);
int obfs_tls12_encrypt(unsigned char *data, int len,
                      unsigned char *output, int *output_len);
int obfs_random_head_encrypt(unsigned char *data, int len,
                            unsigned char *output, int *output_len);
int obfs_salted_sha256_encrypt(unsigned char *data, int len,
                              const unsigned char *salt, int salt_len,
                              unsigned char *output, int *output_len);
int obfs_xor_mask_encrypt(unsigned char *data, int len,
                         const unsigned char *mask, int mask_len,
                         unsigned char *output, int *output_len);
int obfs_base64_encrypt(unsigned char *data, int len,
                       unsigned char *output, int *output_len);

// Traffic analysis resistance функции
int apply_size_obfuscation(unsigned char *data, int len,
                          unsigned char *output, int *output_len);
void apply_timing_obfuscation(void);
int check_replay_attack(struct ss_advanced_context *ctx,
                       const unsigned char *data, int len);

#ifdef __cplusplus
}
#endif

#endif // __SHADOWSOCKS_ADVANCED_H__