add_ch585_demo(freertos_usb_ble
    SOURCES
        main.c
        usb_descriptors.c
    LIBRARIES platform::freertos
)
add_ch585_freertos_ble_support(ch585_demo_freertos_usb_ble)
add_tinyusb_freertos_hid_device(ch585_demo_freertos_usb_ble
    ${CMAKE_CURRENT_SOURCE_DIR})
