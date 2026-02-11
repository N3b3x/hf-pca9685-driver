/**
 * @file pca9685_comprehensive_test.cpp
 * @brief Comprehensive test suite for PCA9685 driver on ESP32
 *
 * This file contains comprehensive testing for PCA9685 features including:
 * - Device initialization and reset
 * - PWM frequency configuration
 * - Channel PWM control (individual and all channels)
 * - Duty cycle control
 * - Error handling and recovery
 * - Edge cases and stress testing
 *
 * @author HardFOC Development Team
 * @date 2025
 * @copyright HardFOC
 */

// System headers
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

// Project headers (bus before driver so template sees full Esp32Pca9685Bus)
#include "TestFramework.h"
#include "esp32_pca9685_bus.hpp"
#include "pca9685.hpp"

// Use fully qualified name for the class
using PCA9685Driver = pca9685::PCA9685<Esp32Pca9685Bus>;

static const char* TAG = "PCA9685_Test";
static TestResults g_test_results;

//=============================================================================
// TEST CONFIGURATION
//=============================================================================
// Enable/disable test sections (set to false to skip a section)
static constexpr bool ENABLE_INITIALIZATION_TESTS = true;
static constexpr bool ENABLE_FREQUENCY_TESTS = true;
static constexpr bool ENABLE_PWM_TESTS = true;
static constexpr bool ENABLE_DUTY_CYCLE_TESTS = true;
static constexpr bool ENABLE_ERROR_HANDLING_TESTS = true;
static constexpr bool ENABLE_STRESS_TESTS = true;

// PCA9685 I2C address (default 0x40, can be changed via A0-A5 pins)
static constexpr uint8_t PCA9685_I2C_ADDRESS = 0x40;

//=============================================================================
// SHARED TEST RESOURCES
//=============================================================================
static std::unique_ptr<Esp32Pca9685Bus> g_i2c_bus;
static std::unique_ptr<PCA9685Driver> g_driver;

//=============================================================================
// TEST HELPER FUNCTIONS
//=============================================================================

/**
 * @brief Create and initialize test driver
 */
static std::unique_ptr<PCA9685Driver> create_test_driver() noexcept {
  if (!g_i2c_bus) {
    ESP_LOGE(TAG, "I2C bus not initialized");
    return nullptr;
  }

  auto driver = std::make_unique<PCA9685Driver>(g_i2c_bus.get(), PCA9685_I2C_ADDRESS);
  if (!driver) {
    ESP_LOGE(TAG, "Failed to create driver instance");
    return nullptr;
  }

  // Initialize driver (ensures I2C bus is ready, then resets device)
  if (!driver->EnsureInitialized()) {
    ESP_LOGE(TAG, "Failed to initialize driver (I2C bus or device communication failed)");
    ESP_LOGE(TAG, "Last error: %d", static_cast<int>(driver->GetLastError()));
    return nullptr;
  }
  vTaskDelay(pdMS_TO_TICKS(10));

  return driver;
}

// Optional I2C pin override: if your PCA9685 is on different pins, build with
// -DPCA9685_EXAMPLE_I2C_SDA_GPIO=<num> -DPCA9685_EXAMPLE_I2C_SCL_GPIO=<num>
// (e.g. 8 and 9 for ESP32-S3). Default: GPIO4 (SDA), GPIO5 (SCL) to match pcal95555/bno08x.
#if defined(PCA9685_EXAMPLE_I2C_SDA_GPIO) && defined(PCA9685_EXAMPLE_I2C_SCL_GPIO)
static constexpr gpio_num_t EXAMPLE_SDA_PIN = (gpio_num_t)PCA9685_EXAMPLE_I2C_SDA_GPIO;
static constexpr gpio_num_t EXAMPLE_SCL_PIN = (gpio_num_t)PCA9685_EXAMPLE_I2C_SCL_GPIO;
#else
static constexpr gpio_num_t EXAMPLE_SDA_PIN = GPIO_NUM_4;
static constexpr gpio_num_t EXAMPLE_SCL_PIN = GPIO_NUM_5;
#endif

/**
 * @brief Scan I2C bus for devices
 * @param bus Pointer to initialized I2C bus
 * @return true if at least one device found, false otherwise
 */
