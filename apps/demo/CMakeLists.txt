function(add_ch585_demo name source)
    set(target ch585_demo_${name})
    add_executable(${target}
        ${source}
        common/demo_support.c
    )
    target_include_directories(${target} PRIVATE common)
    target_link_libraries(${target} PRIVATE platform::log)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wshadow
        -Wundef
    )
    configure_ch585_executable(${target})
endfunction()

add_ch585_demo(blinky blinky/main.c)
add_ch585_demo(uart_echo uart_echo/main.c)
add_ch585_demo(gpio_irq gpio_irq/main.c)
add_ch585_demo(i2c_scan i2c_scan/main.c)
add_ch585_demo(usb_hid usb_hid/main.c)
add_ch585_demo(tinyusb_hid tinyusb_hid/main.c)
add_ch585_demo(json_parse json_parse/main.c)
target_link_libraries(ch585_demo_json_parse PRIVATE platform::json)
add_ch585_demo(spi_flash spi_flash/main.c)
target_link_libraries(ch585_demo_spi_flash PRIVATE platform::spi_flash)
add_ch585_demo(spi_flash_littlefs spi_flash_littlefs/main.c)
target_link_libraries(ch585_demo_spi_flash_littlefs PRIVATE
    platform::littlefs
    platform::spi_flash
)
add_ch585_demo(spi_flash_fatfs spi_flash_fatfs/main.c)
target_link_libraries(ch585_demo_spi_flash_fatfs PRIVATE
    platform::fatfs
    platform::spi_flash
)
target_sources(ch585_demo_tinyusb_hid PRIVATE tinyusb_hid/usb_descriptors.c)
add_tinyusb_hid_device(ch585_demo_tinyusb_hid
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb_hid)

add_executable(ch585_demo_ble_peripheral
    ble_peripheral/main.c
    common/demo_support.c
)
target_include_directories(ch585_demo_ble_peripheral PRIVATE common)
target_link_libraries(ch585_demo_ble_peripheral PRIVATE platform::log)
target_compile_options(ch585_demo_ble_peripheral PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
configure_ch585_ble_executable(ch585_demo_ble_peripheral)

add_executable(ch585_demo_freertos_tasks
    freertos_tasks/main.c
    common/demo_support.c
)
target_include_directories(ch585_demo_freertos_tasks PRIVATE common)
target_link_libraries(ch585_demo_freertos_tasks PRIVATE
    platform::freertos
    platform::log
)
target_compile_options(ch585_demo_freertos_tasks PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
configure_ch585_executable(ch585_demo_freertos_tasks)

add_executable(ch585_demo_freertos_usb_ble
    freertos_usb_ble/main.c
    freertos_usb_ble/usb_descriptors.c
    common/demo_support.c
)
target_include_directories(ch585_demo_freertos_usb_ble PRIVATE
    common
    freertos_usb_ble
)
target_link_libraries(ch585_demo_freertos_usb_ble PRIVATE
    platform::freertos
    platform::log
)
target_compile_options(ch585_demo_freertos_usb_ble PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)
configure_ch585_executable(ch585_demo_freertos_usb_ble)
add_ch585_freertos_ble_support(ch585_demo_freertos_usb_ble)
add_tinyusb_freertos_hid_device(ch585_demo_freertos_usb_ble
    ${CMAKE_CURRENT_SOURCE_DIR}/freertos_usb_ble)
