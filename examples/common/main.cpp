/**
 * @file main.cpp
 * @brief Platform-agnostic example for the PCA9685 driver using a mock I2C bus.
 *
 * This example demonstrates basic usage of the PCA9685 driver with a dummy I2C implementation.
 * Replace the MockI2cBus with your platform's I2C implementation for real hardware.
 */

#include <cstdint>
#include <iostream>
#include <vector>

#include "pca9685.hpp"

/**
 * @brief Mock I2C bus for demonstration purposes.
 */
class MockI2cBus : public PCA9685::I2cBus {
   public:
    bool write(uint8_t addr, const uint8_t* data, size_t len) override {
        std::cout << "I2C Write to 0x" << std::hex << int(addr) << ", len=" << len << std::endl;
        return true;
    }
    bool read(uint8_t addr, uint8_t* data, size_t len) override {
        std::cout << "I2C Read from 0x" << std::hex << int(addr) << ", len=" << len << std::endl;
        // For demo, fill with zeros
        for (size_t i = 0; i < len; ++i) data[i] = 0;
        return true;
    }
};

/**
 * @brief Main entry point for the example.
 */
int main() {
    MockI2cBus i2c;
    PCA9685 pwm(&i2c, 0x40);  // Default address

    if (!pwm.reset()) {
        std::cerr << "Failed to initialize PCA9685!" << std::endl;
        return 1;
    }

    pwm.setPwmFreq(50.0f);   // Standard servo frequency
    pwm.setPwm(0, 0, 2048);  // 50% duty on channel 0
    pwm.setAllPwm(0, 1024);  // 25% duty on all channels

    std::cout << "PCA9685 example complete." << std::endl;
    return 0;
}