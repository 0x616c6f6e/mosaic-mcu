#include <chip_pwm.h>

#include <stddef.h>

#include <chip_system.h>

#include "CH58x_common.h"

#define CH585_PWM_PERIOD_COUNTS 255U

static uint8_t active_channels;
static uint32_t configured_frequency;
static bool active_high[CHIP_PWM_COUNT];
static uint16_t duty_values[CHIP_PWM_COUNT];

static bool pwm_channel_valid(chip_pwm_channel_t channel)
{
    return (channel >= CHIP_PWM_0) && (channel < CHIP_PWM_COUNT);
}

static uint8_t pwm_channel_mask(chip_pwm_channel_t channel)
{
    return (uint8_t)(UINT8_C(1) << (uint8_t)channel);
}

static void pwm_apply(chip_pwm_channel_t channel, FunctionalState state)
{
    uint32_t scaled = ((uint32_t)duty_values[channel] * CH585_PWM_PERIOD_COUNTS + 5000U) /
                      UINT32_C(10000);
    PWMX_ACTOUT(pwm_channel_mask(channel), (uint8_t)scaled,
                active_high[channel] ? High_Level : Low_Level, state);
}

chip_status_t chip_pwm_init(chip_pwm_channel_t channel,
                            const chip_pwm_config_t *config)
{
    uint32_t system_clock;
    uint64_t denominator;
    uint32_t divider;
    uint8_t mask;

    if (!pwm_channel_valid(channel) || (config == NULL) || (config->frequency_hz == 0U)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if ((active_channels != 0U) && (configured_frequency != config->frequency_hz)) {
        return CHIP_ERROR_BUSY;
    }

    system_clock = chip_system_clock_hz();
    denominator = (uint64_t)config->frequency_hz * CH585_PWM_PERIOD_COUNTS;
    if (denominator > system_clock) {
        return CHIP_ERROR_UNSUPPORTED;
    }
    divider = (uint32_t)(((uint64_t)system_clock + denominator - 1U) / denominator);
    if (divider == 0U) {
        divider = 1U;
    }
    if (divider > UINT8_MAX) {
        return CHIP_ERROR_UNSUPPORTED;
    }

    PWMX_CLKCfg((uint8_t)divider);
    PWMX_CycleCfg(PWMX_Cycle_255);
    configured_frequency = config->frequency_hz;
    active_high[channel] = config->active_high;
    duty_values[channel] = 0;
    mask = pwm_channel_mask(channel);
    active_channels |= mask;
    pwm_apply(channel, ENABLE);
    return CHIP_OK;
}

chip_status_t chip_pwm_set_duty(chip_pwm_channel_t channel, uint16_t duty_permyriad)
{
    if (!pwm_channel_valid(channel) || (duty_permyriad > UINT16_C(10000))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if ((active_channels & pwm_channel_mask(channel)) == 0U) {
        return CHIP_ERROR_NOT_READY;
    }

    duty_values[channel] = duty_permyriad;
    pwm_apply(channel, ENABLE);
    return CHIP_OK;
}

chip_status_t chip_pwm_deinit(chip_pwm_channel_t channel)
{
    uint8_t mask;

    if (!pwm_channel_valid(channel)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    mask = pwm_channel_mask(channel);
    if ((active_channels & mask) == 0U) {
        return CHIP_ERROR_NOT_READY;
    }

    pwm_apply(channel, DISABLE);
    active_channels &= (uint8_t)~mask;
    if (active_channels == 0U) {
        configured_frequency = 0;
    }
    return CHIP_OK;
}
