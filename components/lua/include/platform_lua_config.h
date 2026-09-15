#ifndef PLATFORM_LUA_CONFIG_H
#define PLATFORM_LUA_CONFIG_H

#include <stddef.h>

/** @brief Lua VM maximum stack slots for constrained MCU RAM. */
#define LUAI_MAXSTACK 2048
/** @brief Lua VM maximum nested C calls. */
#define LUAI_MAXCCALLS 64

/** @brief Route Lua standard output to the platform callback.
 * @param data Bytes to emit.
 * @param size Data length in bytes.
 */
void platform_lua_write(const char *data, size_t size);
/** @brief Emit a Lua output line ending. */
void platform_lua_write_line(void);
/** @brief Route a Lua error message to the platform callback.
 * @param message NUL-terminated error text.
 */
void platform_lua_write_error(const char *message);
/** @brief Return the seed supplied to the platform Lua runtime.
 * @return Lua random seed.
 */
unsigned int platform_lua_seed(void);

/** @brief Adapt Lua's print hook to the platform output callback. */
#define lua_writestring(data, size) platform_lua_write((data), (size))
/** @brief Adapt Lua's line-ending hook to the platform output callback. */
#define lua_writeline() platform_lua_write_line()
/** @brief Adapt Lua's error hook to the platform output callback. */
#define lua_writestringerror(format, message) \
    ((void)(format), platform_lua_write_error((message)))
/** @brief Seed the Lua runtime using the configured platform seed. */
#define luai_makeseed() platform_lua_seed()

#endif
