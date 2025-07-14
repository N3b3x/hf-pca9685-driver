/**
 * @file Pca9685Pwm.cpp
 * @brief PCA9685 16-channel PWM controller driver implementation.
 */

#include "Pca9685Pwm.h"
#include "BaseGpio.h"
#include <cmath>
#include <algorithm>

Pca9685Pwm::Pca9685Pwm(std::unique_ptr<BaseI2c> i2c_interface, 
                       uint8_t i2c_address,
                       int output_enable_pin)
    : i2c_(std::move(i2c_interface))
    , i2c_address_(i2c_address)
    , output_enable_pin_(output_enable_pin)
    , is_initialized_(false)
    , current_frequency_(1000)
    , prescale_value_(0) {
}

Pca9685Pwm::~Pca9685Pwm() {
    if (is_initialized_) {
        Deinitialize();
    }
}

HfPwmErr Pca9685Pwm::Initialize() {
    if (is_initialized_) {
        return HfPwmErr::PWM_SUCCESS;
    }
    
    if (!i2c_) {
        return HfPwmErr::PWM_INVALID_ARGUMENT;
    }
    
    // Initialize I2C interface
    if (i2c_->Initialize() != HfI2cErr::I2C_SUCCESS) {
        return HfPwmErr::PWM_HARDWARE_ERROR;
    }
    
    // Initialize output enable pin if specified
    if (output_enable_pin_ >= 0) {
        // Configure OE pin as output and enable outputs (active low)
        // Note: This assumes DigitalGpio interface exists
        // Implementation would depend on your GPIO system
    }
    
    // Reset the PCA9685
    HfPwmErr result = Reset();
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Set default frequency (1kHz)
    result = SetFrequency(0, current_frequency_);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Configure MODE2 register
    uint8_t mode2 = static_cast<uint8_t>(Mode2Bits::OUTDRV); // Totem-pole output
    result = WriteRegister(Registers::MODE2, mode2);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Wake up the device
    result = Wakeup();
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    is_initialized_ = true;
    return HfPwmErr::PWM_SUCCESS;
}

HfPwmErr Pca9685Pwm::Deinitialize() {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    // Stop all channels
    for (uint8_t i = 0; i < MAX_CHANNELS; i++) {
        Stop(i);
    }
    
    // Put device to sleep
    Sleep();
    
    // Disable outputs if OE pin is used
    if (output_enable_pin_ >= 0) {
        // Set OE pin high to disable outputs
        // Implementation would depend on your GPIO system
    }
    
    // Deinitialize I2C interface
    if (i2c_) {
        i2c_->Deinitialize();
    }
    
    is_initialized_ = false;
    return HfPwmErr::PWM_SUCCESS;
}

HfPwmErr Pca9685Pwm::ConfigureChannel(uint8_t channel_id, const PwmChannelConfig& config) {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    if (!IsValidChannel(channel_id)) {
        return HfPwmErr::PWM_INVALID_CHANNEL;
    }
    
    // Validate frequency range
    if (config.frequency_hz < MIN_FREQUENCY || config.frequency_hz > MAX_FREQUENCY) {
        return HfPwmErr::PWM_INVALID_FREQUENCY;
    }
    
    // Validate duty cycle range
    if (config.initial_duty_cycle < 0.0f || config.initial_duty_cycle > 1.0f) {
        return HfPwmErr::PWM_INVALID_DUTY_CYCLE;
    }
    
    // Set frequency (affects all channels)
    HfPwmErr result = SetFrequency(channel_id, config.frequency_hz);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Set initial duty cycle
    result = SetDutyCycle(channel_id, config.initial_duty_cycle);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    return HfPwmErr::PWM_SUCCESS;
}

HfPwmErr Pca9685Pwm::SetDutyCycle(uint8_t channel_id, float duty_cycle) {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    if (!IsValidChannel(channel_id)) {
        return HfPwmErr::PWM_INVALID_CHANNEL;
    }
    
    if (duty_cycle < 0.0f || duty_cycle > 1.0f) {
        return HfPwmErr::PWM_INVALID_DUTY_CYCLE;
    }
    
    uint16_t on_time, off_time;
    DutyCycleToTiming(duty_cycle, on_time, off_time);
    
    HfPwmErr result = SetChannelTiming(channel_id, on_time, off_time);
    if (result == HfPwmErr::PWM_SUCCESS) {
        channels_[channel_id].duty_cycle = duty_cycle;
        channels_[channel_id].on_time = on_time;
        channels_[channel_id].off_time = off_time;
    }
    
    return result;
}

