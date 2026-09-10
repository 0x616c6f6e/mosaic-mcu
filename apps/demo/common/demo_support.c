#include "demo_support.h"

#include <chip_system.h>
#include <chip_time.h>
#include <platform_log.h>

static int demo_log_backend(const char *data, size_t size, void *context)
{
    size_t written = 0U;
    chip_status_t status;

    (void)context;
    status = chip_uart_write(DEMO_UART_INSTANCE, (const uint8_t *)data,
                             size, &written, CHIP_TIMEOUT_FOREVER);
    if ((status != CHIP_OK) || (written != size)) {
        return -1;
    }
    return (int)written;
}

static uint32_t demo_log_timestamp(void *context)
{
    (void)context;
    return chip_time_millis();
}

chip_status_t demo_platform_init(void)
{
    chip_system_config_t config = {
        .source = CHIP_CLOCK_INTERNAL,
        .core_clock_hz = DEMO_SYSTEM_CLOCK_HZ,
    };
    chip_status_t status = chip_system_init(&config);

    if (status != CHIP_OK) {
        return status;
    }
    return chip_time_init();
}

chip_status_t demo_led_init(void)
{
    chip_gpio_config_t config = {
        .mode = CHIP_GPIO_OUTPUT_PUSH_PULL,
        .pull = CHIP_GPIO_PULL_NONE,
        .drive = CHIP_GPIO_DRIVE_LOW,
        .initial_level = !DEMO_LED_ACTIVE_HIGH,
    };
    return chip_gpio_init(DEMO_LED_PIN, &config);
}

chip_status_t demo_led_write(bool on)
{
    return chip_gpio_write(DEMO_LED_PIN, (on == DEMO_LED_ACTIVE_HIGH));
}

chip_status_t demo_uart_init(void)
{
    chip_gpio_config_t tx_config = {
        .mode = CHIP_GPIO_OUTPUT_PUSH_PULL,
        .pull = CHIP_GPIO_PULL_NONE,
        .drive = CHIP_GPIO_DRIVE_LOW,
        .initial_level = true,
    };
    chip_gpio_config_t rx_config = {
        .mode = CHIP_GPIO_INPUT,
        .pull = CHIP_GPIO_PULL_UP,
        .drive = CHIP_GPIO_DRIVE_LOW,
        .initial_level = false,
    };
    chip_uart_config_t uart_config = {
        .baud_rate = DEMO_UART_BAUD_RATE,
        .data_bits = 8,
        .stop_bits = 1,
        .parity = CHIP_UART_PARITY_NONE,
    };
    chip_status_t status = chip_gpio_init(DEMO_UART_TX_PIN, &tx_config);

    if (status != CHIP_OK) {
        return status;
    }
    status = chip_gpio_init(DEMO_UART_RX_PIN, &rx_config);
    if (status != CHIP_OK) {
        return status;
    }
    return chip_uart_init(DEMO_UART_INSTANCE, &uart_config);
}

chip_status_t demo_log_init(void)
{
    return demo_log_init_with_timestamp(demo_log_timestamp, NULL);
}

chip_status_t demo_log_init_with_timestamp(platform_log_timestamp_t timestamp,
                                           void *context)
{
    platform_log_config_t config = {
        .backend = demo_log_backend,
        .backend_context = NULL,
        .timestamp = timestamp,
        .timestamp_context = context,
        .level = PLATFORM_LOG_LEVEL_DEBUG,
    };
    chip_status_t status = demo_uart_init();

    if (status != CHIP_OK) {
        return status;
    }
    return (platform_log_init(&config) == 0) ? CHIP_OK : CHIP_ERROR_IO;
}

chip_status_t demo_i2c_pins_init(void)
{
    chip_gpio_config_t config = {
        .mode = CHIP_GPIO_INPUT,
        .pull = CHIP_GPIO_PULL_UP,
        .drive = CHIP_GPIO_DRIVE_LOW,
        .initial_level = true,
    };
    chip_status_t status = chip_gpio_init(DEMO_I2C_SCL_PIN, &config);

    if (status != CHIP_OK) {
        return status;
    }
    return chip_gpio_init(DEMO_I2C_SDA_PIN, &config);
}

void demo_halt(void)
{
    for (;;) {
    }
}
