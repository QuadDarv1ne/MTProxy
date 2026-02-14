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

#define _FILE_OFFSET_BITS 64

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "net/shadowsocks-advanced.h"
#include "common/kprintf.h"
#include "common/common-stats.h"
#include "crypto/aes-optimized.h"

// Статистика для advanced Shadowsocks
struct shadowsocks_advanced_stats {
    long long obfs_encryption_ops;
    long long obfs_decryption_ops;
    long long transport_switches;
    long long traffic_analysis_resistance_activated;
    long long replay_attack_prevented;
    long long total_advanced_connections;
};

static struct shadowsocks_advanced_stats ss_advanced_stats = {0};

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

// Advanced Shadowsocks connection context
struct ss_advanced_context {
    unsigned char encryption_key[32];
    unsigned char encryption_iv[16];
    unsigned char salt[32];
    enum obfs_method obfs_method;
    enum transport_type transport;
    EVP_CIPHER_CTX *cipher_ctx;
    unsigned long long packet_counter;
    unsigned long long bytes_processed;
    time_t last_activity;
    int initialized;
};

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

// Глобальные параметры
static struct traffic_analysis_params global_ta_params = {
    .enable_timing_obfuscation = 1,
    .enable_size_obfuscation = 1,
    .enable_pattern_obfuscation = 1,
    .min_packet_size = 64,
    .max_packet_size = 1400,
    .timing_jitter_ms = 50,
    .padding_pattern = {0}
};

// HTTP Simple obfuscation pattern
static const unsigned char http_simple_header[] = 
    "GET / HTTP/1.1\r\n"
    "Host: www.example.com\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    "Accept-Language: en-US,en;q=0.5\r\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "Connection: keep-alive\r\n"
    "Upgrade-Insecure-Requests: 1\r\n\r\n";

// TLS 1.2 ticket auth pattern
static const unsigned char tls12_client_hello[] = {
    0x16, 0x03, 0x03, 0x00, 0xdc,  // TLS record header
    0x01, 0x00, 0x00, 0xd8,        // Handshake header
    0x03, 0x03,                    // TLS version 1.2
    // Random 32 bytes would go here
    0x00,                          // Session ID length
    0x00, 0x1c,                    // Cipher suites length
    // Cipher suites would go here
    0x01,                          // Compression methods length
    0x00,                          // Compression method null
    0x00, 0x83,                    // Extensions length
    // Extensions would go here
};

// Инициализация advanced Shadowsocks
int shadowsocks_advanced_init(void) {
    // Инициализация криптографических компонентов
    if (aes_optimized_init() != 0) {
        vkprintf(1, "Failed to initialize AES optimization\n");
        return -1;
    }
    
    // Генерация padding pattern
    if (RAND_bytes(global_ta_params.padding_pattern, 
                   sizeof(global_ta_params.padding_pattern)) <= 0) {
        vkprintf(1, "Failed to generate padding pattern\n");
        return -1;
    }
    
    vkprintf(1, "Shadowsocks Advanced initialized with traffic analysis resistance\n");
    return 0;
}

// Создание advanced контекста
struct ss_advanced_context *shadowsocks_advanced_create_context(
    const unsigned char *password, int password_len,
    enum obfs_method obfs, enum transport_type transport) {
    
    struct ss_advanced_context *ctx = calloc(1, sizeof(struct ss_advanced_context));
    if (!ctx) {
        return NULL;
    }
    
    // Генерация ключа из пароля
    unsigned char key_material[64];
    if (EVP_BytesToKey(EVP_aes_256_cbc(), EVP_md5(), NULL,
                       password, password_len, 1,
                       key_material, NULL) != 32) {
        free(ctx);
        return NULL;
    }
    
    memcpy(ctx->encryption_key, key_material, 32);
    
    // Генерация IV и salt
    if (RAND_bytes(ctx->encryption_iv, 16) <= 0 ||
        RAND_bytes(ctx->salt, 32) <= 0) {
        free(ctx);
        return NULL;
    }
    
    ctx->obfs_method = obfs;
    ctx->transport = transport;
    ctx->last_activity = time(NULL);
    ctx->initialized = 1;
    
    // Создание cipher context
    ctx->cipher_ctx = EVP_CIPHER_CTX_new();
    if (!ctx->cipher_ctx) {
        free(ctx);
        return NULL;
    }
    
    ss_advanced_stats.total_advanced_connections++;
    return ctx;
}

