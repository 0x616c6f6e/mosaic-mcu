#include <platform_lua.h>

#include <stdint.h>
#include <stdlib.h>

static platform_lua_output_t lua_output;
static void *lua_output_context;
static unsigned int lua_seed = UINT32_C(0x4D4F5341);

static void *limited_realloc(void *context, void *pointer, size_t old_size,
                             size_t new_size)
{
    platform_lua_allocator_t *allocator = context;
    size_t accounted_old_size = (pointer != NULL) ? old_size : 0U;
    void *new_pointer;

    if (new_size == 0U) {
        free(pointer);
        allocator->used = (accounted_old_size <= allocator->used) ?
                          allocator->used - accounted_old_size : 0U;
        return NULL;
    }
    if ((new_size > accounted_old_size) &&
        ((new_size - accounted_old_size) >
         (allocator->limit - allocator->used))) {
        return NULL;
    }
    new_pointer = realloc(pointer, new_size);
    if (new_pointer == NULL) {
        return NULL;
    }
    allocator->used = allocator->used - accounted_old_size + new_size;
    if (allocator->used > allocator->peak) {
        allocator->peak = allocator->used;
    }
    return new_pointer;
}

void platform_lua_allocator_init(platform_lua_allocator_t *allocator,
                                 size_t limit)
{
    if (allocator == NULL) {
        return;
    }
    allocator->limit = limit;
    allocator->used = 0U;
    allocator->peak = 0U;
}

lua_State *platform_lua_newstate(platform_lua_allocator_t *allocator,
                                 unsigned int seed)
{
    if ((allocator == NULL) || (allocator->limit == 0U)) {
        return NULL;
    }
    lua_seed = seed;
    return lua_newstate(limited_realloc, allocator, seed);
}

static void open_library(lua_State *state, const char *name,
                         lua_CFunction function)
{
    luaL_requiref(state, name, function, 1);
    lua_pop(state, 1);
}

void platform_lua_openlibs(lua_State *state)
{
    if (state == NULL) {
        return;
    }
    open_library(state, LUA_GNAME, luaopen_base);
    lua_pushnil(state);
    lua_setglobal(state, "dofile");
    lua_pushnil(state);
    lua_setglobal(state, "loadfile");
    lua_pushnil(state);
    lua_setglobal(state, "load");
#if PLATFORM_LUA_ENABLE_COROUTINE
    open_library(state, LUA_COLIBNAME, luaopen_coroutine);
#endif
#if PLATFORM_LUA_ENABLE_TABLE
    open_library(state, LUA_TABLIBNAME, luaopen_table);
#endif
#if PLATFORM_LUA_ENABLE_STRING
    open_library(state, LUA_STRLIBNAME, luaopen_string);
#endif
#if PLATFORM_LUA_ENABLE_MATH
    open_library(state, LUA_MATHLIBNAME, luaopen_math);
#endif
#if PLATFORM_LUA_ENABLE_UTF8
    open_library(state, LUA_UTF8LIBNAME, luaopen_utf8);
#endif
}

size_t platform_lua_memory_used(const platform_lua_allocator_t *allocator)
{
    return (allocator != NULL) ? allocator->used : 0U;
}

size_t platform_lua_memory_peak(const platform_lua_allocator_t *allocator)
{
    return (allocator != NULL) ? allocator->peak : 0U;
}

void platform_lua_set_output(platform_lua_output_t output, void *context)
{
    lua_output = output;
    lua_output_context = context;
}

void platform_lua_write(const char *data, size_t size)
{
    if ((lua_output != NULL) && (data != NULL) && (size > 0U)) {
        lua_output(data, size, lua_output_context);
    }
}

void platform_lua_write_line(void)
{
    static const char newline = '\n';

    platform_lua_write(&newline, 1U);
}

void platform_lua_write_error(const char *message)
{
    const char *cursor = message;

    if (cursor == NULL) {
        return;
    }
    while (*cursor != '\0') {
        ++cursor;
    }
    platform_lua_write(message, (size_t)(cursor - message));
}

unsigned int platform_lua_seed(void)
{
    return lua_seed;
}

int platform_lua_disabled_loadfilex(lua_State *state, const char *filename,
                                    const char *mode)
{
    (void)filename;
    (void)mode;
    lua_pushliteral(state, "file loading is disabled");
    return LUA_ERRFILE;
}
