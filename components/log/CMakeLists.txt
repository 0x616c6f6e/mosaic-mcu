add_library(platform_log STATIC
    src/platform_log.c
)
add_library(platform::log ALIAS platform_log)

target_include_directories(platform_log PUBLIC include)
target_compile_features(platform_log PUBLIC c_std_11)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(platform_log PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wshadow
        -Wundef
    )
endif()
