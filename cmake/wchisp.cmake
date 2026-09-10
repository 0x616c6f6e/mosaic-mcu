get_filename_component(PLATFORM_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

find_program(WCHISP_EXECUTABLE
    NAMES wchisp
    HINTS ${PLATFORM_ROOT}/tools/wch/wchisp/bin
    DOC "Path to the wchisp executable"
)

set(WCHISP_TRANSPORT "usb" CACHE STRING "wchisp transport: usb or serial")
set_property(CACHE WCHISP_TRANSPORT PROPERTY STRINGS usb serial)
set(WCHISP_DEVICE_INDEX "" CACHE STRING "Optional USB ISP device index")
set(WCHISP_SERIAL_PORT "" CACHE STRING "Serial port used by wchisp serial transport")
set(WCHISP_BAUDRATE "Baud115200" CACHE STRING "wchisp serial baud rate")
set_property(CACHE WCHISP_BAUDRATE PROPERTY STRINGS Baud115200 Baud1m Baud2m)
set(WCHISP_RETRY_SECONDS "3" CACHE STRING "Seconds to retry ISP device discovery")
set(WCHISP_EXTRA_ARGS "" CACHE STRING "Additional global wchisp arguments")
set(WCHISP_FLASH_ARGS "" CACHE STRING "Additional wchisp flash arguments")

set(wchisp_udev_installer ${PLATFORM_ROOT}/tools/wch/wchisp/install-udev-rule.sh)
if(CMAKE_HOST_UNIX AND EXISTS ${wchisp_udev_installer})
    add_custom_target(wchisp_install_udev_rule
        COMMAND ${wchisp_udev_installer}
        USES_TERMINAL
        COMMENT "Installing the WCH USB ISP udev rule"
        VERBATIM
    )
endif()

function(wchisp_global_arguments output)
    set(arguments)

    if(WCHISP_TRANSPORT STREQUAL "usb")
        list(APPEND arguments --usb)
        if(NOT WCHISP_DEVICE_INDEX STREQUAL "")
            list(APPEND arguments --device ${WCHISP_DEVICE_INDEX})
        endif()
    elseif(WCHISP_TRANSPORT STREQUAL "serial")
        if(WCHISP_SERIAL_PORT STREQUAL "")
            message(FATAL_ERROR
                "WCHISP_SERIAL_PORT is required when WCHISP_TRANSPORT=serial")
        endif()
        list(APPEND arguments --serial --port ${WCHISP_SERIAL_PORT}
             --baudrate ${WCHISP_BAUDRATE})
    else()
        message(FATAL_ERROR "Unsupported WCHISP_TRANSPORT: ${WCHISP_TRANSPORT}")
    endif()

    if(NOT WCHISP_RETRY_SECONDS STREQUAL "")
        list(APPEND arguments --retry ${WCHISP_RETRY_SECONDS})
    endif()
    if(NOT WCHISP_EXTRA_ARGS STREQUAL "")
        separate_arguments(extra_arguments NATIVE_COMMAND "${WCHISP_EXTRA_ARGS}")
        list(APPEND arguments ${extra_arguments})
    endif()
    set(${output} ${arguments} PARENT_SCOPE)
endfunction()

if(WCHISP_EXECUTABLE)
    wchisp_global_arguments(wchisp_arguments)
    add_custom_target(wchisp_probe
        COMMAND ${WCHISP_EXECUTABLE} ${wchisp_arguments} probe
        USES_TERMINAL
        COMMENT "Probing WCH ISP devices"
        VERBATIM
    )
    add_custom_target(wchisp_info
        COMMAND ${WCHISP_EXECUTABLE} ${wchisp_arguments} info
        USES_TERMINAL
        COMMENT "Reading WCH ISP device information"
        VERBATIM
    )
else()
    message(WARNING
        "wchisp was not found; firmware builds are enabled but ISP targets are unavailable")
endif()

function(add_wchisp_firmware_targets target)
    if(NOT WCHISP_EXECUTABLE)
        return()
    endif()
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Unknown firmware target: ${target}")
    endif()

    wchisp_global_arguments(wchisp_arguments)
    set(flash_arguments)
    if(NOT WCHISP_FLASH_ARGS STREQUAL "")
        separate_arguments(flash_arguments NATIVE_COMMAND "${WCHISP_FLASH_ARGS}")
    endif()

    add_custom_target(flash_${target}
        COMMAND ${WCHISP_EXECUTABLE} ${wchisp_arguments}
                flash ${flash_arguments} $<TARGET_FILE:${target}>
        DEPENDS ${target}
        USES_TERMINAL
        COMMENT "Flashing and verifying ${target} with wchisp"
        VERBATIM
    )
    add_custom_target(verify_${target}
        COMMAND ${WCHISP_EXECUTABLE} ${wchisp_arguments}
                verify $<TARGET_FILE:${target}>
        DEPENDS ${target}
        USES_TERMINAL
        COMMENT "Verifying ${target} with wchisp"
        VERBATIM
    )
endfunction()
