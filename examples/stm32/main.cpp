/**
 * @file main.cpp
 * @brief STM32 example for the PCA9685 driver using STM32 HAL I2C.
 *
 * Demonstrates basic usage of the PCA9685 driver with STM32 HAL I2C driver.
 * Requires a user-implemented I2cBus for STM32 (see below).
 */

#include "pca9685.hpp"
#include "stm32f4xx_hal.h"  // Adjust for your STM32 family

/**
 * @brief STM32 I2C bus implementation for PCA9685 driver.
 */
class Stm32I2cBus : public PCA9685::I2cBus {
   public:
    Stm32I2cBus(I2C_HandleTypeDef* hi2c) : hi2c_(hi2c) {}
    bool write(uint8_t addr, const uint8_t* data, size_t len) override {
        return HAL_I2C_Master_Transmit(hi2c_, addr << 1, (uint8_t*)data, len, 100) == HAL_OK;
    }
    bool read(uint8_t addr, uint8_t* data, size_t len) override {
        return HAL_I2C_Master_Receive(hi2c_, addr << 1, data, len, 100) == HAL_OK;
    }

   private:
    I2C_HandleTypeDef* hi2c_;
};

int main() {
    // Initialize HAL and I2C peripheral here (not shown)
    I2C_HandleTypeDef hi2c1;
    // ... HAL and I2C init code ...
    Stm32I2cBus i2c(&hi2c1);
    PCA9685 pwm(&i2c, 0x40);
    if (!pwm.reset()) {
        // Error handling
        while (1);
    }
    pwm.setPwmFreq(50.0f);
    pwm.setPwm(0, 0, 2048);
    // Main loop ...
    while (1) {
    }
}