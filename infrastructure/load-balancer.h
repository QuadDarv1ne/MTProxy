#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include <stdint.h>

/*
 * Система балансировки нагрузки - компонент, который распределяет входящие запросы
 * между несколькими серверами для обеспечения высокой доступности и оптимальной производительности.
 */

// Backend server structure
typedef struct backend_server {
    char address[256];
    int port;
    int weight;
    int active_connections;
    int total_requests;
    float response_time_avg;
    long last_check;
    int health_status;
} backend_server_t;

// Load balancing methods
typedef enum {
    LB_ROUND_ROBIN,
    LB_LEAST_CONNECTIONS,
    LB_WEIGHTED_ROUND_ROBIN,
    LB_IP_HASH
} load_balancing_method_t;

// Main load balancer structure
typedef struct load_balancer {
    backend_server_t *servers;
    int server_count;
    int max_servers;
    load_balancing_method_t method;
    int current_index;
    int total_weight;
} load_balancer_t;

/*
 * Инициализирует систему балансировки нагрузки с указанным методом
 * @param method: метод балансировки нагрузки
 * @return: указатель на новую систему балансировки или NULL в случае ошибки
 */
load_balancer_t *init_load_balancer(load_balancing_method_t method);

/*
 * Добавляет сервер в систему балансировки нагрузки
 * @param lb: указатель на систему балансировки
 * @param address: адрес сервера
 * @param port: порт сервера
 * @param weight: вес сервера для взвешенных методов балансировки
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int add_backend_server(load_balancer_t *lb, const char *address, int port, int weight);

/*
 * Выбирает сервер для обработки запроса клиента
 * @param lb: указатель на систему балансировки
 * @param client_ip: IP-адрес клиента (для метода IP-хэша)
 * @return: индекс выбранного сервера или -1 в случае ошибки
 */
int select_backend_server(load_balancer_t *lb, uint32_t client_ip);

/*
 * Сообщает время ответа сервера для обновления метрик производительности
 * @param lb: указатель на систему балансировки
 * @param server_idx: индекс сервера
 * @param response_time: время ответа в миллисекундах
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int report_response_time(load_balancer_t *lb, int server_idx, float response_time);

/*
 * Проверяет состояние здоровья всех серверов
 * @param lb: указатель на систему балансировки
 */
void health_check_servers(load_balancer_t *lb);

/*
 * Уничтожает систему балансировки нагрузки и освобождает память
 * @param lb: указатель на систему балансировки для уничтожения
 */
void destroy_load_balancer(load_balancer_t *lb);

#endif