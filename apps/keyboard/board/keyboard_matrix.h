#ifndef CH585_KEYBOARD_MATRIX_H
#define CH585_KEYBOARD_MATRIX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <chip_gpio.h>
#include <chip_status.h>

typedef struct {
    const chip_pin_t *row_pins;
    size_t row_count;
    const chip_pin_t *column_pins;
    size_t column_count;
    bool active_level;
    uint32_t settle_time_us;
} keyboard_matrix_config_t;

typedef struct {
    keyboard_matrix_config_t config;
    bool initialized;
} keyboard_matrix_t;

chip_status_t keyboard_matrix_init(
    keyboard_matrix_t *matrix, const keyboard_matrix_config_t *config);
chip_status_t keyboard_matrix_scan(keyboard_matrix_t *matrix, bool *pressed,
                                   size_t capacity);
size_t keyboard_matrix_key_count(const keyboard_matrix_t *matrix);

#endif
