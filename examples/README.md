# PCA9685 Driver Examples

This folder contains platform-agnostic and platform-specific usage examples for the hardware-agnostic PCA9685 PWM controller driver.

## Structure

- `common/`   - Platform-agnostic C++ example using a dummy/mock I2C bus.
- `arduino/`  - Example sketch for Arduino platforms.
- `esp32/`    - Example project for ESP32 (ESP-IDF style).
- `stm32/`    - Example for STM32 (baremetal or HAL-based).

## How to Use

- See each subfolder for platform-specific build and run instructions.
- The `common/` example is ideal for unit testing and as a reference for integration.

---

For API usage, see the main driver `README.md` and the documentation in `../docs/`.
