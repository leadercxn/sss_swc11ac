set(nrf51822_xxAC_ARCH "cortex-m0")

set(nrf51822_xxAC_SOURCE_FILES
    "${NRF_SDK_ROOT}/components/toolchain/system_nrf51.c")

set(nrf51822_xxAC_INCLUDE_DIRS
    "${NRF_SDK_ROOT}/components/toolchain/CMSIS/Include"
    "${NRF_SDK_ROOT}/components/toolchain"
)

if(TOOLCHAIN MATCHES "gcc" OR TOOLCHAIN STREQUAL "clang")
    set(nrf51822_xxAC_SOURCE_FILES ${nrf51822_xxAC_SOURCE_FILES}
        "${NRF_SDK_ROOT}/components/toolchain/gcc/gcc_startup_nrf51.s"
        "${NRF_SDK_ROOT}/components/libraries/hardfault/nrf51/handler/hardfault_handler_gcc.c")
    set(nrf51822_xxAC_INCLUDE_DIRS ${nrf51822_xxAC_INCLUDE_DIRS}
        "${NRF_SDK_ROOT}/components/toolchain/gcc")
    set(nrf51822_xxAC_LINK_INCLUDE_DIR
        "${NRF_SDK_ROOT}/components/toolchain/gcc")
elseif(TOOLCHAIN STREQUAL "armcc")
    set(nrf51822_xxAC_SOURCE_FILES ${nrf51822_xxAC_SOURCE_FILES}
        "${NRF_SDK_ROOT}/components/toolchain/arm/arm_startup_nrf51.s"
        "${NRF_SDK_ROOT}/components/libraries/hardfault/nrf51/handler/hardfault_handler_keil.c")
    set(nrf51822_xxAC_INCLUDE_DIRS ${nrf51822_xxAC_INCLUDE_DIRS}
        "${NRF_SDK_ROOT}/components/toolchain/arm")
    set(nrf51822_xxAC_LINK_INCLUDE_DIR
        "${NRF_SDK_ROOT}/components/toolchain/arm")
else()
    message(FATAL_ERROR "Unknown toolchain ${TOOLCHAIN}")
endif()

set(nrf51822_xxAC_DEFINES
    -DNRF51)

set(nrf51822_xxAC_FAMILY "NRF51")