add_ch585_demo(tinyusb_hid SOURCES
    main.c
    usb_descriptors.c
)
add_tinyusb_hid_device(ch585_demo_tinyusb_hid ${CMAKE_CURRENT_SOURCE_DIR})
