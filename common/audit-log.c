/*
 * Audit Logging System Implementation
 * Реализация системы детального логирования событий
 */

#include "audit-log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <zlib.h>

/* ============================================================================
 * Вспомогательные функции
 * ============================================================================ */

static uint64_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void format_timestamp_iso(uint64_t timestamp_ms, char *buffer, size_t size) {
    time_t seconds = timestamp_ms / 1000;
    struct tm *tm_info = gmtime(&seconds);
    snprintf(buffer, size, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             (int)(timestamp_ms % 1000));
}

static void get_hostname(char *buffer, size_t size) {
    if (gethostname(buffer, size) != 0) {
        strncpy(buffer, "unknown", size - 1);
    }
    buffer[size - 1] = '\0';
}

static pthread_t get_current_thread_id(void) {
    return pthread_self();
}

static void get_thread_name(char *buffer, size_t size) {
#ifdef __linux__
    pthread_getname_np(pthread_self(), buffer, size);
#else
    snprintf(buffer, size, "thread-%lu", (unsigned long)pthread_self());
#endif
}

char *audit_generate_correlation_id(char *buffer, size_t size) {
    uint64_t timestamp = get_timestamp_ms();
    pid_t pid = getpid();
    pthread_t tid = pthread_self();
    uint32_t random = (uint32_t)(timestamp ^ (uint64_t)tid);
    
    snprintf(buffer, size, "%016lx-%04x-%08x",
             (unsigned long)timestamp,
             (unsigned)pid,
             random);
    
    return buffer;
}

/* ============================================================================
 * Строковые представления
 * ============================================================================ */

const char *audit_event_type_to_string(audit_event_type_t event_type) {
    static const char *event_strings[] = {
        "NONE",
        "LOGIN_SUCCESS", "LOGIN_FAILURE", "LOGOUT",
        "SESSION_CREATED", "SESSION_DESTROYED",
        "TOKEN_ISSUED", "TOKEN_REVOKED",
        "PERMISSION_GRANTED", "PERMISSION_DENIED",
        "ROLE_ASSIGNED", "ROLE_REVOKED",
        "RESOURCE_ACCESSED", "ACCESS_DENIED",
        "WHITELIST_ADDED", "BLACKLIST_ADDED",
        "SECURITY_VIOLATION", "RATE_LIMIT_EXCEEDED",
        "DDOS_ATTACK_DETECTED", "INVALID_SIGNATURE",
        "CERTIFICATE_ERROR", "TLS_HANDSHAKE",
        "CONFIG_MODIFIED", "CONFIG_RELOADED",
        "SECRET_ADDED", "SECRET_REMOVED",
        "ADMIN_COMMAND_EXECUTED", "USER_CREATED",
        "USER_DELETED", "API_KEY_CREATED", "API_KEY_REVOKED",
        "CLIENT_CONNECTED", "CLIENT_DISCONNECTED",
        "CONNECTION_TIMEOUT", "CONNECTION_LIMIT_REACHED",
        "TRAFFIC_THRESHOLD", "BANDWIDTH_LIMIT",
        "SYSTEM_START", "SYSTEM_STOP", "SYSTEM_RESTART",
        "HEALTH_CHECK", "CRITICAL_ERROR",
        "RECOVERY_ACTION", "PERFORMANCE_THRESHOLD",
        "RESOURCE_EXHAUSTION"
    };
    
    if (event_type >= 0 && event_type < sizeof(event_strings) / sizeof(event_strings[0])) {
        return event_strings[event_type];
    }
    return "UNKNOWN";
}

const char *audit_category_to_string(audit_category_t category) {
    static const char *category_strings[] = {
        "NONE", "AUTHENTICATION", "AUTHORIZATION", "ACCESS_CONTROL",
        "SECURITY_VIOLATION", "RATE_LIMITING", "DDOS_PROTECTION",
        "ENCRYPTION", "CONFIG_CHANGE", "ADMIN_ACTION",
        "CONNECTION", "DISCONNECTION", "TRAFFIC",
        "SYSTEM", "ERROR", "PERFORMANCE"
    };
    
    if (category >= 0 && category < sizeof(category_strings) / sizeof(category_strings[0])) {
        return category_strings[category];
    }
    return "UNKNOWN";
}

