# Build Scripts

This directory contains build-related scripts and utilities for MTProxy.

## Available Scripts

### build_windows.bat

Windows build script that automatically detects and uses available compilers.

**Features:**
- Auto-detects Visual Studio or MinGW
- Builds using CMake
- Runs tests after successful build

**Usage:**
```cmd
scripts\build_windows.bat
```

**Requirements:**
- CMake installed
- Either Visual Studio or MinGW/GCC

## Build Methods

### Method 1: Using Make (Linux/macOS)

Traditional build method using Makefile:

```bash
# Build
make

# Clean
make clean

# Binary location
objs/bin/mtproto-proxy
```

### Method 2: Using CMake (Cross-platform)

Modern build system supporting multiple platforms:

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build (parallel)
cmake --build . --parallel

# Binary location
build/bin/mtproto-proxy
```

**CMake Options:**

```bash
# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build (default)
cmake -DCMAKE_BUILD_TYPE=Release ..

# With specific compiler
cmake -DCMAKE_C_COMPILER=clang ..

# Enable Profile-Guided Optimization
cmake -DENABLE_PGO=ON ..

# Static linking
cmake -DSTATIC_LINKING=ON ..
```

### Method 3: Using Docker

Build inside Docker container:

```bash
# Build image
docker build -t mtproxy -f docker/Dockerfile .

# Or using docker-compose
cd docker
docker-compose build
```

## Build Configurations

### Release (Default)
- Maximum optimization (-O3)
- No debug symbols
- Recommended for production

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### Debug
- No optimization (-O0)
- Full debug symbols (-g)
- Recommended for development

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

### RelWithDebInfo
- Optimized with debug info (-O2 -g)
- Good for profiling

```bash
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
```

### MinSizeRel
- Optimize for size (-Os)
- Minimal binary size

```bash
cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..
```

## Compiler Support

### GCC
- Minimum version: 7.0
- Recommended: 11.0+
- Full feature support

### Clang
- Minimum version: 10.0
- Recommended: 14.0+
- Full feature support

### MSVC
- Minimum version: Visual Studio 2019
- Recommended: Visual Studio 2022
- Windows-specific optimizations

## Platform-Specific Notes

### Linux
```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install build-essential libssl-dev zlib1g-dev cmake

# Build
make
# or
mkdir build && cd build && cmake .. && make -j$(nproc)
```

### macOS
```bash
# Install dependencies
brew install openssl zlib cmake

# Build
make
# or
mkdir build && cd build && cmake .. && make -j$(sysctl -n hw.ncpu)
```

### Windows
```cmd
REM Using Visual Studio
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

REM Using MinGW
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

## Optimization Flags

The build system automatically applies optimization flags:

**Architecture-specific:**
- `-march=native` - Optimize for current CPU
- `-mtune=native` - Tune for current CPU
- `-mavx2` - AVX2 instructions (if supported)
- `-mavx512f` - AVX-512 instructions (if supported)

**Performance:**
- `-O3` - Maximum optimization
- `-flto` - Link-time optimization
- `-ffast-math` - Fast math operations
- `-funroll-loops` - Loop unrolling

**Security:**
- `-fstack-protector-strong` - Stack protection
- `-D_FORTIFY_SOURCE=2` - Buffer overflow detection

## Troubleshooting

### Build fails with "OpenSSL not found"

**Linux:**
```bash
sudo apt install libssl-dev
```

**macOS:**
```bash
brew install openssl
export OPENSSL_ROOT_DIR=/usr/local/opt/openssl
```

**Windows:**
Download and install OpenSSL from https://slproweb.com/products/Win32OpenSSL.html

### Build fails with "zlib not found"

**Linux:**
```bash
sudo apt install zlib1g-dev
```

**macOS:**
```bash
brew install zlib
```

### CMake version too old

**Linux:**
```bash
# Add Kitware repository for latest CMake
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt update
sudo apt install cmake
```

**macOS:**
```bash
brew upgrade cmake
```

**Windows:**
Download from https://cmake.org/download/

### Compiler not found

**Linux:**
```bash
sudo apt install build-essential
```

**macOS:**
```bash
xcode-select --install
```

**Windows:**
Install Visual Studio or MinGW-w64

## Clean Build

```bash
# Make
make clean

# CMake
rm -rf build/
mkdir build && cd build && cmake ..

# Docker
docker-compose down
docker-compose build --no-cache
```

## Cross-Compilation

### For ARM64 (from x86_64)

```bash
cmake -DCMAKE_SYSTEM_NAME=Linux \
      -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
      -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
      ..
```

### For Windows (from Linux)

```bash
cmake -DCMAKE_SYSTEM_NAME=Windows \
      -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
      ..
```

## Performance Testing

After building, test performance:

```bash
# Run with verbose logging
./mtproto-proxy -v 3 ...

# Monitor with statistics
watch -n 1 'curl -s http://localhost:8888/stats'
```

## Contributing

When adding new build features:
1. Update CMakeLists.txt
2. Test on all supported platforms
3. Update this README
4. Document any new dependencies

---

For more information, see the main [README.md](../README.md)
