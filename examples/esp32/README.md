# PCA9685 ESP32 Examples

This directory contains ESP32 applications for the **PCA9685** 16-channel 12-bit PWM driver. They
use ESP-IDF and the shared build/flash scripts.

## Table of Contents

- [Hardware Overview](#-hardware-overview)
- [Pin Connections](#-pin-connections)
- [Available Applications](#-available-applications)
- [Building and Flashing](#-building-and-flashing)
- [Configuration](#-configuration)
- [Troubleshooting](#-troubleshooting)
- [Documentation](#-documentation)

---

## Hardware Overview

### PCA9685 PWM Controller

The PCA9685 is a 16-channel, 12-bit PWM controller (I²C). It is used to drive LEDs, hobby servos, and other PWM loads.

- **Channels**: 16 (OUT0–OUT15)
- **Resolution**: 12-bit (0–4095 per channel)
- **Frequency**: 24 Hz–1526 Hz (e.g. 50 Hz for servos, 1 kHz for LEDs)
- **I2C**: 7-bit address, default **0x40** (A0–A5 all LOW)

### ESP32 Host

The examples use:

- **I2C**: Default SDA=GPIO4, SCL=GPIO5 (100 kHz for PCA9685). Same pattern as the HardFOC
  pcal9555/bno08x examples.
- **Test indicator**: GPIO14 (e.g. comprehensive test progress).

---

## Pin Connections

### I2C Bus

| PCA9685 Pin | ESP32 GPIO | Function        | Notes                    |
|-------------|------------|-----------------|---------------------------|
| SDA         | GPIO4      | I2C Data        | 4.7 kΩ pull-up to 3.3 V   |
| SCL         | GPIO5      | I2C Clock       | 4.7 kΩ pull-up to 3.3 V   |
| VDD         | 3.3 V      | Power           | 2.3 V–5.5 V               |
| GND         | GND        | Ground          |                          |

### Address (A0–A5)

| A5–A0     | I2C Address (7-bit) |
|-----------|----------------------|
| All GND   | **0x40** (default)   |
| A0=VDD    | 0x41                 |
| …         | 0x40–0x7F            |

**Default in examples**: **0x40**.

### Optional

- **OE (Output Enable)**: Active-low. Leave unconnected or tie to GND to enable outputs.
- **OUT0–OUT15**: Connect to LEDs (with series resistors), servos, or other PWM loads.

---

## Available Applications

| Application                     | Description |
|---------------------------------|-------------|
| **pca9685_comprehensive_test**  | Full driver test suite: I2C init, driver init, PWM frequency, channel PWM, duty cycle, all-channel and full-on/off, prescale readback, sleep/wake, output config, error handling, stress tests. **12 tests**; runs once then prints summary. |
| **pca9685_servo_demo**          | 16-channel hobby servo demo: 1000–2000 µs pulse, velocity-limited motion, synchronized animations (Wave, Breathe, Cascade, Mirror, Converge, Knight Rider, Walk, Organic). **Loops forever.** |

Details: [docs/](docs/) (index, comprehensive test, servo demo).

---

## Building and Flashing

### Prerequisites

- **ESP-IDF** (e.g. `release/v5.5`). Install and source `export.sh` as usual.
- **Target**: Default in config is `esp32s3`; other targets (e.g. `esp32`, `esp32c6`) depend on
  `app_config.yml` and component `targets`.

### First-Time Setup

```bash
cd examples/esp32
chmod +x scripts/*.sh
./scripts/setup_repo.sh
```

### List Applications

```bash
./scripts/build_app.sh list
```

### Build

```bash
# Debug
./scripts/build_app.sh pca9685_comprehensive_test Debug
./scripts/build_app.sh pca9685_servo_demo Debug

# Release
./scripts/build_app.sh pca9685_comprehensive_test Release
```

### Flash and Monitor

```bash
./scripts/flash_app.sh flash_monitor pca9685_comprehensive_test Debug
./scripts/flash_app.sh flash_monitor pca9685_servo_demo Debug
```

### Flash-Only / Monitor-Only

```bash
./scripts/flash_app.sh flash pca9685_comprehensive_test Debug
./scripts/flash_app.sh monitor
```

---

## Configuration

### I2C Pins

Default: **SDA=GPIO4**, **SCL=GPIO5**. To override (e.g. for ESP32-S3 with different pins):

```bash
./scripts/build_app.sh pca9685_comprehensive_test Debug -- \
  -DPCA9685_EXAMPLE_I2C_SDA_GPIO=8 -DPCA9685_EXAMPLE_I2C_SCL_GPIO=9
```

### I2C Address

Default in code is **0x40**. To use another address, change `PCA9685_I2C_ADDRESS` in the app source
(e.g. `main/pca9685_comprehensive_test.cpp` or `main/pca9685_servo_demo.cpp`) and match hardware
(A0–A5).

### Test Sections (Comprehensive Test)

In `main/pca9685_comprehensive_test.cpp` you can enable/disable test sections:

```cpp
static constexpr bool ENABLE_INITIALIZATION_TESTS = true;
static constexpr bool ENABLE_FREQUENCY_TESTS = true;
static constexpr bool ENABLE_PWM_TESTS = true;
static constexpr bool ENABLE_DUTY_CYCLE_TESTS = true;
static constexpr bool ENABLE_ERROR_HANDLING_TESTS = true;
static constexpr bool ENABLE_STRESS_TESTS = true;
```

---

## Troubleshooting

### I2C / Init Failures

- Check SDA/SCL wiring and 4.7 kΩ pull-ups to 3.3 V.
- Confirm address (A0–A5). Default 0x40 = all GND.
- If connection fails, the comprehensive test runs an **I2C bus scan** and prints detected addresses.
- Try 100 kHz: in `Esp32Pca9685Bus::I2CConfig`, `frequency = 100000`.

### Build Errors

- Ensure ESP-IDF is sourced: `idf.py --version`.
- Clean and rebuild: `./scripts/build_app.sh --clean pca9685_comprehensive_test Debug`.

### No PWM Output

- Call `SetPwmFreq()` (e.g. 50 Hz) before setting channels.
- Ensure OE is not driven HIGH (outputs disabled).

More: [../../docs/troubleshooting.md](../../docs/troubleshooting.md).

---

## Documentation

- **Driver docs** (API, quick start, hardware, platform integration): [../../docs/](../../docs/)
- **ESP32 app docs** (comprehensive test, servo demo): [docs/](docs/)
- **API reference**: [../../docs/api_reference.md](../../docs/api_reference.md)

---

## Quick Reference

```bash
./scripts/build_app.sh list
./scripts/build_app.sh pca9685_comprehensive_test Debug
./scripts/flash_app.sh flash_monitor pca9685_comprehensive_test Debug
./scripts/flash_app.sh flash_monitor pca9685_servo_demo Debug
```
