#===============================================================================
# PCA9685 Driver - Build Settings
# Shared variables for target name, includes, sources, and dependencies.
# This file is the SINGLE SOURCE OF TRUTH for the driver version.
#===============================================================================

include_guard(GLOBAL)

# Target name
set(HF_PCA9685_TARGET_NAME "hf_pca9685")

#===============================================================================
# Versioning (single source of truth)
#===============================================================================
set(HF_PCA9685_VERSION_MAJOR 1)
set(HF_PCA9685_VERSION_MINOR 0)
set(HF_PCA9685_VERSION_PATCH 0)
set(HF_PCA9685_VERSION "${HF_PCA9685_VERSION_MAJOR}.${HF_PCA9685_VERSION_MINOR}.${HF_PCA9685_VERSION_PATCH}")

#===============================================================================
# Generate version header from template (into build directory)
#===============================================================================
# The generated header is placed in CMAKE_CURRENT_BINARY_DIR to keep the
# source tree clean.  This eliminates the need for a .gitignore entry and
# allows parallel builds with different version stamps.
set(HF_PCA9685_VERSION_TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/../inc/pca9685_version.h.in")
set(HF_PCA9685_VERSION_HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/hf_pca9685_generated")
set(HF_PCA9685_VERSION_HEADER     "${HF_PCA9685_VERSION_HEADER_DIR}/pca9685_version.h")

# Ensure the output directory exists before configure_file and before any
# consumer (e.g. ESP-IDF idf_component_register) validates include paths.
file(MAKE_DIRECTORY "${HF_PCA9685_VERSION_HEADER_DIR}")

if(EXISTS "${HF_PCA9685_VERSION_TEMPLATE}")
    configure_file(
        "${HF_PCA9685_VERSION_TEMPLATE}"
        "${HF_PCA9685_VERSION_HEADER}"
        @ONLY
    )
    message(STATUS "PCA9685 driver v${HF_PCA9685_VERSION} — generated pca9685_version.h in ${HF_PCA9685_VERSION_HEADER_DIR}")
else()
    message(WARNING "pca9685_version.h.in not found at ${HF_PCA9685_VERSION_TEMPLATE}")
endif()

#===============================================================================
# Public include directories
#===============================================================================
# Two include directories:
#   1. Source tree inc/ — hand-written headers (pca9685.hpp, etc.)
#   2. Build tree generated dir — configure_file outputs (pca9685_version.h)
set(HF_PCA9685_PUBLIC_INCLUDE_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../inc"
    "${HF_PCA9685_VERSION_HEADER_DIR}"
)

#===============================================================================
# Source files (empty for header-only)
#===============================================================================
set(HF_PCA9685_SOURCE_FILES)

#===============================================================================
# ESP-IDF component dependencies
#===============================================================================
set(HF_PCA9685_IDF_REQUIRES driver)
