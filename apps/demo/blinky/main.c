#include <stdbool.h>

#include <chip_time.h>

#include "demo_support.h"

int main(void)
{
    bool led_on = false;

    DEMO_REQUIRE(demo_platform_init());
    DEMO_REQUIRE(demo_led_init());

    for (;;) {
        led_on = !led_on;
        DEMO_REQUIRE(demo_led_write(led_on));
        chip_delay_ms(500);
    }
}
