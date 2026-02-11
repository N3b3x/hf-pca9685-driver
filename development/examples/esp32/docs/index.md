# ESP32 PCA9685 Applications — Documentation

This folder describes the ESP32 applications that use the PCA9685 driver: what they do, how to build and run them, and how they relate to the driver API.

## Overview

| Document | Content |
|----------|---------|
| [Index (this file)](index.md) | Overview and links |
| [Comprehensive Test](comprehensive-test.md) | Test suite: 12 tests, I2C scan, build/run |
| [Servo Demo](servo-demo.md) | 16-channel servo animations, timing, velocity limits |

## Driver Documentation

For the driver itself (class, API, integration):

- **Main docs**: [../../../docs/](../../../docs/) — installation, quick start, hardware setup, platform integration, configuration, API reference, examples, troubleshooting.
- **API reference**: [../../../docs/api_reference.md](../../../docs/api_reference.md) — all public methods, types, and constants.
- **Root README**: [../../../README.md](../../../README.md) — project overview and quick start.

## Applications Summary

| App | Purpose | When to use |
|-----|---------|-------------|
| **pca9685_comprehensive_test** | Validate driver and hardware | After wiring; CI; regression. |
| **pca9685_servo_demo** | Demo 16 servos with smooth animations | Showcase; reference for servo timing and velocity limiting. |

## Build and Flash (short)

From `examples/esp32`:

```bash
./scripts/build_app.sh list
./scripts/build_app.sh pca9685_comprehensive_test Debug
./scripts/flash_app.sh flash_monitor pca9685_comprehensive_test Debug
```

See [../README.md](../README.md) for prerequisites, pin override, and troubleshooting.
