#include "keyboard_layout.h"

#include <stddef.h>

#include <keyboard_matrix.h>

#include "tusb.h"

#define ROW_COUNT    5U
#define COLUMN_COUNT 14U
#define MATRIX_KEY_COUNT (ROW_COUNT * COLUMN_COUNT)
#define DIRECT_KEY_COUNT 1U
#define KEY_COUNT        (MATRIX_KEY_COUNT + DIRECT_KEY_COUNT)
#define RIGHT_ALT_PIN CHIP_PIN(CHIP_GPIO_PORT_B, 22)

static const chip_pin_t row_pins[ROW_COUNT] = {
    CHIP_PIN(CHIP_GPIO_PORT_B, 5),
    CHIP_PIN(CHIP_GPIO_PORT_B, 3),
    CHIP_PIN(CHIP_GPIO_PORT_B, 2),
    CHIP_PIN(CHIP_GPIO_PORT_B, 15),
    CHIP_PIN(CHIP_GPIO_PORT_B, 9),
};

static const chip_pin_t column_pins[COLUMN_COUNT] = {
    CHIP_PIN(CHIP_GPIO_PORT_A, 7),
    CHIP_PIN(CHIP_GPIO_PORT_A, 8),
    CHIP_PIN(CHIP_GPIO_PORT_A, 9),
    CHIP_PIN(CHIP_GPIO_PORT_B, 8),
    CHIP_PIN(CHIP_GPIO_PORT_B, 17),
    CHIP_PIN(CHIP_GPIO_PORT_B, 16),
    CHIP_PIN(CHIP_GPIO_PORT_B, 14),
    CHIP_PIN(CHIP_GPIO_PORT_B, 6),
    CHIP_PIN(CHIP_GPIO_PORT_B, 1),
    CHIP_PIN(CHIP_GPIO_PORT_B, 0),
    CHIP_PIN(CHIP_GPIO_PORT_B, 21),
    CHIP_PIN(CHIP_GPIO_PORT_B, 20),
    CHIP_PIN(CHIP_GPIO_PORT_B, 19),
    CHIP_PIN(CHIP_GPIO_PORT_B, 18),
};

static const keyboard_binding_t bindings[] = {
    /* Row 0: Escape, number row, Backspace. */
    {0U, HID_KEY_ESCAPE},
    {0U, HID_KEY_1},
    {0U, HID_KEY_2},
    {0U, HID_KEY_3},
    {0U, HID_KEY_4},
    {0U, HID_KEY_5},
    {0U, HID_KEY_6},
    {0U, HID_KEY_7},
    {0U, HID_KEY_8},
    {0U, HID_KEY_9},
    {0U, HID_KEY_0},
    {0U, HID_KEY_MINUS},
    {0U, HID_KEY_EQUAL},
    {0U, HID_KEY_BACKSPACE},

    /* Row 1: Tab, Q-P, brackets, Backslash. */
    {0U, HID_KEY_TAB},
    {0U, HID_KEY_Q},
    {0U, HID_KEY_W},
    {0U, HID_KEY_E},
    {0U, HID_KEY_R},
    {0U, HID_KEY_T},
    {0U, HID_KEY_Y},
    {0U, HID_KEY_U},
    {0U, HID_KEY_I},
    {0U, HID_KEY_O},
    {0U, HID_KEY_P},
    {0U, HID_KEY_BRACKET_LEFT},
    {0U, HID_KEY_BRACKET_RIGHT},
    {0U, HID_KEY_BACKSLASH},

    /* Row 2: Caps Lock, A-L, punctuation, Enter, unused. */
    {0U, HID_KEY_CAPS_LOCK},
    {0U, HID_KEY_A},
    {0U, HID_KEY_S},
    {0U, HID_KEY_D},
    {0U, HID_KEY_F},
    {0U, HID_KEY_G},
    {0U, HID_KEY_H},
    {0U, HID_KEY_J},
    {0U, HID_KEY_K},
    {0U, HID_KEY_L},
    {0U, HID_KEY_SEMICOLON},
    {0U, HID_KEY_APOSTROPHE},
    {0U, HID_KEY_ENTER},
    {0U, HID_KEY_NONE},

    /* Row 3: Shift, Z-M, punctuation, Right Shift, Up, Delete. */
    {KEYBOARD_MODIFIER_LEFTSHIFT, 0U},
    {0U, HID_KEY_Z},
    {0U, HID_KEY_X},
    {0U, HID_KEY_C},
    {0U, HID_KEY_V},
    {0U, HID_KEY_B},
    {0U, HID_KEY_N},
    {0U, HID_KEY_M},
    {0U, HID_KEY_COMMA},
    {0U, HID_KEY_PERIOD},
    {0U, HID_KEY_SLASH},
    {KEYBOARD_MODIFIER_RIGHTSHIFT, 0U},
    {0U, HID_KEY_ARROW_UP},
    {0U, HID_KEY_DELETE},

    /* Row 4: modifiers, Space, direct-Alt hole, Fn, and arrow keys. */
    {KEYBOARD_MODIFIER_LEFTCTRL, 0U},
    {KEYBOARD_MODIFIER_LEFTGUI, 0U},
    {KEYBOARD_MODIFIER_LEFTALT, 0U},
    {0U, HID_KEY_SPACE},
    {0U, HID_KEY_NONE},
    {0U, HID_KEY_NONE},
    {0U, HID_KEY_ARROW_LEFT},
    {0U, HID_KEY_ARROW_DOWN},
    {0U, HID_KEY_ARROW_RIGHT},
    {0U, HID_KEY_NONE},
    {0U, HID_KEY_NONE},
    {0U, HID_KEY_NONE},
    {0U, HID_KEY_NONE},
    {0U, HID_KEY_NONE},

    /* Direct active-low key on PB22. */
    {KEYBOARD_MODIFIER_RIGHTALT, 0U},
};

