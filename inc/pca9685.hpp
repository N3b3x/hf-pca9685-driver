/**
 * @file pca9685.hpp
 * @brief Hardware-agnostic driver for the PCA9685 16-channel 12-bit PWM controller (I2C)
 *
 * This driver provides a platform-independent C++ interface for the NXP PCA9685
 * PWM controller. It requires the user to provide an implementation of the I2cBus
 * interface for their platform. No dependencies on project-specific or MCU-specific
 * types are present.
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
#pragma once
#include <cstddef>
#include <cstdint>

namespace PCA9685 {

/**
 * @brief CRTP-based template interface for I2C bus operations
 *
 * This template class provides a hardware-agnostic interface for I2C communication
 * using the CRTP pattern. Platform-specific implementations should inherit from
 * this template with themselves as the template parameter.
 *
 * Benefits of CRTP:
 * - Compile-time polymorphism (no virtual function overhead)
 * - Static dispatch instead of dynamic dispatch
 * - Better optimization opportunities for the compiler
 *
 * Example usage:
 * @code
 * class MyI2C : public PCA9685::I2cBus<MyI2C> {
 * public:
 *   bool write(...) { ... }
 *   bool read(...) { ... }
 * };
 * @endcode
 *
 * @tparam Derived The derived class type (CRTP pattern)
 */
template <typename Derived>
class I2cBus {
public:
  /**
   * @brief Write bytes to a device register.
   * @param addr 7-bit I2C address of the target device.
   * @param reg Register address to write to.
   * @param data Pointer to the data buffer containing bytes to send.
   * @param len Number of bytes to write from the buffer.
   * @return true if the device acknowledges the transfer; false on NACK or error.
   */
  bool write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) {
    return static_cast<Derived*>(this)->write(addr, reg, data, len);
  }

  /**
   * @brief Read bytes from a device register.
   * @param addr 7-bit I2C address of the target device.
   * @param reg Register address to read from.
   * @param data Pointer to the buffer to store received data.
   * @param len Number of bytes to read into the buffer.
   * @return true if the read succeeds; false on NACK or error.
   */
  bool read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) {
    return static_cast<Derived*>(this)->read(addr, reg, data, len);
  }

protected:
  /**
   * @brief Protected constructor to prevent direct instantiation
   */
  I2cBus() = default;

  // Prevent copying
  I2cBus(const I2cBus&) = delete;
  I2cBus& operator=(const I2cBus&) = delete;

  // Allow moving
  I2cBus(I2cBus&&) = default;
  I2cBus& operator=(I2cBus&&) = default;

  /**
   * @brief Protected destructor
   * @note Derived classes can have public destructors
   */
  ~I2cBus() = default;
};

}  // namespace PCA9685

/**
 * @class PCA9685
 * @brief Driver for the PCA9685 16-channel 12-bit PWM controller.
 *
 * This class provides all register-level operations required to configure and operate
 * the PCA9685, including frequency setting, per-channel PWM, and device reset.
 *
 * All I2C operations are routed through the user-supplied I2cBus interface.
 *
 * @tparam I2cType The I2C interface implementation type that inherits from PCA9685::I2cBus<I2cType>
 *
 * @note The driver uses CRTP-based I2C interface for zero virtual call overhead.
 */
template <typename I2cType>
class PCA9685 {
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
  enum Register : uint8_t {
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
   * @param bus Pointer to a user-implemented I2C interface (must inherit from PCA9685::I2cBus<I2cType>).
   * @param address 7-bit I2C address of the PCA9685 device (0x00 to 0x7F).
   */
  PCA9685(I2cType* bus, uint8_t address);

  /**
   * @brief Reset the device to its power-on default state.
   * @return true on success; false on I2C failure.
   */
  bool reset();

  /**
   * @brief Set the PWM frequency for all channels.
   * @param freq_hz Desired frequency in Hz (24-1526 typical).
   * @return true on success; false on I2C failure or invalid parameter.
   */
  bool setPwmFreq(float freq_hz);

  /**
   * @brief Set the PWM on/off time for a channel.
   * @param channel Channel number (0-15).
   * @param on Tick count when signal turns ON (0-4095).
   * @param off Tick count when signal turns OFF (0-4095).
   * @return true on success; false on I2C failure or invalid parameter.
   */
  bool setPwm(uint8_t channel, uint16_t on, uint16_t off);

  /**
   * @brief Set the duty cycle for a channel (0.0-1.0).
   * @param channel Channel number (0-15).
   * @param duty Duty cycle (0.0 = always off, 1.0 = always on).
   * @return true on success; false on I2C failure or invalid parameter.
   */
  bool setDuty(uint8_t channel, float duty);

  /**
   * @brief Set all channels to the same PWM value.
   * @param on Tick count when signal turns ON (0-4095).
   * @param off Tick count when signal turns OFF (0-4095).
   * @return true on success; false on I2C failure.
   */
  bool setAllPwm(uint16_t on, uint16_t off);

  /**
   * @brief Get the last error code.
   * @return Error code from the last operation.
   */
  Error getLastError() const;

  /**
   * @brief Get the current prescale value (frequency divider).
   * @param[out] prescale The prescale register value.
   * @return true on success; false on I2C failure.
   */
  bool getPrescale(uint8_t& prescale);

  /**
   * @brief Set the output enable state (if using OE pin externally).
   * @param enabled True to enable outputs, false to disable.
   * @note This is a stub; actual OE pin control must be handled externally.
   */
  void setOutputEnable(bool enabled) {
    (void)enabled; /* User must implement if needed */
  }

private:
  I2cType* i2c_;
  uint8_t addr_;
  Error lastError_;
  bool initialized_;

  bool writeReg(uint8_t reg, uint8_t value);
  bool readReg(uint8_t reg, uint8_t& value);
  bool writeRegBlock(uint8_t reg, const uint8_t* data, size_t len);
  bool readRegBlock(uint8_t reg, uint8_t* data, size_t len);
  uint8_t calcPrescale(float freq_hz) const;
};

// Include template implementation
#define PCA9685_HEADER_INCLUDED
#include "../src/pca9685.cpp"
#undef PCA9685_HEADER_INCLUDED

}  // namespace PCA9685