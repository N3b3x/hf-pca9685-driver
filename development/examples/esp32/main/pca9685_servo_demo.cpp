/**
 * @file pca9685_servo_demo.cpp
 * @brief Hobby servo demonstration using PCA9685 16-channel PWM controller
 *
 * Demonstrates smooth, velocity-limited control of up to 16 hobby servos
 * with synchronized animations.  Uses standard servo PWM timing (50 Hz,
 * 1000-2000 µs pulse width).
 *
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │  Servo PWM Math (50 Hz on PCA9685)                                │
 * │                                                                     │
 * │  Period     = 20 000 µs  (1 / 50 Hz)                              │
 * │  Resolution = 4096 ticks per period                                │
 * │  1 tick     ≈ 4.883 µs                                             │
 * │                                                                     │
 * │  1000 µs  →  205 ticks   (0°   / full CCW)                        │
 * │  1500 µs  →  307 ticks   (90°  / center)                          │
 * │  2000 µs  →  410 ticks   (180° / full CW)                         │
 * │                                                                     │
 * │  Typical servo speed: 0.15 s / 60° (no load) → ~400°/s            │
 * │  Conservative limit here: ~260°/s → 6 ticks / 20 ms update        │
 * └─────────────────────────────────────────────────────────────────────┘
 *
 * Animations run in sequence:
 *   1. Home        – all servos to 0° (1000 µs), wait for sync
 *   2. Center      – ramp all to 90° (1500 µs)
 *   3. Wave        – sinusoidal wave travelling across all 16 channels
 *   4. Breathe     – all channels pulsate in unison
 *   5. Cascade     – sequential sweep with staggered start
 *   6. Mirror      – channels 0-7 mirror channels 15-8 (butterfly)
 *   7. Converge    – outer servos sweep inward, inner sweep outward
 *   8. Knight Rider – single highlight sweeps back and forth
 *
 * @author HardFOC Development Team
 * @date 2025
 * @copyright HardFOC
 */

// System headers
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>

// Third-party headers (ESP-IDF)
#ifdef __cplusplus
extern "C" {
#endif
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#ifdef __cplusplus
}
#endif

// Project headers (bus before driver so template sees full Esp32Pca9685I2cBus)
#include "esp32_pca9685_bus.hpp"
#include "pca9685.hpp"

// ============================================================================
// Constants
// ============================================================================

static const char* TAG = "ServoDemo";

using PCA9685Driver = pca9685::PCA9685<Esp32Pca9685I2cBus>;

/// Number of servo channels to use (PCA9685 has 16)
static constexpr uint8_t NUM_SERVOS = 16;

/// Servo PWM frequency (standard for hobby servos)
static constexpr float SERVO_FREQ_HZ = 50.0f;

/// Period in microseconds at 50 Hz
static constexpr float SERVO_PERIOD_US = 20000.0f;

/// Conversion factor: ticks per microsecond
static constexpr float TICKS_PER_US = 4096.0f / SERVO_PERIOD_US; // ≈ 0.2048

// Pulse width limits in microseconds
static constexpr uint16_t SERVO_MIN_US = 1000;    ///< 0°   position
static constexpr uint16_t SERVO_CENTER_US = 1500; ///< 90°  position
static constexpr uint16_t SERVO_MAX_US = 2000;    ///< 180° position

// Pre-computed tick values
static constexpr uint16_t SERVO_MIN_TICKS = 205;    ///< 1000 µs → 205 ticks
static constexpr uint16_t SERVO_CENTER_TICKS = 307; ///< 1500 µs → 307 ticks
static constexpr uint16_t SERVO_MAX_TICKS = 410;    ///< 2000 µs → 410 ticks
static constexpr uint16_t SERVO_RANGE_TICKS = SERVO_MAX_TICKS - SERVO_MIN_TICKS; // 205

/// Update period matches PWM frequency (one new setpoint per PWM cycle)
static constexpr uint32_t UPDATE_PERIOD_MS = 20;

/**
 * Maximum ticks to move per 20 ms update.
 *
 * 6 ticks / 20 ms = 300 ticks/s.  Full range (205 ticks) in ~0.68 s.
 * Corresponds to roughly 260°/s — well within typical servo capability
 * (400°/s no-load) and smooth enough to avoid jerk or mechanical stress.
 */
static constexpr int16_t MAX_TICKS_PER_UPDATE = 6;

