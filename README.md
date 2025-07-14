# hf-pca9685-driver

Hardware-agnostic C++ driver for the NXP PCA9685 16-channel 12-bit PWM controller.

## Features
- Platform-independent: requires only a user-implemented I2cBus interface
- No dependencies on project-specific or MCU-specific code
- Set PWM frequency (24 Hz to 1526 Hz typical)
- Set PWM value (on/off time) for each channel
- Reset and configure device
- Error reporting and diagnostics

## Usage

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

## Integration
- For use in larger projects, wrap this driver in a component handler that implements your project's PWM abstraction (e.g., BasePwm).
- See Doxygen documentation for full API details.

## Documentation
- Run `doxygen` in the project root to generate HTML docs in `docs/`.

## License
MIT
