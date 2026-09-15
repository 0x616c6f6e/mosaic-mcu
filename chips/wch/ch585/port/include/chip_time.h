#ifndef CHIP_API_TIME_H
#define CHIP_API_TIME_H

#include <stdbool.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Start the time service.
 * @note This service owns the SoC SysTick interrupt.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_time_init(void);
/** @brief Check whether the time service is ready.
 * @return true when initialized.
 */
bool chip_time_is_initialized(void);
/** @brief Read the monotonic tick count.
 * @return Number of ticks since time initialization.
 */
uint64_t chip_time_ticks(void);
/** @brief Read monotonic time in microseconds.
 * @return Elapsed microseconds since time initialization.
 */
uint64_t chip_time_micros(void);
/** @brief Read monotonic time in milliseconds.
 * @return Elapsed milliseconds (wraps at 32 bits).
 */
uint32_t chip_time_millis(void);
/** @brief Busy-wait for a number of microseconds.
 * @note Has no effect before chip_time_init().
 * @param delay_us Delay in microseconds.
 */
void chip_delay_us(uint32_t delay_us);
/** @brief Busy-wait for a number of milliseconds.
 * @note Has no effect before chip_time_init().
 * @param delay_ms Delay in milliseconds.
 */
void chip_delay_ms(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif
