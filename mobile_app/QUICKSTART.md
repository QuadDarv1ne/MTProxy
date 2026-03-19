# MTProxy Mobile - Кроссплатформенное приложение

## Обзор

Это Flutter-приложение для управления MTProxy сервером на всех популярных платформах:
- **Desktop**: Windows, Linux, macOS
- **Mobile**: iOS, Android

## Быстрый старт

### 1. Установка зависимостей

```bash
cd mobile_app
flutter pub get
```

### 2. Сборка нативной библиотеки

#### Windows (PowerShell)
```powershell
.\build-native-windows.ps1
```

#### Linux/macOS (Bash)
```bash
chmod +x build-native-libs.sh
./build-native-libs.sh
```

### 3. Запуск приложения

```bash
# Windows
flutter run -d windows

# Linux
flutter run -d linux

# macOS
flutter run -d macos

# Android (требуется подключенное устройство/эмулятор)
flutter run -d android

# iOS (требуется macOS)
flutter run -d ios
```

## Структура проекта

```
mobile_app/
├── lib/
│   ├── main.dart                 # Точка входа
│   ├── mtproxy_bindings.dart     # FFI bindings
│   ├── models/                   # Модели данных
│   │   ├── proxy_config.dart
│   │   └── proxy_stats.dart
│   ├── screens/                  # Экраны приложения
│   │   ├── home_screen.dart      # Главная
│   │   ├── config_screen.dart    # Конфигурация
│   │   ├── stats_screen.dart     # Статистика
│   │   └── settings_screen.dart  # Настройки
│   ├── services/                 # Бизнес-логика
│   │   └── mtproxy_service.dart
│   └── widgets/                  # UI компоненты
│       ├── status_card.dart
│       ├── quick_stats.dart
│       ├── proxy_control_button.dart
│       └── secret_list_editor.dart
├── assets/
│   └── libs/                     # Нативные библиотеки
│       ├── windows/mtproxy.dll
│       ├── linux/libmtproxy.so
│       ├── macos/libmtproxy.dylib
│       ├── android/
│       └── ios/
└── pubspec.yaml                  # Зависимости
```

## Функциональность

### Главный экран
- ✅ Индикатор статуса сервера
- ✅ Кнопка запуска/остановки
- ✅ Быстрая статистика
- ✅ Информация о конфигурации

### Конфигурация
- ✅ Настройка порта
- ✅ Управление секретными ключами
- ✅ Генерация секретов
- ✅ Лимит подключений
- ✅ IPv6 настройки

### Статистика
- ✅ Активные подключения
- ✅ Трафик (отправлено/получено)
- ✅ Время работы
- ✅ Использование ресурсов
- ✅ Графики

### Настройки
- ✅ Тёмная/светлая тема
- ✅ Уведомления
- ✅ Автозапуск
- ✅ Выбор языка
- ✅ Сброс настроек

## Архитектура

```
┌─────────────────────────────────────┐
│         Flutter UI Layer            │
│  (Screens, Widgets, Providers)      │
├─────────────────────────────────────┤
│      MTProxyService (BLoC)          │
│  (State Management, Polling)        │
├─────────────────────────────────────┤
│       FFI Bindings Layer            │
│  (Dart ↔ C Interop)                 │
├─────────────────────────────────────┤
│     MTProxy Core (C Library)        │
│  (Native Proxy Implementation)      │
└─────────────────────────────────────┘
```

## API нативной библиотеки

### Основные функции

```c
// Инициализация
int mtproxy_init(void);

// Запуск/остановка
int mtproxy_start(void);
void mtproxy_stop(void);
bool mtproxy_is_running(void);

// Конфигурация
int mtproxy_set_port(uint16_t port);
int mtproxy_add_secret(const char* secret);
int mtproxy_set_max_connections(uint32_t max);

// Статистика
mtproxy_stats_t* mtproxy_get_stats(void);
uint32_t mtproxy_get_active_connections(void);
uint64_t mtproxy_get_bytes_sent(void);
```

## Сборка релизных версий

### Desktop

```bash
# Windows
flutter build windows --release

# Linux
flutter build linux --release

# macOS
flutter build macos --release
```

### Mobile

```bash
# Android APK
flutter build apk --release

# Android App Bundle (для Google Play)
flutter build appbundle --release

# iOS
flutter build ios --release
```

## Требования к сборке

### Windows
- MSYS2 с UCRT64
- OpenSSL, Zlib
- CMake 3.20+
- MinGW-w64

### Linux
- GCC/Clang
- OpenSSL, Zlib
- CMake 3.20+

### macOS
- Xcode Command Line Tools
- OpenSSL (через Homebrew)
- CMake 3.20+

### Android
- Android NDK r21+
- CMake 3.20+

### iOS
- Xcode 12+
- CMake 3.20+

## Отладка

### Логи Flutter

```bash
flutter logs
```

### Логи нативной библиотеки

Настройте уровень логирования в `mtproxy_service.dart`:
```dart
// Включить подробное логирование
final service = MTProxyService(debugMode: true);
```

## Тестирование

```bash
# Запуск тестов
flutter test

# Проверка кода
flutter analyze

# Форматирование
dart format lib/
```

## Лицензия

MIT License
