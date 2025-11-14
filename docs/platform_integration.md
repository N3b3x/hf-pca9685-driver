---
layout: default
title: "🔧 Platform Integration"
description: "Implement the CRTP I2C interface for your platform"
nav_order: 4
parent: "📚 Documentation"
permalink: /docs/platform_integration/
---

# Platform Integration Guide

This guide explains how to implement the hardware abstraction interface for the PCA9685 driver on your platform.

## Understanding CRTP (Curiously Recurring Template Pattern)

The PCA9685 driver uses **CRTP** (Curiously Recurring Template Pattern) for hardware abstraction. This design choice provides several critical benefits for embedded systems:

### Why CRTP Instead of Virtual Functions?

#### 1. **Zero Runtime Overhead**
- **Virtual functions**: Require a vtable lookup (indirect call) = ~5-10 CPU cycles overhead per call
- **CRTP**: Direct function calls = 0 overhead, compiler can inline
- **Impact**: In time-critical embedded code, this matters significantly

#### 2. **Compile-Time Polymorphism**
- **Virtual functions**: Runtime dispatch - the compiler cannot optimize across the abstraction boundary
- **CRTP**: Compile-time dispatch - full optimization, dead code elimination, constant propagation
- **Impact**: Smaller code size, faster execution

#### 3. **Memory Efficiency**
- **Virtual functions**: Each object needs a vtable pointer (4-8 bytes)
- **CRTP**: No vtable pointer needed
- **Impact**: Critical in memory-constrained systems (many MCUs have <64KB RAM)

#### 4. **Type Safety**
- **Virtual functions**: Runtime errors if method not implemented
- **CRTP**: Compile-time errors if method not implemented
- **Impact**: Catch bugs at compile time, not in the field

### How CRTP Works

```cpp
// Base template class (from pca9685.hpp)
template <typename Derived>
class I2cInterface {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
        // Cast 'this' to Derived* and call the derived implementation
        return static_cast<Derived*>(this)->Write(addr, reg, data, len);
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
        return static_cast<Derived*>(this)->Read(addr, reg, data, len);
    }
};

// Your implementation
class MyI2c : public pca9685::I2cInterface<MyI2c> {
public:
    // This method is called directly (no virtual overhead)
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
        // Your platform-specific I2C code
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
        // Your platform-specific I2C code
    }
};
```

The key insight: `static_cast<Derived*>(this)` allows the base class to call methods on the derived class **at compile time**, not runtime.

### Performance Comparison

| Aspect | Virtual Functions | CRTP |
|--------|------------------|------|
| Function call overhead | ~5-10 cycles | 0 cycles (inlined) |
| Code size | Larger (vtables) | Smaller (optimized) |
| Memory per object | +4-8 bytes (vptr) | 0 bytes |
| Compile-time checks | No | Yes |
| Optimization | Limited | Full |

## Interface Definition

The PCA9685 driver requires you to implement the `I2cInterface` template:

