function(add_ch585_keyboard_firmware target)
    set(options HID WEBUSB_CONFIG)
    set(one_value_arguments FLASH_ORIGIN FLASH_SIZE)
    set(multi_value_arguments SOURCES LIBRARIES)
    cmake_parse_arguments(FIRMWARE "${options}" "${one_value_arguments}"
                          "${multi_value_arguments}" ${ARGN})

    if(FIRMWARE_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Unknown arguments for ${target}: ${FIRMWARE_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT FIRMWARE_SOURCES OR NOT FIRMWARE_FLASH_ORIGIN OR
       NOT FIRMWARE_FLASH_SIZE)
        message(FATAL_ERROR
            "${target} requires SOURCES, FLASH_ORIGIN, and FLASH_SIZE")
    endif()

    add_executable(${target} ${FIRMWARE_SOURCES})
    add_dependencies(${target} keyboard_build_info)
    target_include_directories(${target} PRIVATE
        ${KEYBOARD_BUILD_INFO_DIR}
    )
    target_link_libraries(${target} PRIVATE
        keyboard::board
        ${FIRMWARE_LIBRARIES}
    )
    target_compile_options(${target} PRIVATE
        $<$<CONFIG:Release>:-Os>
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wshadow
        -Wundef
    )
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${KEYBOARD_OUTPUT_DIRECTORY}"
    )
    set(usb_options)
    if(FIRMWARE_HID)
        list(APPEND usb_options HID)
    endif()
    if(FIRMWARE_WEBUSB_CONFIG)
        list(APPEND usb_options WEBUSB_CONFIG)
    endif()
    target_add_keyboard_usb(${target} ${usb_options})
    configure_ch585_partitioned_executable(
        ${target} ${FIRMWARE_FLASH_ORIGIN} ${FIRMWARE_FLASH_SIZE})
endfunction()
