---
layout: default
title: "💡 Examples"
description: "Complete example walkthroughs for the PCA9685 driver"
nav_order: 7
parent: "📚 Documentation"
permalink: /docs/examples/
---

# Examples

This guide provides complete, working examples demonstrating various use cases for the PCA9685 driver.

## Example 1: Basic Servo Control

This example shows how to control a standard servo motor using the PCA9685.

```cpp
#include "pca9685.hpp"
#include "driver/i2c.h"

// ESP32 I2C implementation
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

extern "C" void app_main() {
    // Initialize I2C (not shown - use your platform's I2C init)
    
    Esp32I2cBus i2c;
    pca9685::PCA9685<Esp32I2cBus> pwm(&i2c, 0x40);
    
    // Initialize
    if (!pwm.Reset()) {
        printf("Failed to initialize PCA9685!\n");
        return;
    }
    
    // Set frequency for servos (50 Hz)
    pwm.SetPwmFreq(50.0f);
    
    // Move servo to center position (1.5ms pulse = 7.5% duty at 50 Hz)
    pwm.SetDuty(0, 0.075f);
    
    // Move servo to 90 degrees (2.0ms pulse = 10% duty)
    pwm.SetDuty(0, 0.10f);
    
    // Move servo to 0 degrees (1.0ms pulse = 5% duty)
    pwm.SetDuty(0, 0.05f);
    
    printf("Servo control example complete.\n");
}
```

### Explanation

1. **I2C Interface**: Implement `I2cInterface` for your platform (ESP32 example shown)
2. **Initialization**: Call `Reset()` to initialize the device
3. **Frequency**: Set 50 Hz for standard servos (20ms period)
4. **Duty Cycle**: Use `SetDuty()` with values 0.05-0.10 for typical servo range
   - 0.05 = 1.0ms pulse (0 degrees)
   - 0.075 = 1.5ms pulse (center/90 degrees)
   - 0.10 = 2.0ms pulse (180 degrees)

### Expected Output

The servo should move to different positions as the duty cycle changes.

---

## Example 2: LED Dimming

This example demonstrates smooth LED dimming using PWM.

```cpp
#include "pca9685.hpp"

// ... I2C implementation (same as Example 1) ...

void fade_led(pca9685::PCA9685<Esp32I2cBus>& pwm, uint8_t channel) {
    // Set high frequency for smooth dimming (1 kHz)
    pwm.SetPwmFreq(1000.0f);
    
    // Fade in
    for (float duty = 0.0f; duty <= 1.0f; duty += 0.01f) {
        pwm.SetDuty(channel, duty);
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay
    }
    
    // Fade out
    for (float duty = 1.0f; duty >= 0.0f; duty -= 0.01f) {
        pwm.SetDuty(channel, duty);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

extern "C" void app_main() {
    Esp32I2cBus i2c;
    pca9685::PCA9685<Esp32I2cBus> pwm(&i2c, 0x40);
    
    pwm.Reset();
    
    // Fade LED on channel 0
    fade_led(pwm, 0);
}
```

### Explanation

1. **High Frequency**: Use 1 kHz for smooth LED dimming (avoids visible flicker)
2. **Duty Cycle Sweep**: Gradually change duty from 0.0 to 1.0 for fade effect
3. **Timing**: Small delays between updates create smooth transitions

---

## Example 3: Multiple Channels

This example shows controlling multiple channels simultaneously.

```cpp
#include "pca9685.hpp"

// ... I2C implementation ...

extern "C" void app_main() {
    Esp32I2cBus i2c;
    pca9685::PCA9685<Esp32I2cBus> pwm(&i2c, 0x40);
    
    pwm.Reset();
    pwm.SetPwmFreq(1000.0f); // 1 kHz for LEDs
    
    // Set different duty cycles for different channels
    pwm.SetDuty(0, 0.25f);  // Channel 0: 25% brightness
    pwm.SetDuty(1, 0.50f);  // Channel 1: 50% brightness
    pwm.SetDuty(2, 0.75f);  // Channel 2: 75% brightness
    pwm.SetDuty(3, 1.0f);   // Channel 3: 100% brightness
    
    // Set all remaining channels to 50%
    for (uint8_t ch = 4; ch < 16; ch++) {
        pwm.SetDuty(ch, 0.5f);
    }
}
```

### Explanation

Each channel operates independently. You can set different duty cycles for each of the 16 channels.

---

## Example 4: Error Handling

This example demonstrates proper error handling.

```cpp
#include "pca9685.hpp"

// ... I2C implementation ...

extern "C" void app_main() {
    Esp32I2cBus i2c;
    pca9685::PCA9685<Esp32I2cBus> pwm(&i2c, 0x40);
    
    // Initialize with error checking
    if (!pwm.Reset()) {
        auto error = pwm.GetLastError();
        printf("Reset failed: error code %d\n", static_cast<int>(error));
        return;
    }
    
    // Set frequency with validation
    if (!pwm.SetPwmFreq(50.0f)) {
        auto error = pwm.GetLastError();
        if (error == pca9685::PCA9685<Esp32I2cBus>::Error::OutOfRange) {
            printf("Frequency out of range!\n");
        } else if (error == pca9685::PCA9685<Esp32I2cBus>::Error::I2cWrite) {
            printf("I2C write failed!\n");
        }
        return;
    }
    
    // Set channel with error checking
    if (!pwm.SetDuty(0, 0.5f)) {
        auto error = pwm.GetLastError();
        printf("SetDuty failed: error code %d\n", static_cast<int>(error));
    }
}
```

### Explanation

Always check return values and use `GetLastError()` to diagnose issues. This helps identify hardware problems, configuration errors, or I2C communication failures.

---

## Example 5: Bulk Channel Update

This example shows efficient bulk updates using `SetAllPwm()`.

```cpp
#include "pca9685.hpp"

// ... I2C implementation ...

extern "C" void app_main() {
    Esp32I2cBus i2c;
    pca9685::PCA9685<Esp32I2cBus> pwm(&i2c, 0x40);
    
    pwm.Reset();
    pwm.SetPwmFreq(1000.0f);
    
    // Turn all channels on to 50% simultaneously
    pwm.SetAllPwm(0, 2048);
    
    // Turn all channels off
    pwm.SetAllPwm(0, 0);
    
    // Turn all channels fully on
    pwm.SetAllPwm(0, 4095);
}
```

### Explanation

`SetAllPwm()` is more efficient than calling `SetPwm()` 16 times. It writes to the ALL_LED registers, updating all channels in a single I2C transaction.

---

## Running the Examples

### ESP32

The examples are available in the [`examples/esp32`](../examples/esp32/) directory.

```bash
cd examples/esp32
idf.py build flash monitor
```

### Other Platforms

Adapt the I2C interface implementation for your platform (see [Platform Integration](platform_integration.md)) and compile with your platform's toolchain.

## Next Steps

- Review the [API Reference](api_reference.md) for method details
- Check [Troubleshooting](troubleshooting.md) if you encounter issues
- Explore the [examples directory](../examples/) for more examples

---

**Navigation**
⬅️ [API Reference](api_reference.md) | [Next: Troubleshooting ➡️](troubleshooting.md) | [Back to Index](index.md)

