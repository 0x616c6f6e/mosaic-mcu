#include <chip_system.h>

#include <string.h>

#include <chip_time.h>

#include "CH58x_common.h"

static chip_clock_source_t current_clock_source = CHIP_CLOCK_INTERNAL;

static chip_status_t ch585_hse_load_capacitance(uint8_t load_pf)
{
    HSECapTypeDef setting;

    switch (load_pf) {
    case 0U:
        return CHIP_OK;
    case 2U: setting = HSECap_2p; break;
    case 4U: setting = HSECap_4p; break;
    case 6U: setting = HSECap_6p; break;
    case 8U: setting = HSECap_8p; break;
    case 10U: setting = HSECap_10p; break;
    case 12U: setting = HSECap_12p; break;
    case 14U: setting = HSECap_14p; break;
    case 16U: setting = HSECap_16p; break;
    case 18U: setting = HSECap_18p; break;
    case 20U: setting = HSECap_20p; break;
    case 22U: setting = HSECap_22p; break;
    case 24U: setting = HSECap_24p; break;
    default:
        return CHIP_ERROR_INVALID_ARG;
    }
    HSECFG_Capacitance(setting);
    return CHIP_OK;
}

static chip_status_t ch585_clock_setting(const chip_system_config_t *config,
                                         SYS_CLKTypeDef *setting)
{
    if ((config == NULL) || (setting == NULL)) {
        return CHIP_ERROR_INVALID_ARG;
    }

#define MATCH_CLOCK(source_value, hz_value, setting_value) \
    if ((config->source == (source_value)) && \
        (config->core_clock_hz == (hz_value))) { \
        *setting = (setting_value); \
        return CHIP_OK; \
    }

    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(32000), CLK_SOURCE_32KHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(1000000), CLK_SOURCE_HSI_1MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(2000000), CLK_SOURCE_HSI_2MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(4000000), CLK_SOURCE_HSI_4MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(5333333), CLK_SOURCE_HSI_5_3MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(8000000), CLK_SOURCE_HSI_8MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(16000000), CLK_SOURCE_HSI_16MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(13000000), CLK_SOURCE_HSI_PLL_13MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(19500000), CLK_SOURCE_HSI_PLL_19_5MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(24000000), CLK_SOURCE_HSI_PLL_24MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(26000000), CLK_SOURCE_HSI_PLL_26MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(39000000), CLK_SOURCE_HSI_PLL_39MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(52000000), CLK_SOURCE_HSI_PLL_52MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(62400000), CLK_SOURCE_HSI_PLL_62_4MHz)
    MATCH_CLOCK(CHIP_CLOCK_INTERNAL, UINT32_C(78000000), CLK_SOURCE_HSI_PLL_78MHz)

    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(2000000), CLK_SOURCE_HSE_2MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(4000000), CLK_SOURCE_HSE_4MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(6400000), CLK_SOURCE_HSE_6_4MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(8000000), CLK_SOURCE_HSE_8MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(16000000), CLK_SOURCE_HSE_16MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(32000000), CLK_SOURCE_HSE_32MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(13000000), CLK_SOURCE_HSE_PLL_13MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(19500000), CLK_SOURCE_HSE_PLL_19_5MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(24000000), CLK_SOURCE_HSE_PLL_24MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(26000000), CLK_SOURCE_HSE_PLL_26MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(39000000), CLK_SOURCE_HSE_PLL_39MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(52000000), CLK_SOURCE_HSE_PLL_52MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(62400000), CLK_SOURCE_HSE_PLL_62_4MHz)
    MATCH_CLOCK(CHIP_CLOCK_EXTERNAL, UINT32_C(78000000), CLK_SOURCE_HSE_PLL_78MHz)

#undef MATCH_CLOCK
    return CHIP_ERROR_UNSUPPORTED;
}

chip_status_t chip_system_init(const chip_system_config_t *config)
{
    SYS_CLKTypeDef setting;
    chip_status_t status = ch585_clock_setting(config, &setting);

    if (status != CHIP_OK) {
        return status;
    }
    if (chip_time_is_initialized()) {
        return CHIP_ERROR_BUSY;
    }

    if (config->source == CHIP_CLOCK_EXTERNAL) {
        status = ch585_hse_load_capacitance(config->external_crystal_load_pf);
        if (status != CHIP_OK) {
            return status;
        }
    } else if (config->external_crystal_load_pf != 0U) {
        return CHIP_ERROR_INVALID_ARG;
    }

    SetSysClock(setting);
    if (GetSysClock() != config->core_clock_hz) {
        return CHIP_ERROR_IO;
    }
    current_clock_source = config->source;
    return CHIP_OK;
}

uint32_t chip_system_clock_hz(void)
{
    return GetSysClock();
}

chip_clock_source_t chip_system_clock_source(void)
{
    return current_clock_source;
}

uint8_t chip_system_chip_id(void)
{
    return SYS_GetChipID();
}

chip_status_t chip_system_unique_id(uint8_t *buffer, size_t size)
{
    uint32_t aligned_id[CHIP_UNIQUE_ID_SIZE / sizeof(uint32_t)];

    if ((buffer == NULL) || (size < CHIP_UNIQUE_ID_SIZE)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    GET_UNIQUE_ID((uint8_t *)aligned_id);
    memcpy(buffer, aligned_id, CHIP_UNIQUE_ID_SIZE);
    return CHIP_OK;
}

uint32_t chip_system_critical_enter(void)
{
    uint32_t state;

    SYS_DisableAllIrq(&state);
    return state;
}

void chip_system_critical_exit(uint32_t state)
{
    SYS_RecoverIrq(state);
}

void chip_system_reset(void)
{
    SYS_ResetExecute();
    for (;;) {
    }
}

void chip_system_jump(uint32_t address)
{
    uint32_t interrupt_state;
    void (*entry)(void) = (void (*)(void))(uintptr_t)address;

    SYS_DisableAllIrq(&interrupt_state);
    (void)interrupt_state;
    __asm volatile("fence.i" ::: "memory");
    entry();
    for (;;) {
    }
}
