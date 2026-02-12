# Health Check Implementation for MTProxy

## Overview
This document describes the implementation of health check endpoints for MTProxy, providing standardized interfaces for container orchestration and monitoring systems.

## Health Check Endpoints

### 1. Liveness Check
- **Endpoint**: `/api/health/live` or `/livez`
- **Purpose**: Determines if the process is alive and responding
- **Returns**: 200 if process is responsive, 5xx otherwise
- **Use Case**: Used by orchestrators to restart unresponsive services

### 2. Readiness Check
- **Endpoint**: `/api/health/ready` or `/readyz`
- **Purpose**: Determines if the service is ready to accept traffic
- **Returns**: 200 if ready, 5xx otherwise
- **Use Case**: Used by load balancers to remove/add instances from rotation

### 3. Startup Check
- **Endpoint**: `/api/health/startup` or `/startupz`
- **Purpose**: Determines if the service has completed initialization
- **Returns**: 200 if startup complete, 5xx otherwise
- **Use Case**: Used during deployment sequences

## Health Check Implementation

### Health Status Structure
```json
{
  "status": "healthy",
  "timestamp": "2023-07-15T10:30:00.123Z",
  "checks": {
    "memory": {"status": "healthy", "usage": "45%", "threshold": "80%"},
    "connections": {"status": "healthy", "available": 95, "total": 100, "threshold": 10},
    "disk": {"status": "healthy", "usage": "23%", "threshold": "90%"},
    "upstream": {"status": "healthy", "reachable": true},
    "queues": {"status": "healthy", "length": 2, "threshold": 100}
  }
}
```

### Health Check Categories

#### 1. Resource Checks
- **Memory Usage**: Check if memory consumption is below threshold
- **Disk Space**: Verify sufficient disk space availability
- **File Descriptors**: Ensure adequate file descriptor availability
- **Thread Count**: Monitor thread pool health

#### 2. Connectivity Checks
- **Upstream Services**: Verify connectivity to Telegram endpoints
- **DNS Resolution**: Test DNS resolution capabilities
- **Network Latency**: Measure network responsiveness

#### 3. Internal State Checks
- **Connection Pool**: Verify connection pool availability
- **Queue Lengths**: Monitor internal queue depths
- **Buffer Status**: Check buffer pool health

## Implementation Details

### Core Health Check Functions

1. `register_health_check()` - Add new health indicator
2. `get_health_status()` - Overall health assessment
3. `check_component_health()` - Individual component check
4. `serve_health_endpoint()` - HTTP handler for health endpoints

### Configuration Options

- `--enable-health-checks` - Enable health check endpoints
- `--health-port <port>` - Port for health check endpoints
- `--liveness-threshold <seconds>` - Threshold for liveness checks
- `--readiness-timeout <seconds>` - Timeout for readiness checks
- `--health-path-prefix <path>` - Path prefix for health endpoints

### Threshold Configuration

Each health check has configurable thresholds:

```c
typedef struct {
    const char *name;
    int (*checker_func)(void);
    double warning_threshold;
    double error_threshold;
    int enabled;
} health_check_config_t;
```

## Integration Points

### With Existing MTProxy Architecture
- Integrate with existing HTTP server functionality
- Hook into main event loop for periodic checks
- Use existing statistics collection infrastructure
- Leverage connection management system

### With Container Orchestration
- Kubernetes liveness/readiness probe compatibility
- Docker health check integration
- Service mesh health reporting

## Security Considerations

### Endpoint Protection
- Restrict health endpoints to localhost by default
- Support for authentication if exposed externally
- Rate limiting for health check endpoints
- Minimal information disclosure

### Privacy
- Avoid exposing sensitive system information
- Sanitize error messages
- Limit detailed diagnostic data

## Performance Impact

### Minimal Overhead
- Asynchronous check execution
- Cached results where appropriate
- Configurable check frequency
- Non-blocking operations

### Resource Usage
- Low memory footprint
- Minimal CPU impact
- Efficient I/O operations

## Monitoring and Logging

### Health Events
- Log health status changes
- Track check execution times
- Monitor error rates
- Alert on threshold breaches

### Metrics Integration
- Export health status as metrics
- Track check execution frequency
- Monitor response times
- Record error counts

## Deployment Scenarios

### Kubernetes
```
livenessProbe:
  httpGet:
    path: /livez
    port: 8080
  initialDelaySeconds: 30
  periodSeconds: 10

readinessProbe:
  httpGet:
    path: /readyz
    port: 8080
  initialDelaySeconds: 5
  periodSeconds: 5
```

### Docker
```
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
  CMD curl -f http://localhost:8080/readyz || exit 1
```

## Best Practices

### Configuration
- Set appropriate thresholds for your environment
- Configure different timeouts for different check types
- Monitor health check performance
- Regular review of health indicators

### Troubleshooting
- Detailed logging for failed checks
- Separate diagnostic endpoints if needed
- Clear error messages
- Correlation with system logs

## Future Enhancements

### Advanced Features
- Custom health check plugins
- Distributed health aggregation
- Predictive health scoring
- Automated remediation triggers

### Integration
- Third-party monitoring system integration
- Alert system connectivity
- Dashboard integration
- API for external health consumers