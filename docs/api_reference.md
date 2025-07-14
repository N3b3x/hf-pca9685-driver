[⬅️ Back to Index](index.md) | [Next: Configuration Guide ➡️](configuration.md)

# PCA9685 Driver API Reference

---

## Overview

The `PCA9685` class provides a robust, hardware-agnostic interface to the PCA9685 PWM controller. It requires only a user-supplied I2C implementation and is suitable for any platform.

---

## Main Methods

| Method | Description |
|--------|-------------|
| `reset()` | Reset the device to its power-on default state |
| `setPwmFreq(float freq_hz)` | Set the PWM frequency |
| `setPwm(uint8_t channel, uint16_t on, uint16_t off)` | Set PWM timing for a channel |
| `setDuty(uint8_t channel, float duty)` | Set duty cycle (0.0–1.0) for a channel |
| `setAllPwm(uint16_t on, uint16_t off)` | Set all channels at once |
| `getLastError()` | Get the last error code |
| `getPrescale(uint8_t &prescale)` | Get the current prescale value |
| `setOutputEnable(bool enabled)` | (Stub) Set output enable state |

---

## Usage Example

```cpp
#include "pca9685.hpp"
// ...
MyI2cBus i2c;
PCA9685 pwm(&i2c, 0x40);
if (!pwm.reset()) { /* handle error */ }
pwm.setPwmFreq(50.0f); // 50 Hz for servos
pwm.setPwm(0, 0, 2048); // 50% duty on channel 0
```

---

## Error Handling

Most methods return `true` on success, `false` on failure. Use `getLastError()` to retrieve the error code:
- `None`: Success
- `I2cWrite`: I2C write failure
- `I2cRead`: I2C read failure
- `InvalidParam`: Invalid argument
- `DeviceNotFound`: Device not found
- `NotInitialized`: Device not initialized
- `OutOfRange`: Parameter out of range

---

## Integration Tips

- Implement the `I2cBus` interface for your platform (see [Examples](../examples/README.md)).
- Call `reset()` before using other methods.
- Use `setPwmFreq()` before setting channel outputs.
- For multi-chip setups, configure unique I2C addresses.
- See [Configuration Guide](configuration.md) for advanced features.

---

## See Also
- [⬅️ Back to Index](index.md)
- [Next: Configuration Guide ➡️](configuration.md)
