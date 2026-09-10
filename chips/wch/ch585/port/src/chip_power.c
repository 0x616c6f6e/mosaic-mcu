#include <chip_power.h>

#include "CH58x_common.h"

chip_status_t chip_power_set_dcdc(bool enabled)
{
    PWR_DCDCCfg(enabled ? ENABLE : DISABLE);
    return CHIP_OK;
}

void chip_power_idle(void)
{
    LowPower_Idle();
}
