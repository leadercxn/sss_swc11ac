find_program(NRFJPROG nrfjprog)
find_program(MERGEHEX mergehex)
find_program(NRFUTIL nrfutil)

# As here I used a 3rd party tools
if(CMAKE_HOST_WIN32)
    find_program(NRFUTIL adafruit-nrfutil)
elseif(CMAKE_HOST_UNIX)
    find_program(NRFUTIL nrfutil)
endif()

if(NOT NRFJPROG)
    message(STATUS "Tool nrfjprog: not set")
else()
    message(STATUS "Tool nrfjprog: ${NRFJPROG}")
endif()

if(NOT MERGEHEX)
    message(STATUS "Tool mergehex: not set")
else()
    message(STATUS "Tool mergehex: ${MERGEHEX}")
endif()

if(NOT NRFUTIL)
    message(STATUS "Tool nrfutil: not set")
else()
    message(STATUS "Tool nrfutil: ${NRFUTIL}")
endif()

if(NRFJPROG AND MERGEHEX AND NRFUTIL)
    add_custom_target(merge)
    add_custom_target(flash)
    add_custom_target(release)
    add_custom_target(dfu)

    function(add_flash_target target)
        # Both the manual <merge> and <flash> target and depends on
        # the custom command that generates the merged hexfile.
        add_custom_target(merge_${target}
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}_merged.hex)

        add_dependencies(merge merge_${target})

        # Flash target command
        add_custom_target(flash_${target}
            COMMAND ${NRFJPROG} --program ${CMAKE_CURRENT_BINARY_DIR}/${target}.hex -f ${${PLATFORM}_FAMILY} --sectorerase --reset
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}.hex)

        add_dependencies(flash flash_${target})

        # Package release firmware command
        add_custom_target(release_${target}
            COMMAND ${MERGEHEX} -m ${BOOTLOADER_FILE} ${${SOFTDEVICE}_HEX_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${target}.hex -o ${PROJECT_FIRMWARE_DIR}/${target}_${VERSION_STR}_${SHORT_HASH}_${VERSION_DATE}.hex
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}.hex)

        add_dependencies(release release_${target})

        add_custom_target(dfu_${target}
            COMMAND ${NRFUTIL} dfu genpkg --application ${CMAKE_CURRENT_BINARY_DIR}/${target}.hex --application-version 0xff 
            --dev-revision 0xffff --dev-type 0xffff --sd-req 0x80 ${PROJECT_FIRMWARE_DIR}/${target}_dfu_${VERSION_STR}_${SHORT_HASH}_${VERSION_DATE}.zip)

        add_dependencies(dfu dfu_${target})

        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${target}_merged.hex
            COMMAND ${MERGEHEX} -m ${${SOFTDEVICE}_HEX_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${target}.hex -o ${CMAKE_CURRENT_BINARY_DIR}/${target}_merged.hex
            DEPENDS ${target}
            VERBATIM)
    endfunction(add_flash_target)

    add_custom_target(erase
        COMMAND ${NRFJPROG} -e)

    add_custom_target(sd
        COMMAND ${NRFJPROG} --program ${${SOFTDEVICE}_HEX_FILE} -f ${${PLATFORM}_FAMILY})

else()
    message(STATUS "Could not find nRFx command line tools")
    function(add_flash_target target)
        # Not supported
    endfunction()
endif()
