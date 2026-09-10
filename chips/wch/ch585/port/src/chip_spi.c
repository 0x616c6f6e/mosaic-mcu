#include <chip_spi.h>

#include <chip_system.h>

#include "CH58x_common.h"
#include "ch585_internal.h"

#define CH585_SPI_FIFO_DEPTH       UINT8_C(8)
#define CH585_SPI_MAX_TRANSFER     UINT16_C(0x0FFF)

static uint8_t initialized_spis;

static bool spi_valid(chip_spi_t spi)
{
    return (spi >= CHIP_SPI_0) && (spi < CHIP_SPI_COUNT);
}

static bool spi_transfer_complete(chip_spi_t spi)
{
    return (((spi == CHIP_SPI_0) ? R8_SPI0_INT_FLAG : R8_SPI1_INT_FLAG) &
            RB_SPI_FREE) != 0U;
}

static bool spi_count_complete(chip_spi_t spi)
{
    return (((spi == CHIP_SPI_0) ? R8_SPI0_INT_FLAG : R8_SPI1_INT_FLAG) &
            RB_SPI_IF_CNT_END) != 0U;
}

static uint8_t spi_fifo_count(chip_spi_t spi)
{
    return (spi == CHIP_SPI_0) ? R8_SPI0_FIFO_COUNT : R8_SPI1_FIFO_COUNT;
}

static void spi_begin(chip_spi_t spi, bool receive, uint16_t count)
{
    if (spi == CHIP_SPI_0) {
        if (receive) {
            R8_SPI0_CTRL_MOD |= RB_SPI_FIFO_DIR;
        } else {
            R8_SPI0_CTRL_MOD &= (uint8_t)~RB_SPI_FIFO_DIR;
        }
        R16_SPI0_TOTAL_CNT = count;
        R8_SPI0_INT_FLAG = RB_SPI_IF_CNT_END;
    } else {
        if (receive) {
            R8_SPI1_CTRL_MOD |= RB_SPI_FIFO_DIR;
        } else {
            R8_SPI1_CTRL_MOD &= (uint8_t)~RB_SPI_FIFO_DIR;
        }
        R16_SPI1_TOTAL_CNT = count;
        R8_SPI1_INT_FLAG = RB_SPI_IF_CNT_END;
    }
}

static void spi_write_fifo(chip_spi_t spi, uint8_t value)
{
    if (spi == CHIP_SPI_0) {
        R8_SPI0_FIFO = value;
    } else {
        R8_SPI1_FIFO = value;
    }
}

static uint8_t spi_read_fifo(chip_spi_t spi)
{
    return (spi == CHIP_SPI_0) ? R8_SPI0_FIFO : R8_SPI1_FIFO;
}

static uint8_t spi_read_buffer(chip_spi_t spi)
{
    return (spi == CHIP_SPI_0) ? R8_SPI0_BUFFER : R8_SPI1_BUFFER;
}

static chip_status_t spi_wait_complete(chip_spi_t spi, uint64_t started_at,
                                       uint32_t timeout_us)
{
    while (!spi_transfer_complete(spi)) {
        if (ch585_timeout_expired(started_at, timeout_us)) {
            return CHIP_ERROR_TIMEOUT;
        }
    }
    return CHIP_OK;
}

static chip_status_t spi_wait_count_complete(chip_spi_t spi,
                                              uint64_t started_at,
                                              uint32_t timeout_us)
{
    while (!spi_count_complete(spi)) {
        if (ch585_timeout_expired(started_at, timeout_us)) {
            return CHIP_ERROR_TIMEOUT;
        }
    }
    return CHIP_OK;
}

static chip_status_t spi_transmit(chip_spi_t spi, const uint8_t *data,
                                  size_t size, uint64_t started_at,
                                  uint32_t timeout_us)
{
    while (size > 0U) {
        uint16_t count = (size > CH585_SPI_MAX_TRANSFER) ?
                         CH585_SPI_MAX_TRANSFER : (uint16_t)size;
        uint16_t written = 0U;

        spi_begin(spi, false, count);
        while (written < count) {
            if (spi_fifo_count(spi) < CH585_SPI_FIFO_DEPTH) {
                spi_write_fifo(spi, data[written]);
                ++written;
            } else if (ch585_timeout_expired(started_at, timeout_us)) {
                return CHIP_ERROR_TIMEOUT;
            }
        }
        if (spi_wait_count_complete(spi, started_at, timeout_us) != CHIP_OK) {
            return CHIP_ERROR_TIMEOUT;
        }
        data += count;
        size -= count;
    }
    return CHIP_OK;
}

