function(add_tinyusb_hid_device target config_directory)
    set(tinyusb_root ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/tinyusb)
    set(stack_target ${target}_tinyusb)

    if(NOT EXISTS ${tinyusb_root}/src/tusb.c)
        message(FATAL_ERROR "TinyUSB upstream source is missing from components/tinyusb/tinyusb")
    endif()

    add_library(${stack_target} STATIC
        ${tinyusb_root}/src/tusb.c
        ${tinyusb_root}/src/common/tusb_fifo.c
        ${tinyusb_root}/src/device/usbd.c
        ${tinyusb_root}/src/class/hid/hid_device.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/dcd_chip.c
    )
    target_include_directories(${stack_target}
        PUBLIC
            ${tinyusb_root}/src
            ${config_directory}
    )
    target_compile_definitions(${stack_target} PUBLIC
        TUP_DCD_ENDPOINT_MAX=8
        TUP_DCD_EDPT_CLOSE_API=1
    )
    target_compile_options(${stack_target} PRIVATE
        -Wall
        -Wextra
        -Werror
    )
    target_link_libraries(${stack_target} PUBLIC chip::selected)
    target_link_libraries(${target} PRIVATE ${stack_target})
endfunction()

function(add_tinyusb_freertos_hid_device target config_directory)
    add_tinyusb_hid_device(${target} ${config_directory})
    target_link_libraries(${target}_tinyusb PUBLIC platform::freertos)
endfunction()

function(add_tinyusb_msc_device target config_directory)
    set(tinyusb_root ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/tinyusb)
    set(stack_target ${target}_tinyusb)
    set(options HID)
    cmake_parse_arguments(MSC "${options}" "" "" ${ARGN})

    if(MSC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Unknown MSC arguments for ${target}: ${MSC_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT EXISTS ${tinyusb_root}/src/tusb.c)
        message(FATAL_ERROR "TinyUSB upstream source is missing from components/tinyusb/tinyusb")
    endif()

    set(class_sources
        ${tinyusb_root}/src/class/msc/msc_device.c
        ${tinyusb_root}/src/class/vendor/vendor_device.c
    )
    if(MSC_HID)
        list(APPEND class_sources
            ${tinyusb_root}/src/class/hid/hid_device.c
        )
    endif()

    add_library(${stack_target} STATIC
        ${tinyusb_root}/src/tusb.c
        ${tinyusb_root}/src/common/tusb_fifo.c
        ${tinyusb_root}/src/device/usbd.c
        ${class_sources}
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/dcd_chip.c
    )
    target_include_directories(${stack_target}
        PUBLIC
            ${tinyusb_root}/src
            ${config_directory}
    )
    target_compile_definitions(${stack_target} PUBLIC
        TUP_DCD_ENDPOINT_MAX=8
        TUP_DCD_EDPT_CLOSE_API=1
    )
    target_compile_options(${stack_target} PRIVATE
        -Wall
        -Wextra
        -Werror
    )
    target_link_libraries(${stack_target} PUBLIC chip::selected)
    target_link_libraries(${target} PRIVATE ${stack_target})
endfunction()
