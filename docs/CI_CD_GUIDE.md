# CI/CD Руководство по автоматической сборке

## 📋 Обзор

MTProxy использует GitHub Actions для автоматической сборки, тестирования и релиза на всех платформах.

## 🔄 Workflow файлы

### 1. auto-build.yml — Автоматическая сборка

**Триггеры:**
- Push в ветки: `main`, `master`, `dev`
- Теги: `v*`
- Ручной запуск: `workflow_dispatch`

**Собираемые платформы:**

| Платформа | Архитектуры | Артефакт |
|-----------|-------------|----------|
| Linux | x64 | `mtproto-proxy-linux-x64.zip` |
| Windows | x64 | `mtproto-proxy-windows-x64.zip` |
| macOS | Universal | `mtproto-proxy-macos-universal.zip` |
| Android | armeabi-v7a, arm64-v8a, x86_64 | `mtproxy-android-all.zip` |
| iOS | iphoneos (arm64), iphonesimulator (x86_64) | `mtproxy-ios-all.zip` |

**Опции ручного запуска:**
- `build_type`: Release / Debug
- `create_release`: true / false

### 2. auto-version.yml — Автоматическая версия и релиз

**Триггеры:**
- Push в `main` / `master`
- Ручной запуск с выбором типа версии

**Типы версий:**
- `patch`: 1.0.0 → 1.0.1
- `minor`: 1.0.0 → 1.1.0
- `major`: 1.0.0 → 2.0.0
- `prerelease`: 1.0.0 → 1.0.1-beta.1

### 3. ci.yml — Базовая CI

**Триггеры:**
- Push / PR в `main`, `master`, `develop`

**Задачи:**
- Сборка на Linux (gcc, clang)
- Docker образ
- Code quality (cppcheck)
- Security scan (Trivy)

## 🚀 Использование

### Автоматическая сборка при push

Просто запушьте изменения в защищённую ветку:

```bash
git push origin master
```

GitHub Actions автоматически:
1. Соберёт проект на всех платформах
2. Запустит тесты
3. Создаст артефакты
4. (Опционально) Создаст релиз

### Ручной запуск сборки

1. Перейдите на вкладку **Actions** в GitHub
2. Выберите workflow **Auto Build All Platforms**
3. Нажмите **Run workflow**
4. Выберите параметры:
   - Branch: `master`, `dev`, etc.
   - Build type: `Release` или `Debug`
   - Create Release: `true` или `false`
5. Нажмите **Run workflow**

### Создание релиза с новой версией

```bash
# Вариант 1: Автоматически через workflow_dispatch
# Actions → Auto Version & Release → Run workflow
# Выбрать version_type: patch/minor/major

# Вариант 2: Тегом
git tag v1.2.3
git push origin v1.2.3
```

## 📦 Артефакты

### Где найти

Артефакты доступны:
1. На странице workflow run (30 дней)
2. В GitHub Releases (бессрочно)

### Содержимое артефактов

**Linux:**
```
mtproto-proxy-linux-x64/
├── mtproto-proxy      # Основной бинарник
├── mtproxy-admin      # Admin CLI
└── libmtproxy.so      # Shared library
```

**Windows:**
```
mtproto-proxy-windows-x64/
├── mtproto-proxy.exe
├── mtproxy-admin.exe
└── mtproxy.dll
```

**macOS:**
```
mtproto-proxy-macos-universal/
├── mtproto-proxy
├── mtproxy-admin
└── libmtproxy.dylib
```

**Android:**
```
mtproxy-android-*/
└── libmtproxy.so  # Для каждой ABI
```

**iOS:**
```
mtproxy-ios-*/
└── libmtproxy.a  # Статическая библиотека
```

## 🔧 Настройка

### Переменные окружения

| Переменная | Описание | По умолчанию |
|------------|----------|--------------|
| `BUILD_TYPE` | Тип сборки | `Release` |
| `PROJECT_NAME` | Имя проекта | `mtproto-proxy` |

### Secrets

Для создания релизов требуется:
- `GITHUB_TOKEN` (автоматически создаётся)

### Изменение конфигурации

1. Отредактируйте `.github/workflows/auto-build.yml`
2. Измените матрицу сборок (платформы, архитектуры)
3. Настройте шаги сборки при необходимости

## 📊 Статус сборок

Значки статуса для README:

```markdown
[![Linux Build](https://github.com/QuadDarv1ne/MTProxy/actions/workflows/auto-build.yml/badge.svg)](https://github.com/QuadDarv1ne/MTProxy/actions/workflows/auto-build.yml)
[![Windows Build](https://github.com/QuadDarv1ne/MTProxy/actions/workflows/auto-build.yml/badge.svg)](https://github.com/QuadDarv1ne/MTProxy/actions/workflows/auto-build.yml)
[![macOS Build](https://github.com/QuadDarv1ne/MTProxy/actions/workflows/auto-build.yml/badge.svg)](https://github.com/QuadDarv1ne/MTProxy/actions/workflows/auto-build.yml)
```

## 🐛 Troubleshooting

### Сборка не запускается

1. Проверьте `.github/workflows/` файлы на синтаксические ошибки
2. Убедитесь что ветка защищена (если требуется)
3. Проверьте лимиты GitHub Actions

### Артефакты не создаются

1. Проверьте логи сборки на ошибки
2. Убедитесь что пути к бинарникам верные
3. Проверьте `CMakeLists.txt` на правильные output directories

### Релиз не создаётся

1. Проверьте наличие `GITHUB_TOKEN`
2. Убедитесь что все зависимости (jobs) успешны
3. Проверьте права доступа к репозиторию

## 📈 Метрики

- **Время сборки**: ~15-20 минут для всех платформ
- **Время тестов**: ~2-3 минуты
- **Хранение артефактов**: 30 дней (workflow), бессрочно (releases)

## 📝 Примеры

### Запуск только для Linux

```yaml
# В auto-build.yml, закомментируйте ненужные jobs
# или создайте новый workflow с фильтром
```

### Сборка только shared library

```bash
# В CMakeLists.txt убедитесь что BUILD_SHARED_LIB=ON
cmake -DBUILD_SHARED_LIB=ON ..
```

### Кастомная матрица сборок

```yaml
strategy:
  matrix:
    platform: [ubuntu-latest, windows-latest]
    build_type: [Release]
```

---

*Последнее обновление: 20 марта 2026 г.*
