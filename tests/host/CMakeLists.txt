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
add_subdirectory(../../components/spi_flash
    ${CMAKE_CURRENT_BINARY_DIR}/platform-spi-flash)

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

add_executable(ota_webusb_staging_test
    ota_webusb_staging_test.c
    ../../apps/ota/common/ota_webusb_staging.c
    ../../components/ota/src/ota_image.c
)
target_include_directories(ota_webusb_staging_test PRIVATE
    ../../apps/ota/common
    ../../components/ota/include
    ../../components/spi_flash/include
    ../../components/fatfs/include
    ../../components/fatfs/fatfs/source
)
target_compile_options(ota_webusb_staging_test PRIVATE
    -include${CMAKE_CURRENT_SOURCE_DIR}/../../components/fatfs/include/platform_fatfs_config.h
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
    -ffunction-sections
)
target_link_options(ota_webusb_staging_test PRIVATE -Wl,--gc-sections)
target_link_libraries(ota_webusb_staging_test PRIVATE chip::api)
add_test(NAME ota_webusb_staging_test COMMAND ota_webusb_staging_test)

add_executable(ota_system_config_test
    ota_system_config_test.c
    ../../apps/ota/common/ota_system_config.c
)
target_include_directories(ota_system_config_test PRIVATE
    ../../apps/ota/common
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
