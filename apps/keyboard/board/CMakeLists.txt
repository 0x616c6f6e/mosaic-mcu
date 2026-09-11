add_library(ch585_keyboard_board STATIC
    keyboard_board.c
    keyboard_log.c
    keyboard_matrix.c
)
add_library(keyboard::board ALIAS ch585_keyboard_board)

target_include_directories(ch585_keyboard_board PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ch585_keyboard_board PUBLIC
    platform::log
    platform::ota
)
target_compile_features(ch585_keyboard_board PUBLIC c_std_11)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ch585_keyboard_board PRIVATE
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
