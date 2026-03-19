/**
 * @file mtproxy.c
 * @brief Реализация публичного API для интеграции с MTProxy
 */

#include "include/mtproxy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Внутренние структуры
static struct {
    bool initialized;
    bool running;
    uint16_t port;
    char** secrets;
    int secret_count;
    uint32_t max_connections;
    bool enable_ipv6;
    bool enable_stats;
    uint64_t start_time;
    mtproxy_stats_t stats;
    char last_error[256];
} g_mtproxy = {0};

// ============================================================================
// Внутренние функции
// ============================================================================

static void set_error(const char* error) {
    strncpy(g_mtproxy.last_error, error, sizeof(g_mtproxy.last_error) - 1);
    g_mtproxy.last_error[sizeof(g_mtproxy.last_error) - 1] = '\0';
}

static bool is_valid_hex(const char* str) {
    if (!str) return false;
    size_t len = strlen(str);
    if (len != 64) return false;  // 32 байта = 64 hex символа
    
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (!((c >= '0' && c <= '9') || 
              (c >= 'a' && c <= 'f') || 
              (c >= 'A' && c <= 'F'))) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// Основные функции управления
// ============================================================================

MTPROXY_API int mtproxy_init(void) {
    if (g_mtproxy.initialized) {
        return 0;  // Уже инициализирован
    }
    
    // Инициализация по умолчанию
    g_mtproxy.port = 443;
    g_mtproxy.max_connections = 10000;
    g_mtproxy.enable_ipv6 = false;
    g_mtproxy.enable_stats = true;
    g_mtproxy.secrets = NULL;
    g_mtproxy.secret_count = 0;
    g_mtproxy.initialized = true;
    g_mtproxy.running = false;
    
    memset(&g_mtproxy.stats, 0, sizeof(g_mtproxy.stats));
    set_error("");
    
    // Здесь должна быть инициализация основных компонентов MTProxy
    // (сеть, криптография, пулы соединений и т.д.)
    
    return 0;
}

MTPROXY_API int mtproxy_start(void) {
    if (!g_mtproxy.initialized) {
        set_error("MTProxy не инициализирован");
        return -1;
    }
    
    if (g_mtproxy.running) {
        set_error("MTProxy уже запущен");
        return -2;
    }
    
    if (g_mtproxy.secret_count == 0) {
        set_error("Необходимо добавить хотя бы один секретный ключ");
        return -3;
    }
    
    // Здесь должен быть запуск основных компонентов MTProxy
    // Запуск сервера на указанном порту
    // Инициализация обработчиков подключений
    
    g_mtproxy.running = true;
    g_mtproxy.start_time = (uint64_t)time(NULL);
    
    return 0;
}

MTPROXY_API void mtproxy_stop(void) {
    if (!g_mtproxy.running) {
        return;
    }
    
    // Здесь должна быть остановка всех компонентов MTProxy
    // Закрытие всех подключений
    // Освобождение ресурсов
    
    g_mtproxy.running = false;
}

MTPROXY_API bool mtproxy_is_running(void) {
    return g_mtproxy.running;
}

// ============================================================================
// Функции конфигурации
// ============================================================================

MTPROXY_API int mtproxy_set_port(uint16_t port) {
    if (port == 0) {
        set_error("Недопустимый порт");
        return -1;
    }
    
    if (g_mtproxy.running) {
        set_error("Нельзя изменить порт во время работы");
        return -2;
    }
    
    g_mtproxy.port = port;
    return 0;
}

MTPROXY_API int mtproxy_add_secret(const char* secret) {
    if (!secret) {
        set_error("Пустой секрет");
        return -1;
    }
    
    if (!is_valid_hex(secret)) {
        set_error("Неверный формат секрета (должен быть 64 hex символа)");
        return -2;
    }
    
    // Проверка на дубликат
    for (int i = 0; i < g_mtproxy.secret_count; i++) {
        if (strcmp(g_mtproxy.secrets[i], secret) == 0) {
            set_error("Секрет уже существует");
            return -3;
        }
    }
    
    // Добавление секрета
    g_mtproxy.secrets = realloc(g_mtproxy.secrets, 
                                (g_mtproxy.secret_count + 1) * sizeof(char*));
    if (!g_mtproxy.secrets) {
        set_error("Ошибка выделения памяти");
        return -4;
    }
    
    g_mtproxy.secrets[g_mtproxy.secret_count] = strdup(secret);
    if (!g_mtproxy.secrets[g_mtproxy.secret_count]) {
        set_error("Ошибка выделения памяти");
        return -4;
    }
    
    g_mtproxy.secret_count++;
    return 0;
}

MTPROXY_API int mtproxy_remove_secret(const char* secret) {
    if (!secret) {
        return -1;
    }
    
    for (int i = 0; i < g_mtproxy.secret_count; i++) {
        if (strcmp(g_mtproxy.secrets[i], secret) == 0) {
            free(g_mtproxy.secrets[i]);
            
            // Сдвиг массива
            for (int j = i; j < g_mtproxy.secret_count - 1; j++) {
                g_mtproxy.secrets[j] = g_mtproxy.secrets[j + 1];
            }
            
            g_mtproxy.secret_count--;
            g_mtproxy.secrets = realloc(g_mtproxy.secrets, 
                                        g_mtproxy.secret_count * sizeof(char*));
            return 0;
        }
    }
    
    set_error("Секрет не найден");
    return -2;
}

MTPROXY_API void mtproxy_clear_secrets(void) {
    for (int i = 0; i < g_mtproxy.secret_count; i++) {
        free(g_mtproxy.secrets[i]);
    }
    free(g_mtproxy.secrets);
    g_mtproxy.secrets = NULL;
    g_mtproxy.secret_count = 0;
}

MTPROXY_API int mtproxy_set_max_connections(uint32_t max_connections) {
    if (max_connections == 0) {
        set_error("Недопустимое значение max_connections");
        return -1;
    }
    
    g_mtproxy.max_connections = max_connections;
    return 0;
}

MTPROXY_API int mtproxy_set_ipv6(bool enable) {
    if (g_mtproxy.running) {
        set_error("Нельзя изменить IPv6 настройку во время работы");
        return -1;
    }
    
    g_mtproxy.enable_ipv6 = enable;
    return 0;
}

MTPROXY_API int mtproxy_apply_config(const mtproxy_config_t* config) {
    if (!config) {
        set_error("Пустая конфигурация");
        return -1;
    }
    
    int ret;
    
    ret = mtproxy_set_port(config->port);
    if (ret != 0) return ret;
    
    if (config->secret && strlen(config->secret) > 0) {
        ret = mtproxy_add_secret(config->secret);
        if (ret != 0) return ret;
    }
    
    ret = mtproxy_set_max_connections(config->max_connections);
    if (ret != 0) return ret;
    
    ret = mtproxy_set_ipv6(config->enable_ipv6);
    if (ret != 0) return ret;
    
    g_mtproxy.enable_stats = config->enable_stats;
    
    return 0;
}

// ============================================================================
// Функции статистики
// ============================================================================

MTPROXY_API mtproxy_stats_t* mtproxy_get_stats(void) {
    if (!g_mtproxy.initialized) {
        return NULL;
    }
    
    // Обновление статистики
    // Здесь должен быть реальный код получения статистики из ядра MTProxy
    
    g_mtproxy.stats.active_connections = 0;  // Заглушка
    g_mtproxy.stats.total_connections = 0;   // Заглушка
    g_mtproxy.stats.bytes_sent = 0;          // Заглушка
    g_mtproxy.stats.bytes_received = 0;      // Заглушка
    
    if (g_mtproxy.start_time > 0) {
        g_mtproxy.stats.start_time = g_mtproxy.start_time;
        uint64_t uptime = (uint64_t)time(NULL) - g_mtproxy.start_time;
        g_mtproxy.stats.cpu_usage = 0.0;     // Заглушка
        g_mtproxy.stats.memory_usage = 0;    // Заглушка
    }
    
    return &g_mtproxy.stats;
}

MTPROXY_API uint32_t mtproxy_get_active_connections(void) {
    return g_mtproxy.stats.active_connections;
}

MTPROXY_API uint32_t mtproxy_get_total_connections(void) {
    return g_mtproxy.stats.total_connections;
}

MTPROXY_API uint64_t mtproxy_get_bytes_sent(void) {
    return g_mtproxy.stats.bytes_sent;
}

MTPROXY_API uint64_t mtproxy_get_bytes_received(void) {
    return g_mtproxy.stats.bytes_received;
}

MTPROXY_API uint64_t mtproxy_get_uptime(void) {
    if (g_mtproxy.start_time == 0) {
        return 0;
    }
    return (uint64_t)time(NULL) - g_mtproxy.start_time;
}

// ============================================================================
// Утилиты
// ============================================================================

MTPROXY_API int mtproxy_generate_secret(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size < 65) {
        return -1;
    }
    
    // Генерация случайного секрета (32 байта = 64 hex символа)
    static const char hex_chars[] = "0123456789abcdef";
    
    for (int i = 0; i < 64; i++) {
        buffer[i] = hex_chars[rand() % 16];
    }
    buffer[64] = '\0';
    
    return 0;
}

MTPROXY_API bool mtproxy_validate_secret(const char* secret) {
    return is_valid_hex(secret);
}

MTPROXY_API const char* mtproxy_get_version(void) {
    return "1.0.0";
}

MTPROXY_API const char* mtproxy_get_last_error(void) {
    return g_mtproxy.last_error;
}
