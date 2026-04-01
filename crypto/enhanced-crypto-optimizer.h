/*
 * Enhanced Crypto Optimizer Header for MTProxy
 * Implements advanced cryptographic performance monitoring and optimization
 */

#ifndef ENHANCED_CRYPTO_OPTIMIZER_H
#define ENHANCED_CRYPTO_OPTIMIZER_H

#include "crypto-optimizer.h"

// Структура рекомендаций по производительности
typedef struct {
    crypto_optimization_t recommended_optimization;
    int confidence_level;           // 0-100%
    int estimated_improvement_percent;
    int recommendation_flags;       // Битовые флаги
    char recommendation_text[256];  // Текстовое описание
} crypto_perf_recommendations_t;

// Флаги рекомендаций
#define RECOMMEND_FLAG_BENCHMARK_NEEDED    0x01
#define RECOMMEND_FLAG_MAINTAIN_CURRENT    0x02
#define RECOMMEND_FLAG_UPGRADE_AVAILABLE   0x04
#define RECOMMEND_FLAG_PERFORMANCE_ISSUE   0x08

// Функции enhanced crypto optimizer
int crypto_optimizer_measure_performance(crypto_optimizer_t *optimizer,
                                       const unsigned char *key,
                                       const unsigned char *iv,
                                       const unsigned char *data,
                                       size_t data_len);

void crypto_optimizer_get_recommendations(crypto_optimizer_t *optimizer,
                                        crypto_perf_recommendations_t *recommendations);

double crypto_optimizer_predict_performance(crypto_optimizer_t *optimizer,
                                          size_t data_size);

void crypto_optimizer_run_benchmark(crypto_optimizer_t *optimizer,
                                  const unsigned char *key,
                                  const unsigned char *iv,
                                  const unsigned char *data,
                                  size_t data_len);

int crypto_optimizer_auto_tune(crypto_optimizer_t *optimizer);

#endif // ENHANCED_CRYPTO_OPTIMIZER_H