// HTTP Simple obfuscation
static int obfs_http_simple_encrypt(unsigned char *data, int len, 
                                   unsigned char *output, int *output_len) {
    int header_len = sizeof(http_simple_header) - 1;
    int total_len = header_len + len;
    
    if (total_len > *output_len) {
        return -1;
    }
    
    // Копируем HTTP header
    memcpy(output, http_simple_header, header_len);
    
    // Копируем данные
    memcpy(output + header_len, data, len);
    
    *output_len = total_len;
    ss_advanced_stats.obfs_encryption_ops++;
    return 0;
}

// TLS 1.2 ticket auth obfuscation
static int obfs_tls12_encrypt(unsigned char *data, int len,
                             unsigned char *output, int *output_len) {
    int header_len = sizeof(tls12_client_hello);
    int total_len = header_len + len + 16; // +16 for padding
    
    if (total_len > *output_len) {
        return -1;
    }
    
    // Копируем TLS header
    memcpy(output, tls12_client_hello, header_len);
    
    // Добавляем длину данных
    output[header_len] = (len >> 8) & 0xFF;
    output[header_len + 1] = len & 0xFF;
    
    // Копируем данные
    memcpy(output + header_len + 2, data, len);
    
    // Добавляем случайный padding
    if (RAND_bytes(output + header_len + 2 + len, 16) <= 0) {
        return -1;
    }
    
    *output_len = total_len;
    ss_advanced_stats.obfs_encryption_ops++;
    return 0;
}

// Random head obfuscation
static int obfs_random_head_encrypt(unsigned char *data, int len,
                                   unsigned char *output, int *output_len) {
    int random_len = 16 + (rand() % 64); // 16-80 bytes random header
    int total_len = random_len + len;
    
    if (total_len > *output_len) {
        return -1;
    }
    
    // Генерируем случайный заголовок
    if (RAND_bytes(output, random_len) <= 0) {
        return -1;
    }
    
    // Копируем данные
    memcpy(output + random_len, data, len);
    
    *output_len = total_len;
    ss_advanced_stats.obfs_encryption_ops++;
    return 0;
}

// Salted SHA256 obfuscation
static int obfs_salted_sha256_encrypt(unsigned char *data, int len,
                                     const unsigned char *salt, int salt_len,
                                     unsigned char *output, int *output_len) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    if (!mdctx) {
        return -1;
    }
    
    // Вычисляем salted hash
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1 ||
        EVP_DigestUpdate(mdctx, salt, salt_len) != 1 ||
        EVP_DigestUpdate(mdctx, data, len) != 1 ||
        EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }
    
    EVP_MD_CTX_free(mdctx);
    
    int total_len = hash_len + len;
    if (total_len > *output_len) {
        return -1;
    }
    
    // Копируем hash и данные
    memcpy(output, hash, hash_len);
    memcpy(output + hash_len, data, len);
    
    *output_len = total_len;
    ss_advanced_stats.obfs_encryption_ops++;
    return 0;
}

// XOR mask obfuscation
static int obfs_xor_mask_encrypt(unsigned char *data, int len,
                                const unsigned char *mask, int mask_len,
                                unsigned char *output, int *output_len) {
    if (len > *output_len) {
        return -1;
    }
    
    // Применяем XOR маску
    for (int i = 0; i < len; i++) {
        output[i] = data[i] ^ mask[i % mask_len];
    }
    
    *output_len = len;
    ss_advanced_stats.obfs_encryption_ops++;
    return 0;
}

