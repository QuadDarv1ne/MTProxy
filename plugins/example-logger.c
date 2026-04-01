/*
 * Example Plugin for MTProxy
 * Пример плагина для логирования подключений
 * 
 * Компиляция:
 *   gcc -shared -fPIC -o example-logger.so example-logger.c
 * 
 * Установка:
 *   cp example-logger.so /usr/lib/mtproxy/plugins/
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "../include/plugin-system.h"

/* ============================================================================
 * Данные плагина
 * ============================================================================ */

typedef struct {
    char log_file[512];
    FILE *log_fp;
    int connections_logged;
    int connections_blocked;
    bool verbose;
} example_plugin_data_t;

/* ============================================================================
 * Конфигурация
 * ============================================================================ */

static plugin_config_t g_plugin_config = {
    .name = "example-logger",
    .enabled = true,
    .priority = 0,
    .config_path = "/etc/mtproxy/plugins/example-logger.conf",
    .config_data = NULL,
    .config_size = 0
};

/* ============================================================================
 * Хуки
 * ============================================================================ */

static plugin_result_t on_connection_accept(plugin_hook_context_t *ctx, 
                                            void *plugin_data) {
    example_plugin_data_t *data = (example_plugin_data_t *)plugin_data;
    
    if (!data || !data->verbose) {
        return PLUGIN_OK;
    }
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buffer[64];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    
    if (data->log_fp) {
        fprintf(data->log_fp, "[%s] [CONNECT] Client %s:%d accepted (fd=%d)\n",
                time_buffer,
                ctx->client_ip,
                ctx->client_port,
                ctx->connection_fd);
        fflush(data->log_fp);
    }
    
    printf("[%s] [CONNECT] Client %s:%d accepted\n",
           time_buffer, ctx->client_ip, ctx->client_port);
    
    data->connections_logged++;
    
    return PLUGIN_OK;
}

static plugin_result_t on_connection_close(plugin_hook_context_t *ctx, 
                                           void *plugin_data) {
    example_plugin_data_t *data = (example_plugin_data_t *)plugin_data;
    
    if (!data || !data->verbose) {
        return PLUGIN_OK;
    }
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buffer[64];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    
    if (data->log_fp) {
        fprintf(data->log_fp, "[%s] [DISCONNECT] Client %s closed (fd=%d, reason=%s)\n",
                time_buffer,
                ctx->client_ip,
                ctx->connection_fd,
                ctx->result_data);
        fflush(data->log_fp);
    }
    
    printf("[%s] [DISCONNECT] Client %s closed\n", time_buffer, ctx->client_ip);
    
    return PLUGIN_OK;
}

static plugin_result_t on_data_received(plugin_hook_context_t *ctx, 
                                        void *plugin_data) {
    example_plugin_data_t *data = (example_plugin_data_t *)plugin_data;
    
    if (!data || !data->verbose) {
        return PLUGIN_OK;
    }
    
    if (data->log_fp) {
        fprintf(data->log_fp, "[DATA] Received %zu bytes from %s (fd=%d)\n",
                ctx->data_size,
                ctx->client_ip,
                ctx->connection_fd);
        fflush(data->log_fp);
    }
    
    return PLUGIN_OK;
}

static plugin_result_t on_security_check(plugin_hook_context_t *ctx, 
                                         void *plugin_data) {
    example_plugin_data_t *data = (example_plugin_data_t *)plugin_data;
    
    // Пример блокировки по IP
    if (strncmp(ctx->client_ip, "192.168.1.100", 13) == 0) {
        if (data->log_fp) {
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            char time_buffer[64];
            strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
            
            fprintf(data->log_fp, "[%s] [BLOCK] Blocked IP %s\n",
                    time_buffer, ctx->client_ip);
            fflush(data->log_fp);
        }
        
        data->connections_blocked++;
        ctx->result_code = 403;
        strncpy(ctx->result_data, "IP blocked by plugin", sizeof(ctx->result_data) - 1);
        
        return PLUGIN_REJECT;
    }
    
    return PLUGIN_OK;
}

/* ============================================================================
 * Экспортируемые функции плагина
 * ============================================================================ */

PLUGIN_DECLARE_INFO(
    "example-logger",
    "Example plugin for logging connections",
    "1.0.0",
    "MTProxy Team",
    "Apache-2.0"
)

int plugin_init(const plugin_config_t *config, void **plugin_data) {
    if (!plugin_data) {
        return PLUGIN_ERROR;
    }
    
    // Allocate plugin data
    example_plugin_data_t *data = calloc(1, sizeof(example_plugin_data_t));
    if (!data) {
        return PLUGIN_ERROR;
    }
    
    // Default configuration
    strncpy(data->log_file, "/var/log/mtproxy/plugin-logger.log", 
            sizeof(data->log_file) - 1);
    data->verbose = true;
    data->connections_logged = 0;
    data->connections_blocked = 0;
    
    // Parse configuration file if exists
    if (config && config->config_path) {
        FILE *fp = fopen(config->config_path, "r");
        if (fp) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "log_file=", 9) == 0) {
                    char *value = line + 9;
                    value[strcspn(value, "\n")] = 0;
                    strncpy(data->log_file, value, sizeof(data->log_file) - 1);
                } else if (strncmp(line, "verbose=", 8) == 0) {
                    data->verbose = (atoi(line + 8) != 0);
                }
            }
            fclose(fp);
        }
    }
    
    // Open log file
    data->log_fp = fopen(data->log_file, "a");
    if (!data->log_fp) {
        fprintf(stderr, "[example-logger] Warning: Cannot open log file %s\n",
                data->log_file);
        data->log_fp = stdout;
    }
    
    *plugin_data = data;
    
    printf("[example-logger] Plugin initialized (log_file=%s, verbose=%d)\n",
           data->log_file, data->verbose);
    
    return PLUGIN_OK;
}

void plugin_shutdown(void *plugin_data) {
    example_plugin_data_t *data = (example_plugin_data_t *)plugin_data;
    
    if (!data) {
        return;
    }
    
    if (data->log_fp && data->log_fp != stdout) {
        fprintf(data->log_fp, "[example-logger] Plugin shutdown. Total: %d logged, %d blocked\n",
                data->connections_logged, data->connections_blocked);
        fclose(data->log_fp);
    }
    
    printf("[example-logger] Plugin shutdown. Connections: %d logged, %d blocked\n",
           data->connections_logged, data->connections_blocked);
    
    free(data);
}

int plugin_register_hook(plugin_hook_type_t hook_type,
                        plugin_hook_callback_t callback,
                        int priority,
                        void *plugin_data) {
    // This function is called by the plugin manager to register hooks
    // In this example, we register hooks directly in plugin_init
    return PLUGIN_OK;
}

int plugin_unregister_hook(plugin_hook_type_t hook_type,
                          plugin_hook_callback_t callback) {
    return PLUGIN_OK;
}

void plugin_get_stats(plugin_stats_t *stats) {
    if (!stats) {
        return;
    }
    
    memset(stats, 0, sizeof(plugin_stats_t));
    // In a real plugin, you would fill in actual statistics
}

/* ============================================================================
 * Точка входа для регистрации хуков
 * ============================================================================ */

__attribute__((constructor))
static void plugin_on_load(void) {
    printf("[example-logger] Plugin loaded\n");
}

__attribute__((destructor))
static void plugin_on_unload(void) {
    printf("[example-logger] Plugin unloaded\n");
}
