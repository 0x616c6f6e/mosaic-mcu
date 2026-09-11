#define lundump_c
#define LUA_CORE

#include <lua.h>

#include "ldo.h"
#include "lobject.h"
#include "lundump.h"
#include "lzio.h"

LClosure *luaU_undump(lua_State *state, ZIO *stream, Table *anchor,
                      const char *name, int fixed)
{
    (void)stream;
    (void)anchor;
    (void)fixed;
    luaO_pushfstring(state, "%s: binary chunks are disabled", name);
    luaD_throw(state, LUA_ERRSYNTAX);
}