// Base64 encode obfuscation
static int obfs_base64_encrypt(unsigned char *data, int len,
                              unsigned char *output, int *output_len) {
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    int encoded_len = ((len + 2) / 3) * 4;
    if (encoded_len > *output_len) {
        return -1;
    }
    
    int i, j;
    for (i = 0, j = 0; i < len; i += 3, j += 4) {
        uint32_t octet_a = i < len ? data[i] : 0;
        uint32_t octet_b = i + 1 < len ? data[i + 1] : 0;
        uint32_t octet_c = i + 2 < len ? data[i + 2] : 0;
        
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        output[j] = base64_chars[(triple >> 18) & 63];
        output[j + 1] = base64_chars[(triple >> 12) & 63];
        output[j + 2] = (i + 1 < len) ? base64_chars[(triple >> 6) & 63] : '=';
        output[j + 3] = (i + 2 < len) ? base64_chars[triple & 63] : '=';
    }
    
    *output_len = encoded_len;
    ss_advanced_stats.obfs_encryption_ops++;
    return 0;
}

// Advanced encryption с obfuscation
int shadowsocks_advanced_encrypt(struct ss_advanced_context *ctx,
                                const unsigned char *plaintext, int plaintext_len,
                                unsigned char *ciphertext, int *ciphertext_len) {
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    unsigned char temp_buffer[65536];
    int temp_len = sizeof(temp_buffer);
    
    // Применяем obfuscation
    switch (ctx->obfs_method) {
        case OBFS_HTTP_SIMPLE:
            if (obfs_http_simple_encrypt((unsigned char*)plaintext, plaintext_len,
                                       temp_buffer, &temp_len) != 0) {
                return -1;
            }
            break;
            
        case OBFS_TLS12_ticket_auth:
            if (obfs_tls12_encrypt((unsigned char*)plaintext, plaintext_len,
                                 temp_buffer, &temp_len) != 0) {
                return -1;
            }
            break;
            
        case OBFS_RANDOM_HEAD:
            if (obfs_random_head_encrypt((unsigned char*)plaintext, plaintext_len,
                                       temp_buffer, &temp_len) != 0) {
                return -1;
            }
            break;
            
        case OBFS_SALTED_SHA256:
            if (obfs_salted_sha256_encrypt((unsigned char*)plaintext, plaintext_len,
                                         ctx->salt, 32,
                                         temp_buffer, &temp_len) != 0) {
                return -1;
            }
            break;
            
        case OBFS_XOR_MASK:
            if (obfs_xor_mask_encrypt((unsigned char*)plaintext, plaintext_len,
                                    ctx->salt, 32,
                                    temp_buffer, &temp_len) != 0) {
                return -1;
            }
            break;
            
        case OBFS_BASE64_ENCODE:
            if (obfs_base64_encrypt((unsigned char*)plaintext, plaintext_len,
                                  temp_buffer, &temp_len) != 0) {
                return -1;
            }
            break;
            
        default:
            // No obfuscation
            memcpy(temp_buffer, plaintext, plaintext_len);
            temp_len = plaintext_len;
            break;
    }
    
    // Применяем AES шифрование
    if (aes_optimized_encrypt(ctx->encryption_key, ctx->encryption_iv,
                             temp_buffer, ciphertext, temp_len) != temp_len) {
        return -1;
    }
    
    *ciphertext_len = temp_len;
    ctx->packet_counter++;
    ctx->bytes_processed += temp_len;
    ctx->last_activity = time(NULL);
    
    return 0;
}

// Traffic analysis resistance - размер обфускация
static int apply_size_obfuscation(unsigned char *data, int len,
                                 unsigned char *output, int *output_len) {
    if (!global_ta_params.enable_size_obfuscation) {
        memcpy(output, data, len);
        *output_len = len;
        return 0;
    }
    
    int target_size = global_ta_params.min_packet_size + 
                     (rand() % (global_ta_params.max_packet_size - 
                               global_ta_params.min_packet_size));
    
    if (target_size < len) {
        target_size = len;
    }
    
    if (target_size > *output_len) {
        return -1;
    }
    
    // Копируем данные
    memcpy(output, data, len);
    
    // Добавляем padding
    int padding_len = target_size - len;
    if (padding_len > 0) {
        if (RAND_bytes(output + len, padding_len) <= 0) {
            return -1;
        }
    }
    
    *output_len = target_size;
    return 0;
}

