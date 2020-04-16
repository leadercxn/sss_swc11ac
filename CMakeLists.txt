cmake_minimum_required(VERSION 3.14)

option(BUILD_HOST "Build for host (unit test build)" OFF)

if(NOT BUILD_HOST)
    set(CMAKE_SYSTEM_NAME "Generic")
    set(CMAKE_SYSTEM_PROCESSOR "ARM")
endif()

project(template LANGUAGES NONE)

set(CMAKE_CONFIG_DIR "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR} CACHE STRING "" FORCE)
set(PROJECT_FIRMWARE_DIR "${CMAKE_SOURCE_DIR}/firmware" CACHE STRING "")
# Put your prepared bootloader hex file in bootloader folder and change following file path
set(BOOTLOADER_FILE "${CMAKE_SOURCE_DIR}/bootloader/bootloader.hex" CACHE STRING "")

include("${CMAKE_CONFIG_DIR}/git.cmake")
include("${CMAKE_CONFIG_DIR}/platform.cmake")
include("${CMAKE_CONFIG_DIR}/toolchain.cmake")

add_custom_target(pristine
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CONFIG_DIR}/pristine.cmake    
    
#CMAKE_COMMAND   -- the full path to the cmake executable .
# Equivalent to rm -rf build/*
)

if(NOT SDK_ROOT)
    return()
endif()

include("${CMAKE_CONFIG_DIR}/version.cmake")
include("${CMAKE_CONFIG_DIR}/board.cmake")

include("${CMAKE_CONFIG_DIR}/sdk/nRF5_SDK_11.cmake")
include("${CMAKE_CONFIG_DIR}/sdk/project_sdk.cmake")
include("${CMAKE_CONFIG_DIR}/platform/${PLATFORM}.cmake")
include("${CMAKE_CONFIG_DIR}/board/${BOARD}.cmake")

message(STATUS "Platform: ${PLATFORM}")
message(STATUS "Arch: ${${PLATFORM}_ARCH}")
message(STATUS "Board: ${BOARD}")
message(STATUS "Toolchain: ${TOOLCHAIN}")

configure_file(version.h.in ${CMAKE_SOURCE_DIR}/src/config/version.h)

set(ARCH ${${PLATFORM}_ARCH})

enable_language(C ASM)

if(NOT BUILD_HOST)
    set(CMAKE_EXECUTABLE_SUFFIX ".elf")
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
endif()

if(NOT BUILD_HOST)
    include("${CMAKE_CONFIG_DIR}/nrfjprog.cmake")
    add_subdirectory("src")
endif()