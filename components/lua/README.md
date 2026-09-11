# Lua

`platform::lua` integrates a size-optimized Lua 5.5.1 profile for constrained
MCU applications. The build uses 32-bit integers and floats, limits the Lua
stack to 2048 slots and C call nesting to 64, and always exposes the base
library. The default profile also enables table and reduced string/math
libraries. Coroutine and UTF-8 libraries are disabled by default.

Only source chunks are accepted; precompiled bytecode loading is replaced by a
small rejection stub. Dynamic `load`, file I/O, operating-system access,
package loading, and the debug library are disabled. The string library keeps
`byte`, `char`, `len`, `lower`, `rep`, `reverse`, `sub`, and `upper`; pattern
matching, formatting, packing, and dumping are omitted. The math library keeps
`abs`, `ceil`, `floor`, `fmod`, `max`, `min`, `modf`, `tointeger`, `type`, and
`ult`, plus its numeric constants; transcendental and random-number functions
are omitted.

Library selection is controlled when configuring CMake:

```sh
-DPLATFORM_LUA_ENABLE_COROUTINE=ON
-DPLATFORM_LUA_ENABLE_TABLE=OFF
-DPLATFORM_LUA_ENABLE_STRING=OFF
-DPLATFORM_LUA_ENABLE_MATH=OFF
-DPLATFORM_LUA_ENABLE_UTF8=ON
```

Only enable APIs used by the application. Each disabled library is removed
from the source list, so it does not enter the static archive or firmware.

Create states with a bounded allocator instead of `luaL_newstate()`:

```c
platform_lua_allocator_t allocator;

platform_lua_allocator_init(&allocator, 48U * 1024U);
lua_State *state = platform_lua_newstate(&allocator, seed);
platform_lua_openlibs(state);
```

The limit accounts for memory requested by Lua and prevents the VM from
consuming all MCU heap space. `platform_lua_memory_used()` and
`platform_lua_memory_peak()` expose runtime diagnostics. The allocator uses the
C library `realloc` underneath, so the application must still reserve native
stack and non-Lua heap headroom.

Use `platform_lua_set_output()` to route Lua `print`, warning, and panic output
to a board-specific backend. Applications register hardware APIs explicitly;
the component does not expose Chip API functions to scripts by default.
