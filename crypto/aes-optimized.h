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

#ifndef __AES_OPTIMIZED_H__
#define __AES_OPTIMIZED_H__

#include <openssl/evp.h>

#ifdef __cplusplus
extern "C" {
#endif

// Статистика для AES оптимизации
struct aes_optimized_stats {
    long long key_cache_hits;
    long long key_cache_misses;
    long long precomputed_keys_used;
    long long fallback_operations;
    long long total_encryptions;
    long long total_decryptions;
};

// Инициализация оптимизированного AES
int aes_optimized_init(void);

// Оптимизированное шифрование AES
int aes_optimized_encrypt(const unsigned char *key, const unsigned char *iv,
                         const void *plaintext, void *ciphertext, int length);

// Оптимизированное дешифрование AES
int aes_optimized_decrypt(const unsigned char *key, const unsigned char *iv,
                         const void *ciphertext, void *plaintext, int length);

// Предвычисленное шифрование для часто используемых ключей
int aes_optimized_encrypt_precomputed(const unsigned char *key, const unsigned char *iv,
                                    const void *plaintext, void *ciphertext, int length);

// Батчевая обработка AES операций
int aes_optimized_batch_encrypt(const unsigned char *key, const unsigned char *iv,
                               const void **plaintext_array, void **ciphertext_array, 
                               int *length_array, int count);

// Очистка кэша AES
void aes_optimized_cleanup(void);

// Получение статистики AES оптимизации
void aes_optimized_get_stats(struct aes_optimized_stats *stats);

// Вывод статистики в лог
void aes_optimized_print_stats(void);

#ifdef __cplusplus
}
#endif

#endif // __AES_OPTIMIZED_H__