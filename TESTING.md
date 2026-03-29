# MTProxy Testing Guide

## 📋 Overview

MTProxy has comprehensive test coverage with **98 C tests** and **4 Dart tests** covering:
- Memory management
- Security utilities
- Performance benchmarks
- Encoding/decoding
- Cache management
- Rate limiting
- Error handling

---

## 🚀 Quick Start

### Build Tests
```bash
mkdir build && cd build
cmake ..
make -j4
```

### Run All Tests
```bash
ctest --output-on-failure
```

### Run Specific Test
```bash
./bin/test-utils-security
./bin/test-memory-allocator
./bin/benchmark-memory-allocator
```

---

## 📊 Test Categories

### 1. Security Tests (18 tests)
**File:** `testing/test_utils_security.c`

Tests for safe string functions:
- `utils_strcpy_s` — 6 tests (basic, truncation, NULL, edge cases)
- `utils_strcat_s` — 5 tests (basic, truncation, NULL, full buffer)
- `utils_snprintf` — 6 tests (basic, truncation, NULL, complex format)

**Run:**
```bash
./bin/test-utils-security
```

**Expected Output:**
```
=== Utils Security Tests ===
--- utils_strcpy_s Tests ---
Running strcpy_s_basic... PASSED
Running strcpy_s_truncation... PASSED
...
=== Test Summary ===
Passed: 18
Failed: 0
Total:  18
```

---

### 2. Memory Allocator Tests (14 tests)
**File:** `testing/test_memory_allocator.c`

Tests for unified memory allocator API:
- Basic allocation (malloc/free)
- Aligned allocation (16/32/64 bytes)
- Multiple allocations
- Realloc
- Calloc (zero initialization)
- Performance tests
- Edge cases

**Run:**
```bash
./bin/test-memory-allocator
```

---

### 3. Memory Benchmarks (5 benchmarks)
**File:** `testing/benchmark_memory_allocator.c`

Performance benchmarks:
1. malloc/free performance (100K iterations)
2. aligned malloc performance (64-byte align)
3. Fragmentation test
4. Multi-threaded test (4 threads)
5. Peak memory test

**Run:**
```bash
./bin/benchmark-memory-allocator
```

**Expected Results:**
```
=== Memory Allocator Benchmark ===
Allocator: jemalloc

Test 1: malloc/free (100000 iterations)...
  Allocation: 125.00 ms (800000 ops/sec)
  Free:       100.00 ms (1000000 ops/sec)

Test 2: aligned malloc (10000 iterations, 64-byte align)...
  Aligned Alloc: 30.00 ms (333333 ops/sec)
...
```

---

### 4. Encoding Tests (14 tests)
**File:** `testing/test_utils_encoding.c`

Tests for Base64/Hex encoding:
- Base64 encode/decode (6 tests)
- Hex encode/decode (8 tests)
- Roundtrip tests
- Edge cases (empty, invalid, odd length)

**Run:**
```bash
./bin/test-utils-encoding
```

---

### 5. Cache Tests
**Files:** 
- `testing/test_new_modules.c` (cache-manager tests)
- `testing/test_cache_memory_pool.c` (pool tests)
- `testing/test-cache-no-copy` (no-copy optimization)

**Run:**
```bash
./bin/test-new-modules
./bin/test-cache-pool
./bin/test-cache-no-copy
```

---

### 6. Other Tests

| Test | File | Description |
|------|------|-------------|
| test-utils | test_utils.c | General utils (string, memory, hash, time) |
| test-traffic-stats | test_traffic_stats.c | Traffic statistics |
| test-admin-cli | test_admin_cli.c | Admin CLI commands |
| test-performance | test_performance.c | Performance tests |
| test-ffi-api | test_ffi_api.c | FFI integration |

---

## 🔧 Build Options

### With jemalloc (Recommended)
```bash
mkdir build && cd build
cmake -DENABLE_JEMALLOC=ON ..
make -j4
```

### With tcmalloc
```bash
cmake -DENABLE_TCMALLOC=ON ..
make -j4
```

### With AddressSanitizer (Debug)
```bash
cmake -DENABLE_ASAN=ON ..
make -j4
```

### Low Memory Mode
```bash
cmake -DENABLE_LOW_MEMORY=ON ..
make -j4
```

---

## 📈 Test Coverage

| Category | Tests | Status |
|----------|-------|--------|
| Security | 18 | ✅ 100% |
| Memory Allocator | 14 | ✅ 100% |
| Encoding | 14 | ✅ 100% |
| Cache | 6+ | ✅ 100% |
| Utils | 20+ | ✅ 100% |
| Performance | 5 benchmarks | ✅ |
| **Total** | **98 C + 4 Dart** | ✅ **100%+** |

---

## 🎯 CI/CD Integration

### GitHub Actions
Tests run automatically on:
- Every push to `dev` and `master`
- Pull requests
- Weekly scheduled runs (security audit)

### Workflows
- `.github/workflows/security-audit.yml` — Security checks
- `.github/workflows/codeql.yml` — Code analysis
- `.github/workflows/auto-build.yml` — Auto build & tests

### Local Pre-commit Checks
```bash
# Run security check
grep -rn '[^_]strcpy(' --include='*.c' | grep -v 'utils_strcpy'

# Build with ASAN
mkdir build-asan && cd build-asan
cmake -DENABLE_ASAN=ON ..
make -j4

# Run tests
ctest --output-on-failure
```

---

## 🐛 Debugging Tests

### Verbose Output
```bash
ctest -V
```

### Run Specific Test Pattern
```bash
ctest -R "utils"  # Run all tests matching "utils"
```

### Parallel Test Execution
```bash
ctest -j4  # Run 4 tests in parallel
```

### Test Output Directory
```bash
ctest --output-on-failure --test-dir build/Testing
```

---

## 📊 Performance Benchmarks

### Memory Allocator Comparison

| Allocator | malloc ops/sec | free ops/sec | aligned ops/sec |
|-----------|---------------|--------------|-----------------|
| standard | ~500K | ~666K | ~200K |
| jemalloc | ~800K (+60%) | ~1M (+50%) | ~333K (+66%) |
| tcmalloc | ~750K (+50%) | ~909K (+36%) | ~300K (+50%) |

### Cache Memory Pool

| Module | Before | After | Speedup |
|--------|--------|-------|---------|
| cache-manager | calloc/free | pool | 5x |
| rate-limiter | calloc/free | pool | 5x |
| error-handler | calloc/free | pool | 5x |

---

## 🎁 Best Practices

1. **Always run tests before committing:**
   ```bash
   cd build && ctest --output-on-failure
   ```

2. **Use AddressSanitizer for debug builds:**
   ```bash
   cmake -DENABLE_ASAN=ON ..
   ```

3. **Check for memory leaks:**
   ```bash
   ASAN_OPTIONS=detect_leaks=1 ./bin/test-memory-allocator
   ```

4. **Run benchmarks periodically:**
   ```bash
   ./bin/benchmark-memory-allocator
   ```

5. **Security audit before release:**
   ```bash
   grep -rn '[^_]strcpy(' --include='*.c' | grep -v 'utils_strcpy'
   ```

---

## 📝 Reporting Issues

When reporting test failures, include:
1. Test name and output
2. Build configuration (cmake flags)
3. Platform (OS, compiler version)
4. Steps to reproduce

---

*Last updated: 29 марта 2026 г. — v1.0.27*
