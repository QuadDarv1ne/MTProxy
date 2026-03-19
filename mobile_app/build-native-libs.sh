#!/bin/bash
# Скрипт сборки нативной библиотеки MTProxy для всех платформ
# Использование: ./build-native-libs.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
MOBILE_DIR="$SCRIPT_DIR/mobile_app"
LIBS_DIR="$MOBILE_DIR/assets/libs"

echo "=== Сборка нативных библиотек MTProxy ==="
echo "Project root: $PROJECT_ROOT"
echo "Mobile dir: $MOBILE_DIR"
echo "Libs dir: $LIBS_DIR"

# Создание директорий для библиотек
mkdir -p "$LIBS_DIR/windows"
mkdir -p "$LIBS_DIR/linux"
mkdir -p "$LIBS_DIR/macos"
mkdir -p "$LIBS_DIR/android/x86_64"
mkdir -p "$LIBS_DIR/android/arm64-v8a"
mkdir -p "$LIBS_DIR/ios"

# Функция сборки для Linux
build_linux() {
    echo ""
    echo "=== Сборка для Linux ==="
    BUILD_DIR="$PROJECT_ROOT/build-linux"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX="$LIBS_DIR/linux" \
          "$PROJECT_ROOT"
    
    make -j$(nproc)
    
    # Копирование библиотеки
    cp libmtproxy.so "$LIBS_DIR/linux/" || true
    cp libmtproxy.a "$LIBS_DIR/linux/" || true
    
    echo "✓ Сборка для Linux завершена"
}

# Функция сборки для macOS
build_macos() {
    echo ""
    echo "=== Сборка для macOS ==="
    BUILD_DIR="$PROJECT_ROOT/build-macos"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX="$LIBS_DIR/macos" \
          "$PROJECT_ROOT"
    
    make -j$(sysctl -n hw.ncpu)
    
    # Копирование библиотеки
    cp libmtproxy.dylib "$LIBS_DIR/macos/" || true
    cp libmtproxy.a "$LIBS_DIR/macos/" || true
    
    echo "✓ Сборка для macOS завершена"
}

# Функция сборки для Windows (кросс-компиляция или нативная)
build_windows() {
    echo ""
    echo "=== Сборка для Windows ==="
    
    if command -v x86_64-w64-mingw32-gcc &> /dev/null; then
        # Кросс-компиляция с Linux
        BUILD_DIR="$PROJECT_ROOT/build-windows"
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
        cmake -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/cmake/mingw-toolchain.cmake" \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_INSTALL_PREFIX="$LIBS_DIR/windows" \
              "$PROJECT_ROOT"
        
        make -j$(nproc)
        
        cp mtproxy.dll "$LIBS_DIR/windows/" || true
        cp libmtproxy.dll.a "$LIBS_DIR/windows/" || true
        cp libmtproxy.a "$LIBS_DIR/windows/" || true
        
        echo "✓ Сборка для Windows завершена (кросс-компиляция)"
    else
        echo "⚠ MinGW не найден. Пропускаем сборку для Windows."
        echo "  Для сборки установите: sudo apt install mingw-w64"
    fi
}

# Функция сборки для Android
build_android() {
    echo ""
    echo "=== Сборка для Android ==="
    
    if [ -z "$ANDROID_NDK" ]; then
        echo "⚠ ANDROID_NDK не установлен. Пропускаем сборку для Android."
        echo "  Установите Android NDK и задайте переменную ANDROID_NDK"
        return
    fi
    
    # Сборка для x86_64
    echo "Сборка для x86_64..."
    BUILD_DIR="$PROJECT_ROOT/build-android-x86_64"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    $ANDROID_NDK/build/cmake/android.toolchain.cmake \
        -DANDROID_ABI=x86_64 \
        -DANDROID_PLATFORM=android-21 \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$LIBS_DIR/android/x86_64" \
        "$PROJECT_ROOT"
    
    make -j$(nproc)
    cp libmtproxy.so "$LIBS_DIR/android/x86_64/" || true
    
    # Сборка для arm64-v8a
    echo "Сборка для arm64-v8a..."
    BUILD_DIR="$PROJECT_ROOT/build-android-arm64"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
          -DANDROID_ABI=arm64-v8a \
          -DANDROID_PLATFORM=android-21 \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX="$LIBS_DIR/android/arm64-v8a" \
          "$PROJECT_ROOT"
    
    make -j$(nproc)
    cp libmtproxy.so "$LIBS_DIR/android/arm64-v8a/" || true
    
    echo "✓ Сборка для Android завершена"
}

# Функция сборки для iOS
build_ios() {
    echo ""
    echo "=== Сборка для iOS ==="
    
    if [[ "$(uname)" != "Darwin" ]]; then
        echo "⚠ Сборка для iOS возможна только на macOS"
        return
    fi
    
    BUILD_DIR="$PROJECT_ROOT/build-ios"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake -DCMAKE_SYSTEM_NAME=iOS \
          -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX="$LIBS_DIR/ios" \
          "$PROJECT_ROOT"
    
    make -j$(sysctl -n hw.ncpu)
    
    cp libmtproxy.a "$LIBS_DIR/ios/" || true
    
    echo "✓ Сборка для iOS завершена"
}

# Парсинг аргументов
TARGETS="all"
if [ $# -gt 0 ]; then
    TARGETS="$1"
fi

case "$TARGETS" in
    "linux")
        build_linux
        ;;
    "macos")
        build_macos
        ;;
    "windows")
        build_windows
        ;;
    "android")
        build_android
        ;;
    "ios")
        build_ios
        ;;
    "all")
        if [[ "$(uname)" == "Linux" ]]; then
            build_linux
            build_windows
            build_android
        elif [[ "$(uname)" == "Darwin" ]]; then
            build_macos
            build_ios
        else
            echo "⚠ Неизвестная платформа. Доступные цели: linux, macos, windows, android, ios"
            exit 1
        fi
        ;;
    *)
        echo "Неизвестная цель: $TARGETS"
        echo "Доступные цели: linux, macos, windows, android, ios, all"
        exit 1
        ;;
esac

echo ""
echo "=== Сборка завершена ==="
echo "Библиотеки находятся в: $LIBS_DIR"
