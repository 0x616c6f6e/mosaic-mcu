cmake_minimum_required(VERSION 3.20)

project(mosaic_mcu LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(PLATFORM_CHIP "ch585" CACHE STRING "SoC port to build")
set_property(CACHE PLATFORM_CHIP PROPERTY STRINGS ch585)

include(cmake/wchisp.cmake)
add_subdirectory(chips)
add_subdirectory(components)

option(PLATFORM_BUILD_DEMOS "Build hardware test applications" ON)
if(PLATFORM_BUILD_DEMOS AND PLATFORM_CHIP STREQUAL "ch585")
    add_subdirectory(apps/demo)
endif()

option(PLATFORM_BUILD_OTA "Build OTA bootloader and USB disk application" ON)
if(PLATFORM_BUILD_OTA AND PLATFORM_CHIP STREQUAL "ch585")
    add_subdirectory(apps/ota)
endif()
