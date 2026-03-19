# Скрипт сборки нативной библиотеки MTProxy для Windows
# Использование: .\build-native-windows.ps1

param(
    [string]$BuildType = "Release",
    [string]$Generator = "MinGW Makefiles",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectRoot "build-windows"
$LibsDir = Join-Path $ScriptDir "assets\libs\windows"

Write-Host "=== Сборка нативной библиотеки MTProxy для Windows ===" -ForegroundColor Cyan
Write-Host "Project root: $ProjectRoot"
Write-Host "Build dir: $BuildDir"
Write-Host "Libs dir: $LibsDir"

# Очистка
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "Очистка директории сборки..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

# Создание директорий
if (-not (Test-Path $LibsDir)) {
    New-Item -ItemType Directory -Force $LibsDir | Out-Null
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Force $BuildDir | Out-Null
}

# Проверка наличия MSYS2/UCRT64
$Msys2Path = "C:\msys64\ucrt64\bin"
$MingwPath = ""

if (Test-Path $Msys2Path) {
    $env:Path = "$Msys2Path;$($env:Path)"
    $MingwPath = $Msys2Path
    Write-Host "✓ MSYS2/UCRT64 найден: $Msys2Path" -ForegroundColor Green
} else {
    # Проверка альтернативных путей
    $altPaths = @(
        "C:\Program Files\mingw-w64\bin",
        "C:\MinGW\bin",
        "C:\msys64\mingw64\bin"
    )
    
    foreach ($path in $altPaths) {
        if (Test-Path $path) {
            $env:Path = "$path;$($env:Path)"
            $MingwPath = $path
            Write-Host "✓ MinGW найден: $path" -ForegroundColor Green
            break
        }
    }
    
    if (-not $MingwPath) {
        Write-Host "⚠ MinGW не найден. Установите MSYS2 с UCRT64 или MinGW-w64" -ForegroundColor Red
        Write-Host "  MSYS2: https://www.msys2.org/"
        Write-Host "  Команда для установки: pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-zlib"
        exit 1
    }
}

# Проверка наличия cmake
try {
    $cmakeVersion = cmake --version 2>&1 | Select-Object -First 1
    Write-Host "✓ CMake: $cmakeVersion" -ForegroundColor Green
} catch {
    Write-Host "⚠ CMake не найден. Установите CMake 3.20+" -ForegroundColor Red
    exit 1
}

# Переход в директорию сборки
Set-Location $BuildDir

# Конфигурация CMake
Write-Host ""
Write-Host "=== Конфигурация CMake ===" -ForegroundColor Cyan
cmake -G "$Generator" `
      -DCMAKE_BUILD_TYPE=$BuildType `
      -DCMAKE_INSTALL_PREFIX="$LibsDir" `
      -DENABLE_SHARED=ON `
      -DENABLE_STATIC=ON `
      "$ProjectRoot"

if ($LASTEXITCODE -ne 0) {
    Write-Host "⚠ Ошибка конфигурации CMake" -ForegroundColor Red
    exit 1
}

# Сборка
Write-Host ""
Write-Host "=== Сборка ===" -ForegroundColor Cyan
cmake --build . --config $BuildType -j (Get-CimInstance Win32_Processor).NumberOfLogicalProcessors

if ($LASTEXITCODE -ne 0) {
    Write-Host "⚠ Ошибка сборки" -ForegroundColor Red
    exit 1
}

# Копирование библиотек
Write-Host ""
Write-Host "=== Копирование библиотек ===" -ForegroundColor Cyan

$libFiles = @(
    "mtproxy.dll",
    "libmtproxy.dll.a",
    "libmtproxy.a"
)

foreach ($file in $libFiles) {
    $srcPath = Join-Path $BuildDir $file
    $dstPath = Join-Path $LibsDir $file
    
    if (Test-Path $srcPath) {
        Copy-Item $srcPath $dstPath -Force
        Write-Host "✓ Скопировано: $file" -ForegroundColor Green
    } else {
        Write-Host "⚠ Файл не найден: $file" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "=== Сборка завершена ===" -ForegroundColor Green
Write-Host "Библиотеки находятся в: $LibsDir"
Write-Host ""
Write-Host "Для запуска Flutter приложения выполните:" -ForegroundColor Cyan
Write-Host "  cd mobile_app"
Write-Host "  flutter pub get"
Write-Host "  flutter run -d windows"
