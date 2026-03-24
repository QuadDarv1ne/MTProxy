# MTProxy Tests

Комплексные тесты для MTProxy проекта.

## Запуск всех тестов

```bash
cd build/bin

# Модульные тесты
test-new-modules.exe        # 45 тестов (cache, rate-limiter, error-handler)
test-traffic-stats.exe      # 10 тестов (traffic statistics)
test-utils.exe              # utils тесты

# Integration тесты
integration-tests-simple.exe    # 27 тестов (integration modules)
test-ws-tunnel.exe              # 29 тестов (tg-ws-proxy integration)

# Performance тесты
cache-performance-test-simple.exe       # 252K ops, ~2-3M ops/sec
rate-limiter-highload-test-simple.exe   # 202K ops, ~17M ops/sec
```

## Статус тестов

| Тест | Файл | Статус | Результат |
|------|------|--------|-----------|
| **New Modules** | test-new-modules.exe | ✅ | 45/45 (100%) |
| **Traffic Stats** | test-traffic-stats.exe | ✅ | 10/10 (100%) |
| **Integration** | integration-tests-simple.exe | ✅ | 27/27 (100%) |
| **WS Tunnel** | test-ws-tunnel.exe | ✅ | 29/29 (100%) |
| **Cache Perf** | cache-performance-test-simple.exe | ✅ | 252K ops, 99.6% |
| **Rate Limiter** | rate-limiter-highload-test-simple.exe | ✅ | 202K ops, 99.1% |

**Итого:** 110+ тестов, 100% success

## Покрытие модулей

- ✅ config-manager
- ✅ cache-manager (LRU/LFU/FIFO/TTL)
- ✅ rate-limiter (Token Bucket)
- ✅ error-handler (Circuit Breaker)
- ✅ ws-tunnel (tg-ws-proxy)
- ✅ traffic-stats

## Производительность

| Модуль | Операций | Скорость |
|--------|----------|----------|
| Cache Put | 50,000 | ~2M ops/sec |
| Cache Get | 50,000 | ~3M ops/sec |
| Rate Limit Check | 100,000 | ~17M ops/sec |

---

*Последнее обновление: 24 марта 2026 г.*
