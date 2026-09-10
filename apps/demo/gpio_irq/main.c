#include <stdbool.h>

#include <chip_gpio.h>
#include <chip_system.h>
#include <platform_log.h>

#include "demo_support.h"

static volatile bool button_pressed;

static void button_callback(chip_pin_t pin, void *context)
{
    (void)pin;
    (void)context;
    button_pressed = true;
}

int main(void)
{
    chip_gpio_config_t button_config = {
        .mode = CHIP_GPIO_INPUT,
        .pull = CHIP_GPIO_PULL_UP,
        .drive = CHIP_GPIO_DRIVE_LOW,
        .initial_level = false,
    };
    bool led_on = false;
    uint32_t press_count = 0;

    DEMO_REQUIRE(demo_platform_init());
    DEMO_REQUIRE(demo_led_init());
    DEMO_REQUIRE(demo_log_init());
    DEMO_REQUIRE(chip_gpio_init(DEMO_BUTTON_PIN, &button_config));
    DEMO_REQUIRE(chip_gpio_irq_attach(DEMO_BUTTON_PIN, CHIP_GPIO_IRQ_FALLING_EDGE,
                                      button_callback, NULL));
    LOG_INFO("gpio", "CH585 IRQ ready");

    for (;;) {
        if (button_pressed) {
            uint32_t state = chip_system_critical_enter();
            button_pressed = false;
            chip_system_critical_exit(state);

            led_on = !led_on;
            ++press_count;
            DEMO_REQUIRE(demo_led_write(led_on));
            LOG_INFO("gpio", "button count=%lu", (unsigned long)press_count);
        }
    }
}
