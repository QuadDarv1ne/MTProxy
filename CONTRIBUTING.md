# Contributing to MTProxy

Thank you for your interest in contributing to MTProxy! This document provides guidelines and instructions for contributing.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Making Changes](#making-changes)
- [Coding Standards](#coding-standards)
- [Testing](#testing)
- [Submitting Changes](#submitting-changes)

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow
- Follow the project's technical standards

## Getting Started

1. Fork the repository
2. Clone your fork:
   ```bash
   git clone https://github.com/YOUR_USERNAME/MTProxy.git
   cd MTProxy
   ```
3. Add upstream remote:
   ```bash
   git remote add upstream https://github.com/TelegramMessenger/MTProxy.git
   ```

## Development Setup

### Prerequisites

**Linux (Debian/Ubuntu):**
```bash
apt update
apt install build-essential libssl-dev zlib1g-dev cmake git
```

**Linux (CentOS/RHEL/Fedora):**
```bash
# CentOS/RHEL
yum install openssl-devel zlib-devel cmake
yum groupinstall "Development Tools"

# Fedora
dnf install openssl-devel zlib-devel cmake gcc gcc-c++
```

### Building

**Using Make:**
```bash
make
```

**Using CMake:**
```bash
mkdir build && cd build
cmake ..
cmake --build . --parallel
```

### Running Tests

```bash
# Run all tests
make test

# Run specific test suite
./build/bin/test_crypto
./build/bin/test_network
```

## Making Changes

1. Create a feature branch:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. Make your changes following the [coding standards](#coding-standards)

3. Test your changes thoroughly

4. Commit with clear messages:
   ```bash
   git commit -m "Add feature: description of what you added"
   ```

## Coding Standards

### C Code Style

- Use **4 spaces** for indentation (no tabs)
- Maximum line length: **120 characters**
- Use descriptive variable and function names
- Add comments for complex logic
- Follow existing code structure

### Example:

```c
// Good
int calculate_connection_timeout(int base_timeout, int retry_count) {
    // Apply exponential backoff with maximum cap
    int timeout = base_timeout * (1 << retry_count);
    return (timeout > MAX_TIMEOUT) ? MAX_TIMEOUT : timeout;
}

// Bad
int calc(int t,int r){return t*(1<<r);}
```

### File Organization

- Header files (`.h`) should have include guards
- Implementation files (`.c`) should include their header first
- Group related functions together
- Separate public and private functions

### Memory Management

- Always free allocated memory
- Check return values of allocation functions
- Use RAII-style patterns where possible
- Avoid memory leaks

### Error Handling

- Check all return values
- Use consistent error codes
- Log errors appropriately
- Clean up resources on error paths

## Testing

### Writing Tests

- Write tests for new features
- Update tests when modifying existing code
- Ensure tests are deterministic
- Test edge cases and error conditions

### Test Structure

```c
// test_example.c
#include "test_framework.h"
#include "module_to_test.h"

void test_basic_functionality() {
    // Arrange
    int input = 42;
    
    // Act
    int result = function_to_test(input);
    
    // Assert
    assert_equal(result, expected_value);
}

int main() {
    run_test(test_basic_functionality);
    return test_summary();
}
```

## Submitting Changes

### Before Submitting

1. Ensure code compiles without warnings:
   ```bash
   make clean && make
   ```

2. Run all tests:
   ```bash
   make test
   ```

3. Check for memory leaks (if applicable):
   ```bash
   valgrind --leak-check=full ./your_program
   ```

4. Update documentation if needed

### Pull Request Process

1. Push your branch to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

2. Create a Pull Request on GitHub

3. Fill in the PR template with:
   - Description of changes
   - Related issue numbers
   - Testing performed
   - Screenshots (if UI changes)

4. Wait for review and address feedback

### PR Title Format

- `feat: Add new feature`
- `fix: Fix bug in component`
- `docs: Update documentation`
- `refactor: Refactor module`
- `test: Add tests for feature`
- `perf: Improve performance`

## Areas for Contribution

### High Priority

- [ ] Unit tests for core modules
- [ ] Performance benchmarks
- [ ] Documentation improvements
- [ ] Bug fixes

### Medium Priority

- [ ] New protocol support
- [ ] Monitoring improvements
- [ ] Configuration options
- [ ] Platform compatibility

### Low Priority

- [ ] Code refactoring
- [ ] Example applications
- [ ] Tooling improvements

## Questions?

- Open an issue for bugs or feature requests
- Contact maintainers via Telegram (see README)
- Check existing issues and PRs first

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (GPLv2/LGPLv2.1). See [licenses/](licenses/) directory for details.

---

Thank you for contributing to MTProxy! 🚀
