add_library(json_jsmn STATIC
    src/jsmn.c
)
add_library(platform::json ALIAS json_jsmn)
add_library(jsmn::jsmn ALIAS json_jsmn)

target_include_directories(json_jsmn PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/jsmn
)
target_compile_definitions(json_jsmn
    PUBLIC JSMN_STRICT
    INTERFACE JSMN_HEADER
)
target_compile_features(json_jsmn PUBLIC c_std_11)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(json_jsmn PRIVATE
        -Wall
        -Wextra
        -Werror
    )
endif()
