#include <stdint.h>

#include <chip_uart.h>
#include <platform_log.h>

#include "demo_support.h"

int main(void)
{
    uint8_t byte;
    size_t transferred;

    DEMO_REQUIRE(demo_platform_init());
    DEMO_REQUIRE(demo_log_init());
    LOG_INFO("uart", "CH585 echo ready");

    for (;;) {
        if (chip_uart_read(DEMO_UART_INSTANCE, &byte, 1, &transferred,
                           CHIP_TIMEOUT_FOREVER) == CHIP_OK) {
            (void)chip_uart_write(DEMO_UART_INSTANCE, &byte, transferred, NULL,
                                  CHIP_TIMEOUT_FOREVER);
        }
    }
}
