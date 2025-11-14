---
layout: default
title: "📖 API Reference"
description: "Complete API documentation for the PCA9685 driver"
nav_order: 6
parent: "📚 Documentation"
permalink: /docs/api_reference/
---

# API Reference

Complete reference documentation for all public methods and types in the PCA9685 driver.

## Source Code

- **Main Header**: [`inc/pca9685.hpp`](../inc/pca9685.hpp)
- **Implementation**: [`src/pca9685.cpp`](../src/pca9685.cpp)

## Core Classes

### `PCA9685<I2cType>`

Main driver class for interfacing with the PCA9685 PWM controller.

**Template Parameter**: `I2cType` - Your I2C interface implementation (must inherit from `pca9685::I2cInterface<I2cType>`)

**Location**: [`inc/pca9685.hpp#L121`](../inc/pca9685.hpp#L121)

**Constructor:**
```cpp
PCA9685(I2cType* bus, uint8_t address);
```

**Parameters:**
- `bus`: Pointer to your I2C interface implementation
- `address`: 7-bit I2C address of the PCA9685 (0x40 default)

**Location**: [`src/pca9685.cpp#L30`](../src/pca9685.cpp#L30)

## Methods

### Initialization

#### `Reset()`

Reset the device to its power-on default state. Must be called before other operations.

**Signature:**
```cpp
bool Reset();
```

**Location**: [`src/pca9685.cpp#L34`](../src/pca9685.cpp#L34)

**Returns:**
- `true` if reset succeeded
- `false` if I2C write failed

**Example:**
```cpp
if (!pwm.Reset()) {
    auto error = pwm.GetLastError();
    // Handle error
}
```

**See also**: [`GetLastError()`](#getlasterror)

---

### Frequency Control

#### `SetPwmFreq(float freq_hz)`

Set the PWM frequency for all channels. Must be called before setting channel outputs.

**Signature:**
```cpp
bool SetPwmFreq(float freq_hz);
```

**Location**: [`src/pca9685.cpp#L46`](../src/pca9685.cpp#L46)

**Parameters:**
- `freq_hz`: Desired frequency in Hz (24.0 to 1526.0 typical)

**Returns:**
- `true` if frequency was set successfully
- `false` if I2C error or frequency out of range

**Example:**
```cpp
// Set frequency for servos (50 Hz)
if (!pwm.SetPwmFreq(50.0f)) {
    // Handle error
}

// Set frequency for LEDs (1 kHz)
pwm.SetPwmFreq(1000.0f);
```

**Implementation Details**:
- Puts device in sleep mode
- Writes prescale register
- Restores previous mode
- See [`calcPrescale()`](#calcprescale) for frequency calculation

**See also**: [`GetPrescale()`](#getprescale)

---

### Channel Control

#### `SetPwm(uint8_t channel, uint16_t on_time, uint16_t off_time)`

Set the PWM on/off timing for a specific channel.

**Signature:**
```cpp
bool SetPwm(uint8_t channel, uint16_t on_time, uint16_t off_time);
```

**Location**: [`src/pca9685.cpp#L81`](../src/pca9685.cpp#L81)

**Parameters:**
- `channel`: Channel number (0-15)
- `on_time`: Tick count when signal turns ON (0-4095)
- `off_time`: Tick count when signal turns OFF (0-4095)

**Returns:**
- `true` if PWM was set successfully
- `false` if I2C error, channel out of range, or timing values invalid

**Example:**
```cpp
// Channel 0: on at tick 0, off at tick 2048 (50% duty)
pwm.SetPwm(0, 0, 2048);

// Channel 1: on at tick 100, off at tick 2100
pwm.SetPwm(1, 100, 2100);
```

**Note**: The PWM period is 4096 ticks. Duty cycle = (off_time - on_time) / 4096.

**See also**: [`SetDuty()`](#setduty), [`SetAllPwm()`](#setallpwm)

---

#### `SetDuty(uint8_t channel, float duty)`

Set the duty cycle for a channel using a floating-point value (0.0 to 1.0).

**Signature:**
```cpp
bool SetDuty(uint8_t channel, float duty);
```

**Location**: [`src/pca9685.cpp#L105`](../src/pca9685.cpp#L105)

**Parameters:**
- `channel`: Channel number (0-15)
- `duty`: Duty cycle (0.0 = always off, 1.0 = always on)

**Returns:**
- `true` if duty cycle was set successfully
- `false` if I2C error or channel out of range

**Example:**
```cpp
// Set channel 0 to 50% duty
pwm.SetDuty(0, 0.5f);

// Turn channel 1 fully on
pwm.SetDuty(1, 1.0f);

// Turn channel 2 off
pwm.SetDuty(2, 0.0f);
```

**Implementation**: Clamps duty to 0.0-1.0 range, then calls `SetPwm()` with `on_time=0` and calculated `off_time`.

**See also**: [`SetPwm()`](#setpwm)

---

#### `SetAllPwm(uint16_t on_time, uint16_t off_time)`

Set all 16 channels to the same PWM value simultaneously.

**Signature:**
```cpp
bool SetAllPwm(uint16_t on_time, uint16_t off_time);
```

**Location**: [`src/pca9685.cpp#L113`](../src/pca9685.cpp#L113)

**Parameters:**
- `on_time`: Tick count when signal turns ON (0-4095)
- `off_time`: Tick count when signal turns OFF (0-4095)

**Returns:**
- `true` if all channels were set successfully
- `false` if I2C error or timing values invalid

**Example:**
```cpp
// Set all channels to 50% duty
pwm.SetAllPwm(0, 2048);

// Turn all channels off
pwm.SetAllPwm(0, 0);
```

**Use Case**: Synchronized control of all outputs, efficient for bulk updates.

**See also**: [`SetPwm()`](#setpwm)

---

### Status and Diagnostics

#### `GetLastError()`

Get the error code from the last operation.

**Signature:**
```cpp
Error GetLastError() const;
```

**Location**: [`inc/pca9685.hpp#L213`](../inc/pca9685.hpp#L213)

**Returns:**
- `Error` enum value indicating the last error (or `Error::None` if no error)

**Example:**
```cpp
if (!pwm.SetPwmFreq(50.0f)) {
    auto error = pwm.GetLastError();
    switch (error) {
        case pca9685::PCA9685<MyI2c>::Error::I2cWrite:
            // I2C write failed
            break;
        case pca9685::PCA9685<MyI2c>::Error::OutOfRange:
            // Frequency out of range
            break;
        // ...
    }
}
```

**See also**: [Error Codes](#error-codes)

---

#### `GetPrescale(uint8_t &prescale)`

Get the current prescale value (frequency divider).

**Signature:**
```cpp
bool GetPrescale(uint8_t &prescale);
```

**Location**: [`src/pca9685.cpp#L134`](../src/pca9685.cpp#L134)

**Parameters:**
- `prescale`: Reference to store the prescale value (output)

**Returns:**
- `true` if prescale was read successfully
- `false` if I2C read failed

**Example:**
```cpp
uint8_t prescale;
if (pwm.GetPrescale(prescale)) {
    printf("Current prescale: %u\n", prescale);
}
```

**See also**: [`SetPwmFreq()`](#setpwmfreq)

---

#### `SetOutputEnable(bool enabled)`

Stub method for output enable control. The actual OE pin must be controlled externally via GPIO.

**Signature:**
```cpp
void SetOutputEnable(bool enabled);
```

**Location**: [`inc/pca9685.hpp#L227`](../inc/pca9685.hpp#L227)

**Note**: This is a placeholder. You must control the OE pin via your platform's GPIO if needed.

---

## Error Handling

The driver uses an error code system. Most methods return `bool` (true = success, false = failure), and you can query the specific error using `GetLastError()`.

### Error Codes

**Location**: [`inc/pca9685.hpp#L126`](../inc/pca9685.hpp#L126)

| Code | Name | Description |
|------|------|-------------|
| `None` | No error | Operation succeeded |
| `I2cWrite` | I2C write failure | I2C write operation failed (NACK, timeout, etc.) |
| `I2cRead` | I2C read failure | I2C read operation failed |
| `InvalidParam` | Invalid parameter | Invalid method parameter |
| `DeviceNotFound` | Device not found | Device not responding at I2C address |
| `NotInitialized` | Not initialized | `Reset()` not called before operation |
| `OutOfRange` | Out of range | Parameter value outside valid range |

**Example Error Handling:**
```cpp
if (!pwm.SetPwmFreq(2000.0f)) { // Out of range
    auto error = pwm.GetLastError();
    if (error == pca9685::PCA9685<MyI2c>::Error::OutOfRange) {
        // Frequency too high, use valid range
        pwm.SetPwmFreq(1526.0f); // Maximum valid frequency
    }
}
```

## Constants

**Location**: [`inc/pca9685.hpp#L159`](../inc/pca9685.hpp#L159)

| Constant | Value | Description |
|----------|-------|-------------|
| `MAX_CHANNELS` | 16 | Maximum number of PWM channels |
| `MAX_PWM` | 4095 | Maximum PWM value (12-bit resolution) |
| `OSC_FREQ` | 25000000 | Internal oscillator frequency (25 MHz) |

## Type Definitions

### `Error` Enum

Error code enumeration.

**Location**: [`inc/pca9685.hpp#L126`](../inc/pca9685.hpp#L126)

```cpp
enum class Error : uint8_t {
    None = 0,
    I2cWrite = 1,
    I2cRead = 2,
    InvalidParam = 3,
    DeviceNotFound = 4,
    NotInitialized = 5,
    OutOfRange = 6
};
```

### `Register` Enum

Register address enumeration.

**Location**: [`inc/pca9685.hpp#L139`](../inc/pca9685.hpp#L139)

```cpp
enum class Register : uint8_t {
    MODE1 = 0x00,
    MODE2 = 0x01,
    // ... see header for complete list
    PRE_SCALE = 0xFE,
    TESTMODE = 0xFF
};
```

## Thread Safety

The driver is **not thread-safe**. If used in a multi-threaded environment:
- Each `PCA9685` instance should be used by a single thread
- Use external synchronization (mutex, etc.) for shared access
- I2C bus access must be thread-safe in your I2C interface implementation

## Private Methods

The following methods are private but documented for reference:

- `writeReg(uint8_t reg, uint8_t value)` - Write single register
- `readReg(uint8_t reg, uint8_t &value)` - Read single register
- `writeRegBlock(uint8_t reg, const uint8_t *data, size_t len)` - Write register block
- `readRegBlock(uint8_t reg, uint8_t *data, size_t len)` - Read register block
- `calcPrescale(float freq_hz)` - Calculate prescale value from frequency

**Location**: [`src/pca9685.cpp`](../src/pca9685.cpp)

---

**Navigation**
⬅️ [Configuration](configuration.md) | [Next: Examples ➡️](examples.md) | [Back to Index](index.md)