static chip_status_t spi_receive(chip_spi_t spi, uint8_t *data, size_t size,
                                 uint64_t started_at, uint32_t timeout_us)
{
    while (size > 0U) {
        uint16_t count = (size > CH585_SPI_MAX_TRANSFER) ?
                         CH585_SPI_MAX_TRANSFER : (uint16_t)size;
        uint16_t received = 0U;

        spi_begin(spi, true, count);
        while (received < count) {
            if (spi_fifo_count(spi) != 0U) {
                data[received] = spi_read_fifo(spi);
                ++received;
            } else if (ch585_timeout_expired(started_at, timeout_us)) {
                return CHIP_ERROR_TIMEOUT;
            }
        }
        if (spi_wait_count_complete(spi, started_at, timeout_us) != CHIP_OK) {
            return CHIP_ERROR_TIMEOUT;
        }
        data += count;
        size -= count;
    }
    return CHIP_OK;
}

static chip_status_t spi_transceive(chip_spi_t spi, const uint8_t *tx_data,
                                    uint8_t *rx_data, size_t size,
                                    uint64_t started_at, uint32_t timeout_us)
{
    size_t index;

    for (index = 0U; index < size; ++index) {
        chip_status_t status;

        spi_begin(spi, false, 1U);
        spi_write_fifo(spi, tx_data[index]);
        status = spi_wait_complete(spi, started_at, timeout_us);
        if (status != CHIP_OK) {
            return status;
        }
        rx_data[index] = spi_read_buffer(spi);
    }
    return CHIP_OK;
}

chip_status_t chip_spi_init(chip_spi_t spi, const chip_spi_config_t *config)
{
    uint32_t system_clock;
    uint32_t divider;
    ModeBitOrderTypeDef data_mode;

    if (!spi_valid(spi) || (config == NULL) || (config->clock_hz == 0U) ||
        (config->mode > CHIP_SPI_MODE_3) ||
        (config->bit_order > CHIP_SPI_LSB_FIRST)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if ((config->mode != CHIP_SPI_MODE_0) && (config->mode != CHIP_SPI_MODE_3)) {
        return CHIP_ERROR_UNSUPPORTED;
    }

    system_clock = chip_system_clock_hz();
    divider = (uint32_t)(((uint64_t)system_clock + config->clock_hz - 1U) /
                         config->clock_hz);
    if (divider < 2U) {
        divider = 2U;
    }
    if (divider > UINT8_MAX) {
        return CHIP_ERROR_UNSUPPORTED;
    }

    if (config->mode == CHIP_SPI_MODE_0) {
        data_mode = (config->bit_order == CHIP_SPI_MSB_FIRST) ?
                    Mode0_HighBitINFront : Mode0_LowBitINFront;
    } else {
        data_mode = (config->bit_order == CHIP_SPI_MSB_FIRST) ?
                    Mode3_HighBitINFront : Mode3_LowBitINFront;
    }

    if (spi == CHIP_SPI_0) {
        SPI0_MasterDefInit();
        SPI0_CLKCfg((uint8_t)divider);
        SPI0_DataMode(data_mode);
    } else {
        SPI1_MasterDefInit();
        SPI1_CLKCfg((uint8_t)divider);
        SPI1_DataMode(data_mode);
    }
    initialized_spis |= (uint8_t)(UINT8_C(1) << (uint8_t)spi);
    return CHIP_OK;
}

chip_status_t chip_spi_deinit(chip_spi_t spi)
{
    if (!spi_valid(spi)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    if (spi == CHIP_SPI_0) {
        R8_SPI0_CTRL_MOD &= (uint8_t)~(RB_SPI_MOSI_OE | RB_SPI_SCK_OE | RB_SPI_MISO_OE);
    } else {
        R8_SPI1_CTRL_MOD &= (uint8_t)~(RB_SPI_MOSI_OE | RB_SPI_SCK_OE | RB_SPI_MISO_OE);
    }
    initialized_spis &= (uint8_t)~(UINT8_C(1) << (uint8_t)spi);
    return CHIP_OK;
}

chip_status_t chip_spi_transfer(chip_spi_t spi,
                                const uint8_t *tx_data,
                                uint8_t *rx_data,
                                size_t size,
                                uint32_t timeout_us)
{
    uint64_t started_at;
    chip_status_t status;

    if (!spi_valid(spi) || ((tx_data == NULL) && (rx_data == NULL) && (size != 0U))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if ((initialized_spis & (UINT8_C(1) << (uint8_t)spi)) == 0U) {
        return CHIP_ERROR_NOT_READY;
    }
    status = ch585_timeout_start(timeout_us, &started_at);
    if (status != CHIP_OK) {
        return status;
    }

    if (tx_data == NULL) {
        return spi_receive(spi, rx_data, size, started_at, timeout_us);
    }
    if (rx_data == NULL) {
        return spi_transmit(spi, tx_data, size, started_at, timeout_us);
    }
    return spi_transceive(spi, tx_data, rx_data, size, started_at,
                          timeout_us);
}
