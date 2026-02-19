###############################################################################
# HF PCA9685 Driver - ESP-IDF Component Wrapper
###############################################################################
# Purpose:
#   Adapt the generic, hardware-agnostic driver target contract to ESP-IDF's
#   component model using idf_component_register().
#
# Key Rule:
#   Do not duplicate source/include/dependency definitions here.
#   Always consume the shared build settings from driver root.
#
# Note:
#   cmake_minimum_required() is intentionally omitted here — ESP-IDF's
#   project.cmake already sets it and adding another can cause policy conflicts.
###############################################################################

# =============================================================================
# Shared Build Contract Import
# =============================================================================
# Resolve driver root from this component folder and import canonical variables.
# Users can override HF_PCA9685_ROOT if the component is placed outside
# the default directory structure (e.g., referencing via EXTRA_COMPONENT_DIRS).
set(HF_PCA9685_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." CACHE PATH
    "Path to root of hf-pca9685-driver (auto-detected from component location)")

# Normalise and validate the resolved path
get_filename_component(HF_PCA9685_ROOT "${HF_PCA9685_ROOT}" ABSOLUTE)
if(NOT EXISTS "${HF_PCA9685_ROOT}/cmake/hf_pca9685_build_settings.cmake")
    message(FATAL_ERROR
        "Cannot find PCA9685 driver root.\n"
        "  Resolved path: ${HF_PCA9685_ROOT}\n"
        "  Expected file: ${HF_PCA9685_ROOT}/cmake/hf_pca9685_build_settings.cmake\n"
        "  Fix: set HF_PCA9685_ROOT to the correct driver checkout path.")
endif()

include("${HF_PCA9685_ROOT}/cmake/hf_pca9685_build_settings.cmake")

# =============================================================================
# ESP-IDF Component Registration
# =============================================================================
# The component maps the shared contract directly into ESP-IDF primitives.
idf_component_register(
    SRCS ${HF_PCA9685_SOURCE_FILES}
    INCLUDE_DIRS ${HF_PCA9685_PUBLIC_INCLUDE_DIRS}
    REQUIRES ${HF_PCA9685_IDF_REQUIRES}
)

# =============================================================================
# C++ Standard Contract
# =============================================================================
# If sources exist, component is a compiled library -> PRIVATE feature.
# If no sources exist (header-only), component behaves like INTERFACE ->
# INTERFACE feature ensures downstream targets still get the requirement.
if(HF_PCA9685_SOURCE_FILES)
    target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
else()
    target_compile_features(${COMPONENT_LIB} INTERFACE cxx_std_20)
endif()
