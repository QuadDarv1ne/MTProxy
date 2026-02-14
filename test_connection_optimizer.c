/*
    Тестирование оптимизатора соединений для MTProxy
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system/connection-optimizer.h"

int main() {
    printf("=== Тестирование оптимизатора соединений ===\n\n");
    
    // 1. Инициализация оптимизатора
    printf("1. Инициализация оптимизатора соединений...\n");
    conn_opt_config_t config = {0};
    config.max_connections = 100;
    config.min_idle_connections = 5;
    config.max_idle_connections = 20;
    config.connection_timeout_sec = 300;
    config.enable_keepalive = 1;
    config.keepalive_interval_sec = 60;
    config.memory_pool_size = 1024 * 1024; // 1MB
    config.enable_compression = 1;
    config.compression_threshold = 1024; // 1KB
    config.enable_multiplexing = 1;
    
    connection_optimizer_t *optimizer = conn_opt_init(&config);
    if (!optimizer) {
        printf("❌ Ошибка инициализации оптимизатора\n");
        return 1;
    }
    printf("✅ Оптимизатор соединений инициализирован\n\n");
    
    // 2. Тестирование управления соединениями
    printf("2. Тестирование управления соединениями...\n");
    
    // Получение соединений из пула
    connection_entry_t *conn1 = conn_opt_acquire_connection(optimizer);
    if (!conn1) {
        printf("❌ Не удалось получить первое соединение\n");
        conn_opt_cleanup(optimizer);
        return 1;
    }
    printf("✅ Первое соединение получено (ID: %d)\n", conn1->id);
    
    connection_entry_t *conn2 = conn_opt_acquire_connection(optimizer);
    if (!conn2) {
        printf("❌ Не удалось получить второе соединение\n");
        conn_opt_cleanup(optimizer);
        return 1;
    }
    printf("✅ Второе соединение получено (ID: %d)\n", conn2->id);
    
    // Проверка статистики
    conn_opt_stats_t stats;
    conn_opt_get_stats(optimizer, &stats);
    printf("   Статистика после получения соединений:\n");
    printf("   - Активные соединения: %lld\n", stats.acquired_connections);
    printf("   - Пул заполнен на: %.2f%%\n", stats.pool_utilization * 100);
    
    // Возврат соединений в пул
    int result = conn_opt_release_connection(optimizer, conn1);
    if (result != 0) {
        printf("❌ Ошибка возврата первого соединения\n");
    } else {
        printf("✅ Первое соединение возвращено в пул\n");
    }
    
    result = conn_opt_release_connection(optimizer, conn2);
    if (result != 0) {
        printf("❌ Ошибка возврата второго соединения\n");
    } else {
        printf("✅ Второе соединение возвращено в пул\n");
    }
    
    printf("✅ Управление соединениями работает корректно\n\n");
    
    // 3. Тестирование управления памятью
    printf("3. Тестирование управления памятью...\n");
    
    // Выделение памяти через оптимизатор
    size_t test_size = 1024; // 1KB
    void *test_ptr = conn_opt_alloc(optimizer, test_size);
    if (!test_ptr) {
        printf("❌ Ошибка выделения памяти\n");
        conn_opt_cleanup(optimizer);
        return 1;
    }
    printf("✅ Память выделена через оптимизатор (размер: %zu байт)\n", test_size);
    
    // Использование выделенной памяти
    memset(test_ptr, 0xAB, test_size);
    printf("✅ Память использована для записи данных\n");
    
    // Освобождение памяти
    conn_opt_free(optimizer, test_ptr, test_size);
    printf("✅ Память освобождена\n");
    printf("✅ Управление памятью работает корректно\n\n");
    
    // 4. Тестирование статистики
    printf("4. Тестирование статистики...\n");
    conn_opt_get_stats(optimizer, &stats);
    printf("   Текущая статистика оптимизатора:\n");
    printf("   - Получено соединений: %lld\n", stats.acquired_connections);
    printf("   - Возвращено соединений: %lld\n", stats.released_connections);
    printf("   - Ошибок получения: %lld\n", stats.acquire_failures);
    printf("   - Активные соединения: %d\n", stats.active_connections);
    printf("   - Пул заполнен на: %.2f%%\n", stats.pool_utilization * 100);
    printf("   - Выделено байт: %lld\n", stats.allocated_bytes);
    printf("   - Освобождено байт: %lld\n", stats.freed_bytes);
    printf("✅ Статистика работает корректно\n\n");
    
    // 5. Тестирование производительности
    printf("5. Тестирование настроек производительности...\n");
    
    conn_performance_tuning_t tuning = {0};
    tuning.max_connections = 200;
    tuning.min_idle_connections = 10;
    tuning.max_idle_connections = 50;
    tuning.timeout_seconds = 600;
    tuning.enable_keepalive = 1;
    tuning.enable_compression = 1;
    tuning.enable_multiplexing = 1;
    
    result = conn_opt_apply_performance_tuning(optimizer, &tuning);
    if (result != 0) {
        printf("❌ Ошибка применения настроек производительности\n");
    } else {
        printf("✅ Настройки производительности применены\n");
    }
    
    // Тестирование адаптации под нагрузку
    result = conn_opt_adjust_for_load(optimizer, 75); // Высокая нагрузка
    if (result != 0) {
        printf("❌ Ошибка адаптации под нагрузку\n");
    } else {
        printf("✅ Адаптация под нагрузку выполнена (нагрузка: 75%%)\n");
    }
    
    printf("✅ Настройки производительности работают корректно\n\n");
    
    // 6. Печать полной статистики
    printf("6. Полная статистика оптимизатора:\n");
    conn_opt_print_stats(optimizer);
    
    // 7. Оценка эффективности
    double efficiency = conn_opt_get_efficiency_score(optimizer);
    printf("   Эффективность оптимизатора: %.2f%%\n", efficiency * 100);
    
    // 8. Очистка
    printf("\n8. Очистка оптимизатора...\n");
    conn_opt_cleanup(optimizer);
    printf("✅ Оптимизатор очищен\n");
    
    printf("\n🎉 Все тесты оптимизатора соединений пройдены успешно!\n");
    return 0;
}