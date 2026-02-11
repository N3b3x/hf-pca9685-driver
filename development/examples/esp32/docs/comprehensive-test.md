# PCA9685 Comprehensive Test Suite

The **pca9685_comprehensive_test** application runs a full set of tests against the PCA9685 driver and hardware. Use it to verify wiring, I2C address, and all driver features.

## What It Tests

### Test Sections (12 tasks total)

1. **Initialization**
   - **i2c_bus_initialization**: I2C bus init (SDA/SCL, 100 kHz).
   - **driver_initialization**: Driver creation and `Reset()`.

2. **Frequency**
   - **pwm_frequency**: Set 50, 100, 200, 500, 1000 Hz; check invalid 10 Hz / 2000 Hz handling.

3. **PWM**
   - **channel_pwm**: Set 50% duty on each of the 16 channels.

4. **Duty cycle**
   - **duty_cycle**: Sweep duty on all channels; clamp test (negative / >1.0).
   - **all_channel_control**: `SetAllPwm`, `SetChannelFullOn`, `SetChannelFullOff` on all 16.
   - **prescale_readback**: Set frequency, read prescale, verify (50, 200, 1000, 24, 1526 Hz).
   - **sleep_wake**: `Sleep()` / `Wake()` and PWM after wake; 5 sleep/wake cycles.
   - **output_config**: `SetOutputInvert`, `SetOutputDriverMode` (totem-pole / open-drain).

5. **Error handling**
   - **error_handling**: Invalid channel (255), invalid PWM (5000), invalid frequency (5 Hz); `GetLastError`, `ClearError`, `HasAnyError`.

6. **Stress**
   - **stress_rapid_writes**: Many back-to-back `SetDuty` and `SetAllPwm` and frequency changes.
   - **stress_boundary_values**: Min/max/staggered PWM on all channels; FullOn to FullOff to PWM; 5 reset cycles.

### I2C bus scan

If the first attempt to connect to the PCA9685 at the configured address fails, the app runs an **I2C bus scan** (all 0x08 to 0x77) and logs which addresses respond. Use this to confirm the device address and bus.

## Source and Configuration

- **Source**: `main/pca9685_comprehensive_test.cpp`
- **I2C bus**: `main/esp32_pca9685_bus.hpp` (default SDA=GPIO4, SCL=GPIO5, 100 kHz)
- **Address**: `PCA9685_I2C_ADDRESS` (default **0x40**)
- **Sections**: Enable/disable via `ENABLE_*_TESTS` defines at the top of the test file.

## Build and Run

From `examples/esp32`:

```bash
./scripts/build_app.sh pca9685_comprehensive_test Debug
./scripts/flash_app.sh flash_monitor pca9685_comprehensive_test Debug
```

Expected: all 12 tests pass, summary shows **Total: 12, Passed: 12, Failed: 0**. GPIO14 is used as a progress indicator.

## Customization

- **I2C pins**: Build with CMake override, e.g.  
  `-DPCA9685_EXAMPLE_I2C_SDA_GPIO=8 -DPCA9685_EXAMPLE_I2C_SCL_GPIO=9`
- **Address**: Change `PCA9685_I2C_ADDRESS` in the test file to match A0 to A5.
- **Sections**: Set `ENABLE_*_TESTS` to `false` to skip a section.

## Relation to the API

The test exercises the public API as in the main docs API reference:

- Init: `EnsureInitialized()`, `Reset()`, `IsInitialized()`
- Frequency: `SetPwmFreq()`, `GetPrescale()`
- PWM: `SetPwm()`, `SetDuty()`, `SetAllPwm()`, `SetChannelFullOn()`, `SetChannelFullOff()`
- Power: `Sleep()`, `Wake()`
- Output: `SetOutputInvert()`, `SetOutputDriverMode()`
- Errors: `GetLastError()`, `GetErrorFlags()`, `HasError()`, `HasAnyError()`, `ClearError()`, `ClearErrorFlags()`

See `../../../docs/api_reference.md` for full method list.
