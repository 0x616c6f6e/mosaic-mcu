#include "CH58x_common.h"

#include <stdbool.h>

#include <chip_system.h>
#include <chip_time.h>

volatile uint8_t fake_usb_ctrl;
volatile uint8_t fake_udev_ctrl;
volatile uint8_t fake_usb_int_en;
volatile uint8_t fake_usb_dev_ad;
volatile uint8_t fake_usb_mis_st;
volatile uint8_t fake_usb_int_fg;
volatile uint8_t fake_usb_int_st;
volatile uint8_t fake_usb_rx_len;
volatile uint8_t fake_uep4_1_mod;
volatile uint8_t fake_uep2_3_mod;
volatile uint8_t fake_uep567_mod;
volatile uint32_t fake_uep_dma[8];
volatile uint8_t fake_uep_tx_len[8];
volatile uint8_t fake_uep_ctrl[8];
volatile uint16_t fake_pin_config;
volatile uint8_t fake_irq_enabled;
volatile uint16_t fake_power_clock_mask;
volatile uint32_t fake_delay_ms;
volatile uint8_t fake_time_initialized = 1;

void PFIC_ClearPendingIRQ(IRQn_Type irq)
{
    (void)irq;
}

void PFIC_EnableIRQ(IRQn_Type irq)
{
    (void)irq;
    fake_irq_enabled = 1;
}

void PFIC_DisableIRQ(IRQn_Type irq)
{
    (void)irq;
    fake_irq_enabled = 0;
}

void PWR_PeriphClkCfg(FunctionalState state, uint16_t peripheral)
{
    if (state == ENABLE) {
        fake_power_clock_mask |= peripheral;
    } else {
        fake_power_clock_mask &= (uint16_t)~peripheral;
    }
}

uint32_t chip_system_critical_enter(void)
{
    return 0;
}

void chip_system_critical_exit(uint32_t state)
{
    (void)state;
}

bool chip_time_is_initialized(void)
{
    return fake_time_initialized != 0U;
}

void chip_delay_ms(uint32_t delay_ms)
{
    fake_delay_ms += delay_ms;
}
