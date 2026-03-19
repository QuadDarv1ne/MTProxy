# MTProxy Project TODO

## ✅ Выполнено

### Синхронизация веток
- [x] Слияние dev → master выполнено
- [x] Изменения синхронизированы (3 файла: CMakeLists.txt, cpuid.c, kprintf.c)

### Реорганизация CMakeLists.txt
- [x] Объединены NET_SOURCES в одну секцию
- [x] Объединены SECURITY_SOURCES в одну секцию
- [x] Удалены дублирующиеся определения
- [x] Добавлены комментарии для навигации

### Исправления кода
- [x] common/cpuid.c — исправлено использование временной переменной
- [x] common/kprintf.c — исправлен тип для localtime_r

---

## 🔧 Активные задачи

### Сборка и компиляция
- [ ] Настроить сборку на Windows (требуется компилятор C)
- [ ] Проверить работу CMake после слияния
- [ ] Протестировать Makefile сборку

### Код
- [ ] Проверить net/net-tcp-connections.c — не включён в SECURITY_SOURCES
- [ ] Верифицировать security/modular-security.c и simple-security.c
- [ ] Проверить все заголовочные файлы в NET_SOURCES

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

- **Веток:** 2 (master, dev)
- **Файлов в system/:** 82
- **Модулей безопасности:** 4 основных + enhanced
- **Сетевых модулей:** 20+
- **Документов:** 7 в docs/, 3 в security/

---

## 🔍 Замечания по коду

### CMakeLists.txt
- Строки 195-350: требуется проверка порядка подключения модулей
- SECURITY_SOURCES: дублирование удалено, но нужна проверка зависимостей
- NET_SOURCES: все ли файлы нужны в базовой сборке?

### common/cpuid.c
- Исправление: `(unsigned int*) &cached.ecx` — проверить на всех архитектурах

### common/kprintf.c
- Исправление: `(const time_t*)&tv.tv_sec` — может вызвать проблемы на 32-bit

---

## 📝 Заметки

- **Правило:** Качество важнее количества
- **Workflow:** Улучшения в dev → проверка → merge в main
- **Сборка:** CMake предпочтительнее Make для кроссплатформенности
- **Безопасность:** Модульная архитектура с возможностью замены компонентов

---

*Последнее обновление: 19 марта 2026 г.*
