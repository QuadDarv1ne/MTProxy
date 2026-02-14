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

#ifndef __DH_OPTIMIZED_H__
#define __DH_OPTIMIZED_H__

#ifdef __cplusplus
extern "C" {
#endif

// Статистика для DH оптимизации
struct dh_optimized_stats {
    long long precomputed_values_used;
    long long fast_path_operations;
    long long fallback_operations;
    long long total_dh_generations;
    long long cached_results_used;
    long long montgomery_reductions;
};

// Инициализация оптимизированного DH
int dh_optimized_init(void);

// Оптимизированная генерация g^a
int dh_optimized_generate_g_a(unsigned char g_a[256], unsigned char a[256]);

// Оптимизированный DH обмен: вычисление g^(ab)
int dh_optimized_compute_shared_secret(unsigned char shared_secret[256],
                                      const unsigned char g_b[256],
                                      const unsigned char a[256]);

// Оптимизированная предварительная генерация значений DH
int dh_optimized_precompute_batch(int count, unsigned char (*g_a_array)[256], 
                                 unsigned char (*a_array)[256]);

// Очистка кэша и ресурсов DH
void dh_optimized_cleanup(void);

// Получение статистики DH оптимизации
void dh_optimized_get_stats(struct dh_optimized_stats *stats);

// Вывод статистики в лог
void dh_optimized_print_stats(void);

#ifdef __cplusplus
}
#endif

#endif // __DH_OPTIMIZED_H__