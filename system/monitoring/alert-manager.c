/*
    MTProxy Alert Manager
    Реализация системы уведомлений и алертинга
*/

#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

#include "system/monitoring/alert-manager.h"
#include "common/utils.h"
#include "common/kprintf.h"

// ============================================================================
// Глобальные переменные
// ============================================================================

static struct {
    bool initialized;
    alert_channel_config_t channels[ALERT_MAX_CHANNELS];
    int channel_count;
    alert_t alerts[100];  // Кольцевой буфер последних алертов
    int alert_index;
    uint64_t alert_counter;
    alert_manager_stats_t stats;
    alert_send_callback_t custom_callback;
    int64_t last_alert_time;
} g_alert_manager = {0};

// ============================================================================
// Внутренние функции
// ============================================================================

static int64_t get_current_time_ms(void) {
    struct timespec ts;
#ifdef _WIN32
    // Windows implementation
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return (int64_t)((uli.QuadPart - 116444736000000000ULL) / 10000);
#else
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

static alert_channel_config_t* find_channel(alert_channel_t type) {
    for (int i = 0; i < g_alert_manager.channel_count; i++) {
        if (g_alert_manager.channels[i].type == type) {
            return &g_alert_manager.channels[i];
        }
    }
    return NULL;
}

static int send_telegram_alert(const char *token, const char *chat_id, 
                                const alert_t *alert) {
    if (!token || !chat_id || !alert) return -1;
    
    // Формируем сообщение для Telegram
    char message[ALERT_MAX_MESSAGE_LEN + 256] = {0};
    const char *emoji = "";
    
    switch (alert->level) {
        case ALERT_LEVEL_CRITICAL: emoji = "🚨"; break;
        case ALERT_LEVEL_ERROR: emoji = "❌"; break;
        case ALERT_LEVEL_WARNING: emoji = "⚠️"; break;
        case ALERT_LEVEL_INFO: emoji = "ℹ️"; break;
        case ALERT_LEVEL_DEBUG: emoji = "📝"; break;
    }
    
    snprintf(message, sizeof(message),
             "%s *%s*\n\n"
             "*Уровень:* %s\n"
             "*Тип:* %s\n"
             "*Сервер:* %s\n"
             "*Время:* %ld\n\n"
             "%s",
             emoji,
             alert->title,
             alert_level_to_string(alert->level),
             alert_type_to_string(alert->type),
             alert->source,
             (long)alert->timestamp,
             alert->message);
    
    // HTTP запрос к Telegram Bot API
    char url[512] = {0};
    snprintf(url, sizeof(url), 
             "https://api.telegram.org/bot%s/sendMessage", token);
    
    // Тело запроса
    char body[2048] = {0};
    snprintf(body, sizeof(body),
             "{\"chat_id\":\"%s\",\"text\":\"%s\",\"parse_mode\":\"Markdown\"}",
             chat_id, message);
    
    // Отправка HTTP запроса (упрощённая реализация)
    // В production нужно использовать полноценный HTTP клиент
    kprintf("[ALERT] Telegram: Would send to %s: %s\n", chat_id, alert->title);
    
    return 0;
}

static int send_email_alert(const char *smtp_server, const char *to_email,
                            const alert_t *alert) {
    if (!smtp_server || !to_email || !alert) return -1;
    
    char subject[ALERT_MAX_TITLE_LEN + 32] = {0};
    snprintf(subject, sizeof(subject), "[%s] %s - %s",
             alert_level_to_string(alert->level),
             alert_type_to_string(alert->type),
             alert->title);
    
    kprintf("[ALERT] Email: Would send to %s via %s: %s\n", 
            to_email, smtp_server, subject);
    
    // В production: SMTP отправка через libcurl или собственную реализацию
    return 0;
}

static int send_slack_alert(const char *webhook_url, const alert_t *alert) {
    if (!webhook_url || !alert) return -1;
    
    // Цвет в зависимости от уровня
    const char *color = "good";
    const char *emoji = ":information_source:";
    
    switch (alert->level) {
        case ALERT_LEVEL_CRITICAL:
            color = "danger";
            emoji = ":rotating_light:";
            break;
        case ALERT_LEVEL_ERROR:
            color = "danger";
            emoji = ":x:";
            break;
        case ALERT_LEVEL_WARNING:
            color = "warning";
            emoji = ":warning:";
            break;
        case ALERT_LEVEL_INFO:
            color = "good";
            emoji = ":information_source:";
            break;
        default:
            break;
    }
    
    // Slack attachment
    char attachment[1500] = {0};
    snprintf(attachment, sizeof(attachment),
             "{\"attachments\":[{"
             "\"color\":\"%s\","
             "\"pretext\":\"%s %s\","
             "\"title\":\"%s\","
             "\"text\":\"%s\","
             "\"fields\":["
             "{\"title\":\"Level\",\"value\":\"%s\",\"short\":true},"
             "{\"title\":\"Type\",\"value\":\"%s\",\"short\":true},"
             "{\"title\":\"Server\",\"value\":\"%s\",\"short\":true},"
             "{\"title\":\"Time\",\"value\":\"%ld\",\"short\":true}"
             "]"
             "}]}",
             color,
             emoji,
             alert_type_to_string(alert->type),
             alert->title,
             alert->message,
             alert_level_to_string(alert->level),
             alert_type_to_string(alert->type),
             alert->source,
             (long)alert->timestamp);
    
    kprintf("[ALERT] Slack: Would send to webhook: %s\n", alert->title);
    
    // В production: HTTP POST к webhook_url
    return 0;
}

static int send_webhook_alert(const char *webhook_url, const alert_t *alert) {
    if (!webhook_url || !alert) return -1;
    
    // JSON payload
    char payload[1500] = {0};
    snprintf(payload, sizeof(payload),
             "{"
             "\"id\":%llu,"
             "\"level\":\"%s\","
             "\"type\":\"%s\","
             "\"title\":\"%s\","
             "\"message\":\"%s\","
             "\"source\":\"%s\","
             "\"timestamp\":%ld"
             "}",
             (unsigned long long)alert->id,
             alert_level_to_string(alert->level),
             alert_type_to_string(alert->type),
             alert->title,
             alert->message,
             alert->source,
             (long)alert->timestamp);
    
    kprintf("[ALERT] Webhook: Would send to %s: %s\n", webhook_url, alert->title);
    
    // В production: HTTP POST к webhook_url
    return 0;
}

static bool check_rate_limit(alert_channel_config_t *channel) {
    if (!channel) return false;
    
    int64_t now = get_current_time_ms();
    int64_t min_interval = channel->rate_limit_sec * 1000;
    
    if (min_interval > 0 && 
        (now - g_alert_manager.last_alert_time) < min_interval) {
        return false;  // Rate limit превышен
    }
    
    return true;
}

// ============================================================================
// Инициализация и очистка
// ============================================================================

int alert_manager_init(void) {
    if (g_alert_manager.initialized) {
        return 0;  // Уже инициализирован
    }
    
    memset(&g_alert_manager, 0, sizeof(g_alert_manager));
    g_alert_manager.initialized = true;
    g_alert_manager.alert_counter = 0;
    
    kprintf("[ALERT] Alert Manager initialized\n");
    return 0;
}

void alert_manager_cleanup(void) {
    if (!g_alert_manager.initialized) {
        return;
    }
    
    // Очистка каналов
    for (int i = 0; i < g_alert_manager.channel_count; i++) {
        // Очистка получателей
        g_alert_manager.channels[i].recipient_count = 0;
    }
    
    memset(&g_alert_manager, 0, sizeof(g_alert_manager));
    kprintf("[ALERT] Alert Manager cleaned up\n");
}

bool alert_manager_is_initialized(void) {
    return g_alert_manager.initialized;
}

// ============================================================================
// Управление каналами
// ============================================================================

int alert_manager_add_channel(alert_channel_t type, const char *name, int min_level) {
    if (!g_alert_manager.initialized) {
        return -1;
    }
    
    if (g_alert_manager.channel_count >= ALERT_MAX_CHANNELS) {
        kprintf("[ALERT] Error: Maximum channels reached\n");
        return -1;
    }
    
    // Проверяем, существует ли уже такой канал
    if (find_channel(type) != NULL) {
        kprintf("[ALERT] Error: Channel already exists\n");
        return -1;
    }
    
    alert_channel_config_t *channel = &g_alert_manager.channels[g_alert_manager.channel_count];
    memset(channel, 0, sizeof(alert_channel_config_t));
    
    channel->type = type;
    snprintf(channel->name, sizeof(channel->name), "%s", name ? name : "unnamed");
    channel->min_level = min_level;
    channel->enabled = true;
    channel->rate_limit_sec = 5;  // Default 5 секунд между алертами
    channel->aggregate = false;
    
    g_alert_manager.channel_count++;
    g_alert_manager.stats.active_channels++;
    
    kprintf("[ALERT] Channel added: %s (type=%d, min_level=%d)\n", 
            channel->name, type, min_level);
    
    return 0;
}

int alert_manager_remove_channel(alert_channel_t type) {
    if (!g_alert_manager.initialized) {
        return -1;
    }
    
    alert_channel_config_t *channel = find_channel(type);
    if (!channel) {
        return -1;
    }
    
    // Сдвиг каналов
    for (int i = 0; i < g_alert_manager.channel_count - 1; i++) {
        if (g_alert_manager.channels[i].type == type) {
            g_alert_manager.channels[i] = g_alert_manager.channels[i + 1];
        }
    }
    
    g_alert_manager.channel_count--;
    g_alert_manager.stats.active_channels--;
    
    kprintf("[ALERT] Channel removed: %d\n", type);
    return 0;
}

int alert_manager_set_channel_enabled(alert_channel_t type, bool enabled) {
    alert_channel_config_t *channel = find_channel(type);
    if (!channel) {
        return -1;
    }
    
    channel->enabled = enabled;
    kprintf("[ALERT] Channel %s %s\n", channel->name, enabled ? "enabled" : "disabled");
    return 0;
}

int alert_manager_set_channel_rate_limit(alert_channel_t type, int rate_limit_sec) {
    alert_channel_config_t *channel = find_channel(type);
    if (!channel) {
        return -1;
    }
    
    channel->rate_limit_sec = rate_limit_sec > 0 ? rate_limit_sec : 0;
    kprintf("[ALERT] Channel %s rate limit set to %d sec\n", 
            channel->name, rate_limit_sec);
    return 0;
}

// ============================================================================
// Управление получателями
// ============================================================================

int alert_manager_add_recipient(alert_channel_t type, const char *token, 
                                 const char *recipient) {
    if (!g_alert_manager.initialized) {
        return -1;
    }
    
    alert_channel_config_t *channel = find_channel(type);
    if (!channel) {
        kprintf("[ALERT] Error: Channel not found\n");
        return -1;
    }
    
    if (channel->recipient_count >= ALERT_MAX_RECIPIENTS_PER_CHANNEL) {
        kprintf("[ALERT] Error: Maximum recipients reached for channel\n");
        return -1;
    }
    
    alert_recipient_t *rcpt = &channel->recipients[channel->recipient_count];
    memset(rcpt, 0, sizeof(alert_recipient_t));
    
    snprintf(rcpt->token, sizeof(rcpt->token), "%s", token ? token : "");
    snprintf(rcpt->recipient, sizeof(rcpt->recipient), "%s", recipient ? recipient : "");
    rcpt->enabled = true;
    rcpt->created_at = get_current_time_ms();
    
    channel->recipient_count++;
    g_alert_manager.stats.active_recipients++;
    
    kprintf("[ALERT] Recipient added to channel %s: %s\n", 
            channel->name, recipient);
    
    return 0;
}

int alert_manager_remove_recipient(alert_channel_t type, const char *recipient) {
    if (!g_alert_manager.initialized || !recipient) {
        return -1;
    }
    
    alert_channel_config_t *channel = find_channel(type);
    if (!channel) {
        return -1;
    }
    
    // Поиск и удаление получателя
    for (int i = 0; i < channel->recipient_count; i++) {
        if (strcmp(channel->recipients[i].recipient, recipient) == 0) {
            // Сдвиг получателей
            for (int j = i; j < channel->recipient_count - 1; j++) {
                channel->recipients[j] = channel->recipients[j + 1];
            }
            channel->recipient_count--;
            g_alert_manager.stats.active_recipients--;
            
            kprintf("[ALERT] Recipient removed from channel %s: %s\n", 
                    channel->name, recipient);
            return 0;
        }
    }
    
    return -1;  // Получатель не найден
}

int alert_manager_set_custom_callback(alert_send_callback_t callback) {
    if (!g_alert_manager.initialized) {
        return -1;
    }
    
    g_alert_manager.custom_callback = callback;
    kprintf("[ALERT] Custom callback set\n");
    return 0;
}

// ============================================================================
// Отправка алертов
// ============================================================================

uint64_t alert_manager_send_alert(alert_level_t level, alert_type_t type, 
                                   const char *title, const char *message) {
    return alert_manager_send_alert_with_metadata(level, type, title, message, NULL);
}

uint64_t alert_manager_send_alert_with_metadata(alert_level_t level, alert_type_t type,
                                                 const char *title, const char *message,
                                                 const char *metadata) {
    if (!g_alert_manager.initialized || !title) {
        return (uint64_t)-1;
    }
    
    // Создаём алерт
    alert_t alert = {0};
    alert.id = ++g_alert_manager.alert_counter;
    alert.level = level;
    alert.type = type;
    snprintf(alert.title, sizeof(alert.title), "%s", title);
    snprintf(alert.message, sizeof(alert.message), "%s", message ? message : "");
    snprintf(alert.source, sizeof(alert.source), "mtproxy");
    alert.timestamp = get_current_time_ms();
    alert.status = ALERT_STATUS_PENDING;
    
    if (metadata) {
        snprintf(alert.metadata, sizeof(alert.metadata), "%s", metadata);
    }
    
    // Сохраняем в кольцевой буфер
    int idx = g_alert_manager.alert_index % 100;
    g_alert_manager.alerts[idx] = alert;
    g_alert_manager.alert_index++;
    
    // Обновляем статистику
    g_alert_manager.stats.total_alerts++;
    if (level >= 0 && level < 5) {
        g_alert_manager.stats.alerts_by_level[level]++;
    }
    if (type < 10) {
        g_alert_manager.stats.alerts_by_type[type]++;
    }
    g_alert_manager.stats.last_alert_time = alert.timestamp;
    
    // Отправка по каналам
    int sent_count = 0;
    
    for (int i = 0; i < g_alert_manager.channel_count; i++) {
        alert_channel_config_t *channel = &g_alert_manager.channels[i];
        
        // Проверки
        if (!channel->enabled) continue;
        if (level < channel->min_level) continue;
        if (!check_rate_limit(channel)) continue;
        
        // Отправка получателям
        for (int j = 0; j < channel->recipient_count; j++) {
            alert_recipient_t *rcpt = &channel->recipients[j];
            if (!rcpt->enabled) continue;
            
            int result = -1;
            
            switch (channel->type) {
                case ALERT_CHANNEL_TELEGRAM:
                    result = send_telegram_alert(rcpt->token, rcpt->recipient, &alert);
                    break;
                case ALERT_CHANNEL_EMAIL:
                    result = send_email_alert(rcpt->token, rcpt->recipient, &alert);
                    break;
                case ALERT_CHANNEL_SLACK:
                    result = send_slack_alert(rcpt->recipient, &alert);
                    break;
                case ALERT_CHANNEL_WEBHOOK:
                    result = send_webhook_alert(rcpt->recipient, &alert);
                    break;
                case ALERT_CHANNEL_CUSTOM:
                    if (g_alert_manager.custom_callback) {
                        result = g_alert_manager.custom_callback(channel->type, 
                                                                 rcpt->token, 
                                                                 rcpt->recipient, 
                                                                 &alert);
                    }
                    break;
            }
            
            if (result == 0) {
                sent_count++;
                rcpt->alerts_sent++;
            } else {
                rcpt->alerts_failed++;
                g_alert_manager.stats.alerts_failed++;
            }
        }
    }
    
    g_alert_manager.last_alert_time = get_current_time_ms();
    
    if (sent_count > 0) {
        alert.status = ALERT_STATUS_SENT;
        g_alert_manager.stats.alerts_sent++;
    } else {
        alert.status = ALERT_STATUS_FAILED;
    }
    
    // Обновляем статус в буфере
    idx = (g_alert_manager.alert_index - 1) % 100;
    g_alert_manager.alerts[idx].status = alert.status;
    g_alert_manager.alerts[idx].channels_sent = sent_count;
    
    kprintf("[ALERT] Alert #%llu sent: %s (%d channels)\n", 
            (unsigned long long)alert.id, title, sent_count);
    
    return alert.id;
}

int alert_manager_send_to_channel(alert_channel_t channel, const alert_t *alert) {
    if (!g_alert_manager.initialized || !alert) {
        return -1;
    }
    
    alert_channel_config_t *ch = find_channel(channel);
    if (!ch || !ch->enabled) {
        return -1;
    }
    
    int sent_count = 0;
    
    for (int i = 0; i < ch->recipient_count; i++) {
        alert_recipient_t *rcpt = &ch->recipients[i];
        if (!rcpt->enabled) continue;
        
        int result = -1;
        
        switch (channel) {
            case ALERT_CHANNEL_TELEGRAM:
                result = send_telegram_alert(rcpt->token, rcpt->recipient, alert);
                break;
            case ALERT_CHANNEL_EMAIL:
                result = send_email_alert(rcpt->token, rcpt->recipient, alert);
                break;
            case ALERT_CHANNEL_SLACK:
                result = send_slack_alert(rcpt->recipient, alert);
                break;
            case ALERT_CHANNEL_WEBHOOK:
                result = send_webhook_alert(rcpt->recipient, alert);
                break;
            case ALERT_CHANNEL_CUSTOM:
                if (g_alert_manager.custom_callback) {
                    result = g_alert_manager.custom_callback(channel, rcpt->token, 
                                                             rcpt->recipient, alert);
                }
                break;
        }
        
        if (result == 0) {
            sent_count++;
        }
    }
    
    return sent_count > 0 ? 0 : -1;
}

// ============================================================================
// Встроенные алерты
// ============================================================================

void alert_server_down(const char *server_name) {
    char title[128] = {0};
    char message[256] = {0};
    
    snprintf(title, sizeof(title), "Server Down: %s", server_name ? server_name : "unknown");
    snprintf(message, sizeof(message), "MTProxy server %s is not responding", 
             server_name ? server_name : "unknown");
    
    alert_manager_send_alert(ALERT_LEVEL_CRITICAL, ALERT_TYPE_SERVER_DOWN, title, message);
}

void alert_server_up(const char *server_name) {
    char title[128] = {0};
    char message[256] = {0};
    
    snprintf(title, sizeof(title), "Server Up: %s", server_name ? server_name : "unknown");
    snprintf(message, sizeof(message), "MTProxy server %s has started successfully",
             server_name ? server_name : "unknown");
    
    alert_manager_send_alert(ALERT_LEVEL_INFO, ALERT_TYPE_SERVER_UP, title, message);
}

void alert_high_cpu(double cpu_usage, double threshold) {
    char title[128] = {0};
    char message[256] = {0};
    
    snprintf(title, sizeof(title), "High CPU Usage: %.1f%%", cpu_usage);
    snprintf(message, sizeof(message), 
             "CPU usage %.1f%% exceeds threshold %.1f%%", cpu_usage, threshold);
    
    alert_manager_send_alert(ALERT_LEVEL_WARNING, ALERT_TYPE_HIGH_CPU, title, message);
}

void alert_high_memory(uint64_t memory_usage, uint64_t total_memory, double threshold) {
    char title[128] = {0};
    char message[256] = {0};
    double usage_percent = (total_memory > 0) ? 
                           ((double)memory_usage / total_memory * 100.0) : 0;
    
    snprintf(title, sizeof(title), "High Memory Usage: %.1f%%", usage_percent);
    snprintf(message, sizeof(message),
             "Memory usage %.1f%% (%llu/%llu bytes) exceeds threshold %.1f%%",
             usage_percent, (unsigned long long)memory_usage, 
             (unsigned long long)total_memory, threshold);
    
    alert_manager_send_alert(ALERT_LEVEL_WARNING, ALERT_TYPE_HIGH_MEMORY, title, message);
}

void alert_high_connections(uint64_t connections, uint64_t max_connections) {
    char title[128] = {0};
    char message[256] = {0};
    double usage_percent = (max_connections > 0) ? 
                           ((double)connections / max_connections * 100.0) : 0;
    
    snprintf(title, sizeof(title), "High Connections: %llu/%llu",
             (unsigned long long)connections, (unsigned long long)max_connections);
    snprintf(message, sizeof(message),
             "Active connections %llu (%.1f%% of max %llu)",
             (unsigned long long)connections, usage_percent,
             (unsigned long long)max_connections);
    
    alert_manager_send_alert(ALERT_LEVEL_WARNING, ALERT_TYPE_HIGH_CONNECTIONS, title, message);
}

void alert_rate_limit_exceeded(const char *ip, int requests, int limit) {
    char title[128] = {0};
    char message[256] = {0};
    
    snprintf(title, sizeof(title), "Rate Limit Exceeded: %s", ip ? ip : "unknown");
    snprintf(message, sizeof(message),
             "IP %s exceeded rate limit: %d requests (limit: %d)",
             ip ? ip : "unknown", requests, limit);
    
    alert_manager_send_alert(ALERT_LEVEL_WARNING, ALERT_TYPE_RATE_LIMIT, title, message);
}

void alert_security_event(const char *description) {
    char title[128] = {0};
    
    snprintf(title, sizeof(title), "Security Event: %s", description ? description : "unknown");
    
    alert_manager_send_alert(ALERT_LEVEL_ERROR, ALERT_TYPE_SECURITY, title, 
                             description ? description : "Security event detected");
}

// ============================================================================
// Статистика и мониторинг
// ============================================================================

int alert_manager_get_stats(alert_manager_stats_t *stats) {
    if (!g_alert_manager.initialized || !stats) {
        return -1;
    }
    
    memcpy(stats, &g_alert_manager.stats, sizeof(alert_manager_stats_t));
    return 0;
}

int alert_manager_get_stats_string(char *buffer, size_t buffer_size) {
    if (!g_alert_manager.initialized || !buffer || buffer_size < 256) {
        return -1;
    }
    
    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, 
                       "Alert Manager Statistics:\n");
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Total Alerts: %llu\n",
                       (unsigned long long)g_alert_manager.stats.total_alerts);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Sent: %llu, Failed: %llu\n",
                       (unsigned long long)g_alert_manager.stats.alerts_sent,
                       (unsigned long long)g_alert_manager.stats.alerts_failed);
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "  Active Channels: %d, Recipients: %d\n",
                       g_alert_manager.stats.active_channels,
                       g_alert_manager.stats.active_recipients);
    
    return 0;
}

