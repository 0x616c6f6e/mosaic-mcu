#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <chip_gpio.h>
#include <keyboard_matrix.h>

static const chip_pin_t rows[] = {
    CHIP_PIN(CHIP_GPIO_PORT_A, 4),
    CHIP_PIN(CHIP_GPIO_PORT_A, 5),
};
static const chip_pin_t columns[] = {
    CHIP_PIN(CHIP_GPIO_PORT_B, 0),
    CHIP_PIN(CHIP_GPIO_PORT_B, 1),
    CHIP_PIN(CHIP_GPIO_PORT_B, 2),
};
static const bool physical_keys[] = {
    true, false, true,
    false, true, false,
};
static int active_row = -1;
static bool rows_active[2];
static bool columns_discharged[3];
static unsigned int startup_delay_count;
static unsigned int scan_delay_count;
static unsigned int row_init_count;
static unsigned int row_write_count;
static unsigned int column_discharge_count;
static unsigned int column_input_count;
static unsigned int column_read_count;
static unsigned int fail_on_column_read;

static int find_pin(const chip_pin_t *pins, size_t count, chip_pin_t pin)
{
    size_t index;

    for (index = 0U; index < count; ++index) {
        if (pins[index] == pin) {
            return (int)index;
        }
    }
    return -1;
}

chip_status_t chip_gpio_init(chip_pin_t pin,
                            const chip_gpio_config_t *config)
{
    int row = find_pin(rows, sizeof(rows) / sizeof(rows[0]), pin);
    int column = find_pin(columns, sizeof(columns) / sizeof(columns[0]), pin);

    assert(config != NULL);
    if (row >= 0) {
        assert(config->mode == CHIP_GPIO_OUTPUT_PUSH_PULL);
        assert(!config->initial_level);
        rows_active[row] = false;
        ++row_init_count;
        return CHIP_OK;
    }
    assert(column >= 0);
    if (config->mode == CHIP_GPIO_OUTPUT_PUSH_PULL) {
        assert(!config->initial_level);
        columns_discharged[column] = true;
        ++column_discharge_count;
    } else {
        assert(config->mode == CHIP_GPIO_INPUT);
        assert(config->pull == CHIP_GPIO_PULL_DOWN);
        assert(columns_discharged[column]);
        ++column_input_count;
    }
    return CHIP_OK;
}

chip_status_t chip_gpio_write(chip_pin_t pin, bool level)
{
    int row = find_pin(rows, sizeof(rows) / sizeof(rows[0]), pin);

    assert(row >= 0);
    if (level) {
        assert(active_row == -1);
        active_row = row;
        rows_active[row] = true;
    } else {
        assert(active_row == row);
        active_row = -1;
        rows_active[row] = false;
    }
    ++row_write_count;
    return CHIP_OK;
}

chip_status_t chip_gpio_read(chip_pin_t pin, bool *level)
{
    int column = find_pin(columns, sizeof(columns) / sizeof(columns[0]), pin);

    assert(column >= 0);
    assert(active_row >= 0);
    assert(level != NULL);
    ++column_read_count;
    if (column_read_count == fail_on_column_read) {
        return CHIP_ERROR_IO;
    }
    *level = physical_keys[((size_t)active_row * 3U) + (size_t)column];
    return CHIP_OK;
}

void chip_delay_us(uint32_t delay_us)
{
    if (delay_us == UINT32_C(10000)) {
        ++startup_delay_count;
    } else {
        assert(delay_us == 5U);
        ++scan_delay_count;
    }
}

int main(void)
{
    const keyboard_matrix_config_t config = {
        .row_pins = rows,
        .row_count = sizeof(rows) / sizeof(rows[0]),
        .column_pins = columns,
        .column_count = sizeof(columns) / sizeof(columns[0]),
        .active_level = true,
        .settle_time_us = 5U,
    };
    keyboard_matrix_t matrix = {0};
    bool pressed[sizeof(physical_keys) / sizeof(physical_keys[0])] = {false};
    size_t index;

    assert(keyboard_matrix_init(&matrix, &config) == CHIP_OK);
    assert(keyboard_matrix_key_count(&matrix) ==
           (sizeof(physical_keys) / sizeof(physical_keys[0])));
    assert(keyboard_matrix_scan(&matrix, pressed,
                                sizeof(pressed) / sizeof(pressed[0])) ==
           CHIP_OK);
    assert(memcmp(pressed, physical_keys, sizeof(pressed)) == 0);
    assert(startup_delay_count == 2U);
    assert(scan_delay_count == 2U);
    assert(row_init_count == 2U);
    assert(row_write_count == 4U);
    assert(column_discharge_count == 3U);
    assert(column_input_count == 3U);
    assert(active_row == -1);
    for (index = 0U; index < (sizeof(rows) / sizeof(rows[0])); ++index) {
        assert(!rows_active[index]);
    }

    fail_on_column_read = column_read_count + 1U;
    assert(keyboard_matrix_scan(&matrix, pressed,
                                sizeof(pressed) / sizeof(pressed[0])) ==
           CHIP_ERROR_IO);
    assert(active_row == -1);
    assert(!rows_active[0]);
    assert(row_write_count == 6U);

    {
        const chip_pin_t duplicate_columns[] = {rows[0]};
        keyboard_matrix_config_t invalid = config;

        invalid.column_pins = duplicate_columns;
        invalid.column_count = 1U;
        assert(keyboard_matrix_init(&matrix, &invalid) ==
               CHIP_ERROR_INVALID_ARG);
    }
    return 0;
}