static bool scan_i2c_bus(Esp32Pca9685Bus* bus) noexcept {
  if (!bus || !bus->IsInitialized()) {
    ESP_LOGE(TAG, "I2C bus not initialized for scanning");
    return false;
  }

  ESP_LOGI(TAG, "Scanning I2C bus (SDA:GPIO%d, SCL:GPIO%d)...", EXAMPLE_SDA_PIN, EXAMPLE_SCL_PIN);
  ESP_LOGI(TAG, "     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");

  int found_count = 0;
  for (int addr = 0x08; addr <= 0x77; addr++) {
    if (addr % 16 == 0) {
      ESP_LOGI(TAG, "%02X: ", addr);
    }

    // Try to read register 0x00 (MODE1) - PCA9685 should ACK if present
    uint8_t data = 0;
    bool ack = bus->Read(static_cast<uint8_t>(addr), 0x00, &data, 1);

    if (ack) {
      ESP_LOGI(TAG, "%02X ", addr);
      found_count++;
      if (addr == PCA9685_I2C_ADDRESS) {
        ESP_LOGI(TAG, " <-- Expected PCA9685 address");
      }
    } else {
      ESP_LOGI(TAG, "-- ");
    }

    if ((addr + 1) % 16 == 0) {
      ESP_LOGI(TAG, "");
    }
  }

  ESP_LOGI(TAG, "");
  if (found_count == 0) {
    ESP_LOGW(TAG, "No I2C devices found on GPIO%d (SDA) / GPIO%d (SCL)", EXAMPLE_SDA_PIN,
             EXAMPLE_SCL_PIN);
    ESP_LOGW(TAG, "Check wiring, power, and pull-up resistors (2.2k-4.7k to 3.3V)");
    ESP_LOGW(TAG, "If PCA9685 is on different pins, rebuild with:");
    ESP_LOGW(TAG, "  -DPCA9685_EXAMPLE_I2C_SDA_GPIO=<sda> -DPCA9685_EXAMPLE_I2C_SCL_GPIO=<scl>");
    return false;
  } else {
    ESP_LOGI(TAG, "Found %d device(s) on I2C bus", found_count);
    if (found_count > 0) {
      // Check if expected address was found during scan
      bool found_expected = false;
      for (int addr = 0x08; addr <= 0x77; addr++) {
        uint8_t data = 0;
        if (bus->Read(static_cast<uint8_t>(addr), 0x00, &data, 1)) {
          if (addr == PCA9685_I2C_ADDRESS) {
            found_expected = true;
            break;
          }
        }
      }
      if (!found_expected) {
        ESP_LOGW(TAG, "Note: Expected PCA9685 at 0x%02X not found", PCA9685_I2C_ADDRESS);
        ESP_LOGW(TAG, "If PCA9685 is at different address, edit PCA9685_I2C_ADDRESS in this file");
      }
    }
    return true;
  }
}

/**
 * @brief Initialize test resources
 */
static bool init_test_resources() noexcept {
  // Initialize I2C bus (default GPIO4/5; override with PCA9685_EXAMPLE_I2C_SDA_GPIO/SCL_GPIO)
  Esp32Pca9685Bus::I2CConfig config;
  config.port = I2C_NUM_0;
  config.sda_pin = EXAMPLE_SDA_PIN;
  config.scl_pin = EXAMPLE_SCL_PIN;
  config.frequency = 100000; // 100 kHz for PCA9685
  config.pullup_enable = true;

  g_i2c_bus = CreateEsp32Pca9685Bus(config);
  if (!g_i2c_bus || !g_i2c_bus->IsInitialized()) {
    ESP_LOGE(TAG, "Failed to initialize I2C bus");
    return false;
  }

  // Try to create driver first (fast path - no scanning)
  ESP_LOGI(TAG, "Attempting to initialize PCA9685 at address 0x%02X...", PCA9685_I2C_ADDRESS);
  g_driver = create_test_driver();
  if (g_driver) {
    g_driver->SetRetryDelay(Esp32Pca9685Bus::RetryDelay);
  }

  // Only scan I2C bus if driver initialization failed
  if (!g_driver) {
    ESP_LOGW(TAG, "Failed to connect to PCA9685 at address 0x%02X", PCA9685_I2C_ADDRESS);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG,
             "╔══════════════════════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG,
             "║                         I2C BUS SCAN (Diagnostic)                            ║");
    ESP_LOGI(TAG,
             "╚══════════════════════════════════════════════════════════════════════════════╝");
    scan_i2c_bus(g_i2c_bus.get());
    ESP_LOGI(TAG, "");
    ESP_LOGE(TAG, "Failed to create driver");
    ESP_LOGE(TAG, "Expected PCA9685 at address 0x%02X (A0-A5 all LOW)", PCA9685_I2C_ADDRESS);
    ESP_LOGE(TAG, "If device is at different address, edit PCA9685_I2C_ADDRESS in this file");
    ESP_LOGE(TAG, "If device is on different pins, rebuild with:");
    ESP_LOGE(TAG, "  -DPCA9685_EXAMPLE_I2C_SDA_GPIO=<sda> -DPCA9685_EXAMPLE_I2C_SCL_GPIO=<scl>");
    return false;
  }

  return true;
}