const char *audit_level_to_string(audit_level_t level) {
    static const char *level_strings[] = {
        "NONE", "CRITICAL", "SECURITY", "ACCESS", "ALL"
    };
    
    if (level >= 0 && level < sizeof(level_strings) / sizeof(level_strings[0])) {
        return level_strings[level];
    }
    return "UNKNOWN";
}

/* ============================================================================
 * JSON сериализация
 * ============================================================================ */

static void json_escape_string(const char *input, char *output, size_t output_size) {
    size_t j = 0;
    for (size_t i = 0; input[i] && j < output_size - 2; i++) {
        char c = input[i];
        if (c == '"' || c == '\\') {
            if (j < output_size - 3) {
                output[j++] = '\\';
                output[j++] = c;
            }
        } else if (c == '\n') {
            if (j < output_size - 3) {
                output[j++] = '\\';
                output[j++] = 'n';
            }
        } else if (c == '\r') {
            if (j < output_size - 3) {
                output[j++] = '\\';
                output[j++] = 'r';
            }
        } else if (c == '\t') {
            if (j < output_size - 3) {
                output[j++] = '\\';
                output[j++] = 't';
            }
        } else {
            output[j++] = c;
        }
    }
    output[j] = '\0';
}

size_t audit_event_to_json(const audit_event_t *event, char *buffer, size_t size) {
    char escaped_message[AUDIT_LOG_MAX_MESSAGE_LEN * 2];
    char escaped_context[AUDIT_LOG_MAX_CONTEXT_LEN * 2];
    
    json_escape_string(event->message, escaped_message, sizeof(escaped_message));
    json_escape_string(event->context.custom_data, escaped_context, sizeof(escaped_context));
    
    return snprintf(buffer, size,
        "{"
        "\"event_id\":%lu,"
        "\"event_type\":\"%s\","
        "\"category\":\"%s\","
        "\"level\":\"%s\","
        "\"timestamp\":%lu,"
        "\"timestamp_iso\":\"%s\","
        "\"hostname\":\"%s\","
        "\"process_name\":\"%s\","
        "\"process_id\":%d,"
        "\"thread_id\":%lu,"
        "\"message\":\"%s\","
        "\"correlation_id\":\"%s\","
        "\"user_id\":\"%s\","
        "\"ip_address\":\"%s\","
        "\"port\":%d,"
        "\"session_id\":\"%s\","
        "\"bytes_sent\":%lu,"
        "\"bytes_received\":%lu,"
        "\"duration_ms\":%u,"
        "\"result_code\":%d"
        "}",
        (unsigned long)event->event_id,
        audit_event_type_to_string(event->event_type),
        audit_category_to_string(event->category),
        audit_level_to_string(event->level),
        (unsigned long)event->timestamp,
        event->timestamp_iso,
        event->hostname,
        event->process_name,
        (int)event->process_id,
        (unsigned long)event->thread_id,
        escaped_message,
        event->context.correlation_id,
        event->context.user_id,
        event->context.ip_address,
        (int)event->context.port,
        event->context.session_id,
        (unsigned long)event->context.bytes_sent,
        (unsigned long)event->context.bytes_received,
        (unsigned)event->context.duration_ms,
        (int)event->context.result_code);
}

size_t audit_stats_to_json(const audit_stats_t *stats, char *buffer, size_t size) {
    return snprintf(buffer, size,
        "{"
        "\"total_events\":%lu,"
        "\"events_logged\":%lu,"
        "\"events_dropped\":%lu,"
        "\"events_failed\":%lu,"
        "\"bytes_written\":%lu,"
        "\"rotations_performed\":%lu,"
        "\"queue_size_current\":%lu,"
        "\"queue_size_max\":%lu,"
        "\"avg_latency_us\":%.2f,"
        "\"max_latency_us\":%.2f"
        "}",
        (unsigned long)stats->total_events,
        (unsigned long)stats->events_logged,
        (unsigned long)stats->events_dropped,
        (unsigned long)stats->events_failed,
        (unsigned long)stats->bytes_written,
        (unsigned long)stats->rotations_performed,
        (unsigned long)stats->queue_size_current,
        (unsigned long)stats->queue_size_max,
        stats->avg_latency_us,
        stats->max_latency_us);
}

/* ============================================================================
 * Основные функции
 * ============================================================================ */

