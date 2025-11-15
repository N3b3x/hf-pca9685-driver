/**
 * @file main.cpp
 * @brief ESP32 example for the PCA9685 driver using ESP-IDF I2C.
 *
 * Demonstrates basic usage of the PCA9685 driver with ESP-IDF I2C driver.
 * Requires a user-implemented I2cBus for ESP32 (see below).
 */

#include <stdio.h>

#include "driver/i2c.h"
#include "pca9685.hpp"

/**
 * @brief ESP32 I2C bus implementation for PCA9685 driver.
 */
class Esp32I2cBus : public pca9685::I2cInterface<Esp32I2cBus> {
public:
  bool Write(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, (uint8_t*)data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret == ESP_OK;
  }
  bool Read(uint8_t addr, uint8_t reg, uint8_t* data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret == ESP_OK;
  }
};

extern "C" void app_main() {
  // Initialize I2C peripheral here (not shown)
  Esp32I2cBus i2c;
  pca9685::PCA9685<Esp32I2cBus> pwm(&i2c, 0x40);
  if (!pwm.Reset()) {
    printf("Failed to initialize PCA9685!\n");
    return;
  }
  pwm.SetPwmFreq(50.0f);
  pwm.SetPwm(0, 0, 2048);
  printf("PCA9685 ESP32 example complete.\n");
}