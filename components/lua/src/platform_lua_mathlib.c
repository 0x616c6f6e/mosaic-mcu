#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <math.h>

static void push_number_or_integer(lua_State *state, lua_Number value)
{
    lua_Integer integer;

    if ((value >= (lua_Number)LUA_MININTEGER) &&
        (value < -(lua_Number)LUA_MININTEGER)) {
        integer = (lua_Integer)value;
        lua_pushinteger(state, integer);
    } else {
        lua_pushnumber(state, value);
    }
}

static int math_abs(lua_State *state)
{
    if (lua_isinteger(state, 1)) {
        lua_Integer value = lua_tointeger(state, 1);

        if (value < 0) {
            value = (lua_Integer)(0U - (lua_Unsigned)value);
        }
        lua_pushinteger(state, value);
    } else {
        lua_pushnumber(state, fabsf(luaL_checknumber(state, 1)));
    }
    return 1;
}

static int math_ceil(lua_State *state)
{
    if (lua_isinteger(state, 1)) {
        lua_settop(state, 1);
    } else {
        push_number_or_integer(state, ceilf(luaL_checknumber(state, 1)));
    }
    return 1;
}

static int math_floor(lua_State *state)
{
    if (lua_isinteger(state, 1)) {
        lua_settop(state, 1);
    } else {
        push_number_or_integer(state, floorf(luaL_checknumber(state, 1)));
    }
    return 1;
}

static int math_fmod(lua_State *state)
{
    if (lua_isinteger(state, 1) && lua_isinteger(state, 2)) {
        lua_Integer dividend = lua_tointeger(state, 1);
        lua_Integer divisor = lua_tointeger(state, 2);

        luaL_argcheck(state, divisor != 0, 2, "zero");
        if ((dividend == LUA_MININTEGER) && (divisor == -1)) {
            lua_pushinteger(state, 0);
        } else {
            lua_pushinteger(state, dividend % divisor);
        }
    } else {
        lua_Number dividend = luaL_checknumber(state, 1);
        lua_Number divisor = luaL_checknumber(state, 2);

        lua_pushnumber(state, fmodf(dividend, divisor));
    }
    return 1;
}

static int math_max(lua_State *state)
{
    int count = lua_gettop(state);
    int maximum = 1;
    int index;

    luaL_argcheck(state, count >= 1, 1, "value expected");
    for (index = 2; index <= count; ++index) {
        if (lua_compare(state, maximum, index, LUA_OPLT)) {
            maximum = index;
        }
    }
    lua_pushvalue(state, maximum);
    return 1;
}

static int math_min(lua_State *state)
{
    int count = lua_gettop(state);
    int minimum = 1;
    int index;

    luaL_argcheck(state, count >= 1, 1, "value expected");
    for (index = 2; index <= count; ++index) {
        if (lua_compare(state, index, minimum, LUA_OPLT)) {
            minimum = index;
        }
    }
    lua_pushvalue(state, minimum);
    return 1;
}

static int math_modf(lua_State *state)
{
    if (lua_isinteger(state, 1)) {
        lua_settop(state, 1);
        lua_pushnumber(state, 0.0F);
    } else {
        lua_Number value = luaL_checknumber(state, 1);
        lua_Number integer = (value < 0.0F) ? ceilf(value) : floorf(value);

        push_number_or_integer(state, integer);
        lua_pushnumber(state, (value == integer) ? 0.0F : value - integer);
    }
    return 2;
}

static int math_tointeger(lua_State *state)
{
    int valid;
    lua_Integer value = lua_tointegerx(state, 1, &valid);

    if (valid != 0) {
        lua_pushinteger(state, value);
    } else {
        luaL_checkany(state, 1);
        lua_pushnil(state);
    }
    return 1;
}

static int math_type(lua_State *state)
{
    if (lua_type(state, 1) == LUA_TNUMBER) {
        lua_pushstring(state, lua_isinteger(state, 1) ? "integer" : "float");
    } else {
        luaL_checkany(state, 1);
        lua_pushnil(state);
    }
    return 1;
}

static int math_ult(lua_State *state)
{
    lua_Unsigned left = (lua_Unsigned)luaL_checkinteger(state, 1);
    lua_Unsigned right = (lua_Unsigned)luaL_checkinteger(state, 2);

    lua_pushboolean(state, left < right);
    return 1;
}

LUAMOD_API int luaopen_math(lua_State *state)
{
    static const luaL_Reg functions[] = {
        {"abs", math_abs},
        {"ceil", math_ceil},
        {"floor", math_floor},
        {"fmod", math_fmod},
        {"max", math_max},
        {"min", math_min},
        {"modf", math_modf},
        {"tointeger", math_tointeger},
        {"type", math_type},
        {"ult", math_ult},
        {NULL, NULL},
    };

    luaL_newlib(state, functions);
    lua_pushnumber(state, 3.14159265358979323846F);
    lua_setfield(state, -2, "pi");
    lua_pushnumber(state, HUGE_VALF);
    lua_setfield(state, -2, "huge");
    lua_pushinteger(state, LUA_MAXINTEGER);
    lua_setfield(state, -2, "maxinteger");
    lua_pushinteger(state, LUA_MININTEGER);
    lua_setfield(state, -2, "mininteger");
    return 1;
}
