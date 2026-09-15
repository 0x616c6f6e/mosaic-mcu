#ifndef PLATFORM_LUA_H
#define PLATFORM_LUA_H

#include <stddef.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Bounded Lua heap usage and peak usage in bytes. */
typedef struct {
    size_t limit; /**< Maximum live allocation in bytes. */
    size_t used;  /**< Current live allocation in bytes. */
    size_t peak;  /**< Highest live allocation in bytes. */
} platform_lua_allocator_t;

/** @brief Receive Lua standard output bytes. */
typedef void (*platform_lua_output_t)(const char *data, size_t size,
                                      void *context);

/** @brief Configure a bounded allocator for a new Lua state.
 * @param[out] allocator Allocator to initialize.
 * @param limit Maximum live Lua allocation in bytes.
 */
void platform_lua_allocator_init(platform_lua_allocator_t *allocator,
                                 size_t limit);
/** @brief Create a Lua state backed by a bounded allocator.
 * @param allocator Persistent allocator for the state lifetime.
 * @param seed Seed for the Lua runtime.
 * @return New Lua state, or NULL on allocation failure.
 */
lua_State *platform_lua_newstate(platform_lua_allocator_t *allocator,
                                 unsigned int seed);
/** @brief Open the platform's restricted standard Lua libraries.
 * @param state Initialized Lua state.
 */
void platform_lua_openlibs(lua_State *state);
/** @brief Query live Lua allocation.
 * @param allocator Allocator used by the Lua state.
 * @return Live usage in bytes.
 */
size_t platform_lua_memory_used(const platform_lua_allocator_t *allocator);
/** @brief Query peak Lua allocation.
 * @param allocator Allocator used by the Lua state.
 * @return Peak usage in bytes.
 */
size_t platform_lua_memory_peak(const platform_lua_allocator_t *allocator);
/** @brief Redirect Lua output to a callback.
 * @param output Output handler, or NULL to disable it.
 * @param context Opaque handler context.
 */
void platform_lua_set_output(platform_lua_output_t output, void *context);

#ifdef __cplusplus
}
#endif

#endif
