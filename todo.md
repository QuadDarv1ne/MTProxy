# MTProxy Project TODO

## ✅ Выполнено

### Синхронизация веток
- [x] Слияние dev → master выполнено
- [x] Изменения синхронизированы (CMakeLists.txt, cpuid.c, kprintf.c)
- [x] Ветки master и dev синхронизированы с origin

### Реорганизация CMakeLists.txt
- [x] Объединены NET_SOURCES в одну секцию (20 файлов)
- [x] Объединены SECURITY_SOURCES в одну секцию (8 файлов + mtproxy-fixes-simple.h)
- [x] Удалены дублирующиеся определения SECURITY_SOURCES из MTPROTO_SOURCES
- [x] Добавлены комментарии для навигации

### Исправления кода
- [x] common/cpuid.c — исправлено: `(unsigned int){0}` вместо временной переменной
- [x] common/kprintf.c — исправлено: `(const time_t*)&tv.tv_sec` для localtime_r

---

## 🔧 Активные задачи

### Сборка и компиляция
- [ ] Настроить сборку на Windows (требуется компилятор C / MinGW)
- [ ] Проверить работу CMake после слияния
- [ ] Протестировать Makefile сборку
- [ ] Проверить линку с OpenSSL и zlib

### Код — критические замечания
- [x] NET_SOURCES: net-tcp-connections.c включён (строка 229)
- [ ] Проверить security/modular-security.c и simple-security.c
- [ ] Проверить все заголовочные файлы в NET_SOURCES на наличие в дереве
- [ ] common/cpuid.c: проверить работу `(unsigned int){0}` на ARM64
- [ ] common/kprintf.c: `(const time_t*)&tv.tv_sec` — потенциальная проблема на 32-bit системах

### Анализ TODO/FIXME в коде
Найдено 466 отметок TODO/FIXME/BUG/XXX/HACK:
- [ ] jobs/jobs.c — много отладочного вывода (JOBS_DEBUG)
- [ ] engine/engine.c — строка 560: hack для image-engine
- [ ] system/config/ — debugging framework требует проверки

---

## 📋 Структура проекта

### Основные модули
```
engine/          — ядро движка
system/          — системная оптимизация (82 файла)
  ├── optimization/    — оптимизация
  ├── monitoring/      — мониторинг
  ├── integration/     — интеграция компонентов
  ├── diagnostic/      — диагностика
  ├── debugging/       — отладка
  └── config/          — конфигурация

security/        — безопасность
  ├── security-manager.c
  ├── ddos-protection.c
  ├── cert-pinning.c
  └── security-utils.c

security_enhanced/ — расширенная безопасность
net/             — сетевые модули
crypto/          — криптография
mtproto/         — протокол MTProto
```

### Документация
```
docs/
  ├── ADVANCED_LOGGING_RU.md
  ├── CRYPTO_OPTIMIZATIONS_RU.md
  ├── GITIGNORE_DOCUMENTATION_RU.md
  ├── MEMORY_OPTIMIZATION_RU.md
  ├── MODULAR_ARCHITECTURE_RU.md
  ├── OBFUSCATION_ENHANCEMENTS_RU.md
  └── PERFORMANCE_OPTIMIZATIONS_RU.md

security/
  ├── SECURITY_FEATURES.md
  ├── SECURITY_NOTES_RU.md
  └── COMPREHENSIVE_SECURITY_RU.md
```

---

## 🎯 Приоритетные задачи

### Высокий приоритет
1. [ ] Сборка проекта — проверить компиляцию после изменений CMakeLists.txt
2. [ ] Тесты — запустить существующие тесты
3. [ ] Проверка security модулей — убедиться что все модули подключены

### Средний приоритет
4. [ ] Документация CMakeLists.txt — добавить описание структуры
5. [ ] Модульные тесты для новых компонентов
6. [ ] Проверка совместимости Windows/Linux

### Низкий приоритет
7. [ ] Оптимизация CMake — кэширование, PGO
8. [ ] Рефакторинг дублирующегося кода
9. [ ] Обновление README с новой структурой

---

## 📊 Статистика проекта

- **Веток:** 2 (master, dev) — синхронизированы
- **Файлов в system/:** 82
- **Модулей безопасности:** 6 (modular, simple, manager, ddos, cert-pinning, utils)
- **Сетевых модулей:** 20 (в NET_SOURCES)
- **Документов:** 7 в docs/, 3 в security/
- **C-файлов в проекте:** 170
- **TODO/FIXME/BUG отметок:** 466

---

## 🔍 Замечания по коду

### CMakeLists.txt
- ✅ Строки 198-248: NET_SOURCES — все файлы в наличии
- ✅ Строки 251-265: SECURITY_SOURCES — проверены
- ✅ Строки 347-350: MTPROTO_SOURCES — дублирование удалено
- [ ] Проверить порядок инициализации модулей

### common/cpuid.c
- ✅ Исправление применено: `(unsigned int){0}` — rvalue для временного значения
- [ ] Требуется тестирование на ARM64

### common/kprintf.c
- ⚠️ `(const time_t*)&tv.tv_sec` — может вызвать проблемы на 32-bit (time_t может быть 64-bit)
- [ ] Рассмотреть альтернативное решение: `localtime_r((time_t[]){tv.tv_sec}, &t)`

---

## 📝 Заметки

- **Правило:** Качество важнее количества
- **Workflow:** Улучшения в dev → проверка → merge в main
- **Сборка:** CMake предпочтительнее Make для кроссплатформенности
- **Безопасность:** Модульная архитектура с возможностью замены компонентов

---

*Последнее обновление: 19 марта 2026 г. (актуализировано)*
