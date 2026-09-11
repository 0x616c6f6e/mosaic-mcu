#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <chip_time.h>
#include <platform_log.h>
#include <platform_lua.h>

#include "demo_support.h"

#define LUA_MEMORY_LIMIT (48U * 1024U)
#define LUA_OUTPUT_LINE_SIZE 160U

typedef struct {
    char data[LUA_OUTPUT_LINE_SIZE];
    size_t size;
    bool truncated;
} lua_output_buffer_t;

static lua_output_buffer_t output_buffer;

static void flush_lua_output(lua_output_buffer_t *output)
{
    if ((output->size == 0U) && !output->truncated) {
        return;
    }
    output->data[output->size] = '\0';
    LOG_INFO("lua", "%s%s", output->data,
             output->truncated ? "..." : "");
    output->size = 0U;
    output->truncated = false;
}

static void lua_log_output(const char *data, size_t size, void *context)
{
    lua_output_buffer_t *output = context;
    size_t index;

    for (index = 0U; index < size; ++index) {
        if (data[index] == '\n') {
            flush_lua_output(output);
        } else if (output->size < (sizeof(output->data) - 1U)) {
            output->data[output->size++] = data[index];
        } else {
            output->truncated = true;
        }
    }
}

static int lua_mcu_led(lua_State *state)
{
    bool enabled = lua_toboolean(state, 1) != 0;

    if (demo_led_write(enabled) != CHIP_OK) {
        return luaL_error(state, "LED write failed");
    }
    return 0;
}

static int lua_mcu_delay(lua_State *state)
{
    lua_Integer delay_ms = luaL_checkinteger(state, 1);

    if ((delay_ms < 0) || (delay_ms > 10000)) {
        return luaL_error(state, "delay must be between 0 and 10000 ms");
    }
    chip_delay_ms((uint32_t)delay_ms);
    return 0;
}

static int lua_mcu_millis(lua_State *state)
{
    lua_pushinteger(state, (lua_Integer)chip_time_millis());
    return 1;
}

static void register_mcu_module(lua_State *state)
{
    static const luaL_Reg functions[] = {
        {"led", lua_mcu_led},
        {"delay", lua_mcu_delay},
        {"millis", lua_mcu_millis},
        {NULL, NULL},
    };

    luaL_newlib(state, functions);
    lua_setglobal(state, "mcu");
}

static void lua_fail(lua_State *state, const char *stage,
                     const platform_lua_allocator_t *allocator)
{
    const char *message = lua_tostring(state, -1);

    LOG_ERROR("lua", "%s failed: %s", stage,
              (message != NULL) ? message : "unknown error");
    LOG_ERROR("lua", "memory used=%lu peak=%lu limit=%lu",
              (unsigned long)platform_lua_memory_used(allocator),
              (unsigned long)platform_lua_memory_peak(allocator),
              (unsigned long)allocator->limit);
    lua_close(state);
    demo_halt();
}

int main(void)
{
    static const char script[] =
        "print('runtime', _VERSION)\n"
        "local values = {}\n"
        "for i = 1, 6 do values[i] = i * i end\n"
        "print(string.upper('mosaic'), table.concat(values, ','))\n"
        "print('math', math.floor(3.75 * 4), 'chars', string.len('Lua'))\n"
        "for i = 1, 4 do\n"
        "  mcu.led(i % 2 == 1)\n"
        "  print('step', i, 'millis', mcu.millis())\n"
        "  mcu.delay(200)\n"
        "end\n"
        "return #values, values[6]\n";
    platform_lua_allocator_t allocator;
    lua_State *state;
    int status;
    int count_valid;
    int last_value_valid;
    lua_Integer count;
    lua_Integer last_value;

    DEMO_REQUIRE(demo_platform_init());
    DEMO_REQUIRE(demo_led_init());
    DEMO_REQUIRE(demo_log_init());
    platform_lua_set_output(lua_log_output, &output_buffer);
    platform_lua_allocator_init(&allocator, LUA_MEMORY_LIMIT);
    state = platform_lua_newstate(&allocator, chip_time_millis());
    if (state == NULL) {
        LOG_ERROR("lua", "state allocation failed limit=%lu",
                  (unsigned long)allocator.limit);
        demo_halt();
    }
    platform_lua_openlibs(state);
    register_mcu_module(state);

    status = luaL_loadbuffer(state, script, sizeof(script) - 1U, "demo");
    if (status != LUA_OK) {
        lua_fail(state, "compile", &allocator);
    }
    status = lua_pcall(state, 0, 2, 0);
    if (status != LUA_OK) {
        lua_fail(state, "execute", &allocator);
    }
    count = lua_tointegerx(state, -2, &count_valid);
    last_value = lua_tointegerx(state, -1, &last_value_valid);
    if (!count_valid || !last_value_valid) {
        LOG_ERROR("lua", "script returned invalid result types");
        lua_close(state);
        demo_halt();
    }
    LOG_INFO("lua", "completed count=%ld last=%ld used=%lu peak=%lu",
             (long)count, (long)last_value,
             (unsigned long)platform_lua_memory_used(&allocator),
             (unsigned long)platform_lua_memory_peak(&allocator));
    lua_close(state);
    flush_lua_output(&output_buffer);
    LOG_INFO("lua", "closed memory=%lu",
             (unsigned long)platform_lua_memory_used(&allocator));
    DEMO_REQUIRE(demo_led_write(true));
    for (;;) {
        chip_delay_ms(1000U);
    }
}
