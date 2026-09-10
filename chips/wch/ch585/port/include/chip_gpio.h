#ifndef CHIP_API_GPIO_H
#define CHIP_API_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t chip_pin_t;

typedef enum {
    CHIP_GPIO_PORT_A = 0,
    CHIP_GPIO_PORT_B = 1,
} chip_gpio_port_t;

#define CHIP_PIN(port, index) \
    ((chip_pin_t)((((uint16_t)(port)) << 8U) | ((uint16_t)(index) & UINT16_C(0xFF))))
#define CHIP_PIN_PORT(pin)  ((chip_gpio_port_t)(((uint16_t)(pin) >> 8U) & UINT16_C(0xFF)))
#define CHIP_PIN_INDEX(pin) ((uint8_t)((uint16_t)(pin) & UINT16_C(0xFF)))

typedef enum {
    CHIP_GPIO_INPUT,
    CHIP_GPIO_OUTPUT_PUSH_PULL,
    CHIP_GPIO_ANALOG,
} chip_gpio_mode_t;

typedef enum {
    CHIP_GPIO_PULL_NONE,
    CHIP_GPIO_PULL_UP,
    CHIP_GPIO_PULL_DOWN,
} chip_gpio_pull_t;

typedef enum {
    CHIP_GPIO_DRIVE_LOW,
    CHIP_GPIO_DRIVE_HIGH,
} chip_gpio_drive_t;

typedef struct {
    chip_gpio_mode_t mode;
    chip_gpio_pull_t pull;
    chip_gpio_drive_t drive;
    bool initial_level;
} chip_gpio_config_t;

typedef enum {
    CHIP_GPIO_IRQ_LOW_LEVEL,
    CHIP_GPIO_IRQ_HIGH_LEVEL,
    CHIP_GPIO_IRQ_FALLING_EDGE,
    CHIP_GPIO_IRQ_RISING_EDGE,
} chip_gpio_irq_trigger_t;

/* Callbacks execute in interrupt context and must not block. */
typedef void (*chip_gpio_irq_callback_t)(chip_pin_t pin, void *context);

chip_status_t chip_gpio_init(chip_pin_t pin, const chip_gpio_config_t *config);
chip_status_t chip_gpio_deinit(chip_pin_t pin);
chip_status_t chip_gpio_read(chip_pin_t pin, bool *level);
chip_status_t chip_gpio_write(chip_pin_t pin, bool level);
chip_status_t chip_gpio_toggle(chip_pin_t pin);

chip_status_t chip_gpio_irq_attach(chip_pin_t pin,
                                   chip_gpio_irq_trigger_t trigger,
                                   chip_gpio_irq_callback_t callback,
                                   void *context);
chip_status_t chip_gpio_irq_detach(chip_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif
