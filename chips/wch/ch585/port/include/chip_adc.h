#ifndef CHIP_API_ADC_H
#define CHIP_API_ADC_H

#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHIP_ADC_CHANNEL_0 = 0,
    CHIP_ADC_CHANNEL_1,
    CHIP_ADC_CHANNEL_2,
    CHIP_ADC_CHANNEL_3,
    CHIP_ADC_CHANNEL_4,
    CHIP_ADC_CHANNEL_5,
    CHIP_ADC_CHANNEL_6,
    CHIP_ADC_CHANNEL_7,
    CHIP_ADC_CHANNEL_8,
    CHIP_ADC_CHANNEL_9,
    CHIP_ADC_CHANNEL_10,
    CHIP_ADC_CHANNEL_11,
    CHIP_ADC_CHANNEL_12,
    CHIP_ADC_CHANNEL_13,
    CHIP_ADC_CHANNEL_BATTERY,
    CHIP_ADC_CHANNEL_TEMPERATURE,
} chip_adc_channel_t;

typedef enum {
    CHIP_ADC_GAIN_QUARTER,
    CHIP_ADC_GAIN_HALF,
    CHIP_ADC_GAIN_ONE,
    CHIP_ADC_GAIN_TWO,
    CHIP_ADC_GAIN_FOUR,
    CHIP_ADC_GAIN_EIGHT,
    CHIP_ADC_GAIN_SIXTEEN,
} chip_adc_gain_t;

typedef struct {
    chip_adc_channel_t channel;
    chip_adc_gain_t gain;
} chip_adc_config_t;

chip_status_t chip_adc_init(const chip_adc_config_t *config);
chip_status_t chip_adc_deinit(void);
chip_status_t chip_adc_read_raw(uint16_t *value, uint32_t timeout_us);
chip_status_t chip_adc_raw_to_millivolts(uint16_t raw, int32_t *millivolts);
chip_status_t chip_adc_raw_to_celsius(uint16_t raw, int32_t *celsius);

#ifdef __cplusplus
}
#endif

#endif
