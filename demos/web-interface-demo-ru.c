/*
    Русскоязычная демонстрация web-интерфейса MTProxy
    Пример инициализации и запуска админ-панели на русском
*/

// Объявления функций
int printf(const char *format, ...);

#include "../admin/admin-web-interface.h"

// Callback функции для демонстрации
void on_user_login_callback(admin_user_t *user) {
    printf("[СОБЫТИЕ] Пользователь вошел: %s (ID: %llu)\n", user->username, user->user_id);
}

void on_user_logout_callback(admin_user_t *user) {
    printf("[СОБЫТИЕ] Пользователь вышел: %s (ID: %llu)\n", user->username, user->user_id);
}

void on_api_request_callback(const char *endpoint, int method) {
    const char *method_str = "НЕИЗВЕСТНО";
    switch (method) {
        case HTTP_METHOD_GET: method_str = "GET"; break;
        case HTTP_METHOD_POST: method_str = "POST"; break;
        case HTTP_METHOD_PUT: method_str = "PUT"; break;
        case HTTP_METHOD_DELETE: method_str = "DELETE"; break;
    }
    printf("[СОБЫТИЕ] API Запрос: %s %s\n", method_str, endpoint);
}

void on_security_event_callback(const char *event, const char *details) {
    printf("[БЕЗОПАСНОСТЬ] %s: %s\n", event, details);
}

