/**
 * @file esp32_pca9685_test_config.hpp
 * @brief Hardware configuration for PCA9685 driver on ESP32-S3
 *
 * This file contains the actual hardware configuration that is used by the HAL
 * and example applications. Modify these values to match your hardware setup.
 *
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */

#pragma once

#include <cstdint>

//==============================================================================
// COMPILE-TIME CONFIGURATION FLAGS
//==============================================================================

/**
 * @brief Enable detailed I2C transaction logging
 *
 * @details
 * When enabled (set to 1), the Esp32Pca9685I2cBus will log detailed
 * information about each I2C transaction including:
 * - Register read/write addresses and values
 * - PWM ON/OFF register calculations
 * - Prescaler updates
 *
 * When disabled (set to 0), only basic error logging is performed.
 *
 * Default: 0 (disabled) - Set to 1 to enable for debugging
 */
#ifndef ESP32_PCA9685_ENABLE_DETAILED_I2C_LOGGING
#define ESP32_PCA9685_ENABLE_DETAILED_I2C_LOGGING 0
#endif

namespace PCA9685_TestConfig {

/**
 * @brief I2C Pin Configuration for ESP32-S3
 *
 * These pins are used for I2C communication with the PCA9685.
 * Ensure your hardware matches these pin assignments or modify accordingly.
 */
struct I2CPins {
    static constexpr uint8_t SDA = 4;           ///< GPIO4 - I2C SDA (data)
    static constexpr uint8_t SCL = 5;           ///< GPIO5 - I2C SCL (clock)
};

/**
 * @brief Control GPIO Pins for PCA9685
 *
 * Optional pins for device control.
 * Set to -1 if not connected/configured.
 */
struct ControlPins {
    static constexpr int8_t OE = -1;           ///< Output Enable pin (active low, directly wired LOW if not used)
};

/**
 * @brief I2C Communication Parameters
 *
 * The PCA9685 supports I2C frequencies up to 1MHz (Fast Mode Plus).
 * Default to 100kHz for reliable communication with standard wiring.
 *
 * I2C Addressing (per PCA9685 datasheet):
 * - Base address: 0x40 (all address pins LOW)
 * - Address range: 0x40 - 0x7F (6 address bits: A0-A5)
 * - All-call address: 0x70 (enabled by default)
 */
struct I2CParams {
    static constexpr uint32_t FREQUENCY = 100000;      ///< 100kHz I2C frequency (Standard Mode)
    static constexpr uint8_t DEVICE_ADDRESS = 0x40;    ///< 7-bit I2C address (A0-A5 all LOW)
    static constexpr uint8_t ALL_CALL_ADDRESS = 0x70;  ///< All-call address (default)
    static constexpr uint32_t SCL_WAIT_US = 0;         ///< Clock stretching timeout (0 = default)
    static constexpr bool PULLUP_ENABLE = true;         ///< Enable internal pullups
};

/**
 * @brief PWM Controller Specifications
 *
 * PCA9685 is a 16-channel, 12-bit PWM controller.
 */
struct PWMSpecs {
    static constexpr uint8_t NUM_CHANNELS = 16;          ///< Number of PWM output channels
    static constexpr uint16_t RESOLUTION_BITS = 12;      ///< PWM resolution (12-bit = 4096 steps)
    static constexpr uint16_t MAX_COUNT = 4095;           ///< Maximum PWM count value
    static constexpr float MIN_FREQUENCY_HZ = 24.0f;     ///< Minimum PWM frequency (Hz)
    static constexpr float MAX_FREQUENCY_HZ = 1526.0f;   ///< Maximum PWM frequency (Hz)
    static constexpr float DEFAULT_FREQUENCY_HZ = 50.0f;  ///< Default PWM frequency for servos (Hz)
    static constexpr float INTERNAL_OSC_MHZ = 25.0f;     ///< Internal oscillator frequency (MHz)
};

/**
 * @brief Supply Voltage Specifications (volts)
 *
 * VDD: Logic supply for PCA9685
 */
struct SupplyVoltage {
    static constexpr float VDD_MIN = 2.3f;     ///< Minimum VDD voltage (V)
    static constexpr float VDD_NOM = 3.3f;     ///< Nominal VDD voltage (V)
    static constexpr float VDD_MAX = 5.5f;     ///< Maximum VDD voltage (V)
};

/**
 * @brief Temperature Specifications (celsius)
 *
 * Operating temperature range from PCA9685 datasheet.
 */
struct Temperature {
    static constexpr int16_t OPERATING_MIN = -40;    ///< Minimum operating temperature (°C)
    static constexpr int16_t OPERATING_MAX = 85;     ///< Maximum operating temperature (°C)
    static constexpr int16_t WARNING_THRESHOLD = 75; ///< Temperature warning threshold (°C)
};

/**
 * @brief Timing Parameters
 *
 * Timing requirements from the PCA9685 datasheet.
 */
struct Timing {
    static constexpr uint16_t POWER_ON_DELAY_MS = 10;       ///< Power-on initialization delay (ms)
    static constexpr uint16_t RESET_DELAY_MS = 5;           ///< Software reset recovery delay (ms)
    static constexpr uint16_t OSC_STABILIZE_MS = 1;         ///< Oscillator stabilization delay (ms)
};

/**
 * @brief Diagnostic Thresholds
 *
 * Thresholds for health monitoring and error detection.
 */
struct Diagnostics {
    static constexpr uint16_t POLL_INTERVAL_MS = 100;      ///< Diagnostic polling interval (ms)
    static constexpr uint8_t MAX_RETRY_COUNT = 3;          ///< Maximum communication retries
};

/**
 * @brief Test Configuration
 *
 * Default parameters for testing.
 */
struct TestConfig {
    static constexpr uint16_t TEST_DURATION_MS = 5000;      ///< Test duration (ms)
    static constexpr uint16_t SWEEP_STEP_DELAY_MS = 20;     ///< PWM sweep step delay (ms)
    static constexpr float SERVO_MIN_PULSE_MS = 0.5f;       ///< Minimum servo pulse width (ms)
    static constexpr float SERVO_MAX_PULSE_MS = 2.5f;       ///< Maximum servo pulse width (ms)
};

/**
 * @brief Application-specific Configuration
 *
 * Configuration values that can be adjusted per application.
 */
struct AppConfig {
    // Logging
    static constexpr bool ENABLE_DEBUG_LOGGING = true;     ///< Enable detailed debug logs
    static constexpr bool ENABLE_I2C_LOGGING = false;      ///< Enable I2C transaction logs

