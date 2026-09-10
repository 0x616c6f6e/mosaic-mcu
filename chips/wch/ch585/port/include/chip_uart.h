#ifndef CHIP_API_UART_H
#define CHIP_API_UART_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHIP_UART_0 = 0,
    CHIP_UART_1,
    CHIP_UART_2,
    CHIP_UART_3,
    CHIP_UART_COUNT,
} chip_uart_t;

typedef enum {
    CHIP_UART_PARITY_NONE,
    CHIP_UART_PARITY_EVEN,
    CHIP_UART_PARITY_ODD,
} chip_uart_parity_t;

typedef struct {
    uint32_t baud_rate;
    uint8_t data_bits;
    uint8_t stop_bits;
    chip_uart_parity_t parity;
} chip_uart_config_t;

chip_status_t chip_uart_init(chip_uart_t uart, const chip_uart_config_t *config);
chip_status_t chip_uart_deinit(chip_uart_t uart);
chip_status_t chip_uart_write(chip_uart_t uart,
                              const uint8_t *data,
                              size_t size,
                              size_t *written,
                              uint32_t timeout_us);
chip_status_t chip_uart_read(chip_uart_t uart,
                             uint8_t *data,
                             size_t size,
                             size_t *read,
                             uint32_t timeout_us);

#ifdef __cplusplus
}
#endif

#endif
