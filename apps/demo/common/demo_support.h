#ifndef CH585_DEMO_SUPPORT_H
#define CH585_DEMO_SUPPORT_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>
#include <platform_log.h>

#include "demo_board.h"

/** @brief Initialize the demo board clock and time service.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t demo_platform_init(void);
/** @brief Configure the demo status LED GPIO.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t demo_led_init(void);
/** @brief Set the demo status LED state.
 * @param on true to illuminate the LED.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t demo_led_write(bool on);
/** @brief Initialize the demo UART pins and controller.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t demo_uart_init(void);
/** @brief Configure the demo logging backend.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t demo_log_init(void);
/** @brief Configure logging with a custom timestamp callback.
 * @param timestamp Timestamp source.
 * @param context Opaque callback context.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t demo_log_init_with_timestamp(platform_log_timestamp_t timestamp,
                                           void *context);
/** @brief Configure demo I2C signal pins.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t demo_i2c_pins_init(void);
/** @brief Configure demo SPI NOR signal pins.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t demo_spi_flash_pins_init(void);

/** @brief Stop the demo after an unrecoverable error; does not return. */
void demo_halt(void);

/** @brief Stop the demo if a Chip API operation fails.
 * @param expression Expression returning chip_status_t.
 */
#define DEMO_REQUIRE(expression) \
    do { \
        if ((expression) != CHIP_OK) { \
            demo_halt(); \
        } \
    } while (0)

#endif
