find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(KEYBOARD_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(KEYBOARD_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(KEYBOARD_BUILD_INFO_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(KEYBOARD_BUILD_INFO_HEADER "${KEYBOARD_BUILD_INFO_DIR}/build_info.h")
set(KEYBOARD_FIRMWARE_VERSION "1" CACHE STRING "Version stored in FIRMWARE.OTA")

add_custom_target(keyboard_build_info
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_FILE=${KEYBOARD_BUILD_INFO_HEADER}
        -P ${CMAKE_SOURCE_DIR}/cmake/generate_build_info.cmake
    BYPRODUCTS ${KEYBOARD_BUILD_INFO_HEADER}
    COMMENT "Generating keyboard build timestamp"
    VERBATIM
)

add_subdirectory(board)
add_subdirectory(core)
add_subdirectory(usb)

include(cmake/keyboard_firmware.cmake)

add_subdirectory(bootloader)
add_subdirectory(application)

include(cmake/factory_image.cmake)