/// PCA9685 default I2C address
static constexpr uint8_t PCA9685_I2C_ADDRESS = 0x40;

/// Math constant
static constexpr float PI = 3.14159265358979323846f;

// I2C pin overrides (same mechanism as comprehensive test)
#if defined(PCA9685_EXAMPLE_I2C_SDA_GPIO) && defined(PCA9685_EXAMPLE_I2C_SCL_GPIO)
static constexpr gpio_num_t EXAMPLE_SDA_PIN = (gpio_num_t)PCA9685_EXAMPLE_I2C_SDA_GPIO;
static constexpr gpio_num_t EXAMPLE_SCL_PIN = (gpio_num_t)PCA9685_EXAMPLE_I2C_SCL_GPIO;
#else
static constexpr gpio_num_t EXAMPLE_SDA_PIN = GPIO_NUM_4;
static constexpr gpio_num_t EXAMPLE_SCL_PIN = GPIO_NUM_5;
#endif

// ============================================================================
// Servo Controller
// ============================================================================

/**
 * @class ServoController
 * @brief Velocity-limited multi-channel servo manager.
 *
 * Tracks the current position of each servo in PCA9685 ticks and moves
 * toward a target at a bounded rate every update cycle.  This prevents
 * commanding instantaneous jumps that could stall, strip gears, or
 * draw excessive current.
 */
class ServoController {
public:
  ServoController(PCA9685Driver* driver) : driver_(driver) {
    // Start with all positions at minimum (will be synchronized at boot)
    for (uint8_t i = 0; i < NUM_SERVOS; ++i) {
      current_ticks_[i] = SERVO_MIN_TICKS;
      target_ticks_[i] = SERVO_MIN_TICKS;
    }
  }

  // ---- Target setters ----

  /// Set target for a single channel in PCA9685 ticks (clamped to valid range)
  void SetTargetTicks(uint8_t ch, uint16_t ticks) {
    if (ch >= NUM_SERVOS)
      return;
    target_ticks_[ch] = clampTicks(ticks);
  }

  /// Set target for a single channel in microseconds
  void SetTargetUs(uint8_t ch, uint16_t us) {
    SetTargetTicks(ch, usToTicks(us));
  }

  /// Set all channels to the same target (ticks)
  void SetAllTargetTicks(uint16_t ticks) {
    uint16_t clamped = clampTicks(ticks);
    for (uint8_t i = 0; i < NUM_SERVOS; ++i) {
      target_ticks_[i] = clamped;
    }
  }

  /// Set all channels to the same target (microseconds)
  void SetAllTargetUs(uint16_t us) {
    SetAllTargetTicks(usToTicks(us));
  }

  /// Set target as a normalized position 0.0 (min) to 1.0 (max)
  void SetTargetNormalized(uint8_t ch, float norm) {
    if (ch >= NUM_SERVOS)
      return;
    norm = fmaxf(0.0f, fminf(1.0f, norm));
    target_ticks_[ch] = static_cast<uint16_t>(SERVO_MIN_TICKS + norm * SERVO_RANGE_TICKS + 0.5f);
  }

  /// Set all channels to a normalized position
  void SetAllTargetNormalized(float norm) {
    for (uint8_t i = 0; i < NUM_SERVOS; ++i) {
      SetTargetNormalized(i, norm);
    }
  }

  // ---- State queries ----

  /// Check if all channels have reached their target
  bool AllAtTarget() const {
    for (uint8_t i = 0; i < NUM_SERVOS; ++i) {
      if (current_ticks_[i] != target_ticks_[i])
        return false;
    }
    return true;
  }

  /// Get current position in ticks
  uint16_t GetCurrentTicks(uint8_t ch) const {
    return (ch < NUM_SERVOS) ? current_ticks_[ch] : 0;
  }

  /// Get current position in microseconds
  uint16_t GetCurrentUs(uint8_t ch) const {
    return ticksToUs(GetCurrentTicks(ch));
  }

  // ---- Update loop ----