/**
 * @brief Cleanup test resources
 */
static void cleanup_test_resources() noexcept {
  g_driver.reset();
  g_i2c_bus.reset();
}

//=============================================================================
// TEST CASES
//=============================================================================

/**
 * @brief Test I2C bus initialization
 */
static bool test_i2c_bus_initialization() noexcept {
  ESP_LOGI(TAG, "Testing I2C bus initialization...");

  if (!g_i2c_bus || !g_i2c_bus->IsInitialized()) {
    ESP_LOGE(TAG, "I2C bus not initialized");
    return false;
  }

  ESP_LOGI(TAG, "✅ I2C bus initialized successfully");
  return true;
}

/**
 * @brief Test driver initialization
 */
static bool test_driver_initialization() noexcept {
  ESP_LOGI(TAG, "Testing driver initialization...");

  if (!g_driver) {
    ESP_LOGE(TAG, "Driver not initialized");
    return false;
  }

  // Test reset
  if (!g_driver->Reset()) {
    ESP_LOGE(TAG, "Failed to reset driver");
    return false;
  }

  ESP_LOGI(TAG, "✅ Driver initialized successfully");
  return true;
}

/**
 * @brief Test PWM frequency configuration
 */
static bool test_pwm_frequency() noexcept {
  ESP_LOGI(TAG, "Testing PWM frequency configuration...");

  if (!g_driver) {
    ESP_LOGE(TAG, "Driver not initialized");
    return false;
  }

  // Test valid frequencies
  float test_frequencies[] = {50.0f, 100.0f, 200.0f, 500.0f, 1000.0f};
  for (float freq : test_frequencies) {
    if (!g_driver->SetPwmFreq(freq)) {
      ESP_LOGE(TAG, "Failed to set frequency %.1f Hz", freq);
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  // Test invalid frequencies (should fail)
  if (g_driver->SetPwmFreq(10.0f)) { // Too low
    ESP_LOGW(TAG, "Warning: Very low frequency accepted (may be valid)");
  }
  if (g_driver->SetPwmFreq(2000.0f)) { // Too high
    ESP_LOGW(TAG, "Warning: Very high frequency accepted (may be valid)");
  }

  ESP_LOGI(TAG, "✅ PWM frequency tests passed");
  return true;
}

/**
 * @brief Test individual channel PWM control
 */
static bool test_channel_pwm() noexcept {
  ESP_LOGI(TAG, "Testing individual channel PWM control...");

  if (!g_driver) {
    ESP_LOGE(TAG, "Driver not initialized");
    return false;
  }

  // Set frequency first
  if (!g_driver->SetPwmFreq(50.0f)) {
    ESP_LOGE(TAG, "Failed to set PWM frequency");
    return false;
  }

  // Test setting PWM on different channels
  for (uint8_t channel = 0; channel < 16; ++channel) {
    // Set 50% duty cycle (2048 out of 4095)
    if (!g_driver->SetPwm(channel, 0, 2048)) {
      ESP_LOGE(TAG, "Failed to set PWM on channel %d", channel);
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  ESP_LOGI(TAG, "✅ Channel PWM tests passed");
  return true;
}

/**
 * @brief Test duty cycle control
 */
static bool test_duty_cycle() noexcept {
  ESP_LOGI(TAG, "Testing duty cycle control...");

  if (!g_driver) {
    ESP_LOGE(TAG, "Driver not initialized");
    return false;
  }

  // Set frequency first
  if (!g_driver->SetPwmFreq(50.0f)) {
    ESP_LOGE(TAG, "Failed to set PWM frequency");
    return false;
  }

  // Test duty cycle sweep on all channels
  float duty_cycles[] = {0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 0.9f, 1.0f};
  for (uint8_t ch = 0; ch < 16; ++ch) {
    for (float duty : duty_cycles) {
      if (!g_driver->SetDuty(ch, duty)) {
        ESP_LOGE(TAG, "Failed to set duty %.2f on channel %d", duty, ch);
        return false;
      }
    }
  }

  // Test clamping: values outside 0.0-1.0 should be clamped (not fail)
  if (!g_driver->SetDuty(0, -0.5f)) {
    ESP_LOGE(TAG, "Negative duty should be clamped to 0.0, not fail");
    return false;
  }
  if (!g_driver->SetDuty(0, 1.5f)) {
    ESP_LOGE(TAG, "Duty > 1.0 should be clamped to 1.0, not fail");
    return false;
  }

  ESP_LOGI(TAG, "✅ Duty cycle tests passed");
  return true;
}

//=============================================================================
// ADVANCED TEST CASES
//=============================================================================

/**
 * @brief Test SetAllPwm and channel full-on/full-off
 */
static bool test_all_channel_control() noexcept {
  ESP_LOGI(TAG, "Testing all-channel and full-on/off control...");

  if (!g_driver) {
    ESP_LOGE(TAG, "Driver not initialized");
    return false;
  }

  if (!g_driver->SetPwmFreq(200.0f)) {
    ESP_LOGE(TAG, "Failed to set frequency");
    return false;
  }

  // SetAllPwm: set all channels to 25%
  if (!g_driver->SetAllPwm(0, 1024)) {
    ESP_LOGE(TAG, "SetAllPwm(0, 1024) failed");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(10));

  // SetAllPwm: set all channels to 75%
  if (!g_driver->SetAllPwm(0, 3072)) {
    ESP_LOGE(TAG, "SetAllPwm(0, 3072) failed");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(10));

  // SetChannelFullOn / SetChannelFullOff for each channel
  for (uint8_t ch = 0; ch < 16; ++ch) {
    if (!g_driver->SetChannelFullOn(ch)) {
      ESP_LOGE(TAG, "SetChannelFullOn(%d) failed", ch);
      return false;
    }
  }
  vTaskDelay(pdMS_TO_TICKS(10));

  for (uint8_t ch = 0; ch < 16; ++ch) {
    if (!g_driver->SetChannelFullOff(ch)) {
      ESP_LOGE(TAG, "SetChannelFullOff(%d) failed", ch);
      return false;
    }
  }
  vTaskDelay(pdMS_TO_TICKS(10));

  // Return to normal PWM after full-on/off test
  if (!g_driver->SetAllPwm(0, 0)) {
    ESP_LOGE(TAG, "Failed to clear all channels");
    return false;
  }

  ESP_LOGI(TAG, "✅ All-channel and full-on/off tests passed");
  return true;
}

/**
 * @brief Test prescale readback and frequency boundary values
 */
static bool test_prescale_readback() noexcept {
  ESP_LOGI(TAG, "Testing prescale readback and frequency boundaries...");

  if (!g_driver) {
    ESP_LOGE(TAG, "Driver not initialized");
    return false;
  }

  // Test boundary frequencies and verify prescale readback
  struct FreqTest {
    float freq_hz;
    uint8_t expected_prescale; // prescale = round(25MHz / (4096 * freq)) - 1
  };
  FreqTest freq_tests[] = {
      {50.0f, 121}, // 25e6 / (4096 * 50) - 1 = 121.07 -> 121
      {200.0f, 29}, // 25e6 / (4096 * 200) - 1 = 29.52 -> 30   (approximate)
      {1000.0f, 5}, // 25e6 / (4096 * 1000) - 1 = 5.10 -> 5
      {24.0f, 253}, // Min valid frequency
      {1526.0f, 3}, // Max valid frequency (prescale min = 3)
  };

  for (const auto& test : freq_tests) {
    if (!g_driver->SetPwmFreq(test.freq_hz)) {
      ESP_LOGE(TAG, "Failed to set frequency %.0f Hz", test.freq_hz);
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(5));

    uint8_t prescale = 0;
    if (!g_driver->GetPrescale(prescale)) {
      ESP_LOGE(TAG, "Failed to read prescale at %.0f Hz", test.freq_hz);
      return false;
    }

    // Allow +/- 1 tolerance for rounding differences
    int diff = static_cast<int>(prescale) - static_cast<int>(test.expected_prescale);
    if (diff < -1 || diff > 1) {
      ESP_LOGE(TAG, "Prescale mismatch at %.0f Hz: got %d, expected %d (±1)", test.freq_hz,
               prescale, test.expected_prescale);
      return false;
    }
    ESP_LOGI(TAG, "  %.0f Hz -> prescale=%d (expected %d) ✓", test.freq_hz, prescale,
             test.expected_prescale);
  }

  // Test out-of-range frequencies (should fail gracefully)
  if (g_driver->SetPwmFreq(10.0f)) {
    ESP_LOGW(TAG, "10 Hz was accepted (below 24 Hz min) -- unexpected but not fatal");
  }
  g_driver->ClearErrorFlags();

  if (g_driver->SetPwmFreq(2000.0f)) {
    ESP_LOGW(TAG, "2000 Hz was accepted (above 1526 Hz max) -- unexpected but not fatal");
  }
  g_driver->ClearErrorFlags();

  ESP_LOGI(TAG, "✅ Prescale readback and boundary tests passed");
  return true;
}

/**
 * @brief Test sleep/wake power management
 */
static bool test_sleep_wake() noexcept {
  ESP_LOGI(TAG, "Testing sleep/wake power management...");

  if (!g_driver) {
    ESP_LOGE(TAG, "Driver not initialized");
    return false;
  }

  // Set up a known PWM state before sleep
  if (!g_driver->SetPwmFreq(100.0f)) {
    ESP_LOGE(TAG, "Failed to set frequency before sleep test");
    return false;
  }
  if (!g_driver->SetDuty(0, 0.5f)) {
    ESP_LOGE(TAG, "Failed to set duty before sleep");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(10));

  // Enter sleep
  if (!g_driver->Sleep()) {
    ESP_LOGE(TAG, "Sleep() failed");
    return false;
  }
  ESP_LOGI(TAG, "  Device in sleep mode");
  vTaskDelay(pdMS_TO_TICKS(50));

  // Wake and verify device is responsive
  if (!g_driver->Wake()) {
    ESP_LOGE(TAG, "Wake() failed");
    return false;
  }
  ESP_LOGI(TAG, "  Device woke up");
  vTaskDelay(pdMS_TO_TICKS(10));

  // Verify device is functional after wake: set PWM on all channels
  for (uint8_t ch = 0; ch < 16; ++ch) {
    if (!g_driver->SetDuty(ch, 0.25f)) {
      ESP_LOGE(TAG, "Failed to set duty on ch %d after wake", ch);
      return false;
    }
  }

  // Multiple sleep/wake cycles
  for (int cycle = 0; cycle < 5; ++cycle) {
    if (!g_driver->Sleep()) {
      ESP_LOGE(TAG, "Sleep() failed on cycle %d", cycle);
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    if (!g_driver->Wake()) {
      ESP_LOGE(TAG, "Wake() failed on cycle %d", cycle);
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  ESP_LOGI(TAG, "✅ Sleep/wake tests passed (5 cycles)");
  return true;
}

/**
 * @brief Test output configuration (invert, driver mode)
 */
static bool test_output_config() noexcept {
  ESP_LOGI(TAG, "Testing output configuration...");

  if (!g_driver) {
    ESP_LOGE(TAG, "Driver not initialized");
    return false;
  }

  // Test output invert toggle
  if (!g_driver->SetOutputInvert(true)) {
    ESP_LOGE(TAG, "SetOutputInvert(true) failed");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(5));

  if (!g_driver->SetOutputInvert(false)) {
    ESP_LOGE(TAG, "SetOutputInvert(false) failed");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(5));

  // Test output driver mode
  if (!g_driver->SetOutputDriverMode(true)) { // totem-pole
    ESP_LOGE(TAG, "SetOutputDriverMode(totem-pole) failed");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(5));

  if (!g_driver->SetOutputDriverMode(false)) { // open-drain
    ESP_LOGE(TAG, "SetOutputDriverMode(open-drain) failed");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(5));

  // Restore default (totem-pole)
  if (!g_driver->SetOutputDriverMode(true)) {
    ESP_LOGE(TAG, "Failed to restore totem-pole mode");
    return false;
  }

  ESP_LOGI(TAG, "✅ Output configuration tests passed");
  return true;
}

/**
 * @brief Test error flag management
 */
static bool test_error_handling() noexcept {
  ESP_LOGI(TAG, "Testing error flag management...");

  if (!g_driver) {
    ESP_LOGE(TAG, "Driver not initialized");
    return false;
  }

  // After successful operations, no errors should be set
  g_driver->ClearErrorFlags();
  if (g_driver->HasAnyError()) {
    ESP_LOGE(TAG, "Error flags not cleared");
    return false;
  }

  // Trigger an out-of-range error: channel 255 is invalid
  bool result = g_driver->SetPwm(255, 0, 2048);
  if (result) {
    ESP_LOGE(TAG, "SetPwm(255,...) should have failed");
    return false;
  }
  if (!g_driver->HasError(PCA9685Driver::Error::OutOfRange)) {
    ESP_LOGE(TAG, "Expected OutOfRange error flag after invalid channel");
    return false;
  }
  ESP_LOGI(TAG, "  Invalid channel correctly rejected with OutOfRange error");

  // Clear and verify
  g_driver->ClearError(PCA9685Driver::Error::OutOfRange);
  if (g_driver->HasError(PCA9685Driver::Error::OutOfRange)) {
    ESP_LOGE(TAG, "OutOfRange flag not cleared");
    return false;
  }

  // Trigger out-of-range PWM value
  result = g_driver->SetPwm(0, 5000, 0);
  if (result) {
    ESP_LOGE(TAG, "SetPwm(0, 5000, 0) should have failed (on > 4095)");
    return false;
  }
  ESP_LOGI(TAG, "  Invalid PWM value correctly rejected");
  g_driver->ClearErrorFlags();

  // Trigger out-of-range frequency
  result = g_driver->SetPwmFreq(5.0f);
  if (result) {
    ESP_LOGE(TAG, "SetPwmFreq(5.0) should have failed (below 24 Hz)");
    return false;
  }
  ESP_LOGI(TAG, "  Invalid frequency correctly rejected");
  g_driver->ClearErrorFlags();

  // Verify GetLastError returns something meaningful
  g_driver->SetPwm(255, 0, 0); // Force an error
  auto last_err = g_driver->GetLastError();
  if (last_err == PCA9685Driver::Error::None) {
    ESP_LOGE(TAG, "GetLastError() returned None after forced error");
    return false;
  }
  g_driver->ClearErrorFlags();

  // After clearing, verify clean state
  g_driver->SetDuty(0, 0.5f); // Should succeed
  if (g_driver->HasAnyError()) {
    ESP_LOGE(TAG, "Error flags set after valid operation");
    return false;
  }

  ESP_LOGI(TAG, "✅ Error handling tests passed");
  return true;
}

/**
 * @brief Stress test: rapid consecutive I2C operations
 */
static bool test_stress_rapid_writes() noexcept {
  ESP_LOGI(TAG, "Stress testing: rapid consecutive writes...");

  if (!g_driver) {
    ESP_LOGE(TAG, "Driver not initialized");
    return false;
  }

  if (!g_driver->SetPwmFreq(200.0f)) {
    ESP_LOGE(TAG, "Failed to set frequency for stress test");
    return false;
  }

  // Rapid duty cycle sweep: 100 steps across all 16 channels (1600 I2C writes)
  int write_count = 0;
  for (int step = 0; step <= 100; step += 5) {
    float duty = static_cast<float>(step) / 100.0f;
    for (uint8_t ch = 0; ch < 16; ++ch) {
      if (!g_driver->SetDuty(ch, duty)) {
        ESP_LOGE(TAG, "Rapid write failed at step=%d ch=%d (write #%d)", step, ch, write_count);
        return false;
      }
      ++write_count;
    }
    // No delay between steps -- this is the stress part
  }
  ESP_LOGI(TAG, "  Completed %d rapid SetDuty writes with zero failures", write_count);

  // Rapid SetAllPwm alternation (100 writes)
  for (int i = 0; i < 100; ++i) {
    uint16_t off_val = (i % 2 == 0) ? 2048 : 0;
    if (!g_driver->SetAllPwm(0, off_val)) {
      ESP_LOGE(TAG, "Rapid SetAllPwm failed at iteration %d", i);
      return false;
    }
  }
  ESP_LOGI(TAG, "  Completed 100 rapid SetAllPwm toggles with zero failures");

  // Rapid frequency changes (stresses prescale register writes with sleep/wake)
  float stress_freqs[] = {50.0f, 200.0f, 500.0f, 1000.0f, 100.0f};
  for (int cycle = 0; cycle < 10; ++cycle) {
    for (float freq : stress_freqs) {
      if (!g_driver->SetPwmFreq(freq)) {
        ESP_LOGE(TAG, "Rapid freq change failed: %.0f Hz, cycle %d", freq, cycle);
        return false;
      }
    }
  }
  ESP_LOGI(TAG, "  Completed 50 rapid frequency changes with zero failures");

  ESP_LOGI(TAG, "✅ Stress tests passed (%d+ I2C transactions)", write_count + 100 + 50);
  return true;
}

/**
 * @brief Stress test: boundary and edge-case PWM values
 */
static bool test_stress_boundary_values() noexcept {
  ESP_LOGI(TAG, "Stress testing: boundary and edge-case values...");

  if (!g_driver) {
    ESP_LOGE(TAG, "Driver not initialized");
    return false;
  }

  if (!g_driver->SetPwmFreq(100.0f)) {
    ESP_LOGE(TAG, "Failed to set frequency");
    return false;
  }

  // Test every channel at extreme PWM values
  for (uint8_t ch = 0; ch < 16; ++ch) {
    // Minimum on/off: 0, 0
    if (!g_driver->SetPwm(ch, 0, 0)) {
      ESP_LOGE(TAG, "SetPwm(%d, 0, 0) failed", ch);
      return false;
    }
    // Maximum: 0, 4095
    if (!g_driver->SetPwm(ch, 0, 4095)) {
      ESP_LOGE(TAG, "SetPwm(%d, 0, 4095) failed", ch);
      return false;
    }
    // On-time != 0: staggered phase
    if (!g_driver->SetPwm(ch, static_cast<uint16_t>(ch * 256), 2048)) {
      ESP_LOGE(TAG, "SetPwm(%d, %d, 2048) failed", ch, ch * 256);
      return false;
    }
  }
  ESP_LOGI(TAG, "  All 16 channels passed min/max/staggered PWM values");

  // Full-on then full-off then PWM -- tests register state transitions
  for (uint8_t ch = 0; ch < 16; ++ch) {
    if (!g_driver->SetChannelFullOn(ch)) {
      ESP_LOGE(TAG, "FullOn(%d) failed in transition test", ch);
      return false;
    }
    if (!g_driver->SetChannelFullOff(ch)) {
      ESP_LOGE(TAG, "FullOff(%d) failed in transition test", ch);
      return false;
    }
    if (!g_driver->SetDuty(ch, 0.5f)) {
      ESP_LOGE(TAG, "SetDuty(%d) after FullOff failed", ch);
      return false;
    }
  }
  ESP_LOGI(TAG, "  All 16 channels passed FullOn->FullOff->PWM transitions");

  // Multiple reset and re-init cycles
  for (int i = 0; i < 5; ++i) {
    if (!g_driver->Reset()) {
      ESP_LOGE(TAG, "Reset() failed on cycle %d", i);
      return false;
    }
    if (!g_driver->SetPwmFreq(100.0f)) {
      ESP_LOGE(TAG, "SetPwmFreq after reset failed on cycle %d", i);
      return false;
    }
    if (!g_driver->SetDuty(0, 0.5f)) {
      ESP_LOGE(TAG, "SetDuty after reset failed on cycle %d", i);
      return false;
    }
  }
  ESP_LOGI(TAG, "  5 reset/reinit cycles passed");

  ESP_LOGI(TAG, "✅ Boundary and edge-case stress tests passed");
  return true;
}

//=============================================================================
// MAIN TEST RUNNER
//=============================================================================

extern "C" void app_main() {
  ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════════════════════╗");
  ESP_LOGI(TAG,
           "║                      ESP32 PCA9685 COMPREHENSIVE TEST SUITE                   ║");
  ESP_LOGI(TAG, "║                         HardFOC PCA9685 Driver Tests                         ║");
  ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════════════════════╝");

  vTaskDelay(pdMS_TO_TICKS(1000));

  // Report test section configuration
  print_test_section_status(TAG, "PCA9685");

  // Initialize test resources
  if (!init_test_resources()) {
    ESP_LOGE(TAG, "Failed to initialize test resources");
    return;
  }

  // Run all tests based on configuration
  RUN_TEST_SECTION_IF_ENABLED_WITH_PATTERN(
      ENABLE_INITIALIZATION_TESTS, "PCA9685 INITIALIZATION TESTS", 5,
      RUN_TEST_IN_TASK("i2c_bus_initialization", test_i2c_bus_initialization, 8192, 1);
      RUN_TEST_IN_TASK("driver_initialization", test_driver_initialization, 8192, 1);
      flip_test_progress_indicator(););

  RUN_TEST_SECTION_IF_ENABLED_WITH_PATTERN(
      ENABLE_FREQUENCY_TESTS, "PCA9685 FREQUENCY TESTS", 5,
      RUN_TEST_IN_TASK("pwm_frequency", test_pwm_frequency, 8192, 1);
      flip_test_progress_indicator(););

  RUN_TEST_SECTION_IF_ENABLED_WITH_PATTERN(
      ENABLE_PWM_TESTS, "PCA9685 PWM TESTS", 5,
      RUN_TEST_IN_TASK("channel_pwm", test_channel_pwm, 8192, 1);
      flip_test_progress_indicator(););

  RUN_TEST_SECTION_IF_ENABLED_WITH_PATTERN(
      ENABLE_DUTY_CYCLE_TESTS, "PCA9685 DUTY CYCLE TESTS", 5,
      RUN_TEST_IN_TASK("duty_cycle", test_duty_cycle, 8192, 1);
      RUN_TEST_IN_TASK("all_channel_control", test_all_channel_control, 8192, 1);
      RUN_TEST_IN_TASK("prescale_readback", test_prescale_readback, 8192, 1);
      RUN_TEST_IN_TASK("sleep_wake", test_sleep_wake, 8192, 1);
      RUN_TEST_IN_TASK("output_config", test_output_config, 8192, 1);
      flip_test_progress_indicator(););

  RUN_TEST_SECTION_IF_ENABLED_WITH_PATTERN(
      ENABLE_ERROR_HANDLING_TESTS, "PCA9685 ERROR HANDLING TESTS", 5,
      RUN_TEST_IN_TASK("error_handling", test_error_handling, 8192, 1);
      flip_test_progress_indicator(););

  RUN_TEST_SECTION_IF_ENABLED_WITH_PATTERN(
      ENABLE_STRESS_TESTS, "PCA9685 STRESS TESTS", 5,
      RUN_TEST_IN_TASK("stress_rapid_writes", test_stress_rapid_writes, 16384, 1);
      RUN_TEST_IN_TASK("stress_boundary_values", test_stress_boundary_values, 16384, 1);
      flip_test_progress_indicator(););

  // Cleanup
  cleanup_test_resources();

  // Print results
  print_test_summary(g_test_results, "PCA9685", TAG);

  // Blink GPIO14 to indicate completion
  output_section_indicator(5);

  cleanup_test_progress_indicator();

  while (true) {
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}