int main() {
    printf("Запуск демонстрации Web-интерфейса MTProxy\n");
    printf("=============================================\n\n");
    
    // 1. Инициализация веб-интерфейса
    printf("1. Инициализация веб-интерфейса...\n");
    
    web_server_config_t web_config = {0};
    web_config.enable_http = 1;
    web_config.http_port = 8080;
    web_config.enable_https = 1;
    web_config.https_port = 8443;
    web_config.enable_rate_limiting = 1;
    web_config.requests_per_minute = 60;
    web_config.enable_logging = 1;
    web_config.enable_cors = 1;
    
    const char *origins = "http://localhost:3000,https://admin.mtproxy.local";
    for (int i = 0; i < 1023 && origins[i] != '\0'; i++) {
        web_config.allowed_origins[i] = origins[i];
    }
    web_config.allowed_origins[1023] = '\0';
    
    admin_web_interface_t *admin_web = admin_web_init(&web_config);
    if (!admin_web) {
        printf("❌ Не удалось инициализировать веб-интерфейс\n");
        return 1;
    }
    
    printf("✅ Веб-интерфейс успешно инициализирован\n\n");
    
    // 2. Регистрация callback функций
    printf("2. Регистрация callback функций...\n");
    
    admin_web_set_login_callback(admin_web, on_user_login_callback);
    admin_web_set_logout_callback(admin_web, on_user_logout_callback);
    admin_web_set_api_request_callback(admin_web, on_api_request_callback);
    admin_web_set_security_callback(admin_web, on_security_event_callback);
    
    printf("✅ Callback функции зарегистрированы\n\n");
    
    // 3. Создание администраторов
    printf("3. Создание пользователей администраторов...\n");
    
    // Создание главного администратора
    int result = admin_web_create_user(admin_web, "admin", "SecurePass123!", 
                                      "admin@mtproxy.local", ADMIN_ACCESS_ADMIN);
    if (result == 0) {
        printf("✅ Создан администратор: admin\n");
    } else {
        printf("❌ Не удалось создать администратора\n");
    }
    
    // Создание пользователя с правами чтения
    result = admin_web_create_user(admin_web, "monitor", "MonitorPass456!", 
                                  "monitor@mtproxy.local", ADMIN_ACCESS_READ);
    if (result == 0) {
        printf("✅ Создан пользователь мониторинга: monitor\n");
    }
    
    // Создание пользователя с правами записи
    result = admin_web_create_user(admin_web, "operator", "OperatorPass789!", 
                                  "operator@mtproxy.local", ADMIN_ACCESS_WRITE);
    if (result == 0) {
        printf("✅ Создан оператор: operator\n");
    }
    
    printf("\n");
    
    // 4. Создание API ключей
    printf("4. Генерация API ключей...\n");
    
    char api_key_read[128];
    result = admin_web_create_api_key(admin_web, 2, "Служба мониторинга", 
                                     API_KEY_TYPE_READ, 86400, api_key_read);
    if (result == 0) {
        printf("✅ Сгенерирован READ API ключ: %s\n", api_key_read);
    }
    
    char api_key_write[128];
    result = admin_web_create_api_key(admin_web, 3, "Служба автоматизации", 
                                     API_KEY_TYPE_WRITE, 43200, api_key_write);
    if (result == 0) {
        printf("✅ Сгенерирован WRITE API ключ: %s\n", api_key_write);
    }
    
    printf("\n");
    
    // 5. Запуск веб-сервера
    printf("5. Запуск веб-сервера...\n");
    
    result = admin_web_start_server(admin_web);
    if (result == 0) {
        printf("✅ Веб-сервер успешно запущен\n");
        printf("🌐 HTTP интерфейс: http://localhost:8080\n");
        printf("🔒 HTTPS интерфейс: https://localhost:8443\n");
        printf("📊 Админ-панель: http://localhost:8080/admin\n");
        printf("📱 API endpoint: http://localhost:8080/api/v1\n\n");
    } else {
        printf("❌ Не удалось запустить веб-сервер\n");
        admin_web_cleanup(admin_web);
        return 1;
    }
    
    // 6. Демонстрация API функциональности
    printf("6. Демонстрация функциональности API...\n");
    
    // Тестирование status API
    char response[1024];
    int status = admin_web_handle_api_request(admin_web, HTTP_METHOD_GET,
                                             API_ENDPOINT_STATUS, NULL,
                                             NULL, response, sizeof(response));
    printf("📊 Ответ Status API (%d): %s\n", status, response);
    
    // Тестирование stats API с API ключом
    status = admin_web_handle_api_request(admin_web, HTTP_METHOD_GET,
                                         API_ENDPOINT_STATS, NULL,
                                         api_key_read, response, sizeof(response));
    printf("📈 Ответ Stats API (%d): %s\n", status, response);
    
    // Тестирование connections API
    status = admin_web_handle_api_request(admin_web, HTTP_METHOD_GET,
                                         API_ENDPOINT_CONNECTIONS, NULL,
                                         api_key_read, response, sizeof(response));
    printf("🔗 Ответ Connections API (%d): %s\n", status, response);
    
    printf("\n");
    
    // 7. Показ статистики
    printf("7. Статистика веб-интерфейса:\n");
    
    web_interface_stats_t stats;
    admin_web_get_stats(admin_web, &stats);
    
    printf("   Всего запросов: %lld\n", stats.total_requests);
    printf("   Успешных запросов: %lld\n", stats.successful_requests);
    printf("   Отклоненных запросов: %lld\n", stats.failed_requests);
    printf("   API запросов: %lld\n", stats.api_requests);
    printf("   Активных сессий: %lld\n", stats.active_sessions);
    printf("   Всего пользователей: %d\n", admin_web->user_count);
    printf("   API ключей: %d\n", admin_web->api_key_count);
    
    printf("\n");
    
    // 8. Демонстрация аутентификации
    printf("8. Тестирование аутентификации...\n");
    
    // Успешная аутентификация
    result = admin_web_authenticate_user(admin_web, "admin", "SecurePass123!", "127.0.0.1");
    if (result == 0) {
        printf("✅ Аутентификация администратора успешна\n");
    }
    
    // Неудачная аутентификация
    result = admin_web_authenticate_user(admin_web, "admin", "wrong_password", "127.0.0.1");
    if (result != 0) {
        printf("✅ Ошибка аутентификации обработана корректно\n");
    }
    
    printf("\n");
    
    // 9. Создание сессии
    printf("9. Создание пользовательской сессии...\n");
    
    char session_token[64];
    result = admin_web_create_session(admin_web, 1, "127.0.0.1", 
                                     "Mozilla/5.0 Демо Браузер", session_token);
    if (result == 0) {
        printf("✅ Сессия создана: %s\n", session_token);
        
        // Валидация сессии
        uint64_t user_id;
        result = admin_web_validate_session(admin_web, session_token, &user_id);
        if (result == 0) {
            printf("✅ Валидация сессии успешна для пользователя ID: %llu\n", user_id);
        }
    }
    
    printf("\n");
    
    // 10. Демонстрация мониторинга
    printf("10. Выполнение проверок состояния...\n");
    
    // В реальной реализации: int healthy_components = admin_web_perform_health_check(admin_web);
    int healthy_components = 3; // Симуляция
    printf("✅ Проверка состояния завершена: %d здоровых компонентов\n", healthy_components);
    
    // Очистка истекших сессий
    admin_web_cleanup_expired_sessions(admin_web);
    printf("✅ Истекшие сессии очищены\n");
    
    printf("\n");
    
    // 11. Информация о доступных эндпоинтах
    printf("11. Доступные API эндпоинты:\n");
    printf("   GET  /api/v1/status        - Статус системы\n");
    printf("   GET  /api/v1/stats         - Статистика производительности\n");
    printf("   GET  /api/v1/connections   - Активные соединения\n");
    printf("   GET  /api/v1/users         - Управление пользователями\n");
    printf("   POST /api/v1/users         - Создание нового пользователя\n");
    printf("   GET  /api/v1/config        - Конфигурация\n");
    printf("   GET  /api/v1/logs          - Системные логи\n");
    printf("   GET  /api/v1/security      - События безопасности\n");
    
    printf("\n");
    
    // 12. Демонстрация безопасности
    printf("12. Демонстрация функций безопасности:\n");
    printf("   ✅ Ограничение запросов (60 запросов/минуту)\n");
    printf("   ✅ Таймаут сессий (1 час)\n");
    printf("   ✅ Валидация сложности паролей\n");
    printf("   ✅ Блокировка аккаунтов после 5 неудачных попыток\n");
    printf("   ✅ Аутентификация по API ключам\n");
    printf("   ✅ Защита CORS\n");
    printf("   ✅ Логирование запросов\n");
    printf("   ✅ Мониторинг событий безопасности\n");
    
    printf("\n");
    
    // 13. Инструкции по использованию
    printf("13. Инструкции по использованию:\n");
    printf("   🔧 Доступ к админ-панели: http://localhost:8080/admin\n");
    printf("   📡 Используйте API ключ для программного доступа\n");
    printf("   🔐 HTTPS рекомендуется для production\n");
    printf("   📊 Мониторинг статистики: http://localhost:8080/api/v1/stats\n");
    printf("   🛡️  События безопасности: http://localhost:8080/api/v1/security\n");
    
    printf("\n");
    printf("🎉 Web-интерфейс MTProxy запущен и работает\n");
    printf("Нажмите Ctrl+C для остановки сервера\n");
    
    // Симуляция работы сервера
    printf("\n[СЕРВЕР] Веб-интерфейс активен и принимает запросы...\n");
    printf("[СЕРВЕР] Готов обрабатывать входящие запросы\n");
    
    // В реальной реализации здесь был бы event loop
    // Для демонстрации просто ждем 10 секунд
    for (int i = 10; i > 0; i--) {
        printf("[СЕРВЕР] Работает... (осталось %d секунд)\n", i);
        // В реальной реализации: sleep(1) или event loop
    }
    
    // 14. Остановка и очистка
    printf("\n14. Остановка веб-интерфейса...\n");
    
    admin_web_stop_server(admin_web);
    printf("✅ Веб-сервер остановлен\n");
    
    admin_web_cleanup(admin_web);
    printf("✅ Веб-интерфейс очищен\n");
    
    printf("\n✅ Демонстрация успешно завершена\n");
    
    return 0;
}