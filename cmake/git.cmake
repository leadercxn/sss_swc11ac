find_package(Git QUIET)

set(SDK_URL "git@gitlab.sensoro.com:EndDevice/sdk.git" CACHE STRING "")
set(SDK_BRANCH "6e0f3aea" CACHE STRING "" FORCE)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/sdk")
    message(STATUS "Download remote SDK repo")
    if(GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} clone ${SDK_URL} ${CMAKE_SOURCE_DIR}/sdk
        )
        execute_process(
            COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR}/sdk submodule update --init --recursive
        )
    endif()
else()
    message(STATUS "Find sdk repo")
endif()

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        RESULT_VARIABLE SHORT_HASH_RESULT
        OUTPUT_VARIABLE SHORT_HASH
    )
    string(REGEX REPLACE "\n" "" SHORT_HASH ${SHORT_HASH})
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/sdk/nRF5_SDK_11.0.0_89a8197")
    set(SDK_ROOT "${CMAKE_SOURCE_DIR}/sdk" CACHE STRING "")
    set(NRF_SDK_ROOT "${CMAKE_SOURCE_DIR}/sdk/nRF5_SDK_11.0.0_89a8197" CACHE STRING "")
endif()