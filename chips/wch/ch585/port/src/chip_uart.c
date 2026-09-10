#include <chip_uart.h>

#include <chip_system.h>

#include "CH58x_common.h"
#include "ch585_internal.h"

static uint8_t initialized_uarts;

static bool uart_valid(chip_uart_t uart)
{
    return (uart >= CHIP_UART_0) && (uart < CHIP_UART_COUNT);
}

static uint8_t uart_line_status(chip_uart_t uart)
{
    switch (uart) {
    case CHIP_UART_0:
        return UART0_GetLinSTA();
    case CHIP_UART_1:
        return UART1_GetLinSTA();
    case CHIP_UART_2:
        return UART2_GetLinSTA();
    case CHIP_UART_3:
        return UART3_GetLinSTA();
    default:
        return 0;
    }
}

static uint8_t uart_tx_count(chip_uart_t uart)
{
    switch (uart) {
    case CHIP_UART_0:
        return R8_UART0_TFC;
    case CHIP_UART_1:
        return R8_UART1_TFC;
    case CHIP_UART_2:
        return R8_UART2_TFC;
    case CHIP_UART_3:
        return R8_UART3_TFC;
    default:
        return UART_FIFO_SIZE;
    }
}

static uint8_t uart_rx_count(chip_uart_t uart)
{
    switch (uart) {
    case CHIP_UART_0:
        return R8_UART0_RFC;
    case CHIP_UART_1:
        return R8_UART1_RFC;
    case CHIP_UART_2:
        return R8_UART2_RFC;
    case CHIP_UART_3:
        return R8_UART3_RFC;
    default:
        return 0;
    }
}

static void uart_send(chip_uart_t uart, uint8_t value)
{
    switch (uart) {
    case CHIP_UART_0:
        UART0_SendByte(value);
        break;
    case CHIP_UART_1:
        UART1_SendByte(value);
        break;
    case CHIP_UART_2:
        UART2_SendByte(value);
        break;
    case CHIP_UART_3:
        UART3_SendByte(value);
        break;
    default:
        break;
    }
}

static uint8_t uart_receive(chip_uart_t uart)
{
    switch (uart) {
    case CHIP_UART_0:
        return UART0_RecvByte();
    case CHIP_UART_1:
        return UART1_RecvByte();
    case CHIP_UART_2:
        return UART2_RecvByte();
    case CHIP_UART_3:
        return UART3_RecvByte();
    default:
        return 0;
    }
}

chip_status_t chip_uart_init(chip_uart_t uart, const chip_uart_config_t *config)
{
    uint32_t divisor;

    if (!uart_valid(uart) || (config == NULL) || (config->baud_rate == 0U) ||
        (config->data_bits == 0U) || (config->stop_bits == 0U)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if ((config->data_bits != 8U) || (config->stop_bits != 1U) ||
        (config->parity != CHIP_UART_PARITY_NONE)) {
        return CHIP_ERROR_UNSUPPORTED;
    }
    divisor = (uint32_t)((((uint64_t)chip_system_clock_hz() * 10U) /
                          8U / config->baud_rate + 5U) / 10U);
    if ((divisor == 0U) || (divisor > UINT16_MAX)) {
        return CHIP_ERROR_UNSUPPORTED;
    }

    switch (uart) {
    case CHIP_UART_0:
        UART0_DefInit();
        UART0_BaudRateCfg(config->baud_rate);
        break;
    case CHIP_UART_1:
        UART1_DefInit();
        UART1_BaudRateCfg(config->baud_rate);
        break;
    case CHIP_UART_2:
        UART2_DefInit();
        UART2_BaudRateCfg(config->baud_rate);
        break;
    case CHIP_UART_3:
        UART3_DefInit();
        UART3_BaudRateCfg(config->baud_rate);
        break;
    default:
        return CHIP_ERROR_INVALID_ARG;
    }
    initialized_uarts |= (uint8_t)(UINT8_C(1) << (uint8_t)uart);
    return CHIP_OK;
}

chip_status_t chip_uart_deinit(chip_uart_t uart)
{
    if (!uart_valid(uart)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    switch (uart) {
    case CHIP_UART_0:
        UART0_Reset();
        break;
    case CHIP_UART_1:
        UART1_Reset();
        break;
    case CHIP_UART_2:
        UART2_Reset();
        break;
    case CHIP_UART_3:
        UART3_Reset();
        break;
    default:
        return CHIP_ERROR_INVALID_ARG;
    }
    initialized_uarts &= (uint8_t)~(UINT8_C(1) << (uint8_t)uart);
    return CHIP_OK;
}

chip_status_t chip_uart_write(chip_uart_t uart,
                              const uint8_t *data,
                              size_t size,
                              size_t *written,
                              uint32_t timeout_us)
{
    uint64_t started_at;
    size_t count = 0;
    chip_status_t status;

    if (!uart_valid(uart) || ((data == NULL) && (size != 0U))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if ((initialized_uarts & (UINT8_C(1) << (uint8_t)uart)) == 0U) {
        return CHIP_ERROR_NOT_READY;
    }
    if (written != NULL) {
        *written = 0;
    }
    status = ch585_timeout_start(timeout_us, &started_at);
    if (status != CHIP_OK) {
        return status;
    }

    while (count < size) {
        uint8_t line_status = uart_line_status(uart);
        if ((line_status & (STA_ERR_BREAK | STA_ERR_FRAME | STA_ERR_PAR | STA_ERR_FIFOOV)) != 0U) {
            status = CHIP_ERROR_IO;
            break;
        }
        if (uart_tx_count(uart) < UART_FIFO_SIZE) {
            uart_send(uart, data[count++]);
            continue;
        }
        if (ch585_timeout_expired(started_at, timeout_us)) {
            status = CHIP_ERROR_TIMEOUT;
            break;
        }
    }

    if (written != NULL) {
        *written = count;
    }
    return (count == size) ? CHIP_OK : status;
}

chip_status_t chip_uart_read(chip_uart_t uart,
                             uint8_t *data,
                             size_t size,
                             size_t *read,
                             uint32_t timeout_us)
{
    uint64_t started_at;
    size_t count = 0;
    chip_status_t status;

    if (!uart_valid(uart) || ((data == NULL) && (size != 0U))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if ((initialized_uarts & (UINT8_C(1) << (uint8_t)uart)) == 0U) {
        return CHIP_ERROR_NOT_READY;
    }
    if (read != NULL) {
        *read = 0;
    }
    status = ch585_timeout_start(timeout_us, &started_at);
    if (status != CHIP_OK) {
        return status;
    }

    while (count < size) {
        uint8_t line_status = uart_line_status(uart);
        if ((line_status & (STA_ERR_BREAK | STA_ERR_FRAME | STA_ERR_PAR | STA_ERR_FIFOOV)) != 0U) {
            status = CHIP_ERROR_IO;
            break;
        }
        if (uart_rx_count(uart) > 0U) {
            data[count++] = uart_receive(uart);
            continue;
        }
        if (ch585_timeout_expired(started_at, timeout_us)) {
            status = CHIP_ERROR_TIMEOUT;
            break;
        }
    }

    if (read != NULL) {
        *read = count;
    }
    return (count == size) ? CHIP_OK : status;
}
