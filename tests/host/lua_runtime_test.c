#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <platform_lua.h>

static char output_buffer[64];
static size_t output_size;

static void capture_output(const char *data, size_t size, void *context)
{
    (void)context;
    assert(size <= (sizeof(output_buffer) - output_size));
    memcpy(&output_buffer[output_size], data, size);
    output_size += size;
}

int main(void)
{
    static const char script[] =
        "print('lua-ready')\n"
        "return math.floor(2.5 * 4), table.concat({'a', 'b'}, '-'), "
        "string.upper('lua'), string.sub('mosaic', 2, 4), "
        "string.rep('x', 3, '-'), math.fmod(10, 3)";
    platform_lua_allocator_t allocator;
    lua_State *state;

    platform_lua_set_output(capture_output, NULL);
    platform_lua_allocator_init(&allocator, 128U * 1024U);
    state = platform_lua_newstate(&allocator, 1234U);
    assert(state != NULL);
    platform_lua_openlibs(state);

    lua_getglobal(state, "io");
    assert(lua_isnil(state, -1));
    lua_pop(state, 1);
    lua_getglobal(state, "os");
    assert(lua_isnil(state, -1));
    lua_pop(state, 1);
    lua_getglobal(state, "package");
    assert(lua_isnil(state, -1));
    lua_pop(state, 1);
    lua_getglobal(state, "debug");
    assert(lua_isnil(state, -1));
    lua_pop(state, 1);
    lua_getglobal(state, "coroutine");
    assert(lua_isnil(state, -1));
    lua_pop(state, 1);
    lua_getglobal(state, "utf8");
    assert(lua_isnil(state, -1));
    lua_pop(state, 1);
    lua_getglobal(state, "dofile");
    assert(lua_isnil(state, -1));
    lua_pop(state, 1);
    lua_getglobal(state, "loadfile");
    assert(lua_isnil(state, -1));
    lua_pop(state, 1);
    lua_getglobal(state, "load");
    assert(lua_isnil(state, -1));
    lua_pop(state, 1);

    lua_getglobal(state, "math");
    lua_getfield(state, -1, "sin");
    assert(lua_isnil(state, -1));
    lua_pop(state, 2);
    lua_getglobal(state, "string");
    lua_getfield(state, -1, "dump");
    assert(lua_isnil(state, -1));
    lua_pop(state, 2);

    assert(luaL_loadbuffer(state, LUA_SIGNATURE, sizeof(LUA_SIGNATURE) - 1U,
                           "binary") == LUA_ERRSYNTAX);
    assert(strstr(lua_tostring(state, -1), "binary chunks are disabled") !=
           NULL);
    lua_pop(state, 1);

    assert(luaL_loadbuffer(state, script, sizeof(script) - 1U, "test") ==
           LUA_OK);
    assert(lua_pcall(state, 0, 6, 0) == LUA_OK);
    assert(lua_tointeger(state, -6) == 10);
    assert(strcmp(lua_tostring(state, -5), "a-b") == 0);
    assert(strcmp(lua_tostring(state, -4), "LUA") == 0);
    assert(strcmp(lua_tostring(state, -3), "osa") == 0);
    assert(strcmp(lua_tostring(state, -2), "x-x-x") == 0);
    assert(lua_tointeger(state, -1) == 1);
    assert(output_size == sizeof("lua-ready\n") - 1U);
    assert(memcmp(output_buffer, "lua-ready\n", output_size) == 0);
    assert(platform_lua_memory_used(&allocator) > 0U);
    assert(platform_lua_memory_peak(&allocator) >=
           platform_lua_memory_used(&allocator));
    lua_close(state);
    assert(platform_lua_memory_used(&allocator) == 0U);

    platform_lua_allocator_init(&allocator, 64U);
    assert(platform_lua_newstate(&allocator, 1U) == NULL);
    return 0;
}
