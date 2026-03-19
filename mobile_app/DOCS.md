# MTProxy Mobile - Итоговая документация

## 📋 Обзор

**MTProxy Mobile** — это кроссплатформенное Flutter-приложение для управления MTProxy сервером с поддержкой:
- **Desktop**: Windows, Linux, macOS
- **Mobile**: iOS, Android

## 🏗️ Архитектура проекта

```
mobile_app/
├── lib/
│   ├── main.dart                     # Точка входа
│   ├── mtproxy_bindings.dart         # FFI bindings к C-коду
│   │
│   ├── models/                       # Модели данных
│   │   ├── proxy_config.dart         # Конфигурация прокси
│   │   └── proxy_stats.dart          # Статистика
│   │
│   ├── screens/                      # Экраны приложения
│   │   ├── home_screen.dart          # Главная страница
│   │   ├── config_screen.dart        # Конфигурация
│   │   ├── stats_screen.dart         # Статистика
│   │   ├── settings_screen.dart      # Настройки
│   │   ├── logs_screen.dart          # Логи и отладка
│   │   └── setup_wizard.dart         # Мастер настройки
│   │
│   ├── services/                     # Бизнес-логика
│   │   ├── mtproxy_service.dart      # Управление прокси
│   │   ├── notification_service.dart # Уведомления
│   │   └── logger_service.dart       # Логирование
│   │
│   ├── widgets/                      # UI компоненты
│   │   ├── status_card.dart          # Карточка статуса
│   │   ├── quick_stats.dart          # Быстрая статистика
│   │   ├── proxy_control_button.dart # Кнопка старт/стоп
│   │   ├── secret_list_editor.dart   # Редактор секретов
│   │   └── animated_widgets.dart     # Анимированные виджеты
│   │
│   ├── utils/                        # Утилиты
│   │   ├── format_utils.dart         # Форматирование
│   │   ├── validation_utils.dart     # Валидация
│   │   └── secret_generator.dart     # Генератор секретов
│   │
│   ├── theme/                        # Темы оформления
│   │   └── app_theme.dart            # Расширенная тема
│   │
│   └── l10n/                         # Локализация
│       ├── app_localizations.dart    # Класс локализации
│       ├── app_ru.arb                # Русские переводы
│       └── app_en.arb                # Английские переводы
│
├── test/                             # Тесты
│   └── utils/
│       ├── format_utils_test.dart
│       ├── validation_utils_test.dart
│       └── secret_generator_test.dart
│
├── .github/workflows/                # CI/CD
│   ├── flutter-ci.yml                # Основной pipeline
│   ├── version-bump.yml              # Авто-версионирование
│   └── codeql.yml                    # Security анализ
│
├── assets/
│   ├── icons/                        # Иконки
│   └── libs/                         # Нативные библиотеки
│       ├── windows/
│       ├── linux/
│       ├── macos/
│       ├── android/
│       └── ios/
│
├── pubspec.yaml                      # Зависимости
├── analysis_options.yaml             # Линтер
├── l10n.yaml                         # Локализация
└── README.md                         # Документация
```

## 🎯 Основные возможности

### 1. Управление прокси
- ✅ Запуск/остановка сервера
- ✅ Мониторинг статуса в реальном времени
- ✅ Горячая перезагрузка конфигурации

### 2. Конфигурация
- ✅ Настройка порта
- ✅ Управление секретными ключами (генерация, валидация)
- ✅ Лимит подключений
- ✅ IPv6 поддержка
- ✅ Включение/выключение статистики

### 3. Статистика
- ✅ Активные/общие подключения
- ✅ Трафик (отправлено/получено)
- ✅ Время работы (uptime)
- ✅ Использование CPU и памяти
- ✅ Графики и диаграммы

### 4. Уведомления
- ✅ Запуск/остановка прокси
- ✅ Ошибки и предупреждения
- ✅ Виброотклик
- ✅ История уведомлений

### 5. Логирование
- ✅ 4 уровня логов (Debug, Info, Warning, Error)
- ✅ Поиск и фильтрация
- ✅ Экспорт логов
- ✅ Детальный просмотр

### 6. Настройки
- ✅ Светлая/тёмная тема
- ✅ Выбор языка (RU/EN)
- ✅ Автозапуск
- ✅ Сброс настроек