static int audit_queue_push(audit_logger_t *logger, const audit_event_t *event) {
    if (logger->queue_count >= logger->config.queue_size) {
        logger->stats.events_dropped++;
        return -1;
    }
    
    logger->queue[logger->queue_tail] = *event;
    logger->queue_tail = (logger->queue_tail + 1) % logger->config.queue_size;
    logger->queue_count++;
    
    if (logger->queue_count > logger->stats.queue_size_max) {
        logger->stats.queue_size_max = logger->queue_count;
    }
    
    return 0;
}

static int audit_queue_pop(audit_logger_t *logger, audit_event_t *event) {
    if (logger->queue_count == 0) {
        return -1;
    }
    
    *event = logger->queue[logger->queue_head];
    logger->queue_head = (logger->queue_head + 1) % logger->config.queue_size;
    logger->queue_count--;
    
    return 0;
}

static int audit_write_event(audit_logger_t *logger, const audit_event_t *event) {
    if (!logger->current_log) {
        return -1;
    }
    
    uint64_t start_time = get_timestamp_ms();
    
    if (logger->config.json_format) {
        size_t json_len = audit_event_to_json(event, logger->format_buffer, 
                                               sizeof(logger->format_buffer));
        fprintf(logger->current_log, "%s\n", logger->format_buffer);
        logger->stats.bytes_written += json_len + 1;
    } else {
        int len = snprintf(logger->format_buffer, sizeof(logger->format_buffer),
                          "[%s] [%s] [%s] %s (corr_id=%s, user=%s, ip=%s)\n",
                          event->timestamp_iso,
                          audit_level_to_string(event->level),
                          audit_category_to_string(event->category),
                          event->message,
                          event->context.correlation_id,
                          event->context.user_id,
                          event->context.ip_address);
        fprintf(logger->current_log, "%s", logger->format_buffer);
        logger->stats.bytes_written += len;
    }
    
    if (logger->config.sync_to_disk) {
        fflush(logger->current_log);
    }
    
    uint64_t latency = get_timestamp_ms() - start_time;
    if (latency > logger->stats.max_latency_us) {
        logger->stats.max_latency_us = latency * 1000; // convert to microseconds
    }
    
    // Simple moving average for latency
    static double avg_latency = 0;
    static uint64_t latency_count = 0;
    latency_count++;
    avg_latency = avg_latency + (latency * 1000.0 - avg_latency) / latency_count;
    logger->stats.avg_latency_us = avg_latency;
    
    return 0;
}

static int audit_rotate_logs(audit_logger_t *logger) {
    if (logger->current_log) {
        fclose(logger->current_log);
        logger->current_log = NULL;
    }
    
    // Rotation logic: file.1, file.2, ...
    for (int i = logger->config.rotation_count - 1; i >= 1; i--) {
        char old_path[AUDIT_LOG_MAX_PATH_LEN];
        char new_path[AUDIT_LOG_MAX_PATH_LEN];
        
        if (i == 1) {
            snprintf(old_path, sizeof(old_path), "%s", logger->config.log_path);
        } else {
            snprintf(old_path, sizeof(old_path), "%s.%d", logger->config.log_path, i - 1);
        }
        
        snprintf(new_path, sizeof(new_path), "%s.%d", logger->config.log_path, i);
        
        // Compress if enabled
        if (logger->config.compress_rotated && i == 1) {
            char gz_path[AUDIT_LOG_MAX_PATH_LEN];
            snprintf(gz_path, sizeof(gz_path), "%s.gz", new_path);
            // Compression logic would go here
        }
        
        rename(old_path, new_path);
    }
    
    // Open new file
    logger->current_log = fopen(logger->config.log_path, "a");
    if (!logger->current_log) {
        return -1;
    }
    
    logger->current_size = 0;
    logger->stats.rotations_performed++;
    
    return 0;
}

