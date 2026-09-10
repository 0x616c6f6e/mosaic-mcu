#include <chip_gpio.h>

#include <stddef.h>

#include <chip_system.h>

#include "CH58x_common.h"

#define CH585_PORT_A_PIN_COUNT 16U
#define CH585_PORT_B_PIN_COUNT 24U

typedef struct {
    chip_gpio_irq_callback_t callback;
    void *context;
} gpio_irq_slot_t;

static gpio_irq_slot_t port_a_irqs[CH585_PORT_A_PIN_COUNT];
static gpio_irq_slot_t port_b_irqs[CH585_PORT_B_PIN_COUNT];
static bool port_b_high_irq_mapping;

static bool gpio_pin_valid(chip_pin_t pin)
{
    chip_gpio_port_t port = CHIP_PIN_PORT(pin);
    uint8_t index = CHIP_PIN_INDEX(pin);

    return ((port == CHIP_GPIO_PORT_A) && (index < CH585_PORT_A_PIN_COUNT)) ||
           ((port == CHIP_GPIO_PORT_B) && (index < CH585_PORT_B_PIN_COUNT));
}

static uint32_t gpio_pin_mask(chip_pin_t pin)
{
    return UINT32_C(1) << CHIP_PIN_INDEX(pin);
}

static GPIOModeTypeDef gpio_input_mode(chip_gpio_pull_t pull)
{
    switch (pull) {
    case CHIP_GPIO_PULL_UP:
        return GPIO_ModeIN_PU;
    case CHIP_GPIO_PULL_DOWN:
        return GPIO_ModeIN_PD;
    case CHIP_GPIO_PULL_NONE:
    default:
        return GPIO_ModeIN_Floating;
    }
}

static void gpio_set_digital(chip_pin_t pin, FunctionalState state)
{
    uint32_t mask = gpio_pin_mask(pin);

    if (CHIP_PIN_PORT(pin) == CHIP_GPIO_PORT_A) {
        GPIOADigitalCfg(state, (uint16_t)mask);
    } else {
        GPIOBDigitalCfg(state, mask);
    }
}

