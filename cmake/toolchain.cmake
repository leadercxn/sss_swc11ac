if(NOT CMAKE_HOST_UNIX)
    set(CMAKE_OBJECT_PATH_MAX 240)
endif()

if(BUILD_HOST)
    set(TOOLCHAIN "gcc" CACHE STRING "Toolchain used for host build" FORCE)
else()
    set(TOOLCHAIN "gccarmemb" CACHE STRING "Toolchain used for compiling the target")
    set_property(CACHE TOOLCHAIN PROPERTY STRINGS "gccarmemb" "armcc" "clang")
endif()

set(CMAKE_BUILD_TYPE "RELEASE" CACHE STRING "")

if(EXISTS "${CMAKE_CONFIG_DIR}/toolchain/${TOOLCHAIN}.cmake")
    include("${CMAKE_CONFIG_DIR}/toolchain/${TOOLCHAIN}.cmake")
else()
    message(FATAL_ERROR "Toolchain \"${TOOLCHAIN}\" not recognized.")
endif()