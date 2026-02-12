/**
 * @file pca9685_i2c_interface.hpp
 * @brief CRTP-based I2C interface for PCA9685 driver
 * @copyright Copyright (c) 2024-2025 HardFOC. All rights reserved.
 */
#pragma once
#include <cstddef>
#include <cstdint>

namespace pca9685 {

/**
 * @brief CRTP-based template interface for I2C bus operations
 *
 * This template class provides a hardware-agnostic interface for I2C
 * communication using the CRTP pattern. Platform-specific implementations
 * should inherit from this template with themselves as the template parameter.
 *
 * Benefits of CRTP:
 * - Compile-time polymorphism (no virtual function overhead)
 * - Static dispatch instead of dynamic dispatch
 * - Better optimization opportunities for the compiler
 *
 * Example usage:
 * @code
 * class MyI2C : public pca9685::I2cInterface<MyI2C> {
 * public:
 *   bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) noexcept { ... }
 *   bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) noexcept { ... }
 *   bool EnsureInitialized() noexcept { ... }
 * };
 * @endcode
 *
 * @tparam Derived The derived class type (CRTP pattern)
 */
template <typename Derived>
class I2cInterface {
public:
  /**
   * @brief Write bytes to a device register.
   * @param addr 7-bit I2C address of the target device.
   * @param reg Register address to write to.
   * @param data Pointer to the data buffer containing bytes to send.
   * @param len Number of bytes to write from the buffer.
   * @return true if the device acknowledges the transfer; false on NACK or error.
   */
  bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) noexcept {
    return static_cast<Derived*>(this)->Write(addr, reg, data, len);
  }

  /**
   * @brief Read bytes from a device register.
   * @param addr 7-bit I2C address of the target device.
   * @param reg Register address to read from.
   * @param data Pointer to the buffer to store received data.
   * @param len Number of bytes to read into the buffer.
   * @return true if the read succeeds; false on NACK or error.
   */
  bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) noexcept {
    return static_cast<Derived*>(this)->Read(addr, reg, data, len);
  }

  /**
   * @brief Ensure the I2C bus is initialized and ready for communication.
   *
   * This method performs lazy initialization of the I2C bus. It should initialize
   * the I2C hardware, configure pins, set up the bus, and verify it's ready for
   * communication. On subsequent calls, it should return true immediately if
   * already initialized.
   *
   * @return true if initialization succeeded or was already initialized;
   *         false if initialization failed.
   */
  bool EnsureInitialized() noexcept {
    return static_cast<Derived*>(this)->EnsureInitialized();
  }

  /** @brief Copy constructor deleted (non-copyable; avoids slicing). */
  I2cInterface(const I2cInterface&) = delete;
  /** @brief Copy assignment deleted (non-copyable). */
  I2cInterface& operator=(const I2cInterface&) = delete;
  /** @brief Move constructor deleted (non-movable; use derived type to move). */
  I2cInterface(I2cInterface&&) = delete;
  /** @brief Move assignment deleted (non-movable). */
  I2cInterface& operator=(I2cInterface&&) = delete;

protected:
  /**
   * @brief Protected constructor to prevent direct instantiation
   */
  I2cInterface() = default;

  /**
   * @brief Protected destructor
   * @note Derived classes can have public destructors
   */
  ~I2cInterface() = default;
};

} // namespace pca9685
