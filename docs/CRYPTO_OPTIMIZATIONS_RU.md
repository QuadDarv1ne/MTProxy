# Криптографические оптимизации MTProxy

## Обзор

Система криптографических оптимизаций предоставляет высокопроизводительные реализации AES и других криптографических операций с автоматическим выбором лучшей доступной технологии.

## Архитектура

### Основные компоненты

1. **Crypto Optimizer** (`crypto-optimizer.h/.c`) - основной менеджер оптимизаций
2. **Vectorized Crypto** (`vectorized-crypto.h/.c`) - векторизованные операции
3. **AES Optimized** (`aes-optimized.h/.c`) - оптимизированный AES
4. **DH Optimized** (`dh-optimized.h/.c`) - оптимизированный Diffie-Hellman

### Типы оптимизаций

```c
typedef enum {
    CRYPTO_OPT_NONE = 0,        // Без оптимизаций
    CRYPTO_OPT_AES_NI,          // AES-NI инструкции процессора
    CRYPTO_OPT_VECTORIZED,      // Векторизация (SSE/AVX)
    CRYPTO_OPT_PARALLEL,        // Параллельная обработка
    CRYPTO_OPT_BATCH,           // Пакетная обработка
    CRYPTO_OPT_PRECOMPUTED      // Предвычисленные ключи
} crypto_optimization_t;
```

## Основные функции

### Инициализация
```c
crypto_optimizer_t* crypto_optimizer_init(void);
int crypto_optimizer_configure(crypto_optimizer_t *optimizer, 
                              crypto_optimization_t optimization_type);
```

### Оптимизированные операции
```c
int crypto_optimized_encrypt(crypto_optimizer_t *optimizer,
                           const unsigned char *key,
                           const unsigned char *iv,
                           const unsigned char *plaintext,
                           size_t plaintext_len,
                           unsigned char *ciphertext,
                           size_t *ciphertext_len);

int crypto_optimized_decrypt(crypto_optimizer_t *optimizer,
                           const unsigned char *key,
                           const unsigned char *iv,
                           const unsigned char *ciphertext,
                           size_t ciphertext_len,
                           unsigned char *plaintext,
                           size_t *plaintext_len);
```

### Пакетная обработка
```c
int crypto_batch_encrypt(crypto_optimizer_t *optimizer,
                        const unsigned char *key,
                        const unsigned char *iv,
                        unsigned char **plaintext_array,
                        size_t *plaintext_lengths,
                        int array_size,
                        unsigned char **ciphertext_array,
                        size_t *ciphertext_lengths);
```

## Автоматическое обнаружение возможностей

Система автоматически определяет доступные оптимизации:

```c
int capabilities = crypto_optimizer_detect_capabilities();
// Возвращает битовую маску поддерживаемых оптимизаций

crypto_optimization_t best_opt = crypto_optimizer_get_best_optimization();
// Возвращает лучшую доступную оптимизацию
```

### Поддерживаемые технологии:
- **AES-NI**: Аппаратное ускорение AES (Intel/AMD)
- **SSE/AVX**: Векторные инструкции процессора
- **Пакетная обработка**: Обработка множества операций за раз
- **Кэширование ключей**: Предвычисленные контексты шифрования

## Статистика производительности

Система собирает подробную статистику:

```c
typedef struct {
    long long total_operations;           // Всего операций
    long long optimized_operations;       // Оптимизированные операции
    long long fallback_operations;        // Резервные операции
    long long cache_hits;                 // Попадания в кэш
    long long cache_misses;               // Промахи кэша
    double avg_optimization_ratio;        // Средний коэффициент оптимизации
    double total_processing_time_ms;      // Общее время обработки
    double optimized_processing_time_ms;  // Время оптимизированной обработки
} crypto_optimization_stats_t;
```

## Интеграция с существующими системами

### С интеграцией AES:
```c
int crypto_optimizer_integrate_with_aes(crypto_optimizer_t *optimizer);
```

### С интеграцией DH:
```c
int crypto_optimizer_integrate_with_dh(crypto_optimizer_t *optimizer);
```

## Пример использования

```c
#include "crypto/crypto-optimizer.h"

int main() {
    // Инициализация оптимизатора
    crypto_optimizer_t *optimizer = crypto_optimizer_init();
    if (!optimizer) {
        fprintf(stderr, "Failed to initialize crypto optimizer\n");
        return -1;
    }
    
    // Конфигурация (опционально)
    crypto_optimizer_configure(optimizer, CRYPTO_OPT_AES_NI);
    
    // Данные для шифрования
    unsigned char key[32] = {0}; // Ваш ключ
    unsigned char iv[16] = {0};  // Ваш IV
    unsigned char plaintext[1024] = "Hello, World!";
    unsigned char ciphertext[1024];
    size_t ciphertext_len;
    
    // Оптимизированное шифрование
    int result = crypto_optimized_encrypt(
        optimizer,
        key, iv,
        plaintext, strlen((char*)plaintext),
        ciphertext, &ciphertext_len
    );
    
    if (result == 0) {
        printf("Encryption successful, ciphertext length: %zu\n", ciphertext_len);
    }
    
    // Печать статистики
    crypto_optimizer_print_stats(optimizer);
    
    // Очистка
    crypto_optimizer_cleanup(optimizer);
    return 0;
}
```

## Производительность

### Ожидаемые улучшения:
- **AES-NI**: 5-10x ускорение по сравнению с программной реализацией
- **Векторизация**: 2-4x ускорение для блочных операций
- **Кэширование ключей**: 3-5x ускорение при повторных операциях
- **Пакетная обработка**: 2-3x ускорение для множественных операций

### Тестирование производительности:
```bash
# Компиляция тестов
make crypto-benchmark

# Запуск бенчмарка
./bin/crypto-benchmark --iterations=10000 --data-size=1024
```

## Конфигурация

### Параметры оптимизатора:
```c
struct {
    int enable_aes_ni;              // Включить AES-NI
    int enable_vectorization;       // Включить векторизацию
    int enable_batching;           // Включить пакетную обработку
    int enable_precomputation;     // Включить предвычисления
    int cache_size;                // Размер кэша ключей
    int batch_size;                // Размер пакета
    double optimization_threshold_ms; // Порог оптимизации
} config;
```

## Совместимость

- **Windows**: Поддержка через MinGW/MSVC
- **Linux**: Полная поддержка
- **macOS**: Поддержка через Clang
- **ARM**: Ограниченная поддержка (без AES-NI)

## Безопасность

- Все оптимизации проходят тестирование на корректность
- Поддержка безопасного удаления ключей
- Защита от атак по времени
- Проверка целостности данных

## Отладка

### Включение логирования:
```c
// Включение подробного логирования
export CRYPTO_DEBUG=1

// Логирование в файл
export CRYPTO_LOG_FILE=/tmp/crypto.log
```

### Диагностика проблем:
```c
// Проверка поддерживаемых возможностей
int caps = crypto_optimizer_detect_capabilities();
printf("Supported capabilities: 0x%x\n", caps);

// Проверка текущей конфигурации
crypto_optimizer_print_stats(optimizer);
```

## Будущие улучшения

- Поддержка ARM NEON инструкций
- Интеграция с OpenSSL 3.0
- Асинхронные криптографические операции
- Поддержка пост-квантовой криптографии
- GPU-ускорение для тяжелых операций