HfPwmErr Pca9685Pwm::SetFrequency(uint8_t channel_id, uint32_t frequency_hz) {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    if (!IsValidChannel(channel_id)) {
        return HfPwmErr::PWM_INVALID_CHANNEL;
    }
    
    if (frequency_hz < MIN_FREQUENCY || frequency_hz > MAX_FREQUENCY) {
        return HfPwmErr::PWM_INVALID_FREQUENCY;
    }
    
    // Calculate prescale value
    uint8_t prescale = CalculatePrescale(frequency_hz);
    
    // Read current MODE1 register
    uint8_t mode1;
    HfPwmErr result = ReadRegister(Registers::MODE1, mode1);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Set sleep bit to write prescale
    mode1 |= static_cast<uint8_t>(Mode1Bits::SLEEP);
    result = WriteRegister(Registers::MODE1, mode1);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Write prescale value
    result = WriteRegister(Registers::PRE_SCALE, prescale);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Clear sleep bit
    mode1 &= ~static_cast<uint8_t>(Mode1Bits::SLEEP);
    result = WriteRegister(Registers::MODE1, mode1);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Wait for oscillator to stabilize (500us minimum)
    // Note: You may need to implement a delay function
    // DelayUs(500);
    
    current_frequency_ = frequency_hz;
    prescale_value_ = prescale;
    
    return HfPwmErr::PWM_SUCCESS;
}

HfPwmErr Pca9685Pwm::Start(uint8_t channel_id) {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    if (!IsValidChannel(channel_id)) {
        return HfPwmErr::PWM_INVALID_CHANNEL;
    }
    
    // Set channel timing to start PWM
    HfPwmErr result = SetChannelTiming(channel_id, 
                                       channels_[channel_id].on_time,
                                       channels_[channel_id].off_time);
    if (result == HfPwmErr::PWM_SUCCESS) {
        channels_[channel_id].is_active = true;
    }
    
    return result;
}

HfPwmErr Pca9685Pwm::Stop(uint8_t channel_id) {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    if (!IsValidChannel(channel_id)) {
        return HfPwmErr::PWM_INVALID_CHANNEL;
    }
    
    // Set channel to always off
    HfPwmErr result = SetChannelTiming(channel_id, 0, MAX_PWM_VALUE);
    if (result == HfPwmErr::PWM_SUCCESS) {
        channels_[channel_id].is_active = false;
    }
    
    return result;
}

HfPwmErr Pca9685Pwm::GetDutyCycle(uint8_t channel_id, float& duty_cycle) {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    if (!IsValidChannel(channel_id)) {
        return HfPwmErr::PWM_INVALID_CHANNEL;
    }
    
    duty_cycle = channels_[channel_id].duty_cycle;
    return HfPwmErr::PWM_SUCCESS;
}

HfPwmErr Pca9685Pwm::GetFrequency(uint8_t channel_id, uint32_t& frequency_hz) {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    if (!IsValidChannel(channel_id)) {
        return HfPwmErr::PWM_INVALID_CHANNEL;
    }
    
    frequency_hz = current_frequency_;
    return HfPwmErr::PWM_SUCCESS;
}

bool Pca9685Pwm::IsChannelActive(uint8_t channel_id) {
    if (!is_initialized_ || !IsValidChannel(channel_id)) {
        return false;
    }
    
    return channels_[channel_id].is_active;
}

uint8_t Pca9685Pwm::GetMaxChannels() {
    return MAX_CHANNELS;
}

// Advanced features (not supported by PCA9685)
HfPwmErr Pca9685Pwm::SetPhase(uint8_t channel_id, float phase_degrees) {
    return HfPwmErr::PWM_NOT_SUPPORTED;
}

HfPwmErr Pca9685Pwm::ConfigureFade(uint8_t channel_id, const PwmFadeConfig& fade_config) {
    return HfPwmErr::PWM_NOT_SUPPORTED;
}

HfPwmErr Pca9685Pwm::StartFade(uint8_t channel_id) {
    return HfPwmErr::PWM_NOT_SUPPORTED;
}

HfPwmErr Pca9685Pwm::ConfigureComplementary(uint8_t primary_channel, uint8_t secondary_channel, 
                                            const PwmComplementaryConfig& comp_config) {
    return HfPwmErr::PWM_NOT_SUPPORTED;
}

HfPwmErr Pca9685Pwm::SetDeadTime(uint8_t channel_id, uint16_t dead_time_ns) {
    return HfPwmErr::PWM_NOT_SUPPORTED;
}

// Callback management (not supported by PCA9685)
HfPwmErr Pca9685Pwm::RegisterCallback(uint8_t channel_id, PwmCallbackType callback_type, 
                                      PwmCallback callback, void* user_data) {
    return HfPwmErr::PWM_NOT_SUPPORTED;
}

