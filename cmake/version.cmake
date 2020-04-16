file(READ ${CMAKE_SOURCE_DIR}/VERSION ver)

string(REGEX MATCH "VERSION_MAJOR = ([0-9]*)" _ ${ver})
set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})

string(REGEX MATCH "VERSION_MINOR = ([0-9]*)" _ ${ver})
set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_1})

string(REGEX MATCH "EXTRAVERSION = ([a-z0-9]*)" _ ${ver})
set(PROJECT_VERSION_EXTRA ${CMAKE_MATCH_1})

set(VERSION_STR "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_EXTRA}")
message(STATUS "Project version: ${VERSION_STR}")

set(VMAJOR ${PROJECT_VERSION_MAJOR})
set(VMINOR ${PROJECT_VERSION_MINOR})
set(VEXTRA ${PROJECT_VERSION_EXTRA})

string(TIMESTAMP VERSION_DATE "%Y-%m-%d-%H-%M")

if((VMAJOR GREATER 9) OR (VMINOR GREATER 9) OR (VEXTRA GREATER 9))
    message(FATAL_ERROR "Version number cannot be greater than 9")
endif()

math(EXPR PRJ_VERSION "${VMAJOR} * 16 * 16 * 16 + ${VMINOR} * 16 * 16 + ${VEXTRA}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR PRJ_VERSION_BYTE "${VMAJOR} * 16 + ${VMINOR}" OUTPUT_FORMAT HEXADECIMAL)
set(PRJ_VERSION_STRING "\"${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_EXTRA}\"")