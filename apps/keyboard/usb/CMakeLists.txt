function(target_add_keyboard_usb target)
    set(options HID WEBUSB_CONFIG)
    cmake_parse_arguments(USB "${options}" "" "" ${ARGN})

    if(USB_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Unknown USB arguments for ${target}: ${USB_UNPARSED_ARGUMENTS}")
    endif()

    target_sources(${target} PRIVATE
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/keyboard_usb_device.c
    )
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}
    )
    if(USB_HID)
        target_compile_definitions(${target} PRIVATE
            KEYBOARD_USB_HID_ENABLED=1
        )
    endif()
    set(ota_usb_options)
    if(USB_HID)
        list(APPEND ota_usb_options HID)
    endif()
    if(USB_WEBUSB_CONFIG)
        list(APPEND ota_usb_options WEBUSB_CONFIG)
    endif()
    target_add_platform_ota_usb(
        ${target} ${CMAKE_CURRENT_FUNCTION_LIST_DIR} ${ota_usb_options})
    if(USB_HID)
        target_compile_definitions(${target}_tinyusb PUBLIC
            KEYBOARD_USB_HID_ENABLED=1
        )
    endif()
endfunction()
