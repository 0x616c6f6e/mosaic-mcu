#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

static size_t relative_position(lua_Integer position, size_t length)
{
    if (position > 0) {
        return (size_t)position;
    }
    if (position == 0) {
        return 1U;
    }
    if (position < -(lua_Integer)length) {
        return 1U;
    }
    return length + (size_t)position + 1U;
}

static size_t end_position(lua_State *state, int argument,
                           lua_Integer default_value, size_t length)
{
    lua_Integer position = luaL_optinteger(state, argument, default_value);

    if (position > (lua_Integer)length) {
        return length;
    }
    if (position >= 0) {
        return (size_t)position;
    }
    if (position < -(lua_Integer)length) {
        return 0U;
    }
    return length + (size_t)position + 1U;
}

static int string_len(lua_State *state)
{
    size_t length;

    (void)luaL_checklstring(state, 1, &length);
    lua_pushinteger(state, (lua_Integer)length);
    return 1;
}

static int string_sub(lua_State *state)
{
    size_t length;
    const char *value = luaL_checklstring(state, 1, &length);
    size_t start = relative_position(luaL_checkinteger(state, 2), length);
    size_t end = end_position(state, 3, -1, length);

    if (start <= end) {
        lua_pushlstring(state, value + start - 1U, end - start + 1U);
    } else {
        lua_pushliteral(state, "");
    }
    return 1;
}

static int string_reverse(lua_State *state)
{
    size_t length;
    size_t index;
    luaL_Buffer buffer;
    const char *value = luaL_checklstring(state, 1, &length);
    char *output = luaL_buffinitsize(state, &buffer, length);

    for (index = 0U; index < length; ++index) {
        output[index] = value[length - index - 1U];
    }
    luaL_pushresultsize(&buffer, length);
    return 1;
}

static int string_change_case(lua_State *state, int upper)
{
    size_t length;
    size_t index;
    luaL_Buffer buffer;
    const char *value = luaL_checklstring(state, 1, &length);
    char *output = luaL_buffinitsize(state, &buffer, length);

    for (index = 0U; index < length; ++index) {
        unsigned char character = (unsigned char)value[index];

        output[index] = (char)(upper != 0 ? toupper(character) :
                                           tolower(character));
    }
    luaL_pushresultsize(&buffer, length);
    return 1;
}

static int string_lower(lua_State *state)
{
    return string_change_case(state, 0);
}

static int string_upper(lua_State *state)
{
    return string_change_case(state, 1);
}

static int string_rep(lua_State *state)
{
    size_t length;
    size_t separator_length;
    lua_Integer count = luaL_checkinteger(state, 2);
    const char *value = luaL_checklstring(state, 1, &length);
    const char *separator = luaL_optlstring(state, 3, "", &separator_length);

    if ((count <= 0) || ((length == 0U) && (separator_length == 0U))) {
        lua_pushliteral(state, "");
    } else {
        size_t repetitions = (size_t)count;
        size_t unit = length + separator_length;
        size_t total;
        size_t index;
        luaL_Buffer buffer;
        char *output;

        luaL_argcheck(state, unit >= length, 2, "resulting string too large");
        luaL_argcheck(state,
                      (unit == 0U) ||
                          (repetitions <= (SIZE_MAX - separator_length) / unit),
                      2, "resulting string too large");
        total = repetitions * unit - separator_length;
        luaL_argcheck(state, total <= (size_t)LUA_MAXINTEGER, 2,
                      "resulting string too large");
        output = luaL_buffinitsize(state, &buffer, total);
        for (index = 0U; index < repetitions; ++index) {
            memcpy(output, value, length);
            output += length;
            if ((index + 1U) < repetitions) {
                memcpy(output, separator, separator_length);
                output += separator_length;
            }
        }
        luaL_pushresultsize(&buffer, total);
    }
    return 1;
}

static int string_byte(lua_State *state)
{
    size_t length;
    const unsigned char *value =
        (const unsigned char *)luaL_checklstring(state, 1, &length);
    lua_Integer initial = luaL_optinteger(state, 2, 1);
    size_t start = relative_position(initial, length);
    size_t end = end_position(state, 3, initial, length);
    size_t count;
    size_t index;

    if (start > end) {
        return 0;
    }
    count = end - start + 1U;
    luaL_argcheck(state, count <= (size_t)INT_MAX, 3,
                  "string slice too long");
    luaL_checkstack(state, (int)count, "string slice too long");
    for (index = 0U; index < count; ++index) {
        lua_pushinteger(state, value[start + index - 1U]);
    }
    return (int)count;
}

static int string_char(lua_State *state)
{
    int count = lua_gettop(state);
    int index;
    luaL_Buffer buffer;
    char *output = luaL_buffinitsize(state, &buffer, (size_t)count);

    for (index = 1; index <= count; ++index) {
        lua_Unsigned character = (lua_Unsigned)luaL_checkinteger(state, index);

        luaL_argcheck(state, character <= UCHAR_MAX, index,
                      "value out of range");
        output[index - 1] = (char)character;
    }
    luaL_pushresultsize(&buffer, (size_t)count);
    return 1;
}

LUAMOD_API int luaopen_string(lua_State *state)
{
    static const luaL_Reg functions[] = {
        {"byte", string_byte},
        {"char", string_char},
        {"len", string_len},
        {"lower", string_lower},
        {"rep", string_rep},
        {"reverse", string_reverse},
        {"sub", string_sub},
        {"upper", string_upper},
        {NULL, NULL},
    };

    luaL_newlib(state, functions);
    lua_newtable(state);
    lua_pushliteral(state, "");
    lua_pushvalue(state, -2);
    lua_setmetatable(state, -2);
    lua_pop(state, 1);
    lua_pushvalue(state, -2);
    lua_setfield(state, -2, "__index");
    lua_pop(state, 1);
    return 1;
}
