cmake_minimum_required(VERSION 3.20)

project(mosaic_mcu_host_tests LANGUAGES C)

enable_testing()

add_library(chip_api INTERFACE)
add_library(chip::api ALIAS chip_api)
target_include_directories(chip_api INTERFACE
    ../../chips/wch/ch585/port/include
)

add_subdirectory(../../components/log
    ${CMAKE_CURRENT_BINARY_DIR}/platform-log)
add_subdirectory(../../components/json
    ${CMAKE_CURRENT_BINARY_DIR}/platform-json)
add_subdirectory(../../components/lua
    ${CMAKE_CURRENT_BINARY_DIR}/platform-lua)
add_subdirectory(../../components/spi_flash
    ${CMAKE_CURRENT_BINARY_DIR}/platform-spi-flash)
add_subdirectory(../../apps/keyboard/core
    ${CMAKE_CURRENT_BINARY_DIR}/keyboard-core)

add_executable(ch585_usb_device_test
    ch585_usb_device_test.c
    fakes/ch585_registers.c
    ../../chips/wch/ch585/port/src/chip_usb_device.c
)

target_include_directories(ch585_usb_device_test PRIVATE
    fakes
    ../../chips/wch/ch585/port/include
)

target_compile_features(ch585_usb_device_test PRIVATE c_std_11)
target_compile_options(ch585_usb_device_test PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
    -fno-pie
)
target_link_options(ch585_usb_device_test PRIVATE -no-pie)

add_test(NAME ch585_usb_device_test COMMAND ch585_usb_device_test)

