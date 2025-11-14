---
layout: default
title: "🛠️ Installation"
description: "Installation and build instructions for the PCA9685 driver"
nav_order: 1
parent: "📚 Documentation"
permalink: /docs/installation/
---

# Installation

This guide covers how to obtain, build, and verify the PCA9685 driver library.

## Prerequisites

Before installing the driver, ensure you have:

- **C++11 Compiler**: GCC 4.8+, Clang 3.3+, or MSVC 2013+
- **Build System**: Make or CMake (optional, for building static library)
- **Platform SDK**: Your platform's I2C driver (ESP-IDF, STM32 HAL, Arduino Wire, etc.)

## Obtaining the Source

### Option 1: Git Clone

```bash
git clone https://github.com/N3b3x/hf-pca9685-driver.git
cd hf-pca9685-driver
```

### Option 2: Copy Files

Copy the following files into your project:

```
inc/
  └── pca9685.hpp
src/
  └── pca9685.cpp
```

**Note**: The driver uses a header-only template design where `pca9685.cpp` is included by `pca9685.hpp`. You typically only need to include the header file in your project.

## Building the Library

### Using Make

A simple Makefile is provided:

```bash
make
```

This builds `build/libpca9685.a` which can be linked into your application.

### Using CMake

```cmake
add_subdirectory(pca9685-driver)
target_link_libraries(your_target PRIVATE pca9685)
```

Or manually:

```cmake
add_library(pca9685 STATIC
    inc/pca9685.hpp
    src/pca9685.cpp
)
target_include_directories(pca9685 PUBLIC inc)
```

### Using ESP-IDF Component

The driver can be used as an ESP-IDF component. See the [examples/esp32](../examples/esp32/) directory for component integration examples.

## Running Unit Tests

The library includes unit tests. To run them:

```bash
make test
./build/test
```

Expected output:
```
All tests passed.
```

## Verification

To verify the installation:

1. Include the header in a test file:
   ```cpp
   #include "pca9685.hpp"
   ```

2. Compile a simple test:
   ```bash
   g++ -std=c++11 -I inc/ -c src/pca9685.cpp -o test.o
   ```

3. If compilation succeeds, the library is properly installed.

## Next Steps

- Follow the [Quick Start](quickstart.md) guide to create your first application
- Review [Hardware Setup](hardware_setup.md) for wiring instructions
- Check [Platform Integration](platform_integration.md) to implement the I2C interface

---

**Navigation**
⬅️ [Back to Index](index.md) | [Next: Quick Start ➡️](quickstart.md)

