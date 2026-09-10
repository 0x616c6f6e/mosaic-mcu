cmake_minimum_required(VERSION 3.20)

project(mosaic_mcu_host_tests LANGUAGES C)

enable_testing()

add_subdirectory(../../components/log
    ${CMAKE_CURRENT_BINARY_DIR}/platform-log)
add_subdirectory(../../components/json
    ${CMAKE_CURRENT_BINARY_DIR}/platform-json)

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
