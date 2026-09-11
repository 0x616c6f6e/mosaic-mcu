set(KEYBOARD_FACTORY_BIN "${KEYBOARD_OUTPUT_DIRECTORY}/FACTORY.bin")
set(KEYBOARD_FACTORY_HEX "${KEYBOARD_OUTPUT_DIRECTORY}/FACTORY.hex")
set(KEYBOARD_FACTORY_TOOL
    "${CMAKE_SOURCE_DIR}/components/ota/tools/make_factory_image.py")

add_custom_command(
    OUTPUT
        ${KEYBOARD_FACTORY_BIN}
        ${KEYBOARD_FACTORY_HEX}
    COMMAND ${Python3_EXECUTABLE} ${KEYBOARD_FACTORY_TOOL}
        --bootloader ${KEYBOARD_OUTPUT_DIRECTORY}/ch585_keyboard_bootloader.bin
        --application ${KEYBOARD_OUTPUT_DIRECTORY}/ch585_keyboard.bin
        --output ${KEYBOARD_FACTORY_BIN}
        --application-address 0x10000
        --flash-size 0x70000
    COMMAND ${CMAKE_OBJCOPY} -I binary -O ihex
        --change-addresses 0x0
        ${KEYBOARD_FACTORY_BIN}
        ${KEYBOARD_FACTORY_HEX}
    DEPENDS
        ch585_keyboard_bootloader
        ch585_keyboard
        ${KEYBOARD_FACTORY_TOOL}
    VERBATIM
)
add_custom_target(ch585_keyboard_factory ALL
    DEPENDS ${KEYBOARD_FACTORY_BIN} ${KEYBOARD_FACTORY_HEX}
)

if(WCHISP_EXECUTABLE)
    wchisp_global_arguments(keyboard_wchisp_arguments)
    set(keyboard_factory_flash_arguments)
    if(NOT WCHISP_FLASH_ARGS STREQUAL "")
        separate_arguments(keyboard_factory_flash_arguments NATIVE_COMMAND
                           "${WCHISP_FLASH_ARGS}")
    endif()
    add_custom_target(flash_ch585_keyboard_factory
        COMMAND ${WCHISP_EXECUTABLE} ${keyboard_wchisp_arguments} flash
                ${keyboard_factory_flash_arguments} ${KEYBOARD_FACTORY_HEX}
        DEPENDS ch585_keyboard_factory
        USES_TERMINAL
        COMMENT "Flashing CH585 keyboard bootloader and application"
        VERBATIM
    )
    add_custom_target(verify_ch585_keyboard_factory
        COMMAND ${WCHISP_EXECUTABLE} ${keyboard_wchisp_arguments} verify
                ${KEYBOARD_FACTORY_HEX}
        DEPENDS ch585_keyboard_factory
        USES_TERMINAL
        COMMENT "Verifying CH585 keyboard factory image"
        VERBATIM
    )
endif()
