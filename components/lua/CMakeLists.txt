set(LUA_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lua)

if(NOT EXISTS ${LUA_SOURCE_DIR}/lua.h)
    message(FATAL_ERROR "Lua source is missing from components/lua/lua")
endif()

option(PLATFORM_LUA_ENABLE_COROUTINE "Enable the Lua coroutine library" OFF)
option(PLATFORM_LUA_ENABLE_TABLE "Enable the Lua table library" ON)
option(PLATFORM_LUA_ENABLE_STRING "Enable the reduced Lua string library" ON)
option(PLATFORM_LUA_ENABLE_MATH "Enable the reduced Lua math library" ON)
option(PLATFORM_LUA_ENABLE_UTF8 "Enable the Lua UTF-8 library" OFF)

set(LUA_CORE_SOURCES
    ${LUA_SOURCE_DIR}/lapi.c
    ${LUA_SOURCE_DIR}/lcode.c
    ${LUA_SOURCE_DIR}/lctype.c
    ${LUA_SOURCE_DIR}/ldebug.c
    ${LUA_SOURCE_DIR}/ldo.c
    ${LUA_SOURCE_DIR}/ldump.c
    ${LUA_SOURCE_DIR}/lfunc.c
    ${LUA_SOURCE_DIR}/lgc.c
    ${LUA_SOURCE_DIR}/llex.c
    ${LUA_SOURCE_DIR}/lmem.c
    ${LUA_SOURCE_DIR}/lobject.c
    ${LUA_SOURCE_DIR}/lopcodes.c
    ${LUA_SOURCE_DIR}/lparser.c
    ${LUA_SOURCE_DIR}/lstate.c
    ${LUA_SOURCE_DIR}/lstring.c
    ${LUA_SOURCE_DIR}/ltable.c
    ${LUA_SOURCE_DIR}/ltm.c
    ${LUA_SOURCE_DIR}/lvm.c
    ${LUA_SOURCE_DIR}/lzio.c
    ${LUA_SOURCE_DIR}/lauxlib.c
    ${LUA_SOURCE_DIR}/lbaselib.c
)

if(PLATFORM_LUA_ENABLE_COROUTINE)
    list(APPEND LUA_CORE_SOURCES ${LUA_SOURCE_DIR}/lcorolib.c)
endif()
if(PLATFORM_LUA_ENABLE_TABLE)
    list(APPEND LUA_CORE_SOURCES ${LUA_SOURCE_DIR}/ltablib.c)
endif()
if(PLATFORM_LUA_ENABLE_UTF8)
    list(APPEND LUA_CORE_SOURCES ${LUA_SOURCE_DIR}/lutf8lib.c)
endif()

set(PLATFORM_LUA_LIBRARY_SOURCES)
if(PLATFORM_LUA_ENABLE_MATH)
    list(APPEND PLATFORM_LUA_LIBRARY_SOURCES src/platform_lua_mathlib.c)
endif()
if(PLATFORM_LUA_ENABLE_STRING)
    list(APPEND PLATFORM_LUA_LIBRARY_SOURCES src/platform_lua_strlib.c)
endif()

add_library(platform_lua STATIC
    src/platform_lua.c
    src/platform_lua_undump.c
    ${PLATFORM_LUA_LIBRARY_SOURCES}
    ${LUA_CORE_SOURCES}
)
add_library(platform::lua ALIAS platform_lua)
add_library(lua::lua ALIAS platform_lua)

target_include_directories(platform_lua PUBLIC include)
target_include_directories(platform_lua SYSTEM PUBLIC ${LUA_SOURCE_DIR})
target_compile_definitions(platform_lua PUBLIC
    LUA_32BITS
    LUA_USER_H="platform_lua_config.h"
)
target_compile_definitions(platform_lua PRIVATE
    PLATFORM_LUA_ENABLE_COROUTINE=$<BOOL:${PLATFORM_LUA_ENABLE_COROUTINE}>
    PLATFORM_LUA_ENABLE_TABLE=$<BOOL:${PLATFORM_LUA_ENABLE_TABLE}>
    PLATFORM_LUA_ENABLE_STRING=$<BOOL:${PLATFORM_LUA_ENABLE_STRING}>
    PLATFORM_LUA_ENABLE_MATH=$<BOOL:${PLATFORM_LUA_ENABLE_MATH}>
    PLATFORM_LUA_ENABLE_UTF8=$<BOOL:${PLATFORM_LUA_ENABLE_UTF8}>
)
target_compile_features(platform_lua PUBLIC c_std_11)
if(NOT MSVC)
    target_link_libraries(platform_lua PUBLIC m)
endif()

set_source_files_properties(${LUA_SOURCE_DIR}/lbaselib.c PROPERTIES
    COMPILE_DEFINITIONS "luaL_loadfilex=platform_lua_disabled_loadfilex"
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(${LUA_CORE_SOURCES} PROPERTIES
        COMPILE_OPTIONS "-w"
    )
    target_compile_options(platform_lua PRIVATE
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
