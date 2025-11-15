cmake_minimum_required(VERSION 3.10)

# PCA9685 PWM Driver Component
set(COMPONENT_NAME hf_pca9685_pwm)

# Source files
set(COMPONENT_SRCS
    "src/Pca9685Pwm.cpp"
)

# Include directories
set(COMPONENT_INCLUDE_DIRS
    "inc"
    "../../../internal/hf-internal-interface-wrap/inc"
)

# Create the component
idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
    REQUIRES driver
)

# Component information
message(STATUS "PCA9685 PWM Driver Component configured")
