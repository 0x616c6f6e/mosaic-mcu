set(FATFS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/fatfs/source)

if(NOT EXISTS ${FATFS_SOURCE_DIR}/ff.c)
    message(FATAL_ERROR "FatFs source is missing from components/fatfs/fatfs")
endif()

add_library(platform_fatfs STATIC
    ${FATFS_SOURCE_DIR}/ff.c
    ${FATFS_SOURCE_DIR}/ffunicode.c
)
add_library(platform::fatfs ALIAS platform_fatfs)
add_library(fatfs::fatfs ALIAS platform_fatfs)

target_include_directories(platform_fatfs PUBLIC include)
target_include_directories(platform_fatfs SYSTEM PUBLIC ${FATFS_SOURCE_DIR})
target_compile_options(platform_fatfs PRIVATE
    -include${CMAKE_CURRENT_SOURCE_DIR}/include/platform_fatfs_config.h
)
target_compile_features(platform_fatfs PUBLIC c_std_11)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(platform_fatfs PRIVATE
        -Wall
        -Wextra
    )
endif()
