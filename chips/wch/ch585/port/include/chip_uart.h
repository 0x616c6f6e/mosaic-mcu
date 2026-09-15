#ifndef CHIP_API_UART_H
#define CHIP_API_UART_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Available UART controllers. */
typedef enum {
    CHIP_UART_0 = 0, /**< UART0 controller. */
    CHIP_UART_1,     /**< UART1 controller. */
    CHIP_UART_2,     /**< UART2 controller. */
    CHIP_UART_3,     /**< UART3 controller. */
    CHIP_UART_COUNT, /**< Number of UART controllers. */
} chip_uart_t;

/** @brief Serial parity selection. */
typedef enum {
    CHIP_UART_PARITY_NONE, /**< No parity bit. */
    CHIP_UART_PARITY_EVEN, /**< Even parity bit. */
    CHIP_UART_PARITY_ODD,  /**< Odd parity bit. */
} chip_uart_parity_t;

/** @brief UART baud rate, data bits, stop bits, and parity. */
typedef struct {
    uint32_t baud_rate;       /**< Requested baud rate in bits per second. */
    uint8_t data_bits;        /**< Data bits per frame. */
    uint8_t stop_bits;        /**< Stop bits per frame. */
    chip_uart_parity_t parity; /**< Parity mode. */
} chip_uart_config_t;

/** @brief Configure a UART controller.
 * @param uart Controller instance.
 * @param config Serial line settings.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_uart_init(chip_uart_t uart, const chip_uart_config_t *config);
/** @brief Release a UART controller.
 * @param uart Controller instance.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_uart_deinit(chip_uart_t uart);
/** @brief Transmit bytes over UART.
 * @param uart Controller instance.
 * @param data Bytes to send.
 * @param size Number of bytes requested.
 * @param[out] written Receives the number actually transmitted.
 * @param timeout_us Timeout in microseconds.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_uart_write(chip_uart_t uart,
                              const uint8_t *data,
                              size_t size,
                              size_t *written,
                              uint32_t timeout_us);
/** @brief Wait for queued UART output to complete.
 * @param uart Controller instance.
 * @param timeout_us Timeout in microseconds.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_uart_flush(chip_uart_t uart, uint32_t timeout_us);
/** @brief Receive bytes over UART.
 * @param uart Controller instance.
 * @param[out] data Destination buffer.
 * @param size Maximum number of bytes to receive.
 * @param[out] read Receives the number actually read.
 * @param timeout_us Timeout in microseconds.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_uart_read(chip_uart_t uart,
                             uint8_t *data,
                             size_t size,
                             size_t *read,
                             uint32_t timeout_us);

#ifdef __cplusplus
}
#endif

#endif
