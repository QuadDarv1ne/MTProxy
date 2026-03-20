# FFI Integration Testing for MTProxy

## Overview

This document describes the FFI (Foreign Function Interface) integration testing for MTProxy shared library.

## Shared Library Build

### Build Options

```bash
# Build shared library with CMake
mkdir build && cd build
cmake .. -DBUILD_SHARED_LIB=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

### Output Files

- `lib/libmtproxy.so` - Shared library (Linux)
- `bin/mtproxy.dll` - Dynamic library (Windows)
- `include/mtproxy.h` - Public API header

## Public API

The FFI API is defined in `include/mtproxy.h` and provides:

### Core Functions

| Function | Description |
|----------|-------------|
| `mtproxy_init()` | Initialize MTProxy |
| `mtproxy_start()` | Start proxy server |
| `mtproxy_stop()` | Stop proxy server |
| `mtproxy_is_running()` | Check if running |

### Configuration Functions

| Function | Description |
|----------|-------------|
| `mtproxy_set_port(port)` | Set listening port |
| `mtproxy_add_secret(secret)` | Add secret key |
| `mtproxy_remove_secret(secret)` | Remove secret key |
| `mtproxy_clear_secrets()` | Clear all secrets |
| `mtproxy_set_max_connections(max)` | Set max connections |
| `mtproxy_set_ipv6(enable)` | Enable/disable IPv6 |
| `mtproxy_apply_config(config)` | Apply full config |

### Statistics Functions

| Function | Description |
|----------|-------------|
| `mtproxy_get_stats()` | Get current stats |
| `mtproxy_get_active_connections()` | Get active connections count |
| `mtproxy_get_total_connections()` | Get total connections count |
| `mtproxy_get_bytes_sent()` | Get bytes sent |
| `mtproxy_get_bytes_received()` | Get bytes received |
| `mtproxy_get_uptime()` | Get uptime in seconds |

### Utility Functions

| Function | Description |
|----------|-------------|
| `mtproxy_generate_secret(buffer, size)` | Generate random secret |
| `mtproxy_validate_secret(secret)` | Validate secret format |
| `mtproxy_get_version()` | Get library version |
| `mtproxy_get_last_error()` | Get last error message |

## Test Suite

### Test File: `testing/test_ffi_api.c`

Comprehensive test suite with 20+ tests:

```bash
# Build test executable
cmake .. -DBUILD_SHARED_LIB=ON
cmake --build . --parallel -t test-ffi-api

# Run tests
./bin/test-ffi-api
```

### Test Categories

1. **Initialization Tests**
   - `test_init` - Verify initialization
   - `test_version` - Check version string

2. **Configuration Tests**
   - `test_set_port` - Port configuration
   - `test_secrets` - Secret management
   - `test_validate_secret` - Secret validation
   - `test_clear_secrets` - Clear secrets
   - `test_max_connections` - Connection limits
   - `test_ipv6` - IPv6 configuration
   - `test_apply_config` - Full config application

3. **Statistics Tests**
   - `test_stats` - Get statistics
   - `test_connection_counters` - Connection counts
   - `test_traffic_counters` - Traffic counters
   - `test_uptime` - Uptime tracking

4. **Utility Tests**
   - `test_generate_secret` - Secret generation
   - `test_last_error` - Error reporting

5. **Lifecycle Tests**
   - `test_start_stop` - Start/stop lifecycle

## Flutter/Dart Integration

### FFI Bindings

Generated using `ffigen`:

```bash
cd mobile_app
flutter pub run ffigen --config ffigen.yaml
```

### Usage Example (Dart)

```dart
import 'dart:ffi';
import 'package:ffi/ffi.dart';
import 'mtproxy_bindings.dart';

void main() {
  // Load library
  final dylib = DynamicLibrary.open('libmtproxy.so');
  final mtproxy = MtProxyLib(dylib);
  
  // Initialize
  mtproxy.mtproxy_init();
  
  // Add secret
  final secret = '0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef'.toNativeUtf8();
  mtproxy.mtproxy_add_secret(secret.cast());
  
  // Start
  mtproxy.mtproxy_start();
  
  // Get stats
  final stats = mtproxy.mtproxy_get_stats();
  print('Active connections: ${stats.ref.active_connections}');
  
  // Stop
  mtproxy.mtproxy_stop();
}
```

## Known Issues

### Build Issues

Some modules have compilation issues that prevent full shared library build:

1. **distributed-monitor.c** - `time_t` type conflicts
2. **compression-optimizer.c** - Missing forward declarations
3. **Various headers** - Missing `#include <stddef.h>` for `size_t`

### Workaround

For FFI testing, use the static test executable `test-ffi-api` which links against `kdb_common` static library.

## Performance Testing

### Cache Performance

```c
// Test cache with 10000 operations
cache_manager_t* cache = cache_init(1000, 3600);

// Bulk insert
for (int i = 0; i < 10000; i++) {
    char key[64], value[64];
    snprintf(key, sizeof(key), "key_%d", i);
    snprintf(value, sizeof(value), "value_%d", i);
    cache_put(cache, key, value, strlen(value));
}

// Bulk read and measure hit rate
int hits = 0;
for (int i = 0; i < 10000; i++) {
    char key[64];
    snprintf(key, sizeof(key), "key_%d", i);
    if (cache_get(cache, key) != NULL) hits++;
}

printf("Hit rate: %.2f%%\n", (hits / 10000.0) * 100);
```

### Rate Limiter Performance

```c
// Test rate limiter with high load
rate_limiter_t* limiter = rate_limiter_init(100, 60);

// Simulate 1000 requests from 100 clients
for (int client_id = 0; client_id < 100; client_id++) {
    for (int req = 0; req < 10; req++) {
        bool allowed = rate_limiter_check(limiter, client_id);
        // Process or reject
    }
}
```

## Next Steps

1. Fix remaining compilation issues in system modules
2. Complete shared library build
3. Test FFI integration with Flutter mobile app
4. Add performance benchmarks
5. Document API usage patterns

---

*Last updated: 20 марта 2026 г.*
