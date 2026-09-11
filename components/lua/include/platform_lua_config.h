#ifndef PLATFORM_LUA_CONFIG_H
#define PLATFORM_LUA_CONFIG_H

#include <stddef.h>

#define LUAI_MAXSTACK 2048
#define LUAI_MAXCCALLS 64

void platform_lua_write(const char *data, size_t size);
void platform_lua_write_line(void);
void platform_lua_write_error(const char *message);
unsigned int platform_lua_seed(void);

#define lua_writestring(data, size) platform_lua_write((data), (size))
#define lua_writeline() platform_lua_write_line()
#define lua_writestringerror(format, message) \
    ((void)(format), platform_lua_write_error((message)))
#define luai_makeseed() platform_lua_seed()

#endif