  /**
   * @brief Advance all channels one step toward their targets.
   *
   * Call this once per UPDATE_PERIOD_MS.  Each channel moves at most
   * MAX_TICKS_PER_UPDATE ticks toward its target, then the new position
   * is written to the PCA9685.
   *
   * @return true if all I2C writes succeeded; false on any failure.
   */
  bool Update() {
    bool all_ok = true;
    for (uint8_t ch = 0; ch < NUM_SERVOS; ++ch) {
      int16_t delta =
          static_cast<int16_t>(target_ticks_[ch]) - static_cast<int16_t>(current_ticks_[ch]);
      if (delta == 0)
        continue;

      // Clamp velocity
      if (delta > MAX_TICKS_PER_UPDATE) {
        delta = MAX_TICKS_PER_UPDATE;
      } else if (delta < -MAX_TICKS_PER_UPDATE) {
        delta = -MAX_TICKS_PER_UPDATE;
      }

      current_ticks_[ch] = static_cast<uint16_t>(static_cast<int16_t>(current_ticks_[ch]) + delta);

      if (!driver_->SetPwm(ch, 0, current_ticks_[ch])) {
        all_ok = false;
      }
    }
    return all_ok;
  }

  /**
   * @brief Immediately write current_ticks_ to hardware (no ramping).
   *
   * Used once at boot to synchronize the physical servo positions
   * with the software state before any animation starts.
   */
  bool ForceWriteAll() {
    bool all_ok = true;
    for (uint8_t ch = 0; ch < NUM_SERVOS; ++ch) {
      if (!driver_->SetPwm(ch, 0, current_ticks_[ch])) {
        all_ok = false;
      }
    }
    return all_ok;
  }

  // ---- Utility ----

  static uint16_t usToTicks(uint16_t us) {
    return static_cast<uint16_t>(static_cast<float>(us) * TICKS_PER_US + 0.5f);
  }

  static uint16_t ticksToUs(uint16_t ticks) {
    return static_cast<uint16_t>(static_cast<float>(ticks) / TICKS_PER_US + 0.5f);
  }

private:
  PCA9685Driver* driver_;
  uint16_t current_ticks_[NUM_SERVOS];
  uint16_t target_ticks_[NUM_SERVOS];

  static uint16_t clampTicks(uint16_t t) {
    if (t < SERVO_MIN_TICKS)
      return SERVO_MIN_TICKS;
    if (t > SERVO_MAX_TICKS)
      return SERVO_MAX_TICKS;
    return t;
  }
};

// ============================================================================
// Animation helpers
// ============================================================================

/**
 * @brief Run the update loop until all servos reach their targets.
 * @param ctrl   Servo controller
 * @param label  Logging label for this movement
 * @param max_ms Maximum time to wait (safety timeout)
 * @return true if all reached target before timeout
 */
static bool ramp_to_target(ServoController& ctrl, const char* label, uint32_t max_ms = 5000) {
  uint32_t elapsed = 0;
  while (!ctrl.AllAtTarget() && elapsed < max_ms) {
    ctrl.Update();
    vTaskDelay(pdMS_TO_TICKS(UPDATE_PERIOD_MS));
    elapsed += UPDATE_PERIOD_MS;
  }
  if (!ctrl.AllAtTarget()) {
    ESP_LOGW(TAG, "  [%s] timeout after %lu ms", label, (unsigned long)elapsed);
    return false;
  }
  return true;
}

/**
 * @brief Run a time-based animation loop.
 *
 * Calls `compute_targets` every update cycle, which should set new targets
 * on the controller.  The controller then ramps toward them respecting the
 * velocity limit.
 *
 * @param ctrl            Servo controller
 * @param duration_ms     Total animation duration
 * @param compute_targets Callback: (controller, elapsed_ms, total_ms)
 */
static void run_animation(ServoController& ctrl, uint32_t duration_ms,
                          void (*compute_targets)(ServoController& ctrl, uint32_t elapsed_ms,
                                                  uint32_t total_ms)) {
  uint32_t elapsed = 0;
  while (elapsed < duration_ms) {
    compute_targets(ctrl, elapsed, duration_ms);
    ctrl.Update();
    vTaskDelay(pdMS_TO_TICKS(UPDATE_PERIOD_MS));
    elapsed += UPDATE_PERIOD_MS;
  }
}

// ============================================================================
// Animation definitions
// ============================================================================

/**
 * @brief Animation 1: Travelling sine wave
 *
 * Each channel's target is a sine function offset by its channel index,
 * creating a wave that appears to move across all 16 channels.
 *
 * Two full waves travel the length of the servo array.
 * The wave moves at 0.5 Hz (one complete cycle every 2 seconds).
 */
