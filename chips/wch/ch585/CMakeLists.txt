set(CH585_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sdk)
set(CH585_DRIVER_DIR ${CH585_SDK_DIR}/StdPeriphDriver)
set_property(GLOBAL PROPERTY CH585_SDK_ROOT ${CH585_SDK_DIR})

if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "riscv32")
    message(FATAL_ERROR
        "CH585 requires the WCH RISC-V toolchain. Configure with "
        "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/wch-riscv.cmake"
    )
endif()

add_library(chip_api INTERFACE)
add_library(chip::api ALIAS chip_api)
target_include_directories(chip_api INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/port/include
)

add_library(wch_ch585_arch INTERFACE)
target_compile_options(wch_ch585_arch INTERFACE
    -march=rv32imc_zba_zbb_zbc_zbs_xw
    -mabi=ilp32
    -ffunction-sections
    -fdata-sections
)
target_link_options(wch_ch585_arch INTERFACE
    -march=rv32imc_zba_zbb_zbc_zbs_xw
    -mabi=ilp32
)
target_compile_definitions(wch_ch585_arch INTERFACE
    CH58
    CH58X
    CH58x
    CH585
    DISK_LIB_ENABLE=0
)

add_library(wch_ch585_isp STATIC IMPORTED GLOBAL)
set_target_properties(wch_ch585_isp PROPERTIES
    IMPORTED_LOCATION ${CH585_DRIVER_DIR}/libISP585.a
)

add_library(wch_ch585_sdk STATIC
    ${CH585_DRIVER_DIR}/CH58x_adc.c
    ${CH585_DRIVER_DIR}/CH58x_clk.c
    ${CH585_DRIVER_DIR}/CH58x_flash.c
    ${CH585_DRIVER_DIR}/CH58x_gpio.c
    ${CH585_DRIVER_DIR}/CH58x_i2c.c
    ${CH585_DRIVER_DIR}/CH58x_pwm.c
    ${CH585_DRIVER_DIR}/CH58x_pwr.c
    ${CH585_DRIVER_DIR}/CH58x_spi0.c
    ${CH585_DRIVER_DIR}/CH58x_spi1.c
    ${CH585_DRIVER_DIR}/CH58x_sys.c
    ${CH585_DRIVER_DIR}/CH58x_uart0.c
    ${CH585_DRIVER_DIR}/CH58x_uart1.c
    ${CH585_DRIVER_DIR}/CH58x_uart2.c
    ${CH585_DRIVER_DIR}/CH58x_uart3.c
)
set_target_properties(wch_ch585_sdk PROPERTIES C_EXTENSIONS ON)

target_include_directories(wch_ch585_sdk SYSTEM PUBLIC
    ${CH585_DRIVER_DIR}/inc
    ${CH585_SDK_DIR}/RVMSIS
)
target_link_libraries(wch_ch585_sdk PUBLIC wch_ch585_arch)

add_library(ch585_port STATIC
    port/src/chip_adc.c
    port/src/chip_flash.c
    port/src/chip_gpio.c
    port/src/chip_i2c.c
    port/src/chip_power.c
    port/src/chip_pwm.c
    port/src/chip_spi.c
    port/src/chip_system.c
    port/src/chip_time.c
    port/src/chip_uart.c
    port/src/chip_usb_device.c
)
set_target_properties(ch585_port PROPERTIES C_EXTENSIONS ON)
add_library(chip::selected ALIAS ch585_port)

target_include_directories(ch585_port PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/port/src
)

target_link_libraries(ch585_port
    PUBLIC chip::api wch_ch585_arch
    PRIVATE wch_ch585_sdk wch_ch585_isp
)

add_library(wch_ch585_ble_peripheral STATIC IMPORTED GLOBAL)
set_target_properties(wch_ch585_ble_peripheral PROPERTIES
    IMPORTED_LOCATION ${CH585_SDK_DIR}/BLE_LIB/libCH58xBLE_PERI.a
)

add_library(ch585_ble_port STATIC
    port/src/chip_ble.c
    port/src/chip_ble_hid_keyboard.c
)
set_target_properties(ch585_ble_port PROPERTIES C_EXTENSIONS ON)
target_include_directories(ch585_ble_port PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/port/src
    ${CH585_SDK_DIR}/BLE_LIB
)
target_link_libraries(ch585_ble_port
    PUBLIC chip::api wch_ch585_arch
    PRIVATE ch585_port wch_ch585_sdk wch_ch585_isp wch_ch585_ble_peripheral
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ch585_ble_port PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wshadow
        -Wundef
    )
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ch585_port PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wshadow
        -Wundef
    )
endif()

function(configure_ch585_executable target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Unknown executable target: ${target}")
    endif()
    get_property(ch585_sdk_root GLOBAL PROPERTY CH585_SDK_ROOT)

    target_sources(${target} PRIVATE ${ch585_sdk_root}/Startup/startup_CH585.S)
    target_link_libraries(${target} PRIVATE chip::selected)
    target_link_options(${target} PRIVATE
        -nostartfiles
        -T${ch585_sdk_root}/Ld/Link.ld
        -Wl,--gc-sections
    )
    set_target_properties(${target} PROPERTIES SUFFIX ".elf")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${target}>
            $<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:${target}>
            $<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.hex
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target}>
        VERBATIM
    )
    add_wchisp_firmware_targets(${target})
endfunction()

function(configure_ch585_ble_executable target)
    configure_ch585_executable(${target})
    get_property(ch585_sdk_root GLOBAL PROPERTY CH585_SDK_ROOT)
    target_sources(${target} PRIVATE
        ${ch585_sdk_root}/BLE_LIB/ble_task_scheduler.S
    )
    target_link_libraries(${target} PRIVATE ch585_ble_port)
endfunction()
