# PCA9685 Hobby Servo Demo

The **pca9685_servo_demo** application drives up to 16 hobby servos from the PCA9685 with standard 1000–2000 µs pulse width, velocity-limited motion, and synchronized animations.

## Servo Timing (50 Hz)

- **PWM frequency**: 50 Hz (20 ms period), standard for hobby servos.
- **Pulse width**: 1000 µs (≈0°) to 2000 µs (≈180°). Center ≈1500 µs (≈90°).
- **PCA9685**: 4096 ticks per period → 1 tick ≈ 4.88 µs.  
  - 1000 µs → 205 ticks, 1500 µs → 307 ticks, 2000 µs → 410 ticks.

## Velocity Limiting

Servos cannot jump instantly. The demo tracks a **current position** per channel and moves toward a **target** at a bounded rate:

- **Max step**: 6 ticks per 20 ms update (~260°/s).
- Full 0°→180° sweep in ~0.68 s, safe for typical hobby servos.

All moves are ramped from the current setpoint; no instantaneous jumps.

## Startup Sequence

1. **Home**: All channels are set to 1000 µs (0°); 2 s wait for servos to physically align.
2. **Center**: All ramp to 1500 µs (90°).
3. **Range check**: All sweep to 2000 µs, then 1000 µs, then back to 1500 µs.
4. **Animations**: A fixed set of animations runs in a loop.

## Animations (in order, looping)

| Name        | Description |
|------------|-------------|
| **Wave**   | Travelling sine wave across all 16 channels. |
| **Breathe**| All 16 channels pulsate in unison (0°↔180°). |
| **Cascade**| Staggered sweep: each channel starts 200 ms after the previous. |
| **Mirror** | Left half mirrors right (butterfly). |
| **Converge** | Outer vs inner channels: converge/diverge. |
| **Knight Rider** | Single “spotlight” sweeps back and forth with falloff. |
| **Walk**   | Even/odd channels in anti-phase (gait). |
| **Organic**| Multi-frequency superimposed waves. |

Between animations, all channels return to center (1500 µs).

## Source and Configuration

- **Source**: `main/pca9685_servo_demo.cpp`
- **I2C**: Same as comprehensive test (default SDA=GPIO4, SCL=GPIO5; `Esp32Pca9685Bus`).
- **Address**: **0x40** (change in source if needed).
- **Channels**: 16 (OUT0–OUT15). Connect servos to PCA9685 outputs; ensure separate power for servos if needed.

## Build and Run

From `examples/esp32`:

```bash
./scripts/build_app.sh pca9685_servo_demo Debug
./scripts/flash_app.sh flash_monitor pca9685_servo_demo Debug
```

The demo runs indefinitely; exit the monitor with Ctrl+].

## Using the Driver for Servos in Your Code

1. Set **50 Hz**: `pwm.SetPwmFreq(50.0f);`
2. Use **pulse width**, not raw duty 0–1:  
   - 1000 µs → `SetPwm(ch, 0, 205)` (or use a helper that converts µs to ticks).  
   - The demo uses a `ServoController` class that converts 1000–2000 µs to ticks and applies velocity limiting; you can reuse or adapt that logic.
3. Avoid commanding large instantaneous changes; step or ramp toward the target to match servo speed limits.

## Relation to the API

The demo uses:

- `SetPwmFreq(50.0f)`
- `SetOutputDriverMode(true)` (totem-pole)
- `SetPwm(channel, 0, off_ticks)` for each channel every 20 ms

See [../../../docs/api_reference.md](../../../docs/api_reference.md) for full method descriptions.