static void anim_wave_targets(ServoController& ctrl, uint32_t elapsed_ms, uint32_t /*total_ms*/) {
  float time_s = static_cast<float>(elapsed_ms) / 1000.0f;
  for (uint8_t ch = 0; ch < NUM_SERVOS; ++ch) {
    // Phase offset: each channel is 2π/16 apart (one full wave across 16 channels)
    float phase = 2.0f * PI * (static_cast<float>(ch) / NUM_SERVOS);
    // Temporal frequency: 0.5 Hz → one cycle every 2 seconds
    float angle = 2.0f * PI * 0.5f * time_s - phase;
    float norm = 0.5f + 0.5f * sinf(angle); // 0.0 .. 1.0
    ctrl.SetTargetNormalized(ch, norm);
  }
}

/**
 * @brief Animation 2: Synchronized breathe
 *
 * All 16 channels pulsate in perfect unison from 0° to 180° and back.
 * Frequency: 0.33 Hz (3 seconds per breath cycle).
 */
static void anim_breathe_targets(ServoController& ctrl, uint32_t elapsed_ms,
                                 uint32_t /*total_ms*/) {
  float time_s = static_cast<float>(elapsed_ms) / 1000.0f;
  float norm = 0.5f + 0.5f * sinf(2.0f * PI * 0.33f * time_s);
  ctrl.SetAllTargetNormalized(norm);
}

/**
 * @brief Animation 3: Cascade sweep
 *
 * Each channel sweeps from 0° to 180° and back, but each starts 200 ms
 * after the previous one — creating a waterfall/domino effect.
 */
static void anim_cascade_targets(ServoController& ctrl, uint32_t elapsed_ms,
                                 uint32_t /*total_ms*/) {
  static constexpr uint32_t STAGGER_MS = 200; ///< Delay between each channel's start
  static constexpr uint32_t SWEEP_MS = 2000;  ///< Time for one full sweep (up + down)

  for (uint8_t ch = 0; ch < NUM_SERVOS; ++ch) {
    uint32_t ch_start = ch * STAGGER_MS;
    if (elapsed_ms < ch_start) {
      // Not started yet — hold at min
      ctrl.SetTargetNormalized(ch, 0.0f);
    } else {
      uint32_t ch_elapsed = elapsed_ms - ch_start;
      // Triangle wave: ramp up for half the sweep, ramp down for the other half
      uint32_t phase = ch_elapsed % SWEEP_MS;
      float half = static_cast<float>(SWEEP_MS) / 2.0f;
      float norm;
      if (phase < static_cast<uint32_t>(half)) {
        norm = static_cast<float>(phase) / half;
      } else {
        norm = 2.0f - static_cast<float>(phase) / half;
      }
      ctrl.SetTargetNormalized(ch, norm);
    }
  }
}

/**
 * @brief Animation 4: Mirror / butterfly
 *
 * Channel 0 mirrors channel 15, channel 1 mirrors channel 14, etc.
 * The left half (0-7) runs a sine wave; the right half mirrors it.
 * Creates a symmetric butterfly-wing motion.
 */
static void anim_mirror_targets(ServoController& ctrl, uint32_t elapsed_ms, uint32_t /*total_ms*/) {
  float time_s = static_cast<float>(elapsed_ms) / 1000.0f;

  for (uint8_t i = 0; i < NUM_SERVOS / 2; ++i) {
    float phase = 2.0f * PI * (static_cast<float>(i) / (NUM_SERVOS / 2));
    float norm = 0.5f + 0.5f * sinf(2.0f * PI * 0.4f * time_s - phase);
    ctrl.SetTargetNormalized(i, norm);
    // Mirror partner
    ctrl.SetTargetNormalized(NUM_SERVOS - 1 - i, norm);
  }
}

/**
 * @brief Animation 5: Converge / diverge
 *
 * Outer servos sweep inward while inner servos sweep outward,
 * then reverse — creating a pulsing converge/diverge pattern.
 */
