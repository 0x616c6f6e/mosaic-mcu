find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(KEYBOARD_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(KEYBOARD_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(KEYBOARD_VERSION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/version")
set(KEYBOARD_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(KEYBOARD_VERSION_SOURCE "${KEYBOARD_GENERATED_DIR}/firmware_version.c")
set(KEYBOARD_FIRMWARE_VERSION "1" CACHE STRING "Version stored in FIRMWARE.OTA")

add_custom_target(keyboard_version
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_FILE=${KEYBOARD_VERSION_SOURCE}
        -DTEMPLATE_FILE=${KEYBOARD_VERSION_DIR}/firmware_version.c.in
        -DPROJECT_DIR=${CMAKE_SOURCE_DIR}
        -DFIRMWARE_VERSION=${KEYBOARD_FIRMWARE_VERSION}
        -DFIRMWARE_BUILD_TYPE=$<CONFIG>
        -P ${CMAKE_SOURCE_DIR}/cmake/generate_build_info.cmake
    BYPRODUCTS ${KEYBOARD_VERSION_SOURCE}
    COMMENT "Generating keyboard version information"
    VERBATIM
)
set_source_files_properties(${KEYBOARD_VERSION_SOURCE} PROPERTIES GENERATED TRUE)

add_library(ch585_keyboard_board STATIC
    board/keyboard_board.c
    board/keyboard_log.c
    board/keyboard_matrix.c
)
add_library(keyboard::board ALIAS ch585_keyboard_board)
target_include_directories(ch585_keyboard_board PUBLIC
    ${KEYBOARD_SOURCE_DIR}/board
)
target_link_libraries(ch585_keyboard_board PUBLIC
    platform::log
    platform::ota
)
target_compile_features(ch585_keyboard_board PUBLIC c_std_11)

add_library(keyboard_core STATIC core/keyboard_engine.c)
add_library(keyboard::core ALIAS keyboard_core)
target_include_directories(keyboard_core PUBLIC
    ${KEYBOARD_SOURCE_DIR}/core
)
target_compile_features(keyboard_core PUBLIC c_std_11)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    foreach(target ch585_keyboard_board keyboard_core)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Release>:-Os>
            -Wall
            -Wextra
            -Werror
            -Wconversion
            -Wshadow
            -Wundef
            -ffunction-sections
            -fdata-sections
        )
    endforeach()
endif()

function(target_add_keyboard_usb target)
    set(options HID WEBUSB_CONFIG)
    cmake_parse_arguments(USB "${options}" "" "" ${ARGN})

    if(USB_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Unknown USB arguments for ${target}: ${USB_UNPARSED_ARGUMENTS}")
    endif()

    target_sources(${target} PRIVATE
        ${KEYBOARD_SOURCE_DIR}/usb/keyboard_usb_device.c
    )
    target_include_directories(${target} PRIVATE
        ${KEYBOARD_SOURCE_DIR}/usb
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
        ${target} ${KEYBOARD_SOURCE_DIR}/usb ${ota_usb_options})
    if(USB_HID)
        target_compile_definitions(${target}_tinyusb PUBLIC
            KEYBOARD_USB_HID_ENABLED=1
        )
    endif()
endfunction()

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

    add_executable(${target}
        ${FIRMWARE_SOURCES}
        ${KEYBOARD_VERSION_SOURCE}
    )
    add_dependencies(${target} keyboard_version)
    target_include_directories(${target} PRIVATE ${KEYBOARD_VERSION_DIR})
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

add_ch585_keyboard_firmware(ch585_keyboard_bootloader
    SOURCES bootloader/main.c
    FLASH_ORIGIN 0x0
    FLASH_SIZE 0x10000
)

add_ch585_keyboard_firmware(ch585_keyboard
    HID
    WEBUSB_CONFIG
    SOURCES
        application/keyboard_hid.c
        application/keyboard_layout.c
        application/main.c
        application/ota_system_config.c
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
