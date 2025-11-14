/**
 * @file pca9685.cpp
 * @brief Hardware-agnostic implementation for the PCA9685 16-channel 12-bit PWM controller (I2C)
 *
 * @author Nebiyu Tadesse
 * @date 2025
 * @version 1.0
 *
 * @note This file is included by pca9685.hpp for template instantiation.
 *       It should not be compiled separately when included.
 */

#ifndef PCA9685_IMPL
#define PCA9685_IMPL

// Include standard library headers BEFORE the namespace opens
#include <algorithm>
#include <cmath>
#include <string.h> // Use C string.h for ESP-IDF compatibility (cstring has issues with ESP-IDF toolchain)

// When included from header, use relative path; when compiled directly, use standard include
#ifdef PCA9685_HEADER_INCLUDED
#include "../inc/pca9685.hpp"
#else
#include "../inc/pca9685.hpp"
#endif

template <typename I2cType>
pca9685::PCA9685<I2cType>::PCA9685(I2cType* bus, uint8_t address)
    : i2c_(bus), addr_(address), last_error_(Error::None), initialized_(false) {}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::Reset() {
  uint8_t mode1 = 0x00; // Reset value
  if (!writeReg(MODE1, mode1)) {
    last_error_ = Error::I2cWrite;
    return false;
  }
  initialized_ = true;
  last_error_ = Error::None;
  return true;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::SetPwmFreq(float freq_hz) {
  if (!initialized_) {
    last_error_ = Error::NotInitialized;
    return false;
  }
  if (freq_hz < 24.0f || freq_hz > 1526.0f) {
    last_error_ = Error::OutOfRange;
    return false;
  }
  uint8_t prescale = calcPrescale(freq_hz);
  uint8_t oldmode = 0;
  if (!readReg(MODE1, oldmode)) {
    last_error_ = Error::I2cRead;
    return false;
  }
  uint8_t sleep = (oldmode & 0x7F) | 0x10; // sleep
  if (!writeReg(MODE1, sleep)) {
    last_error_ = Error::I2cWrite;
    return false;
  }
  if (!writeReg(PRE_SCALE, prescale)) {
    last_error_ = Error::I2cWrite;
    return false;
  }
  if (!writeReg(MODE1, oldmode)) {
    last_error_ = Error::I2cWrite;
    return false;
  }
  // Wait for oscillator to stabilize (500us typical)
  // (User should delay if needed)
  last_error_ = Error::None;
  return true;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::SetPwm(uint8_t channel, uint16_t on, uint16_t off) {
  if (!initialized_) {
    last_error_ = Error::NotInitialized;
    return false;
  }
  if (channel >= MAX_CHANNELS || on > MAX_PWM || off > MAX_PWM) {
    last_error_ = Error::OutOfRange;
    return false;
  }
  uint8_t reg = LED0_ON_L + 4 * channel;
  uint8_t data[4] = {static_cast<uint8_t>(on & 0xFF), static_cast<uint8_t>((on >> 8) & 0x0F),
                     static_cast<uint8_t>(off & 0xFF), static_cast<uint8_t>((off >> 8) & 0x0F)};
  if (!writeRegBlock(reg, data, 4)) {
    last_error_ = Error::I2cWrite;
    return false;
  }
  last_error_ = Error::None;
  return true;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::SetDuty(uint8_t channel, float duty) {
  if (duty < 0.0f)
    duty = 0.0f;
  if (duty > 1.0f)
    duty = 1.0f;
  uint16_t off = static_cast<uint16_t>(duty * MAX_PWM + 0.5f);
  return SetPwm(channel, 0, off);
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::SetAllPwm(uint16_t on, uint16_t off) {
  if (!initialized_) {
    last_error_ = Error::NotInitialized;
    return false;
  }
  if (on > MAX_PWM || off > MAX_PWM) {
    last_error_ = Error::OutOfRange;
    return false;
  }
  uint8_t data[4] = {static_cast<uint8_t>(on & 0xFF), static_cast<uint8_t>((on >> 8) & 0x0F),
                     static_cast<uint8_t>(off & 0xFF), static_cast<uint8_t>((off >> 8) & 0x0F)};
  if (!writeRegBlock(ALL_LED_ON_L, data, 4)) {
    last_error_ = Error::I2cWrite;
    return false;
  }
  last_error_ = Error::None;
  return true;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::GetPrescale(uint8_t& prescale) {
  if (!initialized_) {
    last_error_ = Error::NotInitialized;
    return false;
  }
  if (!readReg(PRE_SCALE, prescale)) {
    last_error_ = Error::I2cRead;
    return false;
  }
  last_error_ = Error::None;
  return true;
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::writeReg(uint8_t reg, uint8_t value) {
  return i2c_ && i2c_->Write(addr_, reg, &value, 1);
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::readReg(uint8_t reg, uint8_t& value) {
  return i2c_ && i2c_->Read(addr_, reg, &value, 1);
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::writeRegBlock(uint8_t reg, const uint8_t* data, size_t len) {
  return i2c_ && i2c_->Write(addr_, reg, data, len);
}

template <typename I2cType>
bool pca9685::PCA9685<I2cType>::readRegBlock(uint8_t reg, uint8_t* data, size_t len) {
  return i2c_ && i2c_->Read(addr_, reg, data, len);
}

template <typename I2cType>
uint8_t pca9685::PCA9685<I2cType>::calcPrescale(float freq_hz) const {
  float prescaleval = (OSC_FREQ / (4096.0f * freq_hz)) - 1.0f;
  if (prescaleval < 3.0f)
    prescaleval = 3.0f;
  if (prescaleval > 255.0f)
    prescaleval = 255.0f;
  return static_cast<uint8_t>(prescaleval + 0.5f);
}

// Note: Namespace is closed in pca9685.hpp, not here

#endif // PCA9685_IMPL