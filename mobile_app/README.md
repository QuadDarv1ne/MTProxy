# MTProxy Mobile Application

Кроссплатформенное Flutter-приложение для управления MTProxy сервером.

## Поддерживаемые платформы

- **Windows** (10/11)
- **Linux** (Ubuntu, Debian, Fedora, Arch)
- **macOS** (10.15+)
- **iOS** (12.0+)
- **Android** (5.0+)

## Требования

### Для разработки
- Flutter SDK 3.0+
- Dart SDK 3.0+
- CMake (для сборки нативной библиотеки)

### Для сборки нативной библиотеки
- **Windows**: MSYS2 с UCRT64, OpenSSL, Zlib
- **Linux**: GCC, OpenSSL, Zlib
- **macOS**: Xcode Command Line Tools, OpenSSL
- **Android**: Android NDK
- **iOS**: Xcode

## Установка

### 1. Установка зависимостей Flutter

```bash
cd mobile_app
flutter pub get
```

### 2. Сборка нативной библиотеки

#### Windows

```bash
# Установите MSYS2 с UCRT64
# Установите зависимости: pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-zlib

cd ..
mkdir build-win && cd build-win
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
make
copy libmtproxy.dll ..\mobile_app\assets\libs\windows\mtproxy.dll
```

#### Linux

```bash
cd ..
mkdir build-linux && cd build-linux
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cp libmtproxy.so ../mobile_app/assets/libs/linux/
```

#### macOS

```bash
cd ..
mkdir build-macos && cd build-macos
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cp libmtproxy.dylib ../mobile_app/assets/libs/macos/
```

### 3. Генерация FFI bindings

```bash
cd mobile_app
dart run ffigen --config ffigen.yaml
```

## Запуск приложения

### Desktop (Windows/Linux/macOS)

```bash
flutter run -d windows
# или
flutter run -d linux
# или
flutter run -d macos
```

### Mobile (Android/iOS)

```bash
# Android
flutter run -d android

# iOS (требуется macOS)
flutter run -d ios
```

## Сборка релизной версии

### Windows

```bash
flutter build windows --release
```

### Linux

```bash
flutter build linux --release
```

### macOS

```bash
flutter build macos --release
```

### Android APK

```bash
flutter build apk --release
```

### Android App Bundle

```bash
flutter build appbundle --release
```

### iOS

```bash
flutter build ios --release
```

## Структура проекта

```
mobile_app/
├── lib/
│   ├── main.dart                 # Точка входа
│   ├── mtproxy_bindings.dart     # FFI bindings к C-коду
│   ├── models/
│   │   ├── proxy_config.dart     # Модель конфигурации
│   │   └── proxy_stats.dart      # Модель статистики
│   ├── screens/
│   │   ├── home_screen.dart      # Главный экран
│   │   ├── config_screen.dart    # Экран конфигурации
│   │   ├── stats_screen.dart     # Экран статистики
│   │   └── settings_screen.dart  # Экран настроек
│   ├── services/
│   │   └── mtproxy_service.dart  # Сервис управления прокси
│   └── widgets/
│       ├── status_card.dart            # Карточка статуса
│       ├── quick_stats.dart            # Быстрая статистика
│       ├── proxy_control_button.dart   # Кнопка старт/стоп
│       └── secret_list_editor.dart     # Редактор секретов
├── assets/
│   ├── icons/                  # Иконки приложения
│   └── libs/                   # Нативные библиотеки
│       ├── windows/
│       ├── linux/
│       ├── macos/
│       ├── android/
│       └── ios/
├── pubspec.yaml                # Зависимости Flutter
└── ffigen.yaml                 # Конфигурация FFI генератора
```

## Функциональность

### Главный экран
- Статус прокси сервера
- Кнопка запуска/остановки
- Быстрая статистика
- Информация о конфигурации

### Конфигурация
- Настройка порта
- Управление секретными ключами (добавление, удаление, генерация)
- Настройка лимита подключений
- Включение/выключение IPv6
- Включение/выключение статистики

### Статистика
- Активные и tổng подключения
- Отправленный/полученный трафик
- Время работы (uptime)
- Использование CPU и памяти
- Графики динамики подключений

### Настройки
- Тёмная/светлая тема
- Уведомления
- Автозапуск
- Выбор языка
- Сброс настроек

## Интеграция с C-кодом

Приложение использует FFI (Foreign Function Interface) для вызова функций из C-библиотеки MTProxy.

### Основные функции:

```c
int mtproxy_init();           // Инициализация
int mtproxy_start();          // Запуск
void mtproxy_stop();          // Остановка
int mtproxy_set_port(uint16_t port);  // Установка порта
int mtproxy_add_secret(const char* secret);  // Добавление секрета
mtproxy_stats* mtproxy_get_stats();  // Получение статистики
```

## Архитектура

```
┌─────────────────────────────────────┐
│         Flutter UI Layer            │
│  (Screens, Widgets, Providers)      │
├─────────────────────────────────────┤
│      MTProxyService (Business)      │
│  (State Management, Polling)        │
├─────────────────────────────────────┤
│       FFI Bindings Layer            │
│  (Dart ↔ C Interop)                 │
├─────────────────────────────────────┤
│     MTProxy Core (C Library)        │
│  (Native Proxy Implementation)      │
└─────────────────────────────────────┘
```

## Лицензия

MIT License
