#include <chip_time.h>

#include <limits.h>

#include <chip_system.h>

#include "CH58x_common.h"

static volatile uint32_t tick_overflows;
static bool time_initialized;

chip_status_t chip_time_init(void)
{
    uint32_t state = chip_system_critical_enter();

    PFIC_DisableIRQ(SysTick_IRQn);
    SysTick->CTLR = 0;
    SysTick->SR = 0;
    SysTick->CNTL = 0;
    SysTick->CMP = UINT32_MAX;
    tick_overflows = 0;
    PFIC_ClearPendingIRQ(SysTick_IRQn);
    SysTick->CTLR = SysTick_CTLR_STRE | SysTick_CTLR_STCLK |
                    SysTick_CTLR_STIE | SysTick_CTLR_STE;
    time_initialized = true;

    chip_system_critical_exit(state);
    PFIC_EnableIRQ(SysTick_IRQn);
    return CHIP_OK;
}

bool chip_time_is_initialized(void)
{
    return time_initialized;
}

uint64_t chip_time_ticks(void)
{
    uint32_t high;
    uint32_t low;
    uint32_t state;

    if (!time_initialized) {
        return 0;
    }

    state = chip_system_critical_enter();
    high = tick_overflows;
    low = SysTick->CNTL;
    if ((SysTick->SR & SysTick_SR_CNTIF) != 0U) {
        SysTick->SR = 0;
        high = ++tick_overflows;
        low = SysTick->CNTL;
    }
    chip_system_critical_exit(state);

    return ((uint64_t)high << 32U) | low;
}

static uint64_t ticks_to_units(uint64_t ticks, uint32_t units_per_second)
{
    uint32_t clock_hz = chip_system_clock_hz();

    if (clock_hz == 0U) {
        return 0;
    }

    return ((ticks / clock_hz) * units_per_second) +
           (((ticks % clock_hz) * units_per_second) / clock_hz);
}

uint64_t chip_time_micros(void)
{
    return ticks_to_units(chip_time_ticks(), UINT32_C(1000000));
}

uint32_t chip_time_millis(void)
{
    return (uint32_t)ticks_to_units(chip_time_ticks(), UINT32_C(1000));
}

void chip_delay_us(uint32_t delay_us)
{
    uint64_t start;

    if (!time_initialized || (delay_us == 0U)) {
        return;
    }

    start = chip_time_micros();
    while ((chip_time_micros() - start) < delay_us) {
    }
}

void chip_delay_ms(uint32_t delay_ms)
{
    while (delay_ms > 0U) {
        uint32_t chunk = (delay_ms > UINT32_C(4000)) ? UINT32_C(4000) : delay_ms;
        chip_delay_us(chunk * UINT32_C(1000));
        delay_ms -= chunk;
    }
}

__attribute__((weak))
void ch585_systick_hook(void)
{
}

__INTERRUPT
__HIGH_CODE
void SysTick_Handler(void)
{
    SysTick->SR = 0;
    if (time_initialized) {
        ++tick_overflows;
    }
    ch585_systick_hook();
}
