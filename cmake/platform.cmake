if(BUILD_HOST)
else()
    set(PLATFORM "nrf51822_xxAC"
        CACHE STRING "Choose the target platform to build for.")
    set_property(CACHE PLATFORM PROPERTY STRINGS
        "nrf51822_xxAC")
endif()

if(NOT EXISTS "${CMAKE_CONFIG_DIR}/platform/${PLATFORM}.cmake")
    message(FATAL_ERROR "Platform specific file for ${PLATFORM} not found")
endif()