HfPwmErr Pca9685Pwm::UnregisterCallback(uint8_t channel_id, PwmCallbackType callback_type) {
    return HfPwmErr::PWM_NOT_SUPPORTED;
}

// Multi-channel operations
HfPwmErr Pca9685Pwm::StartMultiple(const uint8_t* channel_ids, uint8_t num_channels) {
    if (!channel_ids || num_channels == 0) {
        return HfPwmErr::PWM_INVALID_ARGUMENT;
    }
    
    for (uint8_t i = 0; i < num_channels; i++) {
        HfPwmErr result = Start(channel_ids[i]);
        if (result != HfPwmErr::PWM_SUCCESS) {
            return result;
        }
    }
    
    return HfPwmErr::PWM_SUCCESS;
}

HfPwmErr Pca9685Pwm::StopMultiple(const uint8_t* channel_ids, uint8_t num_channels) {
    if (!channel_ids || num_channels == 0) {
        return HfPwmErr::PWM_INVALID_ARGUMENT;
    }
    
    for (uint8_t i = 0; i < num_channels; i++) {
        HfPwmErr result = Stop(channel_ids[i]);
        if (result != HfPwmErr::PWM_SUCCESS) {
            return result;
        }
    }
    
    return HfPwmErr::PWM_SUCCESS;
}

HfPwmErr Pca9685Pwm::SetDutyCycleMultiple(const uint8_t* channel_ids, const float* duty_cycles, uint8_t num_channels) {
    if (!channel_ids || !duty_cycles || num_channels == 0) {
        return HfPwmErr::PWM_INVALID_ARGUMENT;
    }
    
    for (uint8_t i = 0; i < num_channels; i++) {
        HfPwmErr result = SetDutyCycle(channel_ids[i], duty_cycles[i]);
        if (result != HfPwmErr::PWM_SUCCESS) {
            return result;
        }
    }
    
    return HfPwmErr::PWM_SUCCESS;
}

// PCA9685-specific methods
HfPwmErr Pca9685Pwm::SetOutputEnable(bool enabled) {
    if (output_enable_pin_ < 0) {
        return HfPwmErr::PWM_NOT_SUPPORTED;
    }
    
    // Set OE pin (active low)
    // Implementation would depend on your GPIO system
    // gpio->SetLevel(output_enable_pin_, enabled ? 0 : 1);
    
    return HfPwmErr::PWM_SUCCESS;
}

HfPwmErr Pca9685Pwm::ConfigureExternalClock(uint32_t external_clock_freq) {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    // Read current MODE1 register
    uint8_t mode1;
    HfPwmErr result = ReadRegister(Registers::MODE1, mode1);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Set external clock bit
    mode1 |= static_cast<uint8_t>(Mode1Bits::EXTCLK);
    result = WriteRegister(Registers::MODE1, mode1);
    
    return result;
}

HfPwmErr Pca9685Pwm::SetOutputDriver(bool totem_pole) {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    // Read current MODE2 register
    uint8_t mode2;
    HfPwmErr result = ReadRegister(Registers::MODE2, mode2);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Set output driver bit
    if (totem_pole) {
        mode2 |= static_cast<uint8_t>(Mode2Bits::OUTDRV);
    } else {
        mode2 &= ~static_cast<uint8_t>(Mode2Bits::OUTDRV);
    }
    
    result = WriteRegister(Registers::MODE2, mode2);
    
    return result;
}

HfPwmErr Pca9685Pwm::SetOutputInvert(bool inverted) {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    // Read current MODE2 register
    uint8_t mode2;
    HfPwmErr result = ReadRegister(Registers::MODE2, mode2);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Set output invert bit
    if (inverted) {
        mode2 |= static_cast<uint8_t>(Mode2Bits::INVRT);
    } else {
        mode2 &= ~static_cast<uint8_t>(Mode2Bits::INVRT);
    }
    
    result = WriteRegister(Registers::MODE2, mode2);
    
    return result;
}

HfPwmErr Pca9685Pwm::Sleep() {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    // Read current MODE1 register
    uint8_t mode1;
    HfPwmErr result = ReadRegister(Registers::MODE1, mode1);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Set sleep bit
    mode1 |= static_cast<uint8_t>(Mode1Bits::SLEEP);
    result = WriteRegister(Registers::MODE1, mode1);
    
    return result;
}

