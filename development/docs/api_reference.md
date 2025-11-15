# API Reference

Complete reference documentation for all public methods and types in the PCA9685 driver.

## Source Code

- **Main Header**: [`inc/pca9685.hpp`](../inc/pca9685.hpp)
- **Implementation**: [`src/pca9685.cpp`](../src/pca9685.cpp)

## Core Class

### `PCA9685<I2cType>`

Main driver class for interfacing with the PCA9685 PWM controller.

**Template Parameter**: `I2cType` - Your I2C interface implementation (must inherit from `pca9685::I2cInterface<I2cType>`)

**Location**: [`inc/pca9685.hpp#L121`](../inc/pca9685.hpp#L121)

**Constructor:**
```cpp
PCA9685(I2cType* bus, uint8_t address);
```

**Location**: [`src/pca9685.cpp#L30`](../src/pca9685.cpp#L30)

## Methods

### Initialization

| Method | Signature | Location |
|--------|-----------|----------|
| `Reset()` | `bool Reset()` | [`src/pca9685.cpp#L34`](../src/pca9685.cpp#L34) |

### Frequency Control

| Method | Signature | Location |
|--------|-----------|----------|
| `SetPwmFreq()` | `bool SetPwmFreq(float freq_hz)` | [`src/pca9685.cpp#L46`](../src/pca9685.cpp#L46) |
| `GetPrescale()` | `bool GetPrescale(uint8_t &prescale)` | [`src/pca9685.cpp#L220`](../src/pca9685.cpp#L220) |

### PWM Control

| Method | Signature | Location |
|--------|-----------|----------|
| `SetPwm()` | `bool SetPwm(uint8_t channel, uint16_t on_time, uint16_t off_time)` | [`src/pca9685.cpp#L191`](../src/pca9685.cpp#L191) |
| `SetDuty()` | `bool SetDuty(uint8_t channel, float duty)` | [`src/pca9685.cpp#L199`](../src/pca9685.cpp#L199) |
| `SetAllPwm()` | `bool SetAllPwm(uint16_t on_time, uint16_t off_time)` | [`src/pca9685.cpp#L207`](../src/pca9685.cpp#L207) |

### Utility

| Method | Signature | Location |
|--------|-----------|----------|
| `GetLastError()` | `Error GetLastError() const` | [`inc/pca9685.hpp#L213`](../inc/pca9685.hpp#L213) |
| `SetOutputEnable()` | `void SetOutputEnable(bool enabled)` | [`inc/pca9685.hpp#L227`](../inc/pca9685.hpp#L227) |

## Types

### Enumerations

| Type | Values | Location |
|------|--------|----------|
| `Error` | `None`, `I2cWrite`, `I2cRead`, `InvalidParam`, `DeviceNotFound`, `NotInitialized`, `OutOfRange` | [`inc/pca9685.hpp#L126`](../inc/pca9685.hpp#L126) |
| `Register` | `MODE1`, `MODE2`, `LED0_ON_L`, `LED0_OFF_L`, `PRE_SCALE`, etc. | [`inc/pca9685.hpp#L139`](../inc/pca9685.hpp#L139) |

### Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_CHANNELS` | `16` | Maximum number of PWM channels |
| `MAX_PWM` | `4095` | Maximum PWM value |
| `OSC_FREQ` | `25000000` | Internal oscillator frequency (25 MHz) |

---

**Navigation**
⬅️ [Configuration](configuration.md) | [Next: Examples ➡️](examples.md) | [Back to Index](index.md)
