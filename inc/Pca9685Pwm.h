/**
 * @file Pca9685Pwm.h
 * @brief PCA9685 16-channel PWM controller driver.
 *
 * This file provides a BasePwm implementation for the PCA9685 16-channel 
 * 12-bit PWM controller. The PCA9685 is commonly used for servo control, 
 * LED dimming, and other PWM applications.
 *
 * Features:
 * - 16 PWM channels with 12-bit resolution
 * - I2C interface (up to 1MHz)
 * - Configurable frequency (24Hz to 1526Hz)
 * - Individual channel control
 * - All-call and sub-address support
 * - External clock input support
 * - Output enable pin control
 * - Totem-pole or open-drain outputs
 */
#ifndef HAL_INTERNAL_INTERFACE_DRIVERS_PCA9685PWM_H_
#define HAL_INTERNAL_INTERFACE_DRIVERS_PCA9685PWM_H_

#include "BasePwm.h"
#include "BaseI2c.h"
#include <memory>
#include <array>

/**
 * @class Pca9685Pwm
 * @brief PCA9685 PWM controller implementation.
 *
 * This class implements the BasePwm interface for the PCA9685 16-channel
 * 12-bit PWM controller. It provides full control over all 16 channels
 * with configurable frequency and duty cycle.
 *
 * Hardware connections:
 * - VCC: 2.3V to 5.5V
 * - GND: Ground
 * - SCL: I2C clock
 * - SDA: I2C data
 * - OE: Output enable (optional, active low)
 * - EXTCLK: External clock input (optional)
 * - A0-A5: I2C address selection pins
 *
 * Usage example:
 * @code
 * // Create I2C interface
 * auto i2c = std::make_unique<McuI2c>();
 * 
 * // Create PCA9685 PWM controller
 * Pca9685Pwm pwm_controller(std::move(i2c), 0x40);
 * 
 * if (pwm_controller.Initialize() == HfPwmErr::PWM_SUCCESS) {
 *     // Configure servo channel
 *     PwmChannelConfig config{};
 *     config.frequency_hz = 50; // 50Hz for servo
 *     config.resolution_bits = 12;
 *     config.initial_duty_cycle = 0.075f; // 1.5ms pulse (neutral)
 *     
 *     pwm_controller.ConfigureChannel(0, config);
 *     pwm_controller.Start(0);
 * }
 * @endcode
 */
class Pca9685Pwm : public BasePwm {
public:
    /**
     * @brief Default I2C address for PCA9685.
     */
    static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x40;
    
    /**
     * @brief Maximum number of PWM channels.
     */
    static constexpr uint8_t MAX_CHANNELS = 16;
    
    /**
     * @brief PWM resolution in bits.
     */
    static constexpr uint8_t PWM_RESOLUTION = 12;
    
    /**
     * @brief Maximum PWM value (2^12 - 1).
     */
    static constexpr uint16_t MAX_PWM_VALUE = 4095;
    
    /**
     * @brief Minimum frequency in Hz.
     */
    static constexpr uint32_t MIN_FREQUENCY = 24;
    
    /**
     * @brief Maximum frequency in Hz.
     */
    static constexpr uint32_t MAX_FREQUENCY = 1526;
    
    /**
     * @brief Internal oscillator frequency in Hz.
     */
    static constexpr uint32_t INTERNAL_OSC_FREQ = 25000000;

    /**
     * @brief Construct a new Pca9685Pwm object.
     * 
     * @param i2c_interface I2C interface for communication
     * @param i2c_address I2C address of the PCA9685 (default: 0x40)
     * @param output_enable_pin GPIO pin for output enable (optional)
     */
    explicit Pca9685Pwm(std::unique_ptr<BaseI2c> i2c_interface, 
                        uint8_t i2c_address = DEFAULT_I2C_ADDRESS,
                        int output_enable_pin = -1);
    
    /**
     * @brief Destroy the Pca9685Pwm object.
     */
    ~Pca9685Pwm() override;
    
    // BasePwm interface implementation
    HfPwmErr Initialize() override;
    HfPwmErr Deinitialize() override;
    HfPwmErr ConfigureChannel(uint8_t channel_id, const PwmChannelConfig& config) override;
    HfPwmErr SetDutyCycle(uint8_t channel_id, float duty_cycle) override;
    HfPwmErr SetFrequency(uint8_t channel_id, uint32_t frequency_hz) override;
    HfPwmErr Start(uint8_t channel_id) override;
    HfPwmErr Stop(uint8_t channel_id) override;
    HfPwmErr GetDutyCycle(uint8_t channel_id, float& duty_cycle) override;
    HfPwmErr GetFrequency(uint8_t channel_id, uint32_t& frequency_hz) override;
    bool IsChannelActive(uint8_t channel_id) override;
    uint8_t GetMaxChannels() override;
    
    // Advanced features (not supported by PCA9685)
    HfPwmErr SetPhase(uint8_t channel_id, float phase_degrees) override;
    HfPwmErr ConfigureFade(uint8_t channel_id, const PwmFadeConfig& fade_config) override;
    HfPwmErr StartFade(uint8_t channel_id) override;
    HfPwmErr ConfigureComplementary(uint8_t primary_channel, uint8_t secondary_channel, 
                                    const PwmComplementaryConfig& comp_config) override;
    HfPwmErr SetDeadTime(uint8_t channel_id, uint16_t dead_time_ns) override;
    