HfPwmErr Pca9685Pwm::Wakeup() {
    if (!is_initialized_) {
        return HfPwmErr::PWM_NOT_INITIALIZED;
    }
    
    // Read current MODE1 register
    uint8_t mode1;
    HfPwmErr result = ReadRegister(Registers::MODE1, mode1);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Clear sleep bit
    mode1 &= ~static_cast<uint8_t>(Mode1Bits::SLEEP);
    result = WriteRegister(Registers::MODE1, mode1);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Wait for oscillator to stabilize (500us minimum)
    // Note: You may need to implement a delay function
    // DelayUs(500);
    
    return result;
}

// Private methods
HfPwmErr Pca9685Pwm::WriteRegister(Registers reg, uint8_t value) {
    if (!i2c_) {
        return HfPwmErr::PWM_HARDWARE_ERROR;
    }
    
    uint8_t data[2] = {static_cast<uint8_t>(reg), value};
    
    if (i2c_->Write(i2c_address_, data, 2) != HfI2cErr::I2C_SUCCESS) {
        return HfPwmErr::PWM_HARDWARE_ERROR;
    }
    
    return HfPwmErr::PWM_SUCCESS;
}

HfPwmErr Pca9685Pwm::ReadRegister(Registers reg, uint8_t& value) {
    if (!i2c_) {
        return HfPwmErr::PWM_HARDWARE_ERROR;
    }
    
    uint8_t reg_addr = static_cast<uint8_t>(reg);
    
    if (i2c_->WriteRead(i2c_address_, &reg_addr, 1, &value, 1) != HfI2cErr::I2C_SUCCESS) {
        return HfPwmErr::PWM_HARDWARE_ERROR;
    }
    
    return HfPwmErr::PWM_SUCCESS;
}

uint8_t Pca9685Pwm::CalculatePrescale(uint32_t frequency_hz) {
    // Prescale = round(OSC_FREQ / (4096 * frequency)) - 1
    float prescale_float = static_cast<float>(INTERNAL_OSC_FREQ) / (4096.0f * frequency_hz) - 1.0f;
    uint8_t prescale = static_cast<uint8_t>(std::round(prescale_float));
    
    // Clamp to valid range (3-255)
    return std::max(static_cast<uint8_t>(3), std::min(static_cast<uint8_t>(255), prescale));
}

HfPwmErr Pca9685Pwm::SetChannelTiming(uint8_t channel_id, uint16_t on_time, uint16_t off_time) {
    if (!IsValidChannel(channel_id)) {
        return HfPwmErr::PWM_INVALID_CHANNEL;
    }
    
    // Calculate register addresses for this channel
    uint8_t base_reg = static_cast<uint8_t>(Registers::LED0_ON_L) + (channel_id * 4);
    
    // Write ON_L register
    HfPwmErr result = WriteRegister(static_cast<Registers>(base_reg), on_time & 0xFF);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Write ON_H register
    result = WriteRegister(static_cast<Registers>(base_reg + 1), (on_time >> 8) & 0xFF);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Write OFF_L register
    result = WriteRegister(static_cast<Registers>(base_reg + 2), off_time & 0xFF);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    // Write OFF_H register
    result = WriteRegister(static_cast<Registers>(base_reg + 3), (off_time >> 8) & 0xFF);
    if (result != HfPwmErr::PWM_SUCCESS) {
        return result;
    }
    
    return HfPwmErr::PWM_SUCCESS;
}

void Pca9685Pwm::DutyCycleToTiming(float duty_cycle, uint16_t& on_time, uint16_t& off_time) {
    if (duty_cycle <= 0.0f) {
        on_time = 0;
        off_time = MAX_PWM_VALUE;
    } else if (duty_cycle >= 1.0f) {
        on_time = MAX_PWM_VALUE;
        off_time = 0;
    } else {
        on_time = 0;
        off_time = static_cast<uint16_t>(duty_cycle * MAX_PWM_VALUE);
    }
}

HfPwmErr Pca9685Pwm::Reset() {
    if (!i2c_) {
        return HfPwmErr::PWM_HARDWARE_ERROR;
    }
    
    // Software reset using general call address
    uint8_t reset_cmd[2] = {0x00, 0x06}; // General call address, SWRST command
    
    if (i2c_->Write(0x00, reset_cmd, 2) != HfI2cErr::I2C_SUCCESS) {
        return HfPwmErr::PWM_HARDWARE_ERROR;
    }
    
    // Wait for reset to complete
    // DelayMs(10);
    
    // Initialize all channels to off state
    for (uint8_t i = 0; i < MAX_CHANNELS; i++) {
        channels_[i] = ChannelState{};
    }
    
    return HfPwmErr::PWM_SUCCESS;
}

bool Pca9685Pwm::IsValidChannel(uint8_t channel_id) const {
    return channel_id < MAX_CHANNELS;
}
