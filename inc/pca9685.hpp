/**
 * @file pca9685.hpp
 * @brief Hardware-agnostic driver for the PCA9685 16-channel 12-bit PWM
 * controller (I2C)
 *
 * This driver provides a platform-independent C++ interface for the NXP PCA9685
 * PWM controller. It requires the user to provide an implementation of the
 * I2cBus interface for their platform. No dependencies on project-specific or
 * MCU-specific types are present.
 *
 * Features:
 *   - Set PWM frequency (24 Hz to 1526 Hz typical)
 *   - Set PWM value (on/off time) for each channel
 *   - Reset and configure device
 *   - Error reporting and diagnostics
 *
 * @author Nebiyu Tadesse
 * @date 2025
 * @version 1.0
 */
#ifndef PCA9685_HPP
#define PCA9685_HPP

#include <algorithm> // For std algorithms
#include <array>     // For std::array
#include <cmath>     // For math functions
#include <cstddef>
#include <cstdint>
#include <stdio.h> // NOLINT(modernize-deprecated-headers) - For FILE* used by ESP-IDF headers
#include <string.h> // NOLINT(modernize-deprecated-headers) - For C string functions (ESP-IDF compatibility)

#include "pca9685_i2c_interface.hpp"

namespace pca9685 {

/**
 * @class PCA9685
 * @brief Driver for the PCA9685 16-channel 12-bit PWM controller.
 *
 * This class provides all register-level operations required to configure and
 * operate the PCA9685, including frequency setting, per-channel PWM, and device
 * reset.
 *
 * All I2C operations are routed through the user-supplied I2cBus interface.
 *
 * @tparam I2cType The I2C interface implementation type that inherits from
 * pca9685::I2cBus<I2cType>
 *
 * @note The driver uses CRTP-based I2C interface for zero virtual call
 * overhead.
 */
template <typename I2cType> class PCA9685 {
public:
  /**
   * @brief Error codes for PCA9685 operations.
   */
  enum class Error : uint8_t {
    None = 0,
    I2cWrite = 1,
    I2cRead = 2,
    InvalidParam = 3,
    DeviceNotFound = 4,
    NotInitialized = 5,
    OutOfRange = 6
  };

  /**
   * @brief PCA9685 register map.
   */
  enum class Register : uint8_t {
    MODE1 = 0x00,
    MODE2 = 0x01,
    SUBADR1 = 0x02,
    SUBADR2 = 0x03,
    SUBADR3 = 0x04,
    ALLCALLADR = 0x05,
    LED0_ON_L = 0x06,
    LED0_ON_H = 0x07,
    LED0_OFF_L = 0x08,
    LED0_OFF_H = 0x09,
    // ... LED1-LED15 follow sequentially
    ALL_LED_ON_L = 0xFA,
    ALL_LED_ON_H = 0xFB,
    ALL_LED_OFF_L = 0xFC,
    ALL_LED_OFF_H = 0xFD,
    PRE_SCALE = 0xFE,
    TESTMODE = 0xFF
  };

  static constexpr uint8_t MAX_CHANNELS = 16;
  static constexpr uint16_t MAX_PWM = 4095;
  static constexpr uint32_t OSC_FREQ = 25000000;

  /**
   * @brief Construct a new PCA9685 driver instance.
   * @param bus Pointer to a user-implemented I2C interface (must inherit from
   * pca9685::I2cInterface<I2cType>).
   * @param address 7-bit I2C address of the PCA9685 device (0x00 to 0x7F).
   */
  PCA9685(I2cType *bus, uint8_t address);

  /**
   * @brief Reset the device to its power-on default state.
   * @return true on success; false on I2C failure.
   */
  bool Reset();

  /**
   * @brief Set the PWM frequency for all channels.
   * @param freq_hz Desired frequency in Hz (24-1526 typical).
   * @return true on success; false on I2C failure or invalid parameter.
   */
  bool SetPwmFreq(float freq_hz);

  /**
   * @brief Set the PWM on/off time for a channel.
   * @param channel Channel number (0-15).
   * @param on_time Tick count when signal turns ON (0-4095).
   * @param off_time Tick count when signal turns OFF (0-4095).
   * @return true on success; false on I2C failure or invalid parameter.
   */
  bool SetPwm(uint8_t channel, uint16_t on_time, uint16_t off_time);

  /**
   * @brief Set the duty cycle for a channel (0.0-1.0).
   * @param channel Channel number (0-15).
   * @param duty Duty cycle (0.0 = always off, 1.0 = always on).
   * @return true on success; false on I2C failure or invalid parameter.
   */
  bool SetDuty(uint8_t channel, float duty);

  /**
   * @brief Set all channels to the same PWM value.
   * @param on_time Tick count when signal turns ON (0-4095).
   * @param off_time Tick count when signal turns OFF (0-4095).
   * @return true on success; false on I2C failure.
   */
  bool SetAllPwm(uint16_t on_time, uint16_t off_time);

  /**
   * @brief Get the last error code.
   * @return Error code from the last operation.
   */
  Error GetLastError() const { return last_error_; }

  /**
   * @brief Get the current prescale value (frequency divider).
   * @param[out] prescale The prescale register value.
   * @return true on success; false on I2C failure.
   */
  bool GetPrescale(uint8_t &prescale);

  /**
   * @brief Set the output enable state (if using OE pin externally).
   * @param enabled True to enable outputs, false to disable.
   * @note This is a stub; actual OE pin control must be handled externally.
   */
  void SetOutputEnable(bool enabled) {
    (void)enabled; /* User must implement if needed */
  }

private:
  I2cType *i2c_;
  uint8_t addr_;
  Error last_error_;
  bool initialized_{false};

  bool writeReg(uint8_t reg, uint8_t value);
  bool readReg(uint8_t reg, uint8_t &value);
  bool writeRegBlock(uint8_t reg, const uint8_t *data, size_t len);
  bool readRegBlock(uint8_t reg, uint8_t *data, size_t len);
  [[nodiscard]] uint8_t calcPrescale(float freq_hz) const;
};

// Include template implementation
#define PCA9685_HEADER_INCLUDED
// NOLINTNEXTLINE(bugprone-suspicious-include) - Intentional: template
// implementation file
#include "../src/pca9685.cpp"
#undef PCA9685_HEADER_INCLUDED

} // namespace pca9685

#endif // PCA9685_HPP