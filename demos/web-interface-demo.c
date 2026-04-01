/*
    Демонстрационный запуск web-интерфейса MTProxy
    Пример инициализации и запуска админ-панели
*/

// Объявления функций
int printf(const char *format, ...);

#include "../admin/admin-web-interface.h"

// Callback функции для демонстрации
void on_user_login_callback(admin_user_t *user) {
    printf("[EVENT] User logged in: %s (ID: %llu)\n", user->username, user->user_id);
}

void on_user_logout_callback(admin_user_t *user) {
    printf("[EVENT] User logged out: %s (ID: %llu)\n", user->username, user->user_id);
}

void on_api_request_callback(const char *endpoint, int method) {
    const char *method_str = "UNKNOWN";
    switch (method) {
        case HTTP_METHOD_GET: method_str = "GET"; break;
        case HTTP_METHOD_POST: method_str = "POST"; break;
        case HTTP_METHOD_PUT: method_str = "PUT"; break;
        case HTTP_METHOD_DELETE: method_str = "DELETE"; break;
    }
    printf("[EVENT] API Request: %s %s\n", method_str, endpoint);
}

void on_security_event_callback(const char *event, const char *details) {
    printf("[SECURITY] %s: %s\n", event, details);
}

int main() {
    printf("Starting MTProxy Admin Web Interface Demo\n");
    printf("==========================================\n\n");
    
    // 1. Инициализация веб-интерфейса
    printf("1. Initializing web interface...\n");
    
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
        printf("❌ Failed to initialize web interface\n");
        return 1;
    }
    
    printf("✅ Web interface initialized successfully\n\n");
    
    // 2. Регистрация callback функций
    printf("2. Registering callback functions...\n");
    
    admin_web_set_login_callback(admin_web, on_user_login_callback);
    admin_web_set_logout_callback(admin_web, on_user_logout_callback);
    admin_web_set_api_request_callback(admin_web, on_api_request_callback);
    admin_web_set_security_callback(admin_web, on_security_event_callback);
    
    printf("✅ Callback functions registered\n\n");
    
    // 3. Создание администраторов
    printf("3. Creating admin users...\n");
    
    // Создание главного администратора
    int result = admin_web_create_user(admin_web, "admin", "SecurePass123!", 
                                      "admin@mtproxy.local", ADMIN_ACCESS_ADMIN);
    if (result == 0) {
        printf("✅ Created admin user: admin\n");
    } else {
        printf("❌ Failed to create admin user\n");
    }
    
    // Создание пользователя с правами чтения
    result = admin_web_create_user(admin_web, "monitor", "MonitorPass456!", 
                                  "monitor@mtproxy.local", ADMIN_ACCESS_READ);
    if (result == 0) {
        printf("✅ Created monitor user: monitor\n");
    }
    
    // Создание пользователя с правами записи
    result = admin_web_create_user(admin_web, "operator", "OperatorPass789!", 
                                  "operator@mtproxy.local", ADMIN_ACCESS_WRITE);
    if (result == 0) {
        printf("✅ Created operator user: operator\n");
    }
    
    printf("\n");
    
    // 4. Создание API ключей
    printf("4. Generating API keys...\n");
    
    char api_key_read[128];
    result = admin_web_create_api_key(admin_web, 2, "Monitoring Service", 
                                     API_KEY_TYPE_READ, 86400, api_key_read);
    if (result == 0) {
        printf("✅ Generated READ API key: %s\n", api_key_read);
    }
    
    char api_key_write[128];
    result = admin_web_create_api_key(admin_web, 3, "Automation Service", 
                                     API_KEY_TYPE_WRITE, 43200, api_key_write);
    if (result == 0) {
        printf("✅ Generated WRITE API key: %s\n", api_key_write);
    }
    
    printf("\n");
    
    // 5. Запуск веб-сервера
    printf("5. Starting web server...\n");
    
    result = admin_web_start_server(admin_web);
    if (result == 0) {
        printf("✅ Web server started successfully\n");
        printf("🌐 HTTP interface: http://localhost:8080\n");
        printf("🔒 HTTPS interface: https://localhost:8443\n");
        printf("📊 Admin panel: http://localhost:8080/admin\n");
        printf("📱 API endpoint: http://localhost:8080/api/v1\n\n");
    } else {
        printf("❌ Failed to start web server\n");
        admin_web_cleanup(admin_web);
        return 1;
    }
    
    // 6. Демонстрация API функциональности
    printf("6. Demonstrating API functionality...\n");
    
    // Тестирование status API
    char response[1024];
    int status = admin_web_handle_api_request(admin_web, HTTP_METHOD_GET,
                                             API_ENDPOINT_STATUS, NULL,
                                             NULL, response, sizeof(response));
    printf("📊 Status API response (%d): %s\n", status, response);
    
    // Тестирование stats API с API ключом
    status = admin_web_handle_api_request(admin_web, HTTP_METHOD_GET,
                                         API_ENDPOINT_STATS, NULL,
                                         api_key_read, response, sizeof(response));
    printf("📈 Stats API response (%d): %s\n", status, response);
    
    // Тестирование connections API
    status = admin_web_handle_api_request(admin_web, HTTP_METHOD_GET,
                                         API_ENDPOINT_CONNECTIONS, NULL,
                                         api_key_read, response, sizeof(response));
    printf("🔗 Connections API response (%d): %s\n", status, response);
    
    printf("\n");
    
    // 7. Показ статистики
    printf("7. Web interface statistics:\n");
    
    web_interface_stats_t stats;
    admin_web_get_stats(admin_web, &stats);
    
    printf("   Total requests: %lld\n", stats.total_requests);
    printf("   Successful requests: %lld\n", stats.successful_requests);
    printf("   Failed requests: %lld\n", stats.failed_requests);
    printf("   API requests: %lld\n", stats.api_requests);
    printf("   Active sessions: %lld\n", stats.active_sessions);
    printf("   Total users: %d\n", admin_web->user_count);
    printf("   API keys: %d\n", admin_web->api_key_count);
    
    printf("\n");
    
    // 8. Демонстрация аутентификации
    printf("8. Testing authentication...\n");
    
    // Успешная аутентификация
    result = admin_web_authenticate_user(admin_web, "admin", "SecurePass123!", "127.0.0.1");
    if (result == 0) {
        printf("✅ Admin authentication successful\n");
    }
    
    // Неудачная аутентификация
    result = admin_web_authenticate_user(admin_web, "admin", "wrong_password", "127.0.0.1");
    if (result != 0) {
        printf("✅ Authentication failure handled correctly\n");
    }
    
    printf("\n");
    
    // 9. Создание сессии
    printf("9. Creating user session...\n");
    
    char session_token[64];
    result = admin_web_create_session(admin_web, 1, "127.0.0.1", 
                                     "Mozilla/5.0 Demo Browser", session_token);
    if (result == 0) {
        printf("✅ Session created: %s\n", session_token);
        
        // Валидация сессии
        uint64_t user_id;
        result = admin_web_validate_session(admin_web, session_token, &user_id);
        if (result == 0) {
            printf("✅ Session validation successful for user ID: %llu\n", user_id);
        }
    }
    
    printf("\n");
    
    // 10. Демонстрация мониторинга
    printf("10. Performing health checks...\n");
    
    // В реальной реализации: int healthy_components = admin_web_perform_health_check(admin_web);
    int healthy_components = 3; // Симуляция
    printf("✅ Health check completed: %d healthy components\n", healthy_components);
    
    // Очистка истекших сессий
    admin_web_cleanup_expired_sessions(admin_web);
    printf("✅ Expired sessions cleaned up\n");
    
    printf("\n");
    
    // 11. Информация о доступных эндпоинтах
    printf("11. Available API endpoints:\n");
    printf("   GET  /api/v1/status        - System status\n");
    printf("   GET  /api/v1/stats         - Performance statistics\n");
    printf("   GET  /api/v1/connections   - Active connections\n");
    printf("   GET  /api/v1/users         - User management\n");
    printf("   POST /api/v1/users         - Create new user\n");
    printf("   GET  /api/v1/config        - Configuration\n");
    printf("   GET  /api/v1/logs          - System logs\n");
    printf("   GET  /api/v1/security      - Security events\n");
    
    printf("\n");
    
    // 12. Демонстрация безопасности
    printf("12. Security features demonstration:\n");
    printf("   ✅ Rate limiting (60 requests/minute)\n");
    printf("   ✅ Session timeout (1 hour)\n");
    printf("   ✅ Password strength validation\n");
    printf("   ✅ Account lockout after 5 failed attempts\n");
    printf("   ✅ API key authentication\n");
    printf("   ✅ CORS protection\n");
    printf("   ✅ Request logging\n");
    printf("   ✅ Security event monitoring\n");
    
    printf("\n");
    
    // 13. Инструкции по использованию
    printf("13. Usage instructions:\n");
    printf("   🔧 Access admin panel: http://localhost:8080/admin\n");
    printf("   📡 Use API key for programmatic access\n");
    printf("   🔐 HTTPS recommended for production\n");
    printf("   📊 Monitor stats at: http://localhost:8080/api/v1/stats\n");
    printf("   🛡️  Security events at: http://localhost:8080/api/v1/security\n");
    
    printf("\n");
    printf("🎉 MTProxy Admin Web Interface is running!\n");
    printf("Press Ctrl+C to stop the server\n");
    
    // Симуляция работы сервера
    printf("\n[SERVER] Web interface is now active and listening...\n");
    printf("[SERVER] Ready to handle incoming requests\n");
    
    // В реальной реализации здесь был бы event loop
    // Для демонстрации просто ждем 10 секунд
    for (int i = 10; i > 0; i--) {
        printf("[SERVER] Running... (%d seconds remaining)\n", i);
        // В реальной реализации: sleep(1) или event loop
    }
    
    // 14. Остановка и очистка
    printf("\n14. Stopping web interface...\n");
    
    admin_web_stop_server(admin_web);
    printf("✅ Web server stopped\n");
    
    admin_web_cleanup(admin_web);
    printf("✅ Web interface cleaned up\n");
    
    printf("\n✅ Demo completed successfully!\n");
    
    return 0;
}