static void *audit_writer_thread(void *arg) {
    audit_logger_t *logger = (audit_logger_t *)arg;
    audit_event_t event;
    uint64_t last_flush = get_timestamp_ms();
    
    while (logger->running) {
        pthread_mutex_lock(&logger->mutex);
        
        // Wait for events or timeout
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 100000000; // 100ms timeout
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        
        pthread_cond_timedwait(&logger->cond, &logger->mutex, &ts);
        
        // Process events
        while (audit_queue_pop(logger, &event) == 0) {
            pthread_mutex_unlock(&logger->mutex);
            
            // Check rotation
            if (logger->current_size >= logger->config.rotation_size_bytes) {
                audit_rotate_logs(logger);
            }
            
            // Write event
            if (audit_write_event(logger, &event) == 0) {
                logger->stats.events_logged++;
                
                // Update current size
                struct stat st;
                if (fstat(fileno(logger->current_log), &st) == 0) {
                    logger->current_size = st.st_size;
                }
            } else {
                logger->stats.events_failed++;
            }
            
            // Callback
            if (logger->on_event) {
                logger->on_event(&event, logger->callback_user_data);
            }
            
            pthread_mutex_lock(&logger->mutex);
        }
        
        // Periodic flush
        uint64_t now = get_timestamp_ms();
        if (now - last_flush >= logger->config.flush_interval_ms) {
            if (logger->current_log) {
                fflush(logger->current_log);
            }
            last_flush = now;
        }
        
        pthread_mutex_unlock(&logger->mutex);
    }
    
    return NULL;
}

int audit_logger_init(audit_logger_t *logger, const audit_config_t *config) {
    if (!logger || !config) {
        return -1;
    }
    
    memset(logger, 0, sizeof(audit_logger_t));
    logger->config = *config;
    
    // Validate configuration
    if (logger->config.queue_size == 0) {
        logger->config.queue_size = AUDIT_LOG_DEFAULT_QUEUE_SIZE;
    }
    if (logger->config.rotation_size_bytes == 0) {
        logger->config.rotation_size_bytes = AUDIT_LOG_DEFAULT_ROTATION_SIZE_MB * 1024 * 1024;
    }
    if (logger->config.rotation_count == 0) {
        logger->config.rotation_count = AUDIT_LOG_DEFAULT_ROTATION_COUNT;
    }
    if (logger->config.flush_interval_ms == 0) {
        logger->config.flush_interval_ms = 1000;
    }
    
    // Allocate queue
    logger->queue = calloc(logger->config.queue_size, sizeof(audit_event_t));
    if (!logger->queue) {
        return -1;
    }
    
    // Initialize synchronization primitives
    pthread_mutex_init(&logger->mutex, NULL);
    pthread_cond_init(&logger->cond, NULL);
    
    logger->initialized = true;
    return 0;
}

int audit_logger_start(audit_logger_t *logger) {
    if (!logger || !logger->initialized || logger->running) {
        return -1;
    }
    
    // Create log directory if needed
    char *dir = strdup(logger->config.log_path);
    char *last_slash = strrchr(dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir(dir, 0755);
    }
    free(dir);
    
    // Open log file
    logger->current_log = fopen(logger->config.log_path, "a");
    if (!logger->current_log) {
        return -1;
    }
    
    // Get initial file size
    struct stat st;
    if (fstat(fileno(logger->current_log), &st) == 0) {
        logger->current_size = st.st_size;
    }
    
    strncpy(logger->current_path, logger->config.log_path, sizeof(logger->current_path) - 1);
    
    // Start writer thread
    logger->running = true;
    if (pthread_create(&logger->writer_thread, NULL, audit_writer_thread, logger) != 0) {
        logger->running = false;
        fclose(logger->current_log);
        return -1;
    }
    
    // Log system start
    audit_log_event_fmt(logger, AUDIT_EVENT_SYSTEM_START, AUDIT_CATEGORY_SYSTEM,
                       AUDIT_LEVEL_INFO, "Audit logging system started (version %s)",
                       AUDIT_LOG_VERSION);
    
    return 0;
}

void audit_logger_stop(audit_logger_t *logger) {
    if (!logger || !logger->running) {
        return;
    }
    
    logger->running = false;
    pthread_cond_signal(&logger->cond);
    
    // Wait for writer thread
    pthread_join(logger->writer_thread, NULL);
    
    // Flush and close
    if (logger->current_log) {
        fflush(logger->current_log);
        fclose(logger->current_log);
        logger->current_log = NULL;
    }
}

void audit_logger_cleanup(audit_logger_t *logger) {
    if (!logger) {
        return;
    }
    
    if (logger->running) {
        audit_logger_stop(logger);
    }
    
    if (logger->queue) {
        free(logger->queue);
        logger->queue = NULL;
    }
    
    pthread_mutex_destroy(&logger->mutex);
    pthread_cond_destroy(&logger->cond);
    
    logger->initialized = false;
}

