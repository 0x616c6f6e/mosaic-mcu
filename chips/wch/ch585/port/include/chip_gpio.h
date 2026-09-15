#ifndef CHIP_API_GPIO_H
#define CHIP_API_GPIO_H

#include <stdbool.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Encoded GPIO port and pin index. */
typedef uint16_t chip_pin_t;

/** @brief GPIO port encoded in the high byte of chip_pin_t. */
typedef enum {
    CHIP_GPIO_PORT_A = 0, /**< Port A. */
    CHIP_GPIO_PORT_B = 1, /**< Port B. */
} chip_gpio_port_t;

/** @brief Encode a port and index as a chip_pin_t. */
#define CHIP_PIN(port, index) \
    ((chip_pin_t)((((uint16_t)(port)) << 8U) | ((uint16_t)(index) & UINT16_C(0xFF))))
/** @brief Decode the port from a chip_pin_t. */
#define CHIP_PIN_PORT(pin)  ((chip_gpio_port_t)(((uint16_t)(pin) >> 8U) & UINT16_C(0xFF)))
/** @brief Decode the index within a port from a chip_pin_t. */
#define CHIP_PIN_INDEX(pin) ((uint8_t)((uint16_t)(pin) & UINT16_C(0xFF)))

/** @brief GPIO direction and analog input modes. */
typedef enum {
    CHIP_GPIO_INPUT,            /**< Digital input. */
    CHIP_GPIO_OUTPUT_PUSH_PULL, /**< Actively driven digital output. */
    CHIP_GPIO_ANALOG,           /**< Analog input; digital path disabled. */
} chip_gpio_mode_t;

/** @brief Bias applied to a digital GPIO input. */
typedef enum {
    CHIP_GPIO_PULL_NONE, /**< No internal bias. */
    CHIP_GPIO_PULL_UP,   /**< Internal pull-up. */
    CHIP_GPIO_PULL_DOWN, /**< Internal pull-down. */
} chip_gpio_pull_t;

/** @brief Output driver strength. */
typedef enum {
    CHIP_GPIO_DRIVE_LOW,  /**< Lower drive strength. */
    CHIP_GPIO_DRIVE_HIGH, /**< Higher drive strength. */
} chip_gpio_drive_t;

/** @brief GPIO direction, bias, drive strength, and initial output level. */
typedef struct {
    chip_gpio_mode_t mode;   /**< Direction and digital/analog mode. */
    chip_gpio_pull_t pull;   /**< Input bias. */
    chip_gpio_drive_t drive; /**< Output drive strength. */
    bool initial_level;      /**< Output level applied during init. */
} chip_gpio_config_t;

/** @brief Interrupt level and edge trigger choices. */
typedef enum {
    CHIP_GPIO_IRQ_LOW_LEVEL,    /**< Remains asserted while low. */
    CHIP_GPIO_IRQ_HIGH_LEVEL,   /**< Remains asserted while high. */
    CHIP_GPIO_IRQ_FALLING_EDGE, /**< High-to-low transition. */
    CHIP_GPIO_IRQ_RISING_EDGE,  /**< Low-to-high transition. */
} chip_gpio_irq_trigger_t;

/** @brief GPIO interrupt handler; runs in interrupt context and must not block. */
typedef void (*chip_gpio_irq_callback_t)(chip_pin_t pin, void *context);

/** @brief Configure a GPIO pin.
 * @param pin Encoded port and pin index.
 * @param config Pin mode and electrical configuration.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_gpio_init(chip_pin_t pin, const chip_gpio_config_t *config);
/** @brief Release a configured GPIO pin.
 * @param pin Pin to release.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_gpio_deinit(chip_pin_t pin);
/** @brief Sample a GPIO input.
 * @param pin Pin to sample.
 * @param[out] level Receives the logic level.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_gpio_read(chip_pin_t pin, bool *level);
/** @brief Set the output level of a GPIO pin.
 * @param pin Output pin.
 * @param level true for high, false for low.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_gpio_write(chip_pin_t pin, bool level);
/** @brief Invert the output level of a GPIO pin.
 * @param pin Output pin.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_gpio_toggle(chip_pin_t pin);

/** @brief Attach an interrupt callback to a GPIO pin.
 * @param pin Pin to monitor.
 * @param trigger Level or edge that fires the callback.
 * @param callback Nonblocking handler called in interrupt context.
 * @param context Opaque value passed to the callback.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_gpio_irq_attach(chip_pin_t pin,
                                   chip_gpio_irq_trigger_t trigger,
                                   chip_gpio_irq_callback_t callback,
                                   void *context);
/** @brief Remove the GPIO interrupt callback.
 * @param pin Pin whose callback is removed.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_gpio_irq_detach(chip_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif
