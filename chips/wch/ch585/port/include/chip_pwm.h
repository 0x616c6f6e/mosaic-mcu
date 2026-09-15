#ifndef CHIP_API_PWM_H
#define CHIP_API_PWM_H

#include <stdbool.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Available PWM output channels. */
typedef enum {
    CHIP_PWM_0 = 0,
    CHIP_PWM_1,
    CHIP_PWM_2,
    CHIP_PWM_3,
    CHIP_PWM_4,
    CHIP_PWM_5,
    CHIP_PWM_6,
    CHIP_PWM_7,
    CHIP_PWM_COUNT, /**< Number of PWM channels. */
} chip_pwm_channel_t;

/** @brief PWM frequency in hertz and output polarity. */
typedef struct {
    uint32_t frequency_hz; /**< Output frequency in Hz. */
    bool active_high;      /**< true for active-high pulses. */
} chip_pwm_config_t;

/** @brief Initialize a PWM channel.
 * @param channel Output channel.
 * @param config Frequency and active polarity.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_pwm_init(chip_pwm_channel_t channel,
                            const chip_pwm_config_t *config);
/** @brief Set PWM duty cycle in ten-thousandths of a period.
 * @param channel Output channel.
 * @param duty_permyriad Duty from 0 to 10000 (100%).
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_pwm_set_duty(chip_pwm_channel_t channel, uint16_t duty_permyriad);
/** @brief Stop and release a PWM channel.
 * @param channel Output channel.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_pwm_deinit(chip_pwm_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif
