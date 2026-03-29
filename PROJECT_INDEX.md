# MTProxy Project Index

Добро пожаловать в MTProxy! Этот документ поможет вам быстро найти нужную информацию.

---

## 🚀 Быстрый старт

### Сборка
```bash
mkdir build && cd build
cmake -DENABLE_JEMALLOC=ON ..  # Рекомендуется для production
make -j4
```

### Запуск
```bash
./bin/mtproto-proxy -S <secret> -p 8888
```

### Тесты
```bash
ctest --output-on-failure
```

---

## 📚 Документация

### Основная документация
| Документ | Описание |
|----------|----------|
| [README.md](README.md) | Основная информация о проекте |
| [QUICKSTART.md](QUICKSTART.md) | Быстрый старт |
| [TESTING.md](TESTING.md) | Руководство по тестированию |
| [BENCHMARKS.md](BENCHMARKS.md) | Бенчмарки производительности |
| [CHANGELOG.md](CHANGELOG.md) | История изменений |
| [RELEASE_NOTES_v1.0.27.md](RELEASE_NOTES_v1.0.27.md) | Заметки к релизу v1.0.27 |

### Руководства
| Документ | Описание |
|----------|----------|
| [DEPLOYMENT.md](DEPLOYMENT.md) | Развёртывание в production |
| [API_REFERENCE.md](API_REFERENCE.md) | Справочник по API |
| [ADMIN_CLI_GUIDE.md](ADMIN_CLI_GUIDE.md) | Руководство по admin-cli |
| [PERFORMANCE_TUNING.md](docs/PERFORMANCE_TUNING.md) | Оптимизация производительности |
| [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) | Диагностика проблем |
| [DEBUGGING.md](docs/DEBUGGING.md) | Отладка |

### Документация по улучшениям
| Документ | Описание |
|----------|----------|
| [IMPROVEMENTS_MARCH_2026.md](IMPROVEMENTS_MARCH_2026.md) | Улучшения марта 2026 |
| [MEMORY_FIXES.md](MEMORY_FIXES.md) | Исправления памяти |
| [FIXES_SUMMARY.md](FIXES_SUMMARY.md) | Сводка исправлений |

### Архитектура
| Документ | Описание |
|----------|----------|
| [ROADMAP.md](ROADMAP.md) | Дорожная карта проекта |
| [todo.md](todo.md) | Текущие задачи |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Вклад в проект |
| [SECURITY.md](SECURITY.md) | Безопасность |

---

## 🔧 Сборка

### Опции CMake
```bash
# Стандартная сборка
cmake ..

# С jemalloc (рекомендуется)
cmake -DENABLE_JEMALLOC=ON ..

# С tcmalloc
cmake -DENABLE_TCMALLOC=ON ..

# С AddressSanitizer (debug)
cmake -DENABLE_ASAN=ON ..

# Low memory mode
cmake -DENABLE_LOW_MEMORY=ON ..

# С HTTP/3 QUIC
cmake -DENABLE_HTTP3=ON ..
```

### Платформы
| Платформа | Статус | Примечание |
|-----------|--------|------------|
| **Linux** | ✅ Полная | LTO включено |
| **Windows** | ✅ Single-worker | fork() не поддерживается |
| **macOS** | ✅ CI/CD готов | Требуется тестирование |
| **Android** | ✅ Shared library | FFI готово |
| **iOS** | ✅ Static library | FFI готово |

---

## 📊 Тестирование

### Категории тестов
| Категория | Тестов | Файл |
|-----------|--------|------|
| Security | 18 | test_utils_security.c |
| Memory Allocator | 14 | test_memory_allocator.c |
| Encoding (Base64/Hex) | 14 | test_utils_encoding.c |
| Cache | 6+ | test_new_modules.c |
| Utils | 20+ | test_utils.c |
| Performance | 5 benchmarks | benchmark_memory_allocator.c |
| **Всего** | **98 C + 4 Dart** | |

### Запуск тестов
```bash
# Все тесты
ctest --output-on-failure

# Конкретный тест
./bin/test-utils-security

# Бенчмарки
./bin/benchmark-memory-allocator
```

---

## 📈 Производительность

### Оптимизации v1.0.27
| Модуль | Улучшение | Эффект |
|--------|-----------|--------|
| Cache Memory Pool | 5x | Аллокации |
| jemalloc | +60% | ops/sec |
| tcmalloc | +50% | ops/sec |
| Network Analyzer | ~80% | Снижение CPU |

### Бенчмарки
См. [BENCHMARKS.md](BENCHMARKS.md) для подробной информации.

---

## 🔒 Безопасность

### Security Features
- ✅ Безопасные строковые функции (utils_strcpy_s, utils_strcat_s, utils_snprintf)
- ✅ Security audit CI workflow
- ✅ AddressSanitizer для debug сборок
- ✅ Статический анализ (cppcheck)
- ✅ CodeQL security analysis

### Security Audit
```bash
# Проверка на unsafe функции
grep -rn '[^_]strcpy(' --include='*.c' | grep -v 'utils_strcpy'

# Build с ASAN
cmake -DENABLE_ASAN=ON ..
make -j4
```

---

## 🎁 Функции

### Основные возможности
- ⚡ Высокая производительность
- 🔒 Полная совместимость с MTProto
- 🛡️ Защита от блокировок (random padding)
- 👥 Поддержка нескольких секретных ключей
- 📊 Встроенная статистика
- 🔄 Горячая перезагрузка конфигурации
- 🚀 Модульная архитектура
- 🔐 Расширенная безопасность

### v1.0.27 Highlights
- ✅ Cache Memory Pool (5x быстрее)
- ✅ jemalloc/tcmalloc поддержка
- ✅ Security utils (3 функции + 18 тестов)
- ✅ Network analyzer кэширование (~80% CPU)
- ✅ 98 C тестов + 4 Dart (100%+ покрытие)

---

## 📞 Поддержка

### Ресурсы
- **GitHub Issues**: [Сообщить о проблеме](https://github.com/QuadDarv1ne/MTProxy/issues)
- **Discussions**: [Обсуждения](https://github.com/QuadDarv1ne/MTProxy/discussions)
- **Wiki**: [Wiki](https://github.com/QuadDarv1ne/MTProxy/wiki)

### Контакты
- **Email**: maksimqwe42@mail.ru
- **Telegram**: @Maestro7IT

---

## 📝 Лицензия

MTProxy распространяется под лицензией LGPL v2.

См. [LICENSE](LICENSE) для деталей.

---

## 🙏 Благодарности

Спасибо всем контрибьюторам за вклад в проект!

Полный список контрибьюторов см. в [CONTRIBUTORS.md](CONTRIBUTORS.md).

---

*Last updated: 29 марта 2026 г. — v1.0.27*
