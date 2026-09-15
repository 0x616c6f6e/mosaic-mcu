#ifndef CHIP_API_POWER_H
#define CHIP_API_POWER_H

#include <stdbool.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Enable or disable the SoC DC-DC converter.
 * @param enabled true to enable DC-DC, false to disable it.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_power_set_dcdc(bool enabled);
/** @brief Enter the SoC idle state until an interrupt wakes it. */
void chip_power_idle(void);

#ifdef __cplusplus
}
#endif

#endif