static void anim_converge_targets(ServoController& ctrl, uint32_t elapsed_ms,
                                  uint32_t /*total_ms*/) {
  float time_s = static_cast<float>(elapsed_ms) / 1000.0f;
  float global_phase = sinf(2.0f * PI * 0.3f * time_s); // -1 .. +1

  for (uint8_t ch = 0; ch < NUM_SERVOS; ++ch) {
    // Distance from center (0 at center, 1 at edges)
    float dist = fabsf(static_cast<float>(ch) - 7.5f) / 7.5f;
    // Outer channels move opposite to inner ones
    float norm = 0.5f + 0.5f * global_phase * (2.0f * dist - 1.0f);
    norm = fmaxf(0.0f, fminf(1.0f, norm));
    ctrl.SetTargetNormalized(ch, norm);
  }
}

/**
 * @brief Animation 6: Knight Rider
 *
 * A single "spotlight" servo sweeps to 180° while its neighbors stay
 * near 0°.  The spotlight bounces back and forth across all 16 channels
 * with a smooth Gaussian-like falloff to adjacent channels.
 */
static void anim_knight_targets(ServoController& ctrl, uint32_t elapsed_ms, uint32_t /*total_ms*/) {
  float time_s = static_cast<float>(elapsed_ms) / 1000.0f;

  // Bounce position: triangle wave 0..15..0 at 0.4 Hz
  float cycle_period = 2.5f; // seconds for one full bounce (out and back)
  float phase = fmodf(time_s, cycle_period) / cycle_period; // 0..1
  float pos;
  if (phase < 0.5f) {
    pos = phase * 2.0f * (NUM_SERVOS - 1); // 0 → 15
  } else {
    pos = (1.0f - phase) * 2.0f * (NUM_SERVOS - 1); // 15 → 0
  }

  // Gaussian-like brightness falloff
  static constexpr float SIGMA = 1.5f;
  for (uint8_t ch = 0; ch < NUM_SERVOS; ++ch) {
    float dist = static_cast<float>(ch) - pos;
    float intensity = expf(-(dist * dist) / (2.0f * SIGMA * SIGMA));
    ctrl.SetTargetNormalized(ch, intensity);
  }
}

/**
 * @brief Animation 7: Alternating pairs (walking gait)
 *
 * Even channels (0,2,4,...) move in anti-phase to odd channels (1,3,5,...).
 * Mimics a coordinated walking gait across all 16 channels.
 */
static void anim_walk_targets(ServoController& ctrl, uint32_t elapsed_ms, uint32_t /*total_ms*/) {
  float time_s = static_cast<float>(elapsed_ms) / 1000.0f;
  float norm_even = 0.5f + 0.5f * sinf(2.0f * PI * 0.5f * time_s);
  float norm_odd = 1.0f - norm_even; // Anti-phase

  for (uint8_t ch = 0; ch < NUM_SERVOS; ++ch) {
    ctrl.SetTargetNormalized(ch, (ch % 2 == 0) ? norm_even : norm_odd);
  }
}

/**
 * @brief Animation 8: Multi-speed wave
 *
 * Three sine waves with different frequencies and amplitudes are
 * superimposed, creating an organic, non-repeating motion pattern.
 */
static void anim_organic_targets(ServoController& ctrl, uint32_t elapsed_ms,
                                 uint32_t /*total_ms*/) {
  float time_s = static_cast<float>(elapsed_ms) / 1000.0f;

  for (uint8_t ch = 0; ch < NUM_SERVOS; ++ch) {
    float ch_f = static_cast<float>(ch);
    // Three waves of different frequency/amplitude/direction
    float w1 = 0.4f * sinf(2.0f * PI * 0.3f * time_s - ch_f * 0.4f);
    float w2 = 0.3f * sinf(2.0f * PI * 0.71f * time_s + ch_f * 0.25f);
    float w3 = 0.2f * sinf(2.0f * PI * 1.13f * time_s - ch_f * 0.6f);
    float norm = 0.5f + w1 + w2 + w3;
    norm = fmaxf(0.0f, fminf(1.0f, norm));
    ctrl.SetTargetNormalized(ch, norm);
  }
}

// ============================================================================
// Animation table
// ============================================================================

struct AnimationEntry {
  const char* name;
  void (*compute_targets)(ServoController&, uint32_t, uint32_t);
  uint32_t duration_ms;
  const char* description;
};