    // Callback management (not supported by PCA9685)
    HfPwmErr RegisterCallback(uint8_t channel_id, PwmCallbackType callback_type, 
                              PwmCallback callback, void* user_data) override;
    HfPwmErr UnregisterCallback(uint8_t channel_id, PwmCallbackType callback_type) override;
    
    // Multi-channel operations
    HfPwmErr StartMultiple(const uint8_t* channel_ids, uint8_t num_channels) override;
    HfPwmErr StopMultiple(const uint8_t* channel_ids, uint8_t num_channels) override;
    HfPwmErr SetDutyCycleMultiple(const uint8_t* channel_ids, const float* duty_cycles, uint8_t num_channels) override;
    
    /**
     * @brief PCA9685-specific methods
     */
    
    /**
     * @brief Set the output enable state.
     * 
     * @param enabled True to enable outputs, false to disable
     * @return HfPwmErr::PWM_SUCCESS on success
     */
    HfPwmErr SetOutputEnable(bool enabled);
    
    /**
     * @brief Configure external clock input.
     * 
     * @param external_clock_freq External clock frequency in Hz
     * @return HfPwmErr::PWM_SUCCESS on success
     */
    HfPwmErr ConfigureExternalClock(uint32_t external_clock_freq);
    
    /**
     * @brief Set output driver type.
     * 
     * @param totem_pole True for totem-pole, false for open-drain
     * @return HfPwmErr::PWM_SUCCESS on success
     */
    HfPwmErr SetOutputDriver(bool totem_pole);
    
    /**
     * @brief Set output inversion.
     * 
     * @param inverted True to invert outputs, false for normal
     * @return HfPwmErr::PWM_SUCCESS on success
     */
    HfPwmErr SetOutputInvert(bool inverted);
    
    /**
     * @brief Put the PCA9685 in sleep mode.
     * 
     * @return HfPwmErr::PWM_SUCCESS on success
     */
    HfPwmErr Sleep();
    
    /**
     * @brief Wake up the PCA9685 from sleep mode.
     * 
     * @return HfPwmErr::PWM_SUCCESS on success
     */
    HfPwmErr Wakeup();

private:
    /**
     * @brief PCA9685 register addresses.
     */
    enum class Registers : uint8_t {
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
        ALL_LED_ON_L = 0xFA,
        ALL_LED_ON_H = 0xFB,
        ALL_LED_OFF_L = 0xFC,
        ALL_LED_OFF_H = 0xFD,
        PRE_SCALE = 0xFE,
        TESTMODE = 0xFF
    };
    
    /**
     * @brief MODE1 register bits.
     */
    enum class Mode1Bits : uint8_t {
        RESTART = 0x80,
        EXTCLK = 0x40,
        AI = 0x20,
        SLEEP = 0x10,
        SUB1 = 0x08,
        SUB2 = 0x04,
        SUB3 = 0x02,
        ALLCALL = 0x01
    };
    
    /**
     * @brief MODE2 register bits.
     */
    enum class Mode2Bits : uint8_t {
        INVRT = 0x10,
        OCH = 0x08,
        OUTDRV = 0x04,
        OUTNE1 = 0x02,
        OUTNE0 = 0x01
    };
    
    /**
     * @brief Channel state structure.
     */
    struct ChannelState {
        bool is_active = false;
        float duty_cycle = 0.0f;
        uint16_t on_time = 0;
        uint16_t off_time = 0;
    };
    
    std::unique_ptr<BaseI2c> i2c_;
    uint8_t i2c_address_;
    int output_enable_pin_;
    bool is_initialized_;
    uint32_t current_frequency_;
    uint8_t prescale_value_;
    std::array<ChannelState, MAX_CHANNELS> channels_;
    
    /**
     * @brief Write a byte to a PCA9685 register.
     * 
     * @param reg Register address
     * @param value Value to write
     * @return HfPwmErr::PWM_SUCCESS on success
     */
    HfPwmErr WriteRegister(Registers reg, uint8_t value);
    
    /**
     * @brief Read a byte from a PCA9685 register.
     * 
     * @param reg Register address
     * @param value Reference to store the read value
     * @return HfPwmErr::PWM_SUCCESS on success
     */
    HfPwmErr ReadRegister(Registers reg, uint8_t& value);
    
    /**
     * @brief Calculate prescale value for given frequency.
     * 
     * @param frequency_hz Desired frequency in Hz
     * @return Prescale value
     */
    uint8_t CalculatePrescale(uint32_t frequency_hz);
    
    /**
     * @brief Set PWM timing for a channel.
     * 
     * @param channel_id Channel ID (0-15)
     * @param on_time On time (0-4095)
     * @param off_time Off time (0-4095)
     * @return HfPwmErr::PWM_SUCCESS on success
     */
    HfPwmErr SetChannelTiming(uint8_t channel_id, uint16_t on_time, uint16_t off_time);
    
    /**
     * @brief Convert duty cycle to PWM timing values.
     * 
     * @param duty_cycle Duty cycle (0.0 to 1.0)
     * @param on_time Reference to store on time
     * @param off_time Reference to store off time
     */
    void DutyCycleToTiming(float duty_cycle, uint16_t& on_time, uint16_t& off_time);
    
    /**
     * @brief Reset the PCA9685 to default state.
     * 
     * @return HfPwmErr::PWM_SUCCESS on success
     */
    HfPwmErr Reset();
    
    /**
     * @brief Validate channel ID.
     * 
     * @param channel_id Channel ID to validate
     * @return True if valid, false otherwise
     */
    bool IsValidChannel(uint8_t channel_id) const;
};

#endif // HAL_INTERNAL_INTERFACE_DRIVERS_PCA9685PWM_H_
