add_ch585_keyboard_firmware(ch585_keyboard
    HID
    WEBUSB_CONFIG
    SOURCES
        keyboard_hid.c
        keyboard_layout.c
        main.c
        ota_system_config.c
    LIBRARIES
        keyboard::core
        platform::json
    FLASH_ORIGIN 0x10000
    FLASH_SIZE 0x60000
)

set(KEYBOARD_PACKAGE_TOOL
    "${CMAKE_SOURCE_DIR}/components/ota/tools/pack_image.py")
set_property(TARGET ch585_keyboard APPEND PROPERTY
    LINK_DEPENDS ${KEYBOARD_PACKAGE_TOOL}
)
add_custom_command(TARGET ch585_keyboard POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${KEYBOARD_PACKAGE_TOOL}
        --input $<TARGET_FILE_DIR:ch585_keyboard>/$<TARGET_FILE_BASE_NAME:ch585_keyboard>.bin
        --output ${KEYBOARD_OUTPUT_DIRECTORY}/FIRMWARE.OTA
        --target 0x585
        --address 0x10000
        --max-size 0x60000
        --version ${KEYBOARD_FIRMWARE_VERSION}
    VERBATIM
)
