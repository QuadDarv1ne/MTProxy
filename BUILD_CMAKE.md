# Сборка MTProxy с использованием CMake

## Содержание

1. [Требования](#требования)
2. [Сборка проекта](#сборка-проекта)
3. [Параметры сборки](#параметры-сборки)
4. [Установка](#установка)
5. [Отличия от Make](#отличия-от-make)

## Требования

Для сборки проекта с использованием CMake вам понадобятся:

- CMake версии 3.10 или выше
- Компилятор C (gcc, clang или MSVC)
- Git (для получения хеша коммита)
- Установленные зависимости:
  - OpenSSL development headers
  - zlib development headers
  - libcryptopp-dev (для криптографии)

### Установка зависимостей

#### Ubuntu/Debian:

```bash
sudo apt update
sudo apt install cmake build-essential libssl-dev zlib1g-dev git
```

#### CentOS/RHEL/Fedora:

```bash
# CentOS/RHEL
sudo yum install cmake gcc gcc-c++ openssl-devel zlib-devel git

# Fedora
sudo dnf install cmake gcc gcc-c++ openssl-devel zlib-devel git
```

## Сборка проекта

### Базовая сборка:

```bash
# Клонируйте репозиторий (если еще не клонирован)
git clone https://github.com/TelegramMessenger/MTProxy
cd MTProxy

# Создайте директорию для сборки
mkdir build
cd build

# Сконфигурируйте проект с помощью CMake
cmake ..

# Соберите проект
make

# Или используйте cmake для сборки
cmake --build .
```

### Сборка с указанием типа (Debug/Release):

```bash
# Release сборка (по умолчанию)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Debug сборка
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Сборка
make
```

### Параллельная сборка:

```bash
# Использование нескольких потоков для ускорения сборки
make -j$(nproc)
# или
cmake --build . --parallel
```

## Параметры сборки

### Основные параметры CMake:

- `-DCMAKE_BUILD_TYPE=<Type>` - Тип сборки: Release, Debug, RelWithDebInfo, MinSizeRel
- `-DCMAKE_INSTALL_PREFIX=<path>` - Директория установки (по умолчанию /usr/local)
- `-DCMAKE_C_COMPILER=<compiler>` - Указать компилятор C
- `-DCMAKE_C_FLAGS=<flags>` - Дополнительные флаги компиляции

### Примеры:

```bash
# Сборка с отладочной информацией
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="-g -O0" ..

# Установка в пользовательскую директорию
cmake -DCMAKE_INSTALL_PREFIX=/opt/mtproxy ..
```

## Установка

После успешной сборки вы можете установить MTProxy в систему:

```bash
# Установка в директорию по умолчанию (/usr/local/bin)
sudo make install

# Или с использованием cmake
sudo cmake --install .

# Установка в пользовательскую директорию
cmake --install . --prefix /custom/install/path
```

## Отличия от Make

### Преимущества CMake:

1. **Кроссплатформенность** - работает на Linux, Windows, macOS
2. **Гибкая конфигурация** - легче настраивать параметры сборки
3. **IDE интеграция** - лучше поддержка в современных IDE
4. **Модульность** - проще управлять зависимостями и подпроектами
5. **Параллельная сборка** - более эффективная многопоточная сборка

### Соответствие с оригинальным Makefile:

- CMake создает исполняемый файл `mtproto-proxy` в директории `build/bin/`
- Используются аналогичные флаги оптимизации (`-O3`)
- Поддерживаются те же зависимости (OpenSSL, zlib, pthread)
- Создается статическая библиотека с общими компонентами

### Совместимость:

- Оба метода сборки (Make и CMake) могут использоваться параллельно
- CMake собирает тот же самый исходный код, что и Make
- Результат сборки (исполняемый файл) имеет одинаковую функциональность

## Troubleshooting

### Распространенные проблемы:

#### 1. Не найдены зависимости

**Ошибка**: Could not find OpenSSL/zlib
**Решение**: Убедитесь, что установлены dev-пакеты:

```bash
# Ubuntu/Debian
sudo apt install libssl-dev zlib1g-dev

# CentOS/RHEL
sudo yum install openssl-devel zlib-devel
```

#### 2. Ошибки компиляции

**Проверьте**:

- Совместимость версии компилятора
- Правильность установки зависимостей
- Достаточно ли свободного места

#### 3. Очистка сборки

```bash
# Удаление директории сборки
rm -rf build/
# или очистка Make-файлов сборки
make clean  # внутри директории build
```

## Разработка

### Работа с CMake при разработке:

```bash
# Для быстрой пересборки после изменений
cmake --build . --parallel

# Для обновления конфигурации при добавлении новых файлов
cmake ..

# Для просмотра всех целей сборки
make help
```

### Генерация файлов проекта для IDE:

```bash
# Для Visual Studio
cmake -G "Visual Studio 16 2019" ..

# Для Xcode
cmake -G "Xcode" ..

# Для CodeBlocks
cmake -G "CodeBlocks - Unix Makefiles" ..
```