**Location**: [`inc/pca9685.hpp#L57`](../inc/pca9685.hpp#L57)

```cpp
template <typename Derived>
class I2cInterface {
public:
    // Required methods (implement both)
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);
};
```

**Method Requirements**:
- `Write()`: Write `len` bytes from `data` to register `reg` at I2C address `addr` (7-bit address)
- `Read()`: Read `len` bytes into `data` from register `reg` at I2C address `addr` (7-bit address)
- Both return `true` on success, `false` on failure (NACK, timeout, etc.)

## Implementation Steps

### Step 1: Create Your Implementation Class

```cpp
#include "pca9685.hpp"

class MyPlatformI2c : public pca9685::I2cInterface<MyPlatformI2c> {
private:
    // Your platform-specific members
    i2c_handle_t i2c_handle_;
    
public:
    // Constructor
    MyPlatformI2c(i2c_handle_t handle) : i2c_handle_(handle) {}
    
    // Implement required methods (NO virtual keyword!)
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
        // Your I2C write implementation
        return true;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
        // Your I2C read implementation
        return true;
    }
};
```

### Step 2: Platform-Specific Examples

#### ESP32 (ESP-IDF)

**Location**: Based on [`examples/esp32/main/main.cpp`](../examples/esp32/main/main.cpp)

```cpp
#include "driver/i2c.h"
#include "pca9685.hpp"

class Esp32I2cBus : public pca9685::I2cInterface<Esp32I2cBus> {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, reg, true);
        i2c_master_write(cmd, (uint8_t*)data, len, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        return ret == ESP_OK;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, reg, true);
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
        i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        return ret == ESP_OK;
    }
};
```

#### STM32 (HAL)

```cpp
#include "stm32f4xx_hal.h"
#include "pca9685.hpp"

extern I2C_HandleTypeDef hi2c1;

class STM32I2cBus : public pca9685::I2cInterface<STM32I2cBus> {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
        // STM32 HAL uses 8-bit address (7-bit << 1)
        return HAL_I2C_Mem_Write(&hi2c1, addr << 1, reg, 
                                 I2C_MEMADD_SIZE_8BIT, 
                                 (uint8_t*)data, len, 
                                 HAL_MAX_DELAY) == HAL_OK;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
        return HAL_I2C_Mem_Read(&hi2c1, addr << 1, reg,
                                I2C_MEMADD_SIZE_8BIT,
                                data, len,
                                HAL_MAX_DELAY) == HAL_OK;
    }
};
```

#### Arduino

```cpp
#include <Wire.h>
#include "pca9685.hpp"

class ArduinoI2cBus : public pca9685::I2cInterface<ArduinoI2cBus> {
public:
    bool Write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len) {
        Wire.beginTransmission(addr);
        Wire.write(reg);
        Wire.write(data, len);
        return Wire.endTransmission() == 0;
    }
    
    bool Read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len) {
        Wire.beginTransmission(addr);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0) return false;
        
        Wire.requestFrom(addr, len);
        for (size_t i = 0; i < len && Wire.available(); i++) {
            data[i] = Wire.read();
        }
        return true;
    }
};
```

## Common Pitfalls

### ❌ Don't Use Virtual Functions

```cpp
// WRONG - defeats the purpose of CRTP
class MyI2c : public pca9685::I2cInterface<MyI2c> {
public:
    virtual bool Write(...) override {  // ❌ Virtual keyword not needed
        // ...
    }
};
```

### ✅ Correct CRTP Implementation

```cpp
// CORRECT - no virtual keyword
class MyI2c : public pca9685::I2cInterface<MyI2c> {
public:
    bool Write(...) {  // ✅ Direct implementation
        // ...
    }
};
```

### ❌ Don't Forget the Template Parameter

```cpp
// WRONG - missing template parameter
class MyI2c : public pca9685::I2cInterface {  // ❌ Compiler error
    // ...
};
```

### ✅ Correct Template Parameter

```cpp
// CORRECT - pass your class as template parameter
class MyI2c : public pca9685::I2cInterface<MyI2c> {  // ✅
    // ...
};
```

### ❌ Address Format Confusion

The driver uses **7-bit I2C addresses**. Some platforms use 8-bit addresses (7-bit << 1):

```cpp
// WRONG - if your platform uses 8-bit addresses
i2c_write(addr, ...);  // ❌ Should be addr << 1

// CORRECT
i2c_write(addr << 1, ...);  // ✅ Convert 7-bit to 8-bit
```

## Testing Your Implementation

After implementing the interface, test it:

```cpp
MyPlatformI2c i2c;
pca9685::PCA9685<MyPlatformI2c> pwm(&i2c, 0x40);

if (pwm.Reset()) {
    // Interface works!
    pwm.SetPwmFreq(50.0f);
    pwm.SetDuty(0, 0.5f);
} else {
    auto error = pwm.GetLastError();
    // Debug your I2C implementation
}
```

## Next Steps

- See [Configuration](configuration.md) for driver configuration options
- Check [Examples](examples.md) for complete usage examples
- Review [API Reference](api_reference.md) for all available methods

---

**Navigation**
⬅️ [Hardware Setup](hardware_setup.md) | [Next: Configuration ➡️](configuration.md) | [Back to Index](index.md)

