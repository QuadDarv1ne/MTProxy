/*
 * vlog.h - Structured logging implementation for MTProxy
 *
 * This module provides structured logging capabilities with JSON output,
 * rich metadata, and configurable log levels as described in the
 * STRUCTURED_LOGGING_IMPLEMENTATION.md documentation.
 */

#ifndef VLOG_H
#define VLOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

// Log level enumeration
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4
} log_level_t;

// Field types for structured logging
typedef enum {
    LOG_FIELD_STRING,
    LOG_FIELD_INT,
    LOG_FIELD_DOUBLE,
    LOG_FIELD_BOOL
} log_field_type_t;

// Log field structure
typedef struct {
    char *key;
    union {
        char *str_val;
        int int_val;
        double double_val;
        bool bool_val;
    } value;
    log_field_type_t type;
} log_field_t;

// Log entry structure
typedef struct {
    char *timestamp;          // ISO 8601 formatted timestamp
    log_level_t level;        // Log level
    char *component;          // Component name
    char *message;            // Log message
    log_field_t *fields;      // Structured metadata
    size_t field_count;       // Number of fields
} log_entry_t;

// Logger configuration
typedef struct {
    FILE *output_stream;
    log_level_t min_level;
    bool structured_format;   // true for JSON, false for text
    char *component_name;
    bool enabled;
} structured_logger_t;

// Initialize the logging system
int vlog_init(const char *component, log_level_t min_level, bool structured_format);

// Shutdown the logging system
void vlog_shutdown(void);

// Set the minimum log level
void vlog_set_level(log_level_t level);

// Set output stream (default is stderr)
void vlog_set_output(FILE *stream);

// Main logging function with structured fields
void vlog_with_fields(log_level_t level, const char *component, const char *message, 
                      log_field_t *fields, size_t field_count);

// Convenience functions for each log level
void vlog_debug(const char *component, const char *message, ...);
void vlog_info(const char *component, const char *message, ...);
void vlog_warn(const char *component, const char *message, ...);
void vlog_error(const char *component, const char *message, ...);
void vlog_fatal(const char *component, const char *message, ...);

// Helper functions to create log fields
log_field_t vlog_field_str(const char *key, const char *value);
log_field_t vlog_field_int(const char *key, int value);
log_field_t vlog_field_double(const char *key, double value);
log_field_t vlog_field_bool(const char *key, bool value);

// Format current timestamp in ISO 8601 format
char* vlog_timestamp(void);

// Convert log level to string
const char* vlog_level_to_string(log_level_t level);

// Check if logging is enabled for the given level
bool vlog_should_log(log_level_t level);

// Macro definitions for convenience
#define VLOG_DEBUG(comp, msg, ...) vlog_debug(comp, msg, ##__VA_ARGS__)
#define VLOG_INFO(comp, msg, ...)  vlog_info(comp, msg, ##__VA_ARGS__)
#define VLOG_WARN(comp, msg, ...)  vlog_warn(comp, msg, ##__VA_ARGS__)
#define VLOG_ERROR(comp, msg, ...) vlog_error(comp, msg, ##__VA_ARGS__)
#define VLOG_FATAL(comp, msg, ...) vlog_fatal(comp, msg, ##__VA_ARGS__)

#endif // VLOG_H