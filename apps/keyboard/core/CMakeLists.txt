add_library(keyboard_core STATIC keyboard_engine.c)
add_library(keyboard::core ALIAS keyboard_core)

target_include_directories(keyboard_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(keyboard_core PUBLIC c_std_11)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(keyboard_core PRIVATE
        $<$<CONFIG:Release>:-Os>
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wshadow
        -Wundef
        -ffunction-sections
        -fdata-sections
    )
endif()