    // Performance
    static constexpr bool ENABLE_PERFORMANCE_MONITORING = true;  ///< Enable performance metrics
    static constexpr uint16_t STATS_REPORT_INTERVAL_MS = 10000;  ///< Statistics reporting interval

    // Error handling
    static constexpr bool ENABLE_AUTO_RECOVERY = true;     ///< Enable automatic error recovery
    static constexpr uint8_t MAX_ERROR_COUNT = 10;         ///< Maximum errors before failsafe
};

} // namespace PCA9685_TestConfig

/**
 * @brief Hardware configuration validation
 *
 * Compile-time checks to ensure configuration is valid.
 */
static_assert(PCA9685_TestConfig::I2CParams::FREQUENCY <= 1000000,
              "I2C frequency exceeds PCA9685 maximum of 1MHz");

static_assert(PCA9685_TestConfig::I2CParams::DEVICE_ADDRESS >= 0x40 &&
              PCA9685_TestConfig::I2CParams::DEVICE_ADDRESS <= 0x7F,
              "PCA9685 I2C address must be in range 0x40-0x7F");

static_assert(PCA9685_TestConfig::PWMSpecs::NUM_CHANNELS == 16,
              "PCA9685 has exactly 16 PWM channels");

static_assert(PCA9685_TestConfig::PWMSpecs::RESOLUTION_BITS == 12,
              "PCA9685 is a 12-bit PWM controller");

/**
 * @brief Helper macro for compile-time GPIO pin validation
 */
#define PCA9685_VALIDATE_GPIO(pin) \
    static_assert((pin) >= 0 && (pin) < 49, "Invalid GPIO pin number for ESP32-S3")
