---
layout: default
title: "HardFOC PCA9685 Driver"
description: "Hardware-agnostic C++ driver for the NXP PCA9685 16-channel 12-bit PWM controller"
nav_order: 1
permalink: /
---

# HF-PCA9685 Driver
**Hardware-agnostic C++ driver for the NXP PCA9685 16-channel 12-bit PWM controller**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

## 📚 Table of Contents
1. [Overview](#-overview)
2. [Features](#-features)
3. [Quick Start](#-quick-start)
4. [Installation](#-installation)
5. [API Reference](#-api-reference)
6. [Examples](#-examples)
7. [Documentation](#-documentation)
8. [Contributing](#-contributing)
9. [License](#-license)

## 📦 Overview

The PCA9685 is a 16-channel, 12-bit PWM controller that communicates via I²C. This driver provides a hardware-agnostic interface for controlling the PCA9685, requiring only an implementation of the `I2cBus` interface.

## ✨ Features
- Platform-independent: requires only a user-implemented I2cBus interface
- No dependencies on project-specific or MCU-specific code
- Set PWM frequency (24 Hz to 1526 Hz typical)
- Set PWM value (on/off time) for each channel
- Reset and configure device
- Error reporting and diagnostics

## 🚀 Quick Start

1. **Implement the I2cBus interface** for your platform:

```cpp
class MyI2c : public I2cBus {
public:
    bool write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) override;
    bool read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) override;
};
```

2. **Create and use the driver:**

```cpp
MyI2c i2c;
PCA9685 pwm(&i2c, 0x40); // 0x40 is the default address
pwm.reset();
pwm.setPwmFreq(50.0f); // 50 Hz for servos
pwm.setDuty(0, 0.075f); // Channel 0, 7.5% duty (1.5ms pulse)
```

3. **Error handling:**

```cpp
if (!pwm.setDuty(0, 0.1f)) {
    auto err = pwm.getLastError();
    // handle error
}
```

## 🔧 Installation

1. Copy the driver files into your project
2. Implement the `I2cBus` interface for your platform
3. Include the driver header in your code

## 📖 API Reference

For complete API documentation, see the [docs](docs/) directory or generate Doxygen documentation:
```bash
doxygen _config/Doxyfile
```

## 📊 Examples

For ESP32 examples, see the [examples/esp32](examples/esp32/) directory.

## 📚 Documentation

- Run `doxygen` in the project root to generate HTML docs in `docs/`
- See the [docs](docs/) directory for additional documentation

## 🤝 Contributing

Pull requests and suggestions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## 📄 License

This project is licensed under the **GNU General Public License v3.0**.  
See the [LICENSE](LICENSE) file for details.
