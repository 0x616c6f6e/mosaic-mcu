get_filename_component(CH585_DEMO_ROOT "${CMAKE_CURRENT_LIST_DIR}" DIRECTORY)

function(add_ch585_demo name)
    set(options BLE)
    set(multi_value_arguments SOURCES LIBRARIES)
    cmake_parse_arguments(DEMO "${options}" "" "${multi_value_arguments}"
                          ${ARGN})

    if(DEMO_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Unknown arguments for ch585_demo_${name}: ${DEMO_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT DEMO_SOURCES)
        message(FATAL_ERROR "ch585_demo_${name} has no sources")
    endif()

    set(target ch585_demo_${name})
    add_executable(${target}
        ${DEMO_SOURCES}
        ${CH585_DEMO_ROOT}/common/demo_support.c
    )
    target_include_directories(${target} PRIVATE
        ${CH585_DEMO_ROOT}/common
    )
    target_link_libraries(${target} PRIVATE
        platform::log
        ${DEMO_LIBRARIES}
    )
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wshadow
        -Wundef
    )
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CH585_DEMO_OUTPUT_DIRECTORY}"
    )

    if(DEMO_BLE)
        configure_ch585_ble_executable(${target})
    else()
        configure_ch585_executable(${target})
    endif()
endfunction()