add_executable(ota_usb_descriptors_test
    ota_usb_descriptors_test.c
    ../../apps/keyboard/usb/keyboard_usb_device.c
)
target_include_directories(ota_usb_descriptors_test PRIVATE
    ../../apps/keyboard/usb
)
target_include_directories(ota_usb_descriptors_test SYSTEM PRIVATE
    ../../components/tinyusb/tinyusb/src
)
target_compile_definitions(ota_usb_descriptors_test PRIVATE
    TUP_DCD_ENDPOINT_MAX=8
    TUP_DCD_EDPT_CLOSE_API=1
)
target_compile_options(ota_usb_descriptors_test PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
add_test(NAME ota_usb_descriptors_test COMMAND ota_usb_descriptors_test)

add_executable(keyboard_usb_descriptors_test
    ota_usb_descriptors_test.c
    ../../apps/keyboard/usb/keyboard_usb_device.c
)
target_include_directories(keyboard_usb_descriptors_test PRIVATE
    ../../apps/keyboard/usb
)
target_include_directories(keyboard_usb_descriptors_test SYSTEM PRIVATE
    ../../components/tinyusb/tinyusb/src
)
target_compile_definitions(keyboard_usb_descriptors_test PRIVATE
    KEYBOARD_USB_HID_ENABLED=1
    TUP_DCD_ENDPOINT_MAX=8
    TUP_DCD_EDPT_CLOSE_API=1
)
target_compile_options(keyboard_usb_descriptors_test PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
add_test(NAME keyboard_usb_descriptors_test COMMAND keyboard_usb_descriptors_test)

add_executable(ota_usb_msc_test
    ota_usb_msc_test.c
    ../../components/ota/src/ota_usb_msc.c
)
target_include_directories(ota_usb_msc_test PRIVATE
    ../../apps/keyboard/usb
    ../../components/ota/include
    ../../components/fatfs/include
    ../../components/fatfs/fatfs/source
    ../../components/spi_flash/include
)
target_include_directories(ota_usb_msc_test SYSTEM PRIVATE
    ../../components/tinyusb/tinyusb/src
)
target_compile_definitions(ota_usb_msc_test PRIVATE
    TUP_DCD_ENDPOINT_MAX=8
    TUP_DCD_EDPT_CLOSE_API=1
)
target_compile_options(ota_usb_msc_test PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
target_link_libraries(ota_usb_msc_test PRIVATE chip::api platform::log)
add_test(NAME ota_usb_msc_test COMMAND ota_usb_msc_test)

add_executable(ota_webusb_config_test
    ota_webusb_config_test.c
    ../../components/ota/src/ota_webusb.c
)
target_include_directories(ota_webusb_config_test PRIVATE
    ../../apps/keyboard/usb
    ../../chips/wch/ch585/port/include
    ../../components/fatfs/include
    ../../components/fatfs/fatfs/source
    ../../components/ota/include
    ../../components/spi_flash/include
)
target_include_directories(ota_webusb_config_test SYSTEM PRIVATE
    ../../components/tinyusb/tinyusb/src
)
target_compile_definitions(ota_webusb_config_test PRIVATE
    OTA_WEBUSB_CONFIG_ENABLED=1
    TUP_DCD_ENDPOINT_MAX=8
    TUP_DCD_EDPT_CLOSE_API=1
)
target_compile_options(ota_webusb_config_test PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
target_link_libraries(ota_webusb_config_test PRIVATE platform::log)
add_test(NAME ota_webusb_config_test COMMAND ota_webusb_config_test)

add_executable(platform_log_test platform_log_test.c)
target_link_libraries(platform_log_test PRIVATE platform::log)
target_compile_options(platform_log_test PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
add_test(NAME platform_log_test COMMAND platform_log_test)

add_executable(jsmn_test
    jsmn_test.c
    jsmn_second_translation_unit.c
)
target_link_libraries(jsmn_test PRIVATE platform::json)
target_compile_options(jsmn_test PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
add_test(NAME jsmn_test COMMAND jsmn_test)

add_executable(lua_runtime_test lua_runtime_test.c)
target_link_libraries(lua_runtime_test PRIVATE platform::lua)
target_compile_options(lua_runtime_test PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
add_test(NAME lua_runtime_test COMMAND lua_runtime_test)

add_executable(keyboard_engine_test keyboard_engine_test.c)
target_link_libraries(keyboard_engine_test PRIVATE keyboard::core)
target_compile_options(keyboard_engine_test PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
add_test(NAME keyboard_engine_test COMMAND keyboard_engine_test)

add_executable(keyboard_matrix_test
    keyboard_matrix_test.c
    ../../apps/keyboard/board/keyboard_matrix.c
)
target_include_directories(keyboard_matrix_test PRIVATE
    ../../apps/keyboard/board
)
target_link_libraries(keyboard_matrix_test PRIVATE chip::api)
target_compile_options(keyboard_matrix_test PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
add_test(NAME keyboard_matrix_test COMMAND keyboard_matrix_test)

add_executable(spi_flash_test spi_flash_test.c)
target_link_libraries(spi_flash_test PRIVATE platform::spi_flash)
target_compile_options(spi_flash_test PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
add_test(NAME spi_flash_test COMMAND spi_flash_test)

add_executable(ota_image_test
    ota_image_test.c
    ../../components/ota/src/ota_image.c
)
target_include_directories(ota_image_test PRIVATE
    ../../components/ota/include
    ../../components/fatfs/include
    ../../components/fatfs/fatfs/source
)
target_compile_options(ota_image_test PRIVATE
    -include${CMAKE_CURRENT_SOURCE_DIR}/../../components/fatfs/include/platform_fatfs_config.h
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
    -ffunction-sections
)
target_link_options(ota_image_test PRIVATE -Wl,--gc-sections)
add_test(NAME ota_image_test COMMAND ota_image_test)

add_executable(ota_staging_test
    ota_staging_test.c
    ../../components/ota/src/ota_staging.c
    ../../components/ota/src/ota_image.c
)
target_include_directories(ota_staging_test PRIVATE
    ../../apps/keyboard/board
    ../../components/ota/include
    ../../components/spi_flash/include
    ../../components/fatfs/include
    ../../components/fatfs/fatfs/source
)
target_compile_options(ota_staging_test PRIVATE
    -include${CMAKE_CURRENT_SOURCE_DIR}/../../components/fatfs/include/platform_fatfs_config.h
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
    -ffunction-sections
)
target_link_options(ota_staging_test PRIVATE -Wl,--gc-sections)
target_link_libraries(ota_staging_test PRIVATE chip::api)
add_test(NAME ota_staging_test COMMAND ota_staging_test)

add_executable(ota_system_config_test
    ota_system_config_test.c
    ../../apps/keyboard/application/ota_system_config.c
)
target_include_directories(ota_system_config_test PRIVATE
    ../../apps/keyboard/application
    ../../components/log/include
    ../../components/fatfs/include
    ../../components/fatfs/fatfs/source
)
target_link_libraries(ota_system_config_test PRIVATE platform::json)
target_compile_options(ota_system_config_test PRIVATE
    -include${CMAKE_CURRENT_SOURCE_DIR}/../../components/fatfs/include/platform_fatfs_config.h
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
    -ffunction-sections
)
target_link_options(ota_system_config_test PRIVATE -Wl,--gc-sections)
add_test(NAME ota_system_config_test COMMAND ota_system_config_test)