_Static_assert(KEY_COUNT <= KEYBOARD_ENGINE_MAX_KEYS,
               "Keyboard input count exceeds engine capacity");
_Static_assert((sizeof(bindings) / sizeof(bindings[0])) == KEY_COUNT,
               "Key binding count must match the matrix");

static keyboard_matrix_t matrix;

static bool scan_matrix(bool *pressed, size_t key_count, void *context)
{
    keyboard_matrix_t *keyboard_matrix = context;
    bool right_alt_level;

    if ((key_count != KEY_COUNT) ||
        (keyboard_matrix_key_count(keyboard_matrix) != MATRIX_KEY_COUNT) ||
        (keyboard_matrix_scan(keyboard_matrix, pressed, MATRIX_KEY_COUNT) !=
         CHIP_OK) ||
        (chip_gpio_read(RIGHT_ALT_PIN, &right_alt_level) != CHIP_OK)) {
        return false;
    }
    pressed[MATRIX_KEY_COUNT] = !right_alt_level;
    return true;
}

bool keyboard_layout_init(void)
{
    const keyboard_matrix_config_t config = {
        .row_pins = row_pins,
        .row_count = ROW_COUNT,
        .column_pins = column_pins,
        .column_count = COLUMN_COUNT,
        .active_level = true,
        .settle_time_us = 5U,
    };
    const chip_gpio_config_t right_alt_config = {
        .mode = CHIP_GPIO_INPUT,
        .pull = CHIP_GPIO_PULL_UP,
        .drive = CHIP_GPIO_DRIVE_LOW,
        .initial_level = true,
    };

    return (keyboard_matrix_init(&matrix, &config) == CHIP_OK) &&
           (chip_gpio_init(RIGHT_ALT_PIN, &right_alt_config) == CHIP_OK);
}

const keyboard_engine_config_t *keyboard_layout_config(void)
{
    static const keyboard_engine_config_t config = {
        .bindings = bindings,
        .key_count = KEY_COUNT,
        .debounce_scans = 5U,
        .scan_interval_ms = 1U,
        .scan = scan_matrix,
        .scan_context = &matrix,
    };

    return &config;
}

void keyboard_layout_set_leds(uint8_t leds)
{
    (void)leds;
}