static const AnimationEntry ANIMATIONS[] = {
    {"Wave", anim_wave_targets, 10000, "Travelling sine wave across 16 channels"},
    {"Breathe", anim_breathe_targets, 9000, "All channels pulsate in unison"},
    {"Cascade", anim_cascade_targets, 10000, "Staggered waterfall sweep"},
    {"Mirror", anim_mirror_targets, 10000, "Butterfly: left half mirrors right"},
    {"Converge", anim_converge_targets, 10000, "Outer vs inner: converge/diverge"},
    {"KnightRider", anim_knight_targets, 10000, "Bouncing spotlight with falloff"},
    {"Walk", anim_walk_targets, 8000, "Alternating even/odd pairs (gait)"},
    {"Organic", anim_organic_targets, 12000, "Multi-frequency superimposed waves"},
};

static constexpr size_t NUM_ANIMATIONS = sizeof(ANIMATIONS) / sizeof(ANIMATIONS[0]);

// ============================================================================
// Initialization
// ============================================================================

static std::unique_ptr<Esp32Pca9685I2cBus> g_bus;
static std::unique_ptr<PCA9685Driver> g_driver;

static bool init_hardware() {
  Esp32Pca9685I2cBus::I2CConfig config;
  config.port = I2C_NUM_0;
  config.sda_pin = EXAMPLE_SDA_PIN;
  config.scl_pin = EXAMPLE_SCL_PIN;
  config.frequency = 100000;
  config.pullup_enable = true;

  g_bus = CreateEsp32Pca9685I2cBus(config);
  if (!g_bus || !g_bus->IsInitialized()) {
    ESP_LOGE(TAG, "Failed to initialize I2C bus");
    return false;
  }

  g_driver = std::make_unique<PCA9685Driver>(g_bus.get(), PCA9685_I2C_ADDRESS);
  g_driver->SetRetryDelay(Esp32Pca9685I2cBus::RetryDelay);
  if (!g_driver->EnsureInitialized()) {
    ESP_LOGE(TAG, "Failed to initialize PCA9685 at address 0x%02X", PCA9685_I2C_ADDRESS);
    return false;
  }

  // Set totem-pole output mode (standard for direct servo drive)
  if (!g_driver->SetOutputDriverMode(true)) {
    ESP_LOGW(TAG, "Failed to set totem-pole output mode");
  }

  // Set PWM frequency to 50 Hz (standard servo)
  if (!g_driver->SetPwmFreq(SERVO_FREQ_HZ)) {
    ESP_LOGE(TAG, "Failed to set PWM frequency to %.0f Hz", SERVO_FREQ_HZ);
    return false;
  }

  ESP_LOGI(TAG, "PCA9685 initialized: 50 Hz, totem-pole, address 0x%02X", PCA9685_I2C_ADDRESS);
  return true;
}

// ============================================================================
// Main
// ============================================================================