### 7. Мастер настройки
- ✅ Пошаговая конфигурация
- ✅ Рекомендации по портам
- ✅ Генерация секретов
- ✅ Проверка настроек

## 🔧 Технические особенности

### FFI Интеграция
```dart
// Вызов C-функций из Dart
final lib = MtProxyLib();
await lib.loadLibrary('mtproxy.dll');
lib.init();
lib.start();
lib.setPort(443);
lib.addSecret('hex-secret');
```

### State Management (Provider)
```dart
Consumer<MTProxyService>(
  builder: (context, service, child) {
    return Text(service.isRunning ? 'Running' : 'Stopped');
  },
)
```

### Локализация
```dart
// Использование
Text(AppLocalizations.of(context).appTitle);

// Переключение языка
Locale('en') // или Locale('ru')
```

## 📦 Зависимости

| Пакет | Назначение |
|-------|-----------|
| `provider` | State Management |
| `ffi` | FFI bindings |
| `fl_chart` | Графики |
| `shared_preferences` | Хранение настроек |
| `path_provider` | Пути к файлам |
| `flutter_localizations` | Локализация |
| `intl` | Интернационализация |

## 🚀 Быстрый старт

### 1. Установка зависимостей
```bash
cd mobile_app
flutter pub get
```

### 2. Сборка нативной библиотеки
```bash
# Windows
.\build-native-windows.ps1

# Linux/macOS
./build-native-libs.sh
```

### 3. Запуск
```bash
flutter run -d windows  # или linux/macos/android/ios
```

## 🧪 Тестирование

```bash
# Запуск тестов
flutter test

# Анализ кода
flutter analyze

# Форматирование
dart format lib/
```

## 📱 Сборка релиза

### Desktop
```bash
flutter build windows --release
flutter build linux --release
flutter build macos --release
```

### Mobile
```bash
# Android
flutter build apk --release
flutter build appbundle --release

# iOS
flutter build ios --release
```

## 🔐 Безопасность

- ✅ Валидация всех входных данных
- ✅ Маскировка секретных ключей
- ✅ Безопасная генерация случайных чисел
- ✅ CodeQL анализ в CI/CD
- ✅ Проверка уязвимостей

## 📊 CI/CD Pipeline

| Workflow | Описание |
|----------|---------|
| `flutter-ci.yml` | Тесты, анализ, сборка |
| `version-bump.yml` | Авто-версионирование |
| `codeql.yml` | Security анализ |

### Артефакты сборки
- Windows: `mtproxy-windows.zip`
- Linux: `mtproxy-linux.zip`
- macOS: `mtproxy-macos.zip`
- Android: `mtproxy-android.zip` (APK + AAB)
- iOS: `mtproxy-ios.zip`

## 🎨 UI/UX

### Компоненты
- **Material 3** дизайн
- **Адаптивная** вёрстка
- **Тёмная/светлая** тема
- **Анимации** переходов
- **Responsive** дизайн

### Доступность
- Поддержка TalkBack/VoiceOver
- Клавиатурная навигация
- Высокий контраст
- Масштабирование текста

## 📖 API нативной библиотеки

### Основные функции
```c
int mtproxy_init(void);
int mtproxy_start(void);
void mtproxy_stop(void);
bool mtproxy_is_running(void);

int mtproxy_set_port(uint16_t port);
int mtproxy_add_secret(const char* secret);
int mtproxy_set_max_connections(uint32_t max);

mtproxy_stats_t* mtproxy_get_stats(void);
```

### Структуры
```c
typedef struct {
    uint32_t active_connections;
    uint32_t total_connections;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t start_time;
    double cpu_usage;
    uint32_t memory_usage;
} mtproxy_stats_t;
```

## 🔮 Планы развития

- [ ] Push-уведомления
- [ ] Синхронизация настроек (облако)
- [ ] Поддержка нескольких серверов
- [ ] Расширенные графики
- [ ] Экспорт статистики
- [ ] Плагины для расширений

## 📄 Лицензия

MIT License

## 👥 Авторы

Разработано для проекта MTProxy

---

**Версия документа**: 1.0.0  
**Дата обновления**: Март 2026
