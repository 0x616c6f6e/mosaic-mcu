add_library(platform_ota STATIC
    src/ota_image.c
    src/ota_spi_disk.c
    src/ota_staging.c
)
add_library(platform::ota ALIAS platform_ota)

target_include_directories(platform_ota PUBLIC include)
target_link_libraries(platform_ota PUBLIC
    platform::fatfs
    platform::spi_flash
)
target_compile_features(platform_ota PUBLIC c_std_11)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(platform_ota PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wshadow
        -Wundef
    )
endif()

function(target_add_platform_ota_usb target config_directory)
    set(options HID WEBUSB_CONFIG)
    cmake_parse_arguments(OTA_USB "${options}" "" "" ${ARGN})

    if(OTA_USB_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Unknown OTA USB arguments for ${target}: ${OTA_USB_UNPARSED_ARGUMENTS}")
    endif()

    target_sources(${target} PRIVATE
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/ota_usb_msc.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/ota_webusb.c
    )
    target_link_libraries(${target} PRIVATE platform::ota platform::log)
    if(OTA_USB_WEBUSB_CONFIG)
        target_compile_definitions(${target} PRIVATE
            OTA_WEBUSB_CONFIG_ENABLED=1
        )
    endif()
    if(OTA_USB_HID)
        add_tinyusb_msc_device(${target} ${config_directory} HID)
    else()
        add_tinyusb_msc_device(${target} ${config_directory})
    endif()
    target_compile_options(${target}_tinyusb PRIVATE
        $<$<CONFIG:Release>:-Os>
    )
endfunction()
