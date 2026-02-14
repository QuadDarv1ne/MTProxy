/*
 * vlog.c - Structured logging implementation for MTProxy
 *
 * This module provides structured logging capabilities with JSON output,
 * rich metadata, and configurable log levels as described in the
 * STRUCTURED_LOGGING_IMPLEMENTATION.md documentation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <ctype.h>

#include "vlog.h"
#include "precise-time.h"

// Global logger instance
static structured_logger_t g_logger = {0};

// Initialize the logging system
int vlog_init(const char *component, log_level_t min_level, bool structured_format) {
    g_logger.output_stream = stderr;
    g_logger.min_level = min_level;
    g_logger.structured_format = structured_format;
    
    if (component) {
        g_logger.component_name = strdup(component);
        if (!g_logger.component_name) {
            return -1; // Memory allocation failed
        }
    } else {
        g_logger.component_name = strdup("unknown");
        if (!g_logger.component_name) {
            return -1; // Memory allocation failed
        }
    }
    
    g_logger.enabled = true;
    return 0;
}

// Shutdown the logging system
void vlog_shutdown(void) {
    if (g_logger.component_name) {
        free(g_logger.component_name);
        g_logger.component_name = NULL;
    }
    g_logger.enabled = false;
}

// Set the minimum log level
void vlog_set_level(log_level_t level) {
    g_logger.min_level = level;
}

// Set output stream (default is stderr)
void vlog_set_output(FILE *stream) {
    if (stream) {
        g_logger.output_stream = stream;
    }
}

// Format current timestamp in ISO 8601 format
char* vlog_timestamp(void) {
    static char timestamp[32]; // YYYY-MM-DDTHH:MM:SS.mmmZ should fit
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    
    // Get milliseconds
    // Use standard time functions instead of precise_time for timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int ms = tv.tv_usec / 1000;
    
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec,
             ms);
             
    return timestamp;
}

// Convert log level to string
const char* vlog_level_to_string(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        default:              return "UNKNOWN";
    }
}

// Check if logging is enabled for the given level
bool vlog_should_log(log_level_t level) {
    return g_logger.enabled && level >= g_logger.min_level;
}

// Escape JSON string to handle special characters
static void escape_json_string(char *dest, const char *src, size_t dest_size) {
    size_t src_len = strlen(src);
    size_t dest_idx = 0;
    
    for (size_t i = 0; i < src_len && dest_idx < dest_size - 1; i++) {
        switch (src[i]) {
            case '"':
                if (dest_idx + 2 < dest_size) {
                    dest[dest_idx++] = '\\';
                    dest[dest_idx++] = '"';
                }
                break;
            case '\\':
                if (dest_idx + 2 < dest_size) {
                    dest[dest_idx++] = '\\';
                    dest[dest_idx++] = '\\';
                }
                break;
            case '\b':
                if (dest_idx + 2 < dest_size) {
                    dest[dest_idx++] = '\\';
                    dest[dest_idx++] = 'b';
                }
                break;
            case '\f':
                if (dest_idx + 2 < dest_size) {
                    dest[dest_idx++] = '\\';
                    dest[dest_idx++] = 'f';
                }
                break;
            case '\n':
                if (dest_idx + 2 < dest_size) {
                    dest[dest_idx++] = '\\';
                    dest[dest_idx++] = 'n';
                }
                break;
            case '\r':
                if (dest_idx + 2 < dest_size) {
                    dest[dest_idx++] = '\\';
                    dest[dest_idx++] = 'r';
                }
                break;
            case '\t':
                if (dest_idx + 2 < dest_size) {
                    dest[dest_idx++] = '\\';
                    dest[dest_idx++] = 't';
                }
                break;
            default:
                if ((unsigned char)src[i] >= 0x20) {  // Printable ASCII
                    dest[dest_idx++] = src[i];
                } else {  // Non-printable, output as \u00xx
                    if (dest_idx + 6 < dest_size) {
                        snprintf(dest + dest_idx, dest_size - dest_idx, "\\u%04x", (unsigned char)src[i]);
                        dest_idx += 6;
                    }
                }
                break;
        }
    }
    dest[dest_idx] = '\0';
}

// Print structured log entry in JSON format
static void print_structured_log(log_level_t level, const char *component, 
                                const char *message, log_field_t *fields, size_t field_count) {
    fprintf(g_logger.output_stream, "{");
    fprintf(g_logger.output_stream, "\"timestamp\":\"%s\",", vlog_timestamp());
    fprintf(g_logger.output_stream, "\"level\":\"%s\",", vlog_level_to_string(level));
    fprintf(g_logger.output_stream, "\"component\":\"%s\",", component);
    fprintf(g_logger.output_stream, "\"message\":\"%s\"", message);
    
    if (fields && field_count > 0) {
        fprintf(g_logger.output_stream, ",\"fields\":{");
        
        for (size_t i = 0; i < field_count; i++) {
            if (i > 0) fprintf(g_logger.output_stream, ",");
            
            // Print field key
            char escaped_key[256];
            escape_json_string(escaped_key, fields[i].key, sizeof(escaped_key));
            fprintf(g_logger.output_stream, "\"%s\":", escaped_key);
            
            // Print field value based on type
            switch (fields[i].type) {
                case LOG_FIELD_STRING:
                    if (fields[i].value.str_val) {
                        char escaped_value[512];
                        escape_json_string(escaped_value, fields[i].value.str_val, sizeof(escaped_value));
                        fprintf(g_logger.output_stream, "\"%s\"", escaped_value);
                    } else {
                        fprintf(g_logger.output_stream, "null");
                    }
                    break;
                case LOG_FIELD_INT:
                    fprintf(g_logger.output_stream, "%d", fields[i].value.int_val);
                    break;
                case LOG_FIELD_DOUBLE:
                    fprintf(g_logger.output_stream, "%.2f", fields[i].value.double_val);
                    break;
                case LOG_FIELD_BOOL:
                    fprintf(g_logger.output_stream, "%s", fields[i].value.bool_val ? "true" : "false");
                    break;
                default:
                    fprintf(g_logger.output_stream, "null");
                    break;
            }
        }
        fprintf(g_logger.output_stream, "}");
    }
    
    fprintf(g_logger.output_stream, "}\n");
    fflush(g_logger.output_stream);
}

// Print simple text log entry
static void print_simple_log(log_level_t level, const char *component, 
                            const char *message, log_field_t *fields, size_t field_count) {
    fprintf(g_logger.output_stream, "[%s][%s][%s] %s", 
            vlog_timestamp(), vlog_level_to_string(level), component, message);
    
    if (fields && field_count > 0) {
        fprintf(g_logger.output_stream, " |");
        for (size_t i = 0; i < field_count; i++) {
            fprintf(g_logger.output_stream, " %s=", fields[i].key);
            
            switch (fields[i].type) {
                case LOG_FIELD_STRING:
                    if (fields[i].value.str_val) {
                        fprintf(g_logger.output_stream, "%s", fields[i].value.str_val);
                    } else {
                        fprintf(g_logger.output_stream, "NULL");
                    }
                    break;
                case LOG_FIELD_INT:
                    fprintf(g_logger.output_stream, "%d", fields[i].value.int_val);
                    break;
                case LOG_FIELD_DOUBLE:
                    fprintf(g_logger.output_stream, "%.2f", fields[i].value.double_val);
                    break;
                case LOG_FIELD_BOOL:
                    fprintf(g_logger.output_stream, "%s", fields[i].value.bool_val ? "true" : "false");
                    break;
                default:
                    fprintf(g_logger.output_stream, "unknown");
                    break;
            }
        }
    }
    
    fprintf(g_logger.output_stream, "\n");
    fflush(g_logger.output_stream);
}

// Main logging function with structured fields
void vlog_with_fields(log_level_t level, const char *component, const char *message, 
                      log_field_t *fields, size_t field_count) {
    if (!g_logger.enabled || level < g_logger.min_level || !message) {
        return;
    }
    
    const char *comp = component ? component : g_logger.component_name;
    if (!comp) comp = "unknown";
    
    if (g_logger.structured_format) {
        print_structured_log(level, comp, message, fields, field_count);
    } else {
        print_simple_log(level, comp, message, fields, field_count);
    }
}

// Helper function to create log fields
log_field_t vlog_field_str(const char *key, const char *value) {
    log_field_t field;
    field.key = (char*)key;
    field.value.str_val = (char*)value;
    field.type = LOG_FIELD_STRING;
    return field;
}

log_field_t vlog_field_int(const char *key, int value) {
    log_field_t field;
    field.key = (char*)key;
    field.value.int_val = value;
    field.type = LOG_FIELD_INT;
    return field;
}

log_field_t vlog_field_double(const char *key, double value) {
    log_field_t field;
    field.key = (char*)key;
    field.value.double_val = value;
    field.type = LOG_FIELD_DOUBLE;
    return field;
}

log_field_t vlog_field_bool(const char *key, bool value) {
    log_field_t field;
    field.key = (char*)key;
    field.value.bool_val = value;
    field.type = LOG_FIELD_BOOL;
    return field;
}

// Internal function to handle variable arguments and format message
static void vlog_with_format(log_level_t level, const char *component, const char *format, va_list args) {
    if (!g_logger.enabled || level < g_logger.min_level || !format) {
        return;
    }
    
    char message[1024];
    vsnprintf(message, sizeof(message), format, args);
    
    vlog_with_fields(level, component, message, NULL, 0);
}

// Convenience functions for each log level
void vlog_debug(const char *component, const char *message, ...) {
    if (!vlog_should_log(LOG_LEVEL_DEBUG)) return;
    
    va_list args;
    va_start(args, message);
    vlog_with_format(LOG_LEVEL_DEBUG, component, message, args);
    va_end(args);
}

void vlog_info(const char *component, const char *message, ...) {
    if (!vlog_should_log(LOG_LEVEL_INFO)) return;
    
    va_list args;
    va_start(args, message);
    vlog_with_format(LOG_LEVEL_INFO, component, message, args);
    va_end(args);
}

void vlog_warn(const char *component, const char *message, ...) {
    if (!vlog_should_log(LOG_LEVEL_WARN)) return;
    
    va_list args;
    va_start(args, message);
    vlog_with_format(LOG_LEVEL_WARN, component, message, args);
    va_end(args);
}

void vlog_error(const char *component, const char *message, ...) {
    if (!vlog_should_log(LOG_LEVEL_ERROR)) return;
    
    va_list args;
    va_start(args, message);
    vlog_with_format(LOG_LEVEL_ERROR, component, message, args);
    va_end(args);
}

void vlog_fatal(const char *component, const char *message, ...) {
    if (!vlog_should_log(LOG_LEVEL_FATAL)) return;
    
    va_list args;
    va_start(args, message);
    vlog_with_format(LOG_LEVEL_FATAL, component, message, args);
    va_end(args);
}