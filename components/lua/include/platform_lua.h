#ifndef PLATFORM_LUA_H
#define PLATFORM_LUA_H

#include <stddef.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t limit;
    size_t used;
    size_t peak;
} platform_lua_allocator_t;

typedef void (*platform_lua_output_t)(const char *data, size_t size,
                                      void *context);

void platform_lua_allocator_init(platform_lua_allocator_t *allocator,
                                 size_t limit);
lua_State *platform_lua_newstate(platform_lua_allocator_t *allocator,
                                 unsigned int seed);
void platform_lua_openlibs(lua_State *state);
size_t platform_lua_memory_used(const platform_lua_allocator_t *allocator);
size_t platform_lua_memory_peak(const platform_lua_allocator_t *allocator);
void platform_lua_set_output(platform_lua_output_t output, void *context);

#ifdef __cplusplus
}
#endif

#endif