int audit_log_event(audit_logger_t *logger,
                   audit_event_type_t event_type,
                   audit_category_t category,
                   audit_level_t level,
                   const char *message,
                   const audit_context_t *context) {
    if (!logger || !logger->running || !message) {
        return -1;
    }
    
    // Check if category and level are enabled
    if (!audit_is_category_enabled(logger, category) ||
        !audit_is_level_enabled(logger, level)) {
        return 0; // Not an error, just skipped
    }
    
    audit_event_t event = {0};
    
    // Fill event data
    event.event_id = ++logger->event_counter;
    event.event_type = event_type;
    event.category = category;
    event.level = level;
    event.timestamp = get_timestamp_ms();
    format_timestamp_iso(event.timestamp, event.timestamp_iso, sizeof(event.timestamp_iso));
    get_hostname(event.hostname, sizeof(event.hostname));
    snprintf(event.process_name, sizeof(event.process_name), "mtproto-proxy");
    event.process_id = getpid();
    get_thread_name(event.thread_name, sizeof(event.thread_name));
    event.thread_id = get_current_thread_id();
    strncpy(event.message, message, sizeof(event.message) - 1);
    
    // Context
    if (context) {
        event.context = *context;
    } else {
        // Generate default correlation ID
        audit_generate_correlation_id(event.context.correlation_id, 
                                     sizeof(event.context.correlation_id));
    }
    
    logger->stats.total_events++;
    
    // Update statistics by category and level
    if (category >= 0 && category < 16) {
        logger->stats.events_by_category[category]++;
    }
    if (level >= 0 && level <= 4) {
        logger->stats.events_by_level[level]++;
    }
    
    // Push to queue
    pthread_mutex_lock(&logger->mutex);
    int result = audit_queue_push(logger, &event);
    pthread_cond_signal(&logger->cond);
    pthread_mutex_unlock(&logger->mutex);
    
    return result;
}

int audit_log_event_fmt(audit_logger_t *logger,
                       audit_event_type_t event_type,
                       audit_category_t category,
                       audit_level_t level,
                       const char *format, ...) {
    if (!logger || !format) {
        return -1;
    }
    
    char message[AUDIT_LOG_MAX_MESSAGE_LEN];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    return audit_log_event(logger, event_type, category, level, message, NULL);
}

int audit_log_critical(audit_logger_t *logger,
                      audit_event_type_t event_type,
                      const char *message) {
    return audit_log_event(logger, event_type, AUDIT_CATEGORY_ERROR, 
                          AUDIT_LEVEL_CRITICAL, message, NULL);
}

int audit_log_security(audit_logger_t *logger,
                      audit_event_type_t event_type,
                      const char *ip_address,
                      const char *user_id,
                      const char *message) {
    audit_context_t context = {0};
    if (ip_address) {
        strncpy(context.ip_address, ip_address, sizeof(context.ip_address) - 1);
    }
    if (user_id) {
        strncpy(context.user_id, user_id, sizeof(context.user_id) - 1);
    }
    
    return audit_log_event(logger, event_type, AUDIT_CATEGORY_SECURITY_VIOLATION,
                          AUDIT_LEVEL_SECURITY, message, &context);
}

int audit_log_access(audit_logger_t *logger,
                    audit_event_type_t event_type,
                    const char *resource,
                    int allowed,
                    const char *reason) {
    audit_context_t context = {0};
    snprintf(context.custom_data, sizeof(context.custom_data),
            "resource=%s,allowed=%d,reason=%s",
            resource ? resource : "unknown",
            allowed,
            reason ? reason : "none");
    
    return audit_log_event(logger, event_type, AUDIT_CATEGORY_ACCESS_CONTROL,
                          AUDIT_LEVEL_ACCESS, 
                          allowed ? "Access granted" : "Access denied",
                          &context);
}