extern "C" void app_main() {
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════════════════════╗");
  ESP_LOGI(TAG, "║              PCA9685 HOBBY SERVO DEMONSTRATION (16 channels)                 ║");
  ESP_LOGI(TAG, "║                                                                              ║");
  ESP_LOGI(TAG, "║  Pulse range : 1000 µs (0°) → 2000 µs (180°)                                 ║");
  ESP_LOGI(TAG, "║  PWM freq    : 50 Hz   (20 ms period)                                        ║");
  ESP_LOGI(TAG, "║  Max velocity: ~260°/s (6 ticks / 20 ms update)                              ║");
  ESP_LOGI(TAG, "║                                                                              ║");
  ESP_LOGI(TAG, "║  This demo shows a variety of servo animations that can be run in sequence.  ║");
  ESP_LOGI(TAG, "║  The animations are:                                                         ║");
  ESP_LOGI(TAG, "║                                                                              ║");
  ESP_LOGI(TAG, "║  1. Wave         - Travelling sine wave across all 16 channels               ║");
  ESP_LOGI(TAG, "║  2. Breathe      - All channels pulsate in unison                            ║");
  ESP_LOGI(TAG, "║  3. Cascade      - Sequential sweep with staggered start                     ║");
  ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════════════════════╝");
  ESP_LOGI(TAG, "");

  vTaskDelay(pdMS_TO_TICKS(500));

  // ---- Hardware init ----
  if (!init_hardware()) {
    ESP_LOGE(TAG, "Hardware initialization failed.  Halting.");
    while (true) {
      vTaskDelay(pdMS_TO_TICKS(10000));
    }
  }

  ServoController ctrl(g_driver.get());

  // ========================================================================
  // Phase 0: Synchronize all servos to home position (1000 µs / 0°)
  // ========================================================================
  ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════════════════════╗");
  ESP_LOGI(TAG, "║  Phase 0: HOMING — moving all servos to 0° (1000 µs)                         ║");
  ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════════════════════╝");

  // Immediately write minimum pulse to all channels so every servo starts
  // moving to the home position right now (no ramping for this first write —
  // the servos are in an unknown state and need a known starting point).
  ctrl.ForceWriteAll();

  // Wait for all servos to physically reach their home position.
  // Worst case: a servo was at 180° (2000 µs) and needs to travel 180°.
  // At typical 400°/s that takes ~0.45 s.  We wait 2 s for margin and to
  // let any oscillation settle.
  ESP_LOGI(TAG, "  Waiting 2 s for servos to reach home position...");
  vTaskDelay(pdMS_TO_TICKS(2000));
  ESP_LOGI(TAG, "  ✅ All servos homed at 1000 µs (0°)");

  // ========================================================================
  // Phase 1: Ramp to center (1500 µs / 90°) — first smooth move
  // ========================================================================
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════════════════════╗");
  ESP_LOGI(TAG, "║  Phase 1: CENTER — ramping all to 90° (1500 µs)                              ║");
  ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════════════════════╝");

  ctrl.SetAllTargetUs(SERVO_CENTER_US);
  ramp_to_target(ctrl, "center");
  ESP_LOGI(TAG,
           "  ✅ All servos at center (1500 µs).  "
           "Position tracked: %d µs",
           ctrl.GetCurrentUs(0));
  vTaskDelay(pdMS_TO_TICKS(1000));

  // ========================================================================
  // Phase 2: Full range check — sweep min → max → min
  // ========================================================================
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════════════════════╗");
  ESP_LOGI(TAG, "║  Phase 2: RANGE CHECK — full sweep 0° → 180° → 0°                            ║");
  ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════════════════════╝");

  // Sweep to max
  ctrl.SetAllTargetUs(SERVO_MAX_US);
  ramp_to_target(ctrl, "sweep-max");
  ESP_LOGI(TAG, "  All at 180° (%d µs)", ctrl.GetCurrentUs(0));
  vTaskDelay(pdMS_TO_TICKS(500));

  // Sweep back to min
  ctrl.SetAllTargetUs(SERVO_MIN_US);
  ramp_to_target(ctrl, "sweep-min");
  ESP_LOGI(TAG, "  All at 0° (%d µs)", ctrl.GetCurrentUs(0));
  vTaskDelay(pdMS_TO_TICKS(500));

  // Return to center for animation start
  ctrl.SetAllTargetUs(SERVO_CENTER_US);
  ramp_to_target(ctrl, "return-center");
  ESP_LOGI(TAG, "  ✅ Range check complete.  Servos at center.");
  vTaskDelay(pdMS_TO_TICKS(500));

  // ========================================================================
  // Phase 3: Run all animations in sequence, looping forever
  // ========================================================================
  ESP_LOGI(TAG, "");
  ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════════════════════╗");
  ESP_LOGI(TAG, "║  Phase 3: ANIMATIONS — %d patterns, looping forever                          ║",
           (int)NUM_ANIMATIONS);
  ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════════════════════╝");

  uint32_t loop_count = 0;
  while (true) {
    ++loop_count;
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    ESP_LOGI(TAG, "  Animation loop #%lu", (unsigned long)loop_count);
    ESP_LOGI(TAG, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

    for (size_t i = 0; i < NUM_ANIMATIONS; ++i) {
      const AnimationEntry& anim = ANIMATIONS[i];

      ESP_LOGI(TAG, "");
      ESP_LOGI(TAG, "  ┌── [%d/%d] %s  (%lu s)", (int)(i + 1), (int)NUM_ANIMATIONS, anim.name,
               (unsigned long)(anim.duration_ms / 1000));
      ESP_LOGI(TAG, "  │   %s", anim.description);

      // Run the animation
      run_animation(ctrl, anim.duration_ms, anim.compute_targets);

      ESP_LOGI(TAG, "  └── %s complete", anim.name);

      // After each animation, smoothly return to center before the next one.
      // This gives a clean starting state and a brief visual pause.
      ctrl.SetAllTargetUs(SERVO_CENTER_US);
      ramp_to_target(ctrl, "transition");
      vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "  All %d animations completed.  Restarting in 2 s...", (int)NUM_ANIMATIONS);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
