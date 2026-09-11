add_ch585_demo(lua_usb_fatfs
    SOURCES
        main.c
        usb_descriptors.c
        usb_msc.c
    LIBRARIES
        platform::lua
        platform::ota
)
add_tinyusb_msc_device(ch585_demo_lua_usb_fatfs ${CMAKE_CURRENT_SOURCE_DIR})