// Traffic analysis resistance - тайминг обфускация
static void apply_timing_obfuscation(void) {
    if (!global_ta_params.enable_timing_obfuscation) {
        return;
    }
    
    // Случайная задержка
    int jitter = rand() % (global_ta_params.timing_jitter_ms * 2) - 
                global_ta_params.timing_jitter_ms;
    
    if (jitter > 0) {
        struct timespec ts = {0, jitter * 1000000}; // наносекунды
        nanosleep(&ts, NULL);
    }
    
    ss_advanced_stats.traffic_analysis_resistance_activated++;
}

// Advanced decryption
int shadowsocks_advanced_decrypt(struct ss_advanced_context *ctx,
                                const unsigned char *ciphertext, int ciphertext_len,
                                unsigned char *plaintext, int *plaintext_len) {
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    unsigned char temp_buffer[65536];
    
    // Применяем AES дешифрование
    if (aes_optimized_decrypt(ctx->encryption_key, ctx->encryption_iv,
                             ciphertext, temp_buffer, ciphertext_len) != ciphertext_len) {
        return -1;
    }
    
    // Применяем размер обфускацию
    int temp_len = sizeof(temp_buffer);
    if (apply_size_obfuscation(temp_buffer, ciphertext_len,
                              plaintext, &temp_len) != 0) {
        return -1;
    }
    
    // Применяем тайминг обфускацию
    apply_timing_obfuscation();
    
    *plaintext_len = temp_len;
    ctx->packet_counter++;
    ctx->bytes_processed += temp_len;
    ctx->last_activity = time(NULL);
    
    ss_advanced_stats.obfs_decryption_ops++;
    return 0;
}

// Проверка на replay атаки
static int check_replay_attack(struct ss_advanced_context *ctx,
                              const unsigned char *data, int len) {
    // Простая проверка на основе packet counter
    // В реальной реализации нужно использовать более сложные методы
    static unsigned long long last_counter = 0;
    
    if (ctx->packet_counter <= last_counter) {
        ss_advanced_stats.replay_attack_prevented++;
        return -1; // Возможная replay атака
    }
    
    last_counter = ctx->packet_counter;
    return 0;
}

// Смена transport типа
int shadowsocks_advanced_switch_transport(struct ss_advanced_context *ctx,
                                         enum transport_type new_transport) {
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    ctx->transport = new_transport;
    ss_advanced_stats.transport_switches++;
    
    vkprintf(2, "Switched transport to type %d\n", new_transport);
    return 0;
}

// Получение статистики
void shadowsocks_advanced_get_stats(struct shadowsocks_advanced_stats *stats) {
    if (stats) {
        memcpy(stats, &ss_advanced_stats, sizeof(struct shadowsocks_advanced_stats));
    }
}

// Вывод статистики
void shadowsocks_advanced_print_stats(void) {
    vkprintf(1, "Shadowsocks Advanced Statistics:\n");
    vkprintf(1, "  Obfs Encryption Operations: %lld\n", ss_advanced_stats.obfs_encryption_ops);
    vkprintf(1, "  Obfs Decryption Operations: %lld\n", ss_advanced_stats.obfs_decryption_ops);
    vkprintf(1, "  Transport Switches: %lld\n", ss_advanced_stats.transport_switches);
    vkprintf(1, "  Traffic Analysis Resistance: %lld\n", ss_advanced_stats.traffic_analysis_resistance_activated);
    vkprintf(1, "  Replay Attacks Prevented: %lld\n", ss_advanced_stats.replay_attack_prevented);
    vkprintf(1, "  Total Advanced Connections: %lld\n", ss_advanced_stats.total_advanced_connections);
}

// Очистка контекста
void shadowsocks_advanced_destroy_context(struct ss_advanced_context *ctx) {
    if (ctx) {
        if (ctx->cipher_ctx) {
            EVP_CIPHER_CTX_free(ctx->cipher_ctx);
        }
        free(ctx);
    }
}

// Настройка traffic analysis параметров
int shadowsocks_advanced_set_ta_params(const struct traffic_analysis_params *params) {
    if (!params) {
        return -1;
    }
    
    memcpy(&global_ta_params, params, sizeof(struct traffic_analysis_params));
    return 0;
}