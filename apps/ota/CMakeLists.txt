find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(OTA_COMMON_SOURCES
    common/ota_board.c
    common/ota_log.c
    common/ota_usb_msc.c
    common/ota_webusb.c
    common/ota_webusb_staging.c
    common/usb_descriptors.c
)

function(configure_ota_target target)
    target_include_directories(${target} PRIVATE common)
    target_link_libraries(${target} PRIVATE platform::ota platform::log)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wshadow
        -Wundef
    )
    add_tinyusb_msc_device(${target} ${CMAKE_CURRENT_SOURCE_DIR}/common)
endfunction()

add_executable(ch585_ota_bootloader
    bootloader/main.c
    ${OTA_COMMON_SOURCES}
)
configure_ota_target(ch585_ota_bootloader)
configure_ch585_partitioned_executable(ch585_ota_bootloader 0x0 0x10000)

set(OTA_FIRMWARE_VERSION "1" CACHE STRING "Version stored in FIRMWARE.OTA")
add_executable(ch585_ota_usb_disk
    usb_disk/main.c
    common/ota_system_config.c
    ${OTA_COMMON_SOURCES}
)
configure_ota_target(ch585_ota_usb_disk)
target_link_libraries(ch585_ota_usb_disk PRIVATE platform::json)
configure_ch585_partitioned_executable(ch585_ota_usb_disk 0x10000 0x60000)

add_custom_command(TARGET ch585_ota_usb_disk POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/pack_image.py
        --input $<TARGET_FILE_DIR:ch585_ota_usb_disk>/$<TARGET_FILE_BASE_NAME:ch585_ota_usb_disk>.bin
        --output $<TARGET_FILE_DIR:ch585_ota_usb_disk>/FIRMWARE.OTA
        --target 0x585
        --address 0x10000
        --max-size 0x60000
        --version ${OTA_FIRMWARE_VERSION}
    VERBATIM
)

add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/FACTORY.bin
        ${CMAKE_CURRENT_BINARY_DIR}/FACTORY.hex
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/make_factory_image.py
        --bootloader ${CMAKE_CURRENT_BINARY_DIR}/ch585_ota_bootloader.bin
        --application ${CMAKE_CURRENT_BINARY_DIR}/ch585_ota_usb_disk.bin
        --output ${CMAKE_CURRENT_BINARY_DIR}/FACTORY.bin
        --application-address 0x10000
        --flash-size 0x70000
    COMMAND ${CMAKE_OBJCOPY} -I binary -O ihex
        --change-addresses 0x0
        ${CMAKE_CURRENT_BINARY_DIR}/FACTORY.bin
        ${CMAKE_CURRENT_BINARY_DIR}/FACTORY.hex
    DEPENDS
        ch585_ota_bootloader
        ch585_ota_usb_disk
        ${CMAKE_CURRENT_SOURCE_DIR}/make_factory_image.py
    VERBATIM
)
add_custom_target(ch585_ota_factory ALL
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/FACTORY.bin
        ${CMAKE_CURRENT_BINARY_DIR}/FACTORY.hex
)

if(WCHISP_EXECUTABLE)
    wchisp_global_arguments(ota_wchisp_arguments)
    set(ota_factory_flash_arguments)
    if(NOT WCHISP_FLASH_ARGS STREQUAL "")
        separate_arguments(ota_factory_flash_arguments NATIVE_COMMAND
                           "${WCHISP_FLASH_ARGS}")
    endif()
    add_custom_target(flash_ch585_ota_factory
        COMMAND ${WCHISP_EXECUTABLE} ${ota_wchisp_arguments} flash
                ${ota_factory_flash_arguments}
                ${CMAKE_CURRENT_BINARY_DIR}/FACTORY.hex
        DEPENDS ch585_ota_factory
        USES_TERMINAL
        COMMENT "Flashing CH585 OTA bootloader and application"
        VERBATIM
    )
    add_custom_target(verify_ch585_ota_factory
        COMMAND ${WCHISP_EXECUTABLE} ${ota_wchisp_arguments} verify
                ${CMAKE_CURRENT_BINARY_DIR}/FACTORY.hex
        DEPENDS ch585_ota_factory
        USES_TERMINAL
        COMMENT "Verifying CH585 OTA factory image"
        VERBATIM
    )
endif()
