#include "keyboard_log.h"

#include <stddef.h>
#include <stdint.h>

#include <chip_gpio.h>
#include <chip_time.h>
#include <chip_uart.h>
#include <platform_log.h>

#define OTA_LOG_UART      CHIP_UART_0
#define OTA_LOG_UART_TX   CHIP_PIN(CHIP_GPIO_PORT_B, 7)
#define OTA_LOG_UART_RX   CHIP_PIN(CHIP_GPIO_PORT_B, 4)
#define OTA_LOG_BAUD_RATE UINT32_C(115200)

static int keyboard_log_backend(const char *data, size_t size, void *context)
{
    size_t written = 0U;
    chip_status_t status;

    (void)context;
    status = chip_uart_write(OTA_LOG_UART, (const uint8_t *)data, size,
                             &written, CHIP_TIMEOUT_FOREVER);
    return ((status == CHIP_OK) && (written == size)) ? (int)written : -1;
}

static uint32_t keyboard_log_timestamp(void *context)
{
    (void)context;
    return chip_time_millis();
}

chip_status_t keyboard_log_init(void)
{
    const chip_gpio_config_t tx_config = {
        .mode = CHIP_GPIO_OUTPUT_PUSH_PULL,
        .pull = CHIP_GPIO_PULL_NONE,
        .drive = CHIP_GPIO_DRIVE_LOW,
        .initial_level = true,
    };
    const chip_gpio_config_t rx_config = {
        .mode = CHIP_GPIO_INPUT,
        .pull = CHIP_GPIO_PULL_UP,
        .drive = CHIP_GPIO_DRIVE_LOW,
        .initial_level = false,
    };
    const chip_uart_config_t uart_config = {
        .baud_rate = OTA_LOG_BAUD_RATE,
        .data_bits = 8U,
        .stop_bits = 1U,
        .parity = CHIP_UART_PARITY_NONE,
    };
    const platform_log_config_t log_config = {
        .backend = keyboard_log_backend,
        .backend_context = NULL,
        .timestamp = keyboard_log_timestamp,
        .timestamp_context = NULL,
        .level = PLATFORM_LOG_LEVEL_DEBUG,
    };
    chip_status_t status = chip_gpio_init(OTA_LOG_UART_TX, &tx_config);

    if (status == CHIP_OK) {
        status = chip_gpio_init(OTA_LOG_UART_RX, &rx_config);
    }
    if (status == CHIP_OK) {
        status = chip_uart_init(OTA_LOG_UART, &uart_config);
    }
    if ((status == CHIP_OK) && (platform_log_init(&log_config) != 0)) {
        status = CHIP_ERROR_IO;
    }
    return status;
}

chip_status_t keyboard_log_flush(void)
{
    return chip_uart_flush(OTA_LOG_UART, CHIP_TIMEOUT_FOREVER);
}
