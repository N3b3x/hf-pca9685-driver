[⬅️ Back to Index](index.md) | [Prev: API Reference](api_reference.md) | [Next: Contributing ➡️](contributing.md)

# PCA9685 Configuration Guide

---

## Device Address

| Pin | Address Bit |
|-----|-------------|
| A0  | 0           |
| A1  | 1           |
| A2  | 2           |
| A3  | 3           |
| A4  | 4           |
| A5  | 5           |

Default I2C address: `0x40` (can be changed via A0–A5 pins)

---

## Register Map (Summary)

| Register | Purpose |
|----------|---------|
| `MODE1`  | Main config (reset, sleep, auto-increment) |
| `MODE2`  | Output config (open-drain, totem-pole, inversion) |
| `PRE_SCALE` | PWM frequency setting |
| `LEDn_ON`, `LEDn_OFF` | Per-channel PWM control |

See the [datasheet](../datasheet/) for the full register map.

---

## Setting PWM Frequency

- Use `setPwmFreq(float freq_hz)`
- Valid range: ~24 Hz to 1526 Hz
- **Formula:**

  ```
  prescale = round(25,000,000 / (4096 * freq_hz)) - 1
  ```

- Example: For 50 Hz (servos):

  ```
  prescale = round(25,000,000 / (4096 * 50)) - 1 = 121
  ```

---

## Setting PWM Output

- Use `setPwm(channel, on, off)` for individual channels
- Use `setAllPwm(on, off)` for all channels
- Use `setDuty(channel, duty)` for floating-point duty cycle (0.0–1.0)
- `on` and `off` are 12-bit values (0–4095)

---

## Advanced Features

- Output inversion, open-drain/TTL, and more via `MODE2`
- Sleep mode for low power
- All-call, subaddressing, and software reset

---

## Real-World Example

```cpp
pwm.setPwmFreq(1000.0f); // 1 kHz for LEDs
for (uint8_t ch = 0; ch < 16; ++ch) {
    pwm.setDuty(ch, 0.5f); // 50% duty on all channels
}
```

---

## See Also
- [⬅️ Back to Index](index.md)
- [Prev: API Reference](api_reference.md)
- [Next: Contributing ➡️](contributing.md)
