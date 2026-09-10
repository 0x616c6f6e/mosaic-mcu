set(FREERTOS_KERNEL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-Kernel)

if(NOT EXISTS ${FREERTOS_KERNEL_DIR}/tasks.c)
    message(FATAL_ERROR
        "FreeRTOS kernel source is missing from components/freertos/FreeRTOS-Kernel")
endif()

if(NOT PLATFORM_CHIP STREQUAL "ch585")
    message(STATUS "FreeRTOS: no port selected for ${PLATFORM_CHIP}")
    return()
endif()

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE config)

add_library(freertos_kernel_port_headers INTERFACE)
target_include_directories(freertos_kernel_port_headers INTERFACE
    portable/wch/ch585
)

add_library(freertos_kernel_port OBJECT
    portable/wch/ch585/port.c
    portable/wch/ch585/portASM.S
)
set_target_properties(freertos_kernel_port PROPERTIES C_EXTENSIONS ON)
target_link_libraries(freertos_kernel_port
    PRIVATE freertos_config wch_ch585_sdk
    PUBLIC freertos_kernel_port_headers
)
target_include_directories(freertos_kernel_port PRIVATE
    ${FREERTOS_KERNEL_DIR}/include
)
target_compile_options(freertos_kernel_port PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wundef
)

set(FREERTOS_PORT A_CUSTOM_PORT)
set(FREERTOS_HEAP 4)
add_subdirectory(${FREERTOS_KERNEL_DIR} kernel)

target_link_libraries(freertos_kernel PUBLIC wch_ch585_arch)
set_target_properties(freertos_kernel PROPERTIES C_EXTENSIONS ON)
add_library(platform::freertos ALIAS freertos_kernel)

function(add_ch585_freertos_ble_support target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR "Unknown executable target: ${target}")
    endif()
    target_sources(${target} PRIVATE
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/portable/wch/ch585/ble_task_scheduler.S
    )
    target_link_libraries(${target} PRIVATE
        platform::freertos
        ch585_ble_port
    )
endfunction()
