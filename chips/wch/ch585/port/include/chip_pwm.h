#ifndef CHIP_API_PWM_H
#define CHIP_API_PWM_H

#include <stdbool.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHIP_PWM_0 = 0,
    CHIP_PWM_1,
    CHIP_PWM_2,
    CHIP_PWM_3,
    CHIP_PWM_4,
    CHIP_PWM_5,
    CHIP_PWM_6,
    CHIP_PWM_7,
    CHIP_PWM_COUNT,
} chip_pwm_channel_t;

typedef struct {
    uint32_t frequency_hz;
    bool active_high;
} chip_pwm_config_t;

chip_status_t chip_pwm_init(chip_pwm_channel_t channel,
                            const chip_pwm_config_t *config);
chip_status_t chip_pwm_set_duty(chip_pwm_channel_t channel, uint16_t duty_permyriad);
chip_status_t chip_pwm_deinit(chip_pwm_channel_t channel);

#ifdef __cplusplus
}
#endif

#endif
