#ifndef CH585_DEMO_SUPPORT_H
#define CH585_DEMO_SUPPORT_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>
#include <platform_log.h>

#include "demo_board.h"

chip_status_t demo_platform_init(void);
chip_status_t demo_led_init(void);
chip_status_t demo_led_write(bool on);
chip_status_t demo_uart_init(void);
chip_status_t demo_log_init(void);
chip_status_t demo_log_init_with_timestamp(platform_log_timestamp_t timestamp,
                                           void *context);
chip_status_t demo_i2c_pins_init(void);

void demo_halt(void);

#define DEMO_REQUIRE(expression) \
    do { \
        if ((expression) != CHIP_OK) { \
            demo_halt(); \
        } \
    } while (0)

#endif
