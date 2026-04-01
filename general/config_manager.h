#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdint.h>
#include "memory-pool.h"
#include "ddos-protection-enhanced.h"
#include "protocol-manager.h"
#include "load-balancer.h"
#include "perf_monitor/perf-monitor.h"
#include "security_enhanced/security-enhanced.h"
#include "conn_pool/conn-pool.h"
#include "thread_system/thread-system.h"

/*
 * Структура конфигурации для MTProxy
 * Содержит параметры для всех компонентов системы
 */
typedef struct mtproxy_config {
    // Параметры пула памяти
    size_t memory_pool_size;
    
    // Параметры защиты от DDoS
    int max_requests_per_minute;
    int max_concurrent_connections;
    
    // Параметры балансировки нагрузки
    load_balancing_method_t load_balancing_method;
    
    // Параметры протокола
    proxy_protocol_t default_protocol;
    
    // Настройки производительности
    int worker_threads;
    int connection_timeout;
    int idle_timeout;
    
    // Настройки безопасности
    int enable_encryption;
    int enable_compression;
    
} mtproxy_config_t;

/*
 * Инициализирует конфигурацию MTProxy с параметрами по умолчанию
 * @return: указатель на новую конфигурацию или NULL в случае ошибки
 */
mtproxy_config_t *init_default_config();

/*
 * Загружает конфигурацию из файла
 * @param filename: имя файла конфигурации
 * @return: указатель на конфигурацию или NULL в случае ошибки
 */
mtproxy_config_t *load_config_from_file(const char *filename);

/*
 * Сохраняет конфигурацию в файл
 * @param config: указатель на конфигурацию для сохранения
 * @param filename: имя файла для сохранения
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int save_config_to_file(mtproxy_config_t *config, const char *filename);

/*
 * Освобождает память, выделенную для конфигурации
 * @param config: указатель на конфигурацию для освобождения
 */
void free_config(mtproxy_config_t *config);

#endif