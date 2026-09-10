add_library(platform_spi_flash STATIC
    src/spi_flash.c
)
add_library(platform::spi_flash ALIAS platform_spi_flash)

target_include_directories(platform_spi_flash PUBLIC include)
if(TARGET chip::selected)
    target_link_libraries(platform_spi_flash PUBLIC chip::selected)
else()
    target_link_libraries(platform_spi_flash PUBLIC chip::api)
endif()
target_compile_features(platform_spi_flash PUBLIC c_std_11)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(platform_spi_flash PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wshadow
        -Wundef
    )
endif()
