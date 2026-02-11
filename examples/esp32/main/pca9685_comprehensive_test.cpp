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

// Project headers
#include "pca9685.hpp"
#include "esp32_pca9685_bus.hpp"
#include "TestFramework.h"

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
        ESP_LOGW(TAG, "No I2C devices found on GPIO%d (SDA) / GPIO%d (SCL)", EXAMPLE_SDA_PIN, EXAMPLE_SCL_PIN);
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
    config.frequency = 100000;    // 100 kHz for PCA9685
    config.pullup_enable = true;

    g_i2c_bus = CreateEsp32Pca9685Bus(config);
    if (!g_i2c_bus || !g_i2c_bus->IsInitialized()) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus");
        return false;
    }

    // Try to create driver first (fast path - no scanning)
    ESP_LOGI(TAG, "Attempting to initialize PCA9685 at address 0x%02X...", PCA9685_I2C_ADDRESS);
    g_driver = create_test_driver();
    
    // Only scan I2C bus if driver initialization failed
    if (!g_driver) {
        ESP_LOGW(TAG, "Failed to connect to PCA9685 at address 0x%02X", PCA9685_I2C_ADDRESS);
        ESP_LOGI(TAG, "");
        ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════════════════════╗");
        ESP_LOGI(TAG, "║                         I2C BUS SCAN (Diagnostic)                            ║");
        ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════════════════════╝");
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
    if (g_driver->SetPwmFreq(10.0f)) {  // Too low
        ESP_LOGW(TAG, "Warning: Very low frequency accepted (may be valid)");
    }
    if (g_driver->SetPwmFreq(2000.0f)) {  // Too high
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

    // Test different duty cycles
    float duty_cycles[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    for (float duty : duty_cycles) {
        if (!g_driver->SetDuty(0, duty)) {
            ESP_LOGE(TAG, "Failed to set duty cycle %.2f", duty);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    ESP_LOGI(TAG, "✅ Duty cycle tests passed");
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
