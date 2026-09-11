#include "keyboard_matrix.h"

#include <chip_time.h>

static bool pins_are_unique(const keyboard_matrix_config_t *config)
{
    size_t first;
    size_t second;

    for (first = 0U; first < config->row_count; ++first) {
        for (second = first + 1U; second < config->row_count; ++second) {
            if (config->row_pins[first] == config->row_pins[second]) {
                return false;
            }
        }
        for (second = 0U; second < config->column_count; ++second) {
            if (config->row_pins[first] == config->column_pins[second]) {
                return false;
            }
        }
    }
    for (first = 0U; first < config->column_count; ++first) {
        for (second = first + 1U; second < config->column_count; ++second) {
            if (config->column_pins[first] == config->column_pins[second]) {
                return false;
            }
        }
    }
    return true;
}

static chip_status_t set_column_inactive(chip_pin_t pin)
{
    const chip_gpio_config_t config = {
        .mode = CHIP_GPIO_INPUT,
        .pull = CHIP_GPIO_PULL_NONE,
        .drive = CHIP_GPIO_DRIVE_LOW,
        .initial_level = false,
    };

    return chip_gpio_init(pin, &config);
}

static chip_status_t set_column_active(chip_pin_t pin, bool active_level)
{
    const chip_gpio_config_t config = {
        .mode = CHIP_GPIO_OUTPUT_PUSH_PULL,
        .pull = CHIP_GPIO_PULL_NONE,
        .drive = CHIP_GPIO_DRIVE_LOW,
        .initial_level = active_level,
    };

    return chip_gpio_init(pin, &config);
}

chip_status_t keyboard_matrix_init(
    keyboard_matrix_t *matrix, const keyboard_matrix_config_t *config)
{
    chip_gpio_config_t row_config;
    size_t index;

    if ((matrix == NULL) || (config == NULL) ||
        (config->row_pins == NULL) || (config->column_pins == NULL) ||
        (config->row_count == 0U) || (config->column_count == 0U) ||
        (config->row_count > (SIZE_MAX / config->column_count)) ||
        !pins_are_unique(config)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    row_config.mode = CHIP_GPIO_INPUT;
    row_config.pull = config->active_level ? CHIP_GPIO_PULL_DOWN :
                                             CHIP_GPIO_PULL_UP;
    row_config.drive = CHIP_GPIO_DRIVE_LOW;
    row_config.initial_level = !config->active_level;
    for (index = 0U; index < config->row_count; ++index) {
        if (chip_gpio_init(config->row_pins[index], &row_config) != CHIP_OK) {
            return CHIP_ERROR_IO;
        }
    }
    for (index = 0U; index < config->column_count; ++index) {
        if (set_column_inactive(config->column_pins[index]) != CHIP_OK) {
            return CHIP_ERROR_IO;
        }
    }
    matrix->config = *config;
    matrix->initialized = true;
    return CHIP_OK;
}

chip_status_t keyboard_matrix_scan(keyboard_matrix_t *matrix, bool *pressed,
                                   size_t capacity)
{
    size_t key_count = keyboard_matrix_key_count(matrix);
    size_t column;

    if ((matrix == NULL) || !matrix->initialized || (pressed == NULL) ||
        (capacity < key_count)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    for (column = 0U; column < matrix->config.column_count; ++column) {
        chip_pin_t column_pin = matrix->config.column_pins[column];
        size_t row;

        if (set_column_active(column_pin, matrix->config.active_level) !=
            CHIP_OK) {
            return CHIP_ERROR_IO;
        }
        chip_delay_us(matrix->config.settle_time_us);
        for (row = 0U; row < matrix->config.row_count; ++row) {
            bool level;

            if (chip_gpio_read(matrix->config.row_pins[row], &level) !=
                CHIP_OK) {
                (void)set_column_inactive(column_pin);
                return CHIP_ERROR_IO;
            }
            pressed[(row * matrix->config.column_count) + column] =
                (level == matrix->config.active_level);
        }
        if (set_column_inactive(column_pin) != CHIP_OK) {
            return CHIP_ERROR_IO;
        }
    }
    return CHIP_OK;
}

size_t keyboard_matrix_key_count(const keyboard_matrix_t *matrix)
{
    if ((matrix == NULL) || !matrix->initialized) {
        return 0U;
    }
    return matrix->config.row_count * matrix->config.column_count;
}