chip_status_t chip_gpio_init(chip_pin_t pin, const chip_gpio_config_t *config)
{
    uint32_t mask;
    GPIOModeTypeDef vendor_mode;

    if (!gpio_pin_valid(pin) || (config == NULL)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if ((config->pull > CHIP_GPIO_PULL_DOWN) ||
        (config->drive > CHIP_GPIO_DRIVE_HIGH)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    mask = gpio_pin_mask(pin);
    if (config->mode == CHIP_GPIO_ANALOG) {
        gpio_set_digital(pin, DISABLE);
        return CHIP_OK;
    }

    gpio_set_digital(pin, ENABLE);
    if (config->mode == CHIP_GPIO_INPUT) {
        vendor_mode = gpio_input_mode(config->pull);
    } else if (config->mode == CHIP_GPIO_OUTPUT_PUSH_PULL) {
        if (CHIP_PIN_PORT(pin) == CHIP_GPIO_PORT_A) {
            config->initial_level ? GPIOA_SetBits(mask) : GPIOA_ResetBits(mask);
        } else {
            config->initial_level ? GPIOB_SetBits(mask) : GPIOB_ResetBits(mask);
        }
        vendor_mode = (config->drive == CHIP_GPIO_DRIVE_HIGH) ?
                      GPIO_ModeOut_PP_20mA : GPIO_ModeOut_PP_5mA;
    } else {
        return CHIP_ERROR_INVALID_ARG;
    }

    if (CHIP_PIN_PORT(pin) == CHIP_GPIO_PORT_A) {
        GPIOA_ModeCfg(mask, vendor_mode);
    } else {
        GPIOB_ModeCfg(mask, vendor_mode);
    }
    return CHIP_OK;
}

chip_status_t chip_gpio_deinit(chip_pin_t pin)
{
    chip_gpio_config_t config = {
        .mode = CHIP_GPIO_INPUT,
        .pull = CHIP_GPIO_PULL_NONE,
        .drive = CHIP_GPIO_DRIVE_LOW,
        .initial_level = false,
    };

    if (!gpio_pin_valid(pin)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    (void)chip_gpio_irq_detach(pin);
    return chip_gpio_init(pin, &config);
}

chip_status_t chip_gpio_read(chip_pin_t pin, bool *level)
{
    uint32_t mask;

    if (!gpio_pin_valid(pin) || (level == NULL)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    mask = gpio_pin_mask(pin);
    *level = (CHIP_PIN_PORT(pin) == CHIP_GPIO_PORT_A) ?
             (GPIOA_ReadPortPin(mask) != 0U) : (GPIOB_ReadPortPin(mask) != 0U);
    return CHIP_OK;
}

chip_status_t chip_gpio_write(chip_pin_t pin, bool level)
{
    uint32_t mask;

    if (!gpio_pin_valid(pin)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    mask = gpio_pin_mask(pin);
    if (CHIP_PIN_PORT(pin) == CHIP_GPIO_PORT_A) {
        level ? GPIOA_SetBits(mask) : GPIOA_ResetBits(mask);
    } else {
        level ? GPIOB_SetBits(mask) : GPIOB_ResetBits(mask);
    }
    return CHIP_OK;
}

chip_status_t chip_gpio_toggle(chip_pin_t pin)
{
    uint32_t mask;

    if (!gpio_pin_valid(pin)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    mask = gpio_pin_mask(pin);
    if (CHIP_PIN_PORT(pin) == CHIP_GPIO_PORT_A) {
        GPIOA_InverseBits(mask);
    } else {
        GPIOB_InverseBits(mask);
    }
    return CHIP_OK;
}

static GPIOITModeTpDef gpio_irq_mode(chip_gpio_irq_trigger_t trigger)
{
    switch (trigger) {
    case CHIP_GPIO_IRQ_LOW_LEVEL:
        return GPIO_ITMode_LowLevel;
    case CHIP_GPIO_IRQ_HIGH_LEVEL:
        return GPIO_ITMode_HighLevel;
    case CHIP_GPIO_IRQ_FALLING_EDGE:
        return GPIO_ITMode_FallEdge;
    case CHIP_GPIO_IRQ_RISING_EDGE:
    default:
        return GPIO_ITMode_RiseEdge;
    }
}

static bool gpio_port_b_mapping_conflicts(uint8_t index)
{
    if ((index == 22U) || (index == 23U)) {
        return (port_b_irqs[8].callback != NULL) || (port_b_irqs[9].callback != NULL);
    }
    if ((index == 8U) || (index == 9U)) {
        return (port_b_irqs[22].callback != NULL) || (port_b_irqs[23].callback != NULL);
    }
    return false;
}

chip_status_t chip_gpio_irq_attach(chip_pin_t pin,
                                   chip_gpio_irq_trigger_t trigger,
                                   chip_gpio_irq_callback_t callback,
                                   void *context)
{
    uint8_t index;
    uint32_t mask;
    uint32_t state;

    if (!gpio_pin_valid(pin) || (callback == NULL) ||
        (trigger > CHIP_GPIO_IRQ_RISING_EDGE)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    index = CHIP_PIN_INDEX(pin);
    mask = gpio_pin_mask(pin);
    if ((CHIP_PIN_PORT(pin) == CHIP_GPIO_PORT_B) &&
        (index > 15U) && (index < 22U)) {
        return CHIP_ERROR_UNSUPPORTED;
    }

    state = chip_system_critical_enter();
    if ((CHIP_PIN_PORT(pin) == CHIP_GPIO_PORT_B) &&
        gpio_port_b_mapping_conflicts(index)) {
        chip_system_critical_exit(state);
        return CHIP_ERROR_BUSY;
    }

    if (CHIP_PIN_PORT(pin) == CHIP_GPIO_PORT_A) {
        port_a_irqs[index].callback = callback;
        port_a_irqs[index].context = context;
        GPIOA_ITModeCfg(mask, gpio_irq_mode(trigger));
    } else {
        if ((index == 22U) || (index == 23U)) {
            GPIOPinRemap(ENABLE, RB_PIN_INTX);
            port_b_high_irq_mapping = true;
        } else if ((index == 8U) || (index == 9U)) {
            GPIOPinRemap(DISABLE, RB_PIN_INTX);
            port_b_high_irq_mapping = false;
        }
        port_b_irqs[index].callback = callback;
        port_b_irqs[index].context = context;
        GPIOB_ITModeCfg(mask, gpio_irq_mode(trigger));
    }
    chip_system_critical_exit(state);
    PFIC_EnableIRQ((CHIP_PIN_PORT(pin) == CHIP_GPIO_PORT_A) ? GPIO_A_IRQn : GPIO_B_IRQn);
    return CHIP_OK;
}

chip_status_t chip_gpio_irq_detach(chip_pin_t pin)
{
    uint8_t index;
    uint32_t mask;
    uint32_t state;

    if (!gpio_pin_valid(pin)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    index = CHIP_PIN_INDEX(pin);
    mask = gpio_pin_mask(pin);
    state = chip_system_critical_enter();
    if (CHIP_PIN_PORT(pin) == CHIP_GPIO_PORT_A) {
        R16_PA_INT_EN &= (uint16_t)~mask;
        GPIOA_ClearITFlagBit((uint16_t)mask);
        port_a_irqs[index].callback = NULL;
        port_a_irqs[index].context = NULL;
    } else {
        uint16_t raw_mask = (uint16_t)(((index == 22U) || (index == 23U)) ?
                                      (mask >> 14U) : mask);
        R16_PB_INT_EN &= (uint16_t)~raw_mask;
        R16_PB_INT_IF = raw_mask;
        port_b_irqs[index].callback = NULL;
        port_b_irqs[index].context = NULL;
    }
    chip_system_critical_exit(state);
    return CHIP_OK;
}

static void gpio_dispatch(gpio_irq_slot_t *slots, uint8_t index, chip_gpio_port_t port)
{
    chip_gpio_irq_callback_t callback = slots[index].callback;

    if (callback != NULL) {
        callback(CHIP_PIN(port, index), slots[index].context);
    }
}

__INTERRUPT
void GPIOA_IRQHandler(void)
{
    uint16_t flags = GPIOA_ReadITFlagPort();
    uint8_t index;

    R16_PA_INT_IF = flags;
    for (index = 0; index < CH585_PORT_A_PIN_COUNT; ++index) {
        if ((flags & (UINT16_C(1) << index)) != 0U) {
            gpio_dispatch(port_a_irqs, index, CHIP_GPIO_PORT_A);
        }
    }
}

__INTERRUPT
void GPIOB_IRQHandler(void)
{
    uint16_t flags = R16_PB_INT_IF;
    uint8_t index;

    R16_PB_INT_IF = flags;
    for (index = 0; index < 16U; ++index) {
        uint8_t callback_index = index;

        if ((flags & (UINT16_C(1) << index)) == 0U) {
            continue;
        }
        if (port_b_high_irq_mapping && ((index == 8U) || (index == 9U))) {
            callback_index = (uint8_t)(index + 14U);
        }
        gpio_dispatch(port_b_irqs, callback_index, CHIP_GPIO_PORT_B);
    }
}
