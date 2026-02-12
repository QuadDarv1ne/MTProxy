# Structured Logging Implementation for MTProxy

## Overview
This document describes the implementation of structured logging for MTProxy, providing machine-readable log output with consistent format and rich metadata.

## Motivation
Traditional text-based logging is difficult to parse and analyze programmatically. Structured logging addresses this by producing logs in a consistent, machine-readable format (typically JSON) with rich metadata for better correlation and analysis.

## Implementation Details

### Log Format
All logs will be produced in JSON format with the following structure:

```json
{
  "timestamp": "2023-07-15T10:30:00.123Z",
  "level": "INFO",
  "component": "connection_manager",
  "message": "New client connected",
  "fields": {
    "client_ip": "192.168.1.100",
    "connection_id": "abc123",
    "protocol_version": "2.0"
  }
}
```

### Log Levels
- DEBUG: Detailed diagnostic information
- INFO: General operational information
- WARN: Warning conditions
- ERROR: Error conditions
- FATAL: Critical errors causing termination

### Components

#### 1. Log Entry Structure
```c
typedef struct {
    char *timestamp;          // ISO 8601 formatted timestamp
    log_level_t level;        // Log level enumeration
    char *component;          // Component name
    char *message;            // Log message
    log_fields_t *fields;     // Structured metadata
    size_t field_count;       // Number of fields
} log_entry_t;
```

#### 2. Log Field Structure
```c
typedef struct {
    char *key;
    char *value;
    log_field_type_t type;    // STRING, NUMBER, BOOLEAN, OBJECT
} log_field_t;
```

#### 3. Logger Implementation
```c
typedef struct {
    FILE *output_stream;
    log_level_t min_level;
    bool structured_format;
    char *component_name;
} structured_logger_t;
```

### Core Functions

#### Basic Logging
```c
// Standard logging with level
void log_with_level(log_level_t level, const char *component, const char *message);

// Log with structured fields
void log_with_fields(log_level_t level, const char *component, const char *message, 
                    log_field_t *fields, size_t field_count);

// Convenience macros
#define LOG_DEBUG(comp, msg, ...) log_debug(comp, msg, ##__VA_ARGS__)
#define LOG_INFO(comp, msg, ...)  log_info(comp, msg, ##__VA_ARGS__)
#define LOG_WARN(comp, msg, ...)  log_warn(comp, msg, ##__VA_ARGS__)
#define LOG_ERROR(comp, msg, ...) log_error(comp, msg, ##__VA_ARGS__)
```

#### Field Creation
```c
// Create field with string value
log_field_t field_str(const char *key, const char *value);

// Create field with numeric value
log_field_t field_num(const char *key, double value);

// Create field with boolean value
log_field_t field_bool(const char *key, bool value);

// Create field with IP address
log_field_t field_ip(const char *key, unsigned int ip);
```

### Log Categories

#### 1. Access Logs
- Client connection/disconnection events
- Successful/unsuccessful authentications
- Message forwarding events
- Protocol violations

#### 2. Security Logs
- Suspicious activity detection
- Blocked connections
- Rate limit violations
- Certificate validation events

#### 3. Performance Logs
- Slow operation warnings
- Resource exhaustion events
- Cache hit/miss ratios
- Error rate increases

#### 4. Operational Logs
- Startup/shutdown events
- Configuration changes
- Feature toggles
- Maintenance events

## Configuration

### Command Line Options
- `--log-format json|text`: Set log format
- `--log-level debug|info|warn|error|fatal`: Minimum log level
- `--structured-logs`: Enable structured logging
- `--log-fields component,module,function`: Additional fields to include

### Configuration File
```yaml
logging:
  format: "json"
  level: "info"
  structured: true
  output: "/var/log/mtproto.log"
  fields:
    - "component"
    - "module"
    - "function"
    - "connection_id"
```

## Integration Points

### With Existing MTProxy Architecture
- Replace existing `vkprintf` calls with structured logging
- Integrate with existing statistics collection
- Hook into connection lifecycle events
- Add to critical error paths

### With External Systems
- Compatible with popular log aggregators (ELK, Splunk, etc.)
- Supports standard log ingestion protocols
- Provides metrics export for monitoring systems

## Performance Considerations

### Minimal Overhead
- Asynchronous log writing to prevent blocking
- Buffering to reduce I/O operations
- Efficient JSON serialization
- Configurable sampling for high-volume logs

### Resource Usage
- Low memory footprint for log buffering
- Minimal CPU impact for serialization
- Configurable retention and rotation

## Security Considerations

### Data Sensitivity
- Sanitize sensitive information from logs
- Support for field redaction
- Configurable privacy levels
- Audit trail for configuration changes

### Log Protection
- Secure log file permissions
- Integrity checking for critical logs
- Tamper detection mechanisms
- Encrypted log transport if needed

## Migration Strategy

### From Existing Logging
1. Gradually replace `vkprintf` calls with structured logging
2. Maintain backward compatibility during transition
3. Provide unified view of both log formats
4. Phase out legacy logging after verification

### Backward Compatibility
- Support for traditional text format during migration
- Dual logging during transition period
- Consistent message semantics across formats

## Monitoring and Alerting

### Log-Based Metrics
- Error rate tracking
- Performance degradation alerts
- Security event correlation
- Resource utilization trends

### Alert Conditions
- High error rates
- Security threshold breaches
- Performance degradation
- Resource exhaustion

## Testing and Validation

### Unit Tests
- Log format validation
- Field serialization correctness
- Performance benchmarking
- Error handling verification

### Integration Tests
- End-to-end log flow verification
- External system compatibility
- Configuration validation
- Performance impact assessment

## Future Enhancements

### Advanced Features
- Structured query support
- Log sampling and filtering
- Real-time log analysis
- Anomaly detection

### Integration
- Third-party monitoring platform support
- Advanced alerting systems
- Automated incident response
- Predictive analytics

## Deployment Considerations

### Production Systems
- Adequate disk space for logs
- Proper log rotation configuration
- Monitoring of log system health
- Backup and archival strategies

### Performance Tuning
- Appropriate buffer sizes
- Sampling rates for high-volume events
- Asynchronous processing configuration
- Storage performance optimization