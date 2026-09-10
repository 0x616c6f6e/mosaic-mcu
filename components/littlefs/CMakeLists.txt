set(LITTLEFS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/littlefs)

if(NOT EXISTS ${LITTLEFS_SOURCE_DIR}/lfs.c)
    message(FATAL_ERROR
        "littlefs source is missing from components/littlefs/littlefs")
endif()

add_library(platform_littlefs STATIC
    ${LITTLEFS_SOURCE_DIR}/lfs.c
    ${LITTLEFS_SOURCE_DIR}/lfs_util.c
)
add_library(platform::littlefs ALIAS platform_littlefs)
add_library(littlefs::littlefs ALIAS platform_littlefs)

target_include_directories(platform_littlefs SYSTEM PUBLIC
    ${LITTLEFS_SOURCE_DIR}
)
target_compile_definitions(platform_littlefs PUBLIC
    LFS_NO_MALLOC
    LFS_NO_DEBUG
    LFS_NO_WARN
    LFS_NO_ERROR
)
target_compile_features(platform_littlefs PUBLIC c_std_11)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(platform_littlefs PRIVATE
        -Wall
        -Wextra
        -Wno-unused-function
    )
endif()
