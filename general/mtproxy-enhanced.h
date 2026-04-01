#ifndef MT_PROXY_ENHANCED_H
#define MT_PROXY_ENHANCED_H

/*
 * Улучшенная версия MTProxy с дополнительными компонентами для
 * производительности, безопасности и масштабируемости
 */

#include "config_manager.h"
#include "memory-pool.h"
#include "ddos-protection-enhanced.h"
#include "protocol-manager.h"
#include "load-balancer.h"
#include "perf_monitor/perf-monitor.h"
#include "security_enhanced/security-enhanced.h"
#include "conn_pool/conn-pool.h"
#include "thread_system/thread-system.h"

/*
 * Основная структура MTProxy
 * Объединяет все улучшенные компоненты
 */
typedef struct mtproxy_instance {
    mtproxy_config_t *config;      // Конфигурация
    memory_pool_t *mem_pool;       // Пул памяти
    ddos_protector_t *ddos_protect; // Защита от DDoS
    load_balancer_t *load_balancer; // Балансировка нагрузки
    protocol_handler_t *protocols;  // Обработчики протоколов
    int running;                   // Состояние работы
} mtproxy_instance_t;

/*
 * Инициализирует новый экземпляр MTProxy с улучшенными компонентами
 * @param config: конфигурация для инициализации
 * @return: указатель на новый экземпляр или NULL в случае ошибки
 */
mtproxy_instance_t *init_mtproxy_instance(mtproxy_config_t *config);

/*
 * Запускает экземпляр MTProxy
 * @param instance: указатель на экземпляр для запуска
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int start_mtproxy_instance(mtproxy_instance_t *instance);

/*
 * Останавливает экземпляр MTProxy
 * @param instance: указатель на экземпляр для остановки
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int stop_mtproxy_instance(mtproxy_instance_t *instance);

/*
 * Уничтожает экземпляр MTProxy и освобождает ресурсы
 * @param instance: указатель на экземпляр для уничтожения
 */
void destroy_mtproxy_instance(mtproxy_instance_t *instance);

/*
 * Обрабатывает входящее соединение
 * @param instance: указатель на экземпляр
 * @param client_data: данные от клиента
 * @return: результат обработки
 */
int handle_client_connection(mtproxy_instance_t *instance, void *client_data);

#endif