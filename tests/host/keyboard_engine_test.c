#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <keyboard_engine.h>

typedef struct {
    bool keys[KEYBOARD_ENGINE_MAX_KEYS];
    unsigned int scans;
} fake_matrix_t;

static bool scan_matrix(bool *pressed, size_t key_count, void *context)
{
    fake_matrix_t *matrix = context;

    memcpy(pressed, matrix->keys, key_count * sizeof(pressed[0]));
    ++matrix->scans;
    return true;
}

static void test_debounce_and_schedule(void)
{
    static const keyboard_binding_t bindings[] = {
        {.modifier = 0U, .usage = 4U},
        {.modifier = 0x02U, .usage = 0U},
        {.modifier = 0U, .usage = 5U},
    };
    fake_matrix_t matrix = {0};
    const keyboard_engine_config_t config = {
        .bindings = bindings,
        .key_count = sizeof(bindings) / sizeof(bindings[0]),
        .debounce_scans = 2U,
        .scan_interval_ms = 5U,
        .scan = scan_matrix,
        .scan_context = &matrix,
    };
    keyboard_engine_t engine;
    uint8_t report[KEYBOARD_HID_REPORT_SIZE];
    bool changed;

    assert(keyboard_engine_init(&engine, &config));
    matrix.keys[0] = true;
    assert(keyboard_engine_poll(&engine, 0U, report, &changed));
    assert(!changed);
    assert(matrix.scans == 1U);
    assert(keyboard_engine_poll(&engine, 4U, report, &changed));
    assert(!changed);
    assert(matrix.scans == 1U);
    assert(keyboard_engine_poll(&engine, 5U, report, &changed));
    assert(changed);
    assert(report[0] == 0U);
    assert(report[2] == 4U);

    matrix.keys[1] = true;
    assert(keyboard_engine_poll(&engine, 10U, report, &changed));
    assert(!changed);
    assert(keyboard_engine_poll(&engine, 15U, report, &changed));
    assert(changed);
    assert(report[0] == 0x02U);
    assert(report[2] == 4U);

    matrix.keys[0] = false;
    matrix.keys[1] = false;
    assert(keyboard_engine_poll(&engine, 20U, report, &changed));
    assert(!changed);
    assert(keyboard_engine_poll(&engine, 25U, report, &changed));
    assert(changed);
    assert(memcmp(report, (uint8_t[KEYBOARD_HID_REPORT_SIZE]){0},
                  sizeof(report)) == 0);
}

static void test_rollover(void)
{
    static const keyboard_binding_t bindings[] = {
        {.usage = 4U}, {.usage = 5U}, {.usage = 6U}, {.usage = 7U},
        {.usage = 8U}, {.usage = 9U}, {.usage = 10U},
    };
    fake_matrix_t matrix = {0};
    const keyboard_engine_config_t config = {
        .bindings = bindings,
        .key_count = sizeof(bindings) / sizeof(bindings[0]),
        .debounce_scans = 1U,
        .scan_interval_ms = 1U,
        .scan = scan_matrix,
        .scan_context = &matrix,
    };
    keyboard_engine_t engine;
    uint8_t report[KEYBOARD_HID_REPORT_SIZE];
    bool changed;
    size_t index;

    for (index = 0U; index < config.key_count; ++index) {
        matrix.keys[index] = true;
    }
    assert(keyboard_engine_init(&engine, &config));
    assert(keyboard_engine_poll(&engine, 0U, report, &changed));
    assert(changed);
    for (index = 2U; index < sizeof(report); ++index) {
        assert(report[index] == KEYBOARD_HID_ERROR_ROLLOVER);
    }
}

static void test_disabled_engine(void)
{
    const keyboard_engine_config_t config = {0};
    keyboard_engine_t engine;
    uint8_t report[KEYBOARD_HID_REPORT_SIZE];
    bool changed = true;

    assert(keyboard_engine_init(&engine, &config));
    assert(keyboard_engine_poll(&engine, 0U, report, &changed));
    assert(!changed);
    assert(memcmp(report, (uint8_t[KEYBOARD_HID_REPORT_SIZE]){0},
                  sizeof(report)) == 0);
}

int main(void)
{
    test_debounce_and_schedule();
    test_rollover();
    test_disabled_engine();
    return 0;
}
