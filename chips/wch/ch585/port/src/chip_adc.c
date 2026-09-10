#include <chip_adc.h>

#include <stdbool.h>

#include "CH58x_common.h"
#include "ch585_internal.h"

static chip_adc_config_t adc_config;
static bool adc_initialized;

static chip_status_t adc_gain_to_vendor(chip_adc_gain_t gain, ADC_SignalPGATypeDef *vendor_gain)
{
    if (vendor_gain == NULL) {
        return CHIP_ERROR_INVALID_ARG;
    }

    switch (gain) {
    case CHIP_ADC_GAIN_QUARTER:
        *vendor_gain = ADC_PGA_1_4;
        break;
    case CHIP_ADC_GAIN_HALF:
        *vendor_gain = ADC_PGA_1_2;
        break;
    case CHIP_ADC_GAIN_ONE:
        *vendor_gain = ADC_PGA_0;
        break;
    case CHIP_ADC_GAIN_TWO:
        *vendor_gain = ADC_PGA_2;
        break;
    case CHIP_ADC_GAIN_FOUR:
        *vendor_gain = ADC_PGA_4;
        break;
    case CHIP_ADC_GAIN_EIGHT:
        *vendor_gain = ADC_PGA_8;
        break;
    case CHIP_ADC_GAIN_SIXTEEN:
        *vendor_gain = ADC_PGA_16;
        break;
    default:
        return CHIP_ERROR_INVALID_ARG;
    }
    return CHIP_OK;
}

chip_status_t chip_adc_init(const chip_adc_config_t *config)
{
    ADC_SignalPGATypeDef vendor_gain;
    chip_status_t status;

    if ((config == NULL) || (config->channel > CHIP_ADC_CHANNEL_TEMPERATURE)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    status = adc_gain_to_vendor(config->gain, &vendor_gain);
    if (status != CHIP_OK) {
        return status;
    }

    adc_config = *config;
    if (config->channel <= CHIP_ADC_CHANNEL_13) {
        ADC_ExtSingleChSampInit(SampleFreq_4_or_2, vendor_gain);
        ADC_ChannelCfg((ADC_SingleChannelTypeDef)config->channel);
    } else if (config->channel == CHIP_ADC_CHANNEL_BATTERY) {
        ADC_InterBATSampInit();
        adc_config.gain = CHIP_ADC_GAIN_QUARTER;
    } else {
        ADC_InterTSSampInit();
    }
    adc_initialized = true;
    return CHIP_OK;
}

chip_status_t chip_adc_deinit(void)
{
    if (!adc_initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    if (adc_config.channel == CHIP_ADC_CHANNEL_TEMPERATURE) {
        ADC_DisableTSPower();
    }
    R8_ADC_CFG = (uint8_t)(R8_ADC_CFG & (uint8_t)~RB_ADC_POWER_ON);
    adc_initialized = false;
    return CHIP_OK;
}

chip_status_t chip_adc_read_raw(uint16_t *value, uint32_t timeout_us)
{
    uint64_t started_at;
    chip_status_t status;

    if (value == NULL) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!adc_initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    status = ch585_timeout_start(timeout_us, &started_at);
    if (status != CHIP_OK) {
        return status;
    }

    ADC_StartUp();
    while ((R8_ADC_CONVERT & RB_ADC_START) != 0U) {
        if (ch585_timeout_expired(started_at, timeout_us)) {
            return CHIP_ERROR_TIMEOUT;
        }
    }
    while ((R8_ADC_CONVERT & RB_ADC_EOC_X) != 0U) {
        if (ch585_timeout_expired(started_at, timeout_us)) {
            return CHIP_ERROR_TIMEOUT;
        }
    }
    *value = (uint16_t)(ADC_ReadConverValue() & RB_ADC_DATA);
    return CHIP_OK;
}

chip_status_t chip_adc_raw_to_millivolts(uint16_t raw, int32_t *millivolts)
{
    if (millivolts == NULL) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!adc_initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    if (adc_config.channel == CHIP_ADC_CHANNEL_TEMPERATURE) {
        return CHIP_ERROR_UNSUPPORTED;
    }

    switch (adc_config.gain) {
    case CHIP_ADC_GAIN_QUARTER:
        *millivolts = ADC_VoltConverSignalPGA_MINUS_12dB(raw);
        break;
    case CHIP_ADC_GAIN_HALF:
        *millivolts = ADC_VoltConverSignalPGA_MINUS_6dB(raw);
        break;
    case CHIP_ADC_GAIN_ONE:
        *millivolts = ADC_VoltConverSignalPGA_0dB(raw);
        break;
    case CHIP_ADC_GAIN_TWO:
        *millivolts = ADC_VoltConverSignalPGA_6dB(raw);
        break;
    case CHIP_ADC_GAIN_FOUR:
        *millivolts = ADC_VoltConverSignalPGA_12dB(raw);
        break;
    case CHIP_ADC_GAIN_EIGHT:
        *millivolts = ADC_VoltConverSignalPGA_18dB(raw);
        break;
    case CHIP_ADC_GAIN_SIXTEEN:
        *millivolts = ADC_VoltConverSignalPGA_24dB(raw);
        break;
    default:
        return CHIP_ERROR_INVALID_ARG;
    }
    return CHIP_OK;
}

chip_status_t chip_adc_raw_to_celsius(uint16_t raw, int32_t *celsius)
{
    if (celsius == NULL) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!adc_initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    if (adc_config.channel != CHIP_ADC_CHANNEL_TEMPERATURE) {
        return CHIP_ERROR_UNSUPPORTED;
    }

    *celsius = adc_to_temperature_celsius(raw);
    return CHIP_OK;
}
