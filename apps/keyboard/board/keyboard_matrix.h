#ifndef CH585_KEYBOARD_MATRIX_H
#define CH585_KEYBOARD_MATRIX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <chip_gpio.h>
#include <chip_status.h>

/** @brief Row-driven matrix wiring, active polarity, and scan settle time. */
typedef struct {
    const chip_pin_t *row_pins;    /**< Rows driven as digital outputs. */
    size_t row_count;              /**< Number of row pins. */
    const chip_pin_t *column_pins; /**< Columns sampled as biased inputs. */
    size_t column_count;           /**< Number of column pins. */
    bool active_level;             /**< Driven row level representing a key. */
    uint32_t settle_time_us;       /**< Delay after activating each row in us. */
} keyboard_matrix_config_t;

/** @brief Matrix scanner initialized with persistent pin arrays. */
typedef struct {
    keyboard_matrix_config_t config; /**< Shallow copy; pin arrays stay owned by caller. */
    bool initialized;                /**< true after GPIO setup succeeds. */
} keyboard_matrix_t;

/** @brief Configure rows as outputs and columns as biased inputs.
 * @note Pin arrays must remain valid for the matrix lifetime.
 * @param[out] matrix Matrix scanner to initialize.
 * @param config Pin arrays, active polarity, and settle time in microseconds.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t keyboard_matrix_init(
    keyboard_matrix_t *matrix, const keyboard_matrix_config_t *config);
/** @brief Sample keys in row-major order, driving one row at a time.
 * @param matrix Initialized matrix scanner.
 * @param[out] pressed Receives row_count * column_count key states.
 * @param capacity Number of bool entries available in pressed.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t keyboard_matrix_scan(keyboard_matrix_t *matrix, bool *pressed,
                                   size_t capacity);
/** @brief Query the number of matrix key positions.
 * @param matrix Initialized matrix scanner.
 * @return Row count multiplied by column count, or zero if unavailable.
 */
size_t keyboard_matrix_key_count(const keyboard_matrix_t *matrix);

#endif
