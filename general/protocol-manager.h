#ifndef PROTOCOL_MANAGER_H
#define PROTOCOL_MANAGER_H

/*
 * Менеджер протоколов - компонент, который управляет поддерживаемыми протоколами прокси-сервера
 * и обеспечивает возможность динамической регистрации и обработки различных типов протоколов.
 */

// Define supported proxy protocols
typedef enum {
    PROTO_MT_PROTO,
    PROTO_SHADOWSOCKS,
    PROTO_SOCKS5,
    PROTO_HTTP_CONNECT
} proxy_protocol_t;

// Handler structure for protocol processing
typedef struct protocol_handler {
    proxy_protocol_t protocol_type;
    int (*init)(void *config);
    int (*process)(void *data, size_t len);
    int (*encrypt)(void *data, size_t len);
    int (*decrypt)(void *data, size_t len);
} protocol_handler_t;

/*
 * Регистрирует обработчик для указанного протокола
 * @param proto: тип протокола для регистрации
 * @param handler: указатель на структуру обработчика протокола
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int register_protocol_handler(proxy_protocol_t proto, protocol_handler_t *handler);

/*
 * Обрабатывает данные для указанного протокола
 * @param proto: тип протокола для обработки
 * @param data: указатель на данные для обработки
 * @param len: длина данных в байтах
 * @return: результат обработки
 */
int handle_protocol_data(proxy_protocol_t proto, void *data, size_t len);

/*
 * Переключается с одного протокола на другой
 * @param from: исходный протокол
 * @param to: целевой протокол
 * @return: 0 в случае успеха, -1 в случае ошибки
 */
int switch_protocol(proxy_protocol_t from, proxy_protocol_t to);

#endif