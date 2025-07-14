/**
 * @file main.ino
 * @brief Arduino example for the PCA9685 driver.
 *
 * Demonstrates basic usage of the PCA9685 driver with the Arduino Wire library.
 * Requires a user-implemented I2cBus for Arduino (see below).
 */

#include <Wire.h>

#include "pca9685.hpp"

/**
 * @brief Arduino I2C bus implementation for PCA9685 driver.
 */
class ArduinoI2cBus : public PCA9685::I2cBus {
   public:
    bool write(uint8_t addr, const uint8_t* data, size_t len) override {
        Wire.beginTransmission(addr);
        Wire.write(data, len);
        return Wire.endTransmission() == 0;
    }
    bool read(uint8_t addr, uint8_t* data, size_t len) override {
        Wire.requestFrom(addr, (uint8_t)len);
        for (size_t i = 0; i < len && Wire.available(); ++i) {
            data[i] = Wire.read();
        }
        return true;
    }
};

ArduinoI2cBus i2c;
PCA9685 pwm(&i2c, 0x40);

void setup() {
    Wire.begin();
    Serial.begin(115200);
    if (!pwm.reset()) {
        Serial.println("Failed to initialize PCA9685!");
        while (1);
    }
    pwm.setPwmFreq(60.0f);  // 60 Hz for LEDs/servos
    pwm.setPwm(0, 0, 2048); // 50% duty on channel 0
}

void loop() {
    // Sweep channel 0
    for (uint16_t off = 0; off < 4096; off += 256) {
        pwm.setPwm(0, 0, off);
        delay(100);
    }
}