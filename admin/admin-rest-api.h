/*
    Admin REST API Header for MTProxy
    REST API для управления и мониторинга прокси
*/

#ifndef ADMIN_REST_API_H
#define ADMIN_REST_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Инициализация REST API сервера
 * 
 * @param bind_address Адрес для прослушивания (NULL для 0.0.0.0)
 * @param port Порт для прослушивания
 * @param api_key API ключ для авторизации (NULL для отключения)
 * @return 0 при успехе, -1 при ошибке
 */
int rest_api_init(const char* bind_address, uint16_t port, const char* api_key);

/**
 * Остановка и очистка REST API сервера
 */
void rest_api_cleanup(void);

/**
 * Проверка инициализации REST API
 * 
 * @return 1 если инициализирован, 0 если нет
 */
int rest_api_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif // ADMIN_REST_API_H
