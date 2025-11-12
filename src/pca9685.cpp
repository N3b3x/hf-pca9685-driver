/**
 * @file pca9685.cpp
 * @brief Hardware-agnostic implementation for the PCA9685 16-channel 12-bit PWM controller (I2C)
 *
 * @author Nebiyu Tadesse
 * @date 2025
 * @version 1.0
 */
#include "pca9685.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

PCA9685::PCA9685(I2cBus* bus, uint8_t address)
    : i2c_(bus), addr_(address), lastError_(Error::None), initialized_(false) {}

bool PCA9685::reset() {
  uint8_t mode1 = 0x00; // Reset value
  if (!writeReg(MODE1, mode1)) {
    lastError_ = Error::I2cWrite;
    return false;
  }
  initialized_ = true;
  lastError_ = Error::None;
  return true;
}

bool PCA9685::setPwmFreq(float freq_hz) {
  if (!initialized_) {
    lastError_ = Error::NotInitialized;
    return false;
  }
  if (freq_hz < 24.0f || freq_hz > 1526.0f) {
    lastError_ = Error::OutOfRange;
    return false;
  }
  uint8_t prescale = calcPrescale(freq_hz);
  uint8_t oldmode = 0;
  if (!readReg(MODE1, oldmode)) {
    lastError_ = Error::I2cRead;
    return false;
  }
  uint8_t sleep = (oldmode & 0x7F) | 0x10; // sleep
  if (!writeReg(MODE1, sleep)) {
    lastError_ = Error::I2cWrite;
    return false;
  }
  if (!writeReg(PRE_SCALE, prescale)) {
    lastError_ = Error::I2cWrite;
    return false;
  }
  if (!writeReg(MODE1, oldmode)) {
    lastError_ = Error::I2cWrite;
    return false;
  }
  // Wait for oscillator to stabilize (500us typical)
  // (User should delay if needed)
  lastError_ = Error::None;
  return true;
}

bool PCA9685::setPwm(uint8_t channel, uint16_t on, uint16_t off) {
  if (!initialized_) {
    lastError_ = Error::NotInitialized;
    return false;
  }
  if (channel >= MAX_CHANNELS || on > MAX_PWM || off > MAX_PWM) {
    lastError_ = Error::OutOfRange;
    return false;
  }
  uint8_t reg = LED0_ON_L + 4 * channel;
  uint8_t data[4] = {static_cast<uint8_t>(on & 0xFF), static_cast<uint8_t>((on >> 8) & 0x0F),
                     static_cast<uint8_t>(off & 0xFF), static_cast<uint8_t>((off >> 8) & 0x0F)};
  if (!writeRegBlock(reg, data, 4)) {
    lastError_ = Error::I2cWrite;
    return false;
  }
  lastError_ = Error::None;
  return true;
}

bool PCA9685::setDuty(uint8_t channel, float duty) {
  if (duty < 0.0f)
    duty = 0.0f;
  if (duty > 1.0f)
    duty = 1.0f;
  uint16_t off = static_cast<uint16_t>(duty * MAX_PWM + 0.5f);
  return setPwm(channel, 0, off);
}

bool PCA9685::setAllPwm(uint16_t on, uint16_t off) {
  if (!initialized_) {
    lastError_ = Error::NotInitialized;
    return false;
  }
  if (on > MAX_PWM || off > MAX_PWM) {
    lastError_ = Error::OutOfRange;
    return false;
  }
  uint8_t data[4] = {static_cast<uint8_t>(on & 0xFF), static_cast<uint8_t>((on >> 8) & 0x0F),
                     static_cast<uint8_t>(off & 0xFF), static_cast<uint8_t>((off >> 8) & 0x0F)};
  if (!writeRegBlock(ALL_LED_ON_L, data, 4)) {
    lastError_ = Error::I2cWrite;
    return false;
  }
  lastError_ = Error::None;
  return true;
}

PCA9685::Error PCA9685::getLastError() const {
  return lastError_;
}

bool PCA9685::getPrescale(uint8_t& prescale) {
  if (!initialized_) {
    lastError_ = Error::NotInitialized;
    return false;
  }
  if (!readReg(PRE_SCALE, prescale)) {
    lastError_ = Error::I2cRead;
    return false;
  }
  lastError_ = Error::None;
  return true;
}

bool PCA9685::writeReg(uint8_t reg, uint8_t value) {
  return i2c_ && i2c_->write(addr_, reg, &value, 1);
}

bool PCA9685::readReg(uint8_t reg, uint8_t& value) {
  return i2c_ && i2c_->read(addr_, reg, &value, 1);
}

bool PCA9685::writeRegBlock(uint8_t reg, const uint8_t* data, size_t len) {
  return i2c_ && i2c_->write(addr_, reg, data, len);
}

bool PCA9685::readRegBlock(uint8_t reg, uint8_t* data, size_t len) {
  return i2c_ && i2c_->read(addr_, reg, data, len);
}

uint8_t PCA9685::calcPrescale(float freq_hz) const {
  float prescaleval = (OSC_FREQ / (4096.0f * freq_hz)) - 1.0f;
  if (prescaleval < 3.0f)
    prescaleval = 3.0f;
  if (prescaleval > 255.0f)
    prescaleval = 255.0f;
  return static_cast<uint8_t>(prescaleval + 0.5f);
}