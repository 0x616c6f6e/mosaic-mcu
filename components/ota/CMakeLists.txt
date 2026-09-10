add_library(platform_ota STATIC
    src/ota_image.c
    src/ota_spi_disk.c
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