void alert_manager_reset_stats(void) {
    if (!g_alert_manager.initialized) {
        return;
    }
    
    memset(&g_alert_manager.stats, 0, sizeof(g_alert_manager.stats));
    kprintf("[ALERT] Statistics reset\n");
}

const alert_t* alert_manager_get_last_alert(void) {
    if (!g_alert_manager.initialized || g_alert_manager.alert_index == 0) {
        return NULL;
    }
    
    int idx = (g_alert_manager.alert_index - 1) % 100;
    return &g_alert_manager.alerts[idx];
}

const alert_t* alert_manager_get_alert_by_id(uint64_t alert_id) {
    if (!g_alert_manager.initialized || alert_id == 0) {
        return NULL;
    }
    
    // Поиск в кольцевом буфере
    for (int i = 0; i < 100; i++) {
        if (g_alert_manager.alerts[i].id == alert_id) {
            return &g_alert_manager.alerts[i];
        }
    }
    
    return NULL;
}

// ============================================================================
// Утилиты
// ============================================================================

const char* alert_level_to_string(alert_level_t level) {
    switch (level) {
        case ALERT_LEVEL_DEBUG: return "DEBUG";
        case ALERT_LEVEL_INFO: return "INFO";
        case ALERT_LEVEL_WARNING: return "WARNING";
        case ALERT_LEVEL_ERROR: return "ERROR";
        case ALERT_LEVEL_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

const char* alert_type_to_string(alert_type_t type) {
    switch (type) {
        case ALERT_TYPE_SERVER_DOWN: return "ServerDown";
        case ALERT_TYPE_SERVER_UP: return "ServerUp";
        case ALERT_TYPE_HIGH_CPU: return "HighCPU";
        case ALERT_TYPE_HIGH_MEMORY: return "HighMemory";
        case ALERT_TYPE_HIGH_CONNECTIONS: return "HighConnections";
        case ALERT_TYPE_RATE_LIMIT: return "RateLimit";
        case ALERT_TYPE_SECURITY: return "Security";
        case ALERT_TYPE_CONFIG_CHANGED: return "ConfigChanged";
        case ALERT_TYPE_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

const char* alert_status_to_string(alert_status_t status) {
    switch (status) {
        case ALERT_STATUS_PENDING: return "Pending";
        case ALERT_STATUS_SENT: return "Sent";
        case ALERT_STATUS_FAILED: return "Failed";
        case ALERT_STATUS_ACKNOWLEDGED: return "Acknowledged";
        default: return "Unknown";
    }
}

alert_level_t alert_level_from_string(const char *str) {
    if (!str) return ALERT_LEVEL_INFO;
    
    if (strcmp(str, "DEBUG") == 0 || strcmp(str, "debug") == 0) 
        return ALERT_LEVEL_DEBUG;
    if (strcmp(str, "INFO") == 0 || strcmp(str, "info") == 0) 
        return ALERT_LEVEL_INFO;
    if (strcmp(str, "WARNING") == 0 || strcmp(str, "warn") == 0) 
        return ALERT_LEVEL_WARNING;
    if (strcmp(str, "ERROR") == 0 || strcmp(str, "error") == 0) 
        return ALERT_LEVEL_ERROR;
    if (strcmp(str, "CRITICAL") == 0 || strcmp(str, "critical") == 0) 
        return ALERT_LEVEL_CRITICAL;
    
    return ALERT_LEVEL_INFO;
}
