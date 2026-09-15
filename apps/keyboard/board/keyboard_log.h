#ifndef CH585_KEYBOARD_LOG_H
#define CH585_KEYBOARD_LOG_H

#include <chip_status.h>

/** @brief Initialize the board UART logging backend.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t keyboard_log_init(void);
/** @brief Flush pending board log output.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t keyboard_log_flush(void);

#endif
