#ifndef CHIP_API_ADC_H
#define CHIP_API_ADC_H

#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief ADC external pins and internal sensing channels. */
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
    CHIP_ADC_CHANNEL_BATTERY,     /**< Internal battery voltage channel. */
    CHIP_ADC_CHANNEL_TEMPERATURE, /**< Internal temperature sensor. */
} chip_adc_channel_t;

/** @brief ADC analog gain selection. */
typedef enum {
    CHIP_ADC_GAIN_QUARTER,
    CHIP_ADC_GAIN_HALF,
    CHIP_ADC_GAIN_ONE,
    CHIP_ADC_GAIN_TWO,
    CHIP_ADC_GAIN_FOUR,
    CHIP_ADC_GAIN_EIGHT,
    CHIP_ADC_GAIN_SIXTEEN,
} chip_adc_gain_t;

/** @brief ADC input channel and gain. */
typedef struct {
    chip_adc_channel_t channel; /**< Input to sample. */
    chip_adc_gain_t gain;       /**< Analog gain applied to the input. */
} chip_adc_config_t;

/** @brief Initialize the ADC for one input channel.
 * @param config Channel and gain settings.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_adc_init(const chip_adc_config_t *config);
/** @brief Release the ADC.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_adc_deinit(void);
/** @brief Acquire one ADC conversion.
 * @param[out] value Receives the raw sample.
 * @param timeout_us Conversion timeout in microseconds.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_adc_read_raw(uint16_t *value, uint32_t timeout_us);
/** @brief Convert a raw ADC sample to millivolts.
 * @param raw Sample from chip_adc_read_raw().
 * @param[out] millivolts Receives the voltage in mV.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_adc_raw_to_millivolts(uint16_t raw, int32_t *millivolts);
/** @brief Convert a temperature-channel sample to degrees Celsius.
 * @param raw Sample from the temperature channel.
 * @param[out] celsius Receives the temperature in degrees Celsius.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_adc_raw_to_celsius(uint16_t raw, int32_t *celsius);

#ifdef __cplusplus
}
#endif

#endif