int audit_log_config_change(audit_logger_t *logger,
                           const char *param,
                           const char *old_value,
                           const char *new_value,
                           const char *admin_id) {
    audit_context_t context = {0};
    snprintf(context.custom_data, sizeof(context.custom_data),
            "param=%s,old=%s,new=%s,admin=%s",
            param ? param : "unknown",
            old_value ? old_value : "none",
            new_value ? new_value : "none",
            admin_id ? admin_id : "unknown");
    
    if (admin_id) {
        strncpy(context.user_id, admin_id, sizeof(context.user_id) - 1);
    }
    
    return audit_log_event(logger, AUDIT_EVENT_CONFIG_MODIFIED, 
                          AUDIT_CATEGORY_CONFIG_CHANGE, AUDIT_LEVEL_ACCESS,
                          "Configuration changed", &context);
}

int audit_log_client_connect(audit_logger_t *logger,
                            int client_fd,
                            const char *ip_address,
                            uint16_t port,
                            const char *secret_id) {
    audit_context_t context = {0};
    strncpy(context.ip_address, ip_address, sizeof(context.ip_address) - 1);
    context.port = port;
    snprintf(context.custom_data, sizeof(context.custom_data),
            "fd=%d,secret=%s", client_fd, secret_id ? secret_id : "unknown");
    
    return audit_log_event(logger, AUDIT_EVENT_CLIENT_CONNECTED,
                          AUDIT_CATEGORY_CONNECTION, AUDIT_LEVEL_ACCESS,
                          "Client connected", &context);
}

int audit_log_client_disconnect(audit_logger_t *logger,
                               int client_fd,
                               const char *ip_address,
                               const char *reason,
                               uint32_t duration_ms,
                               uint64_t bytes_sent,
                               uint64_t bytes_received) {
    audit_context_t context = {0};
    strncpy(context.ip_address, ip_address, sizeof(context.ip_address) - 1);
    context.duration_ms = duration_ms;
    context.bytes_sent = bytes_sent;
    context.bytes_received = bytes_received;
    snprintf(context.custom_data, sizeof(context.custom_data),
            "fd=%d,reason=%s,duration=%ums,sent=%lu,recv=%lu",
            client_fd, reason ? reason : "unknown",
            (unsigned)duration_ms,
            (unsigned long)bytes_sent,
            (unsigned long)bytes_received);
    
    return audit_log_event(logger, AUDIT_EVENT_CLIENT_DISCONNECTED,
                          AUDIT_CATEGORY_DISCONNECTION, AUDIT_LEVEL_ACCESS,
                          "Client disconnected", &context);
}

void audit_logger_get_stats(audit_logger_t *logger, audit_stats_t *stats) {
    if (!logger || !stats) {
        return;
    }
    
    pthread_mutex_lock(&logger->mutex);
    *stats = logger->stats;
    stats->queue_size_current = logger->queue_count;
    pthread_mutex_unlock(&logger->mutex);
}

void audit_logger_reset_stats(audit_logger_t *logger) {
    if (!logger) {
        return;
    }
    
    pthread_mutex_lock(&logger->mutex);
    memset(&logger->stats, 0, sizeof(audit_stats_t));
    pthread_mutex_unlock(&logger->mutex);
}

int audit_logger_flush(audit_logger_t *logger) {
    if (!logger || !logger->current_log) {
        return -1;
    }
    
    pthread_mutex_lock(&logger->mutex);
    fflush(logger->current_log);
    pthread_mutex_unlock(&logger->mutex);
    
    return 0;
}

int audit_logger_rotate(audit_logger_t *logger) {
    if (!logger) {
        return -1;
    }
    
    pthread_mutex_lock(&logger->mutex);
    int result = audit_rotate_logs(logger);
    pthread_mutex_unlock(&logger->mutex);
    
    return result;
}

void audit_logger_set_callback(audit_logger_t *logger,
                              audit_event_callback_t callback,
                              void *user_data) {
    if (!logger) {
        return;
    }
    
    pthread_mutex_lock(&logger->mutex);
    logger->on_event = callback;
    logger->callback_user_data = user_data;
    pthread_mutex_unlock(&logger->mutex);
}

int audit_is_category_enabled(audit_logger_t *logger, audit_category_t category) {
    if (!logger || !logger->config.enabled) {
        return 0;
    }
    
    // Simple level-based check
    return category <= logger->config.level;
}

int audit_is_level_enabled(audit_logger_t *logger, audit_level_t level) {
    if (!logger || !logger->config.enabled) {
        return 0;
    }
    
    return level <= logger->config.level;
}
