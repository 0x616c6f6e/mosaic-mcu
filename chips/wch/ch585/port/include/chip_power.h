#ifndef CHIP_API_POWER_H
#define CHIP_API_POWER_H

#include <stdbool.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

chip_status_t chip_power_set_dcdc(bool enabled);
void chip_power_idle(void);

#ifdef __cplusplus
}
#endif

#endif
