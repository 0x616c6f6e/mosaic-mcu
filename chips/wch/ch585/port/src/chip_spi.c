#include <chip_spi.h>

#include <chip_system.h>

#include "CH58x_common.h"
#include "ch585_internal.h"

static uint8_t initialized_spis;

static bool spi_valid(chip_spi_t spi)
{
    return (spi >= CHIP_SPI_0) && (spi < CHIP_SPI_COUNT);
}

static bool spi_ready(chip_spi_t spi)
{
    return (spi == CHIP_SPI_0) ? (R8_SPI0_FIFO_COUNT == 0U) :
                                (R8_SPI1_FIFO_COUNT == 0U);
}

static void spi_prepare_byte(chip_spi_t spi)
{
    if (spi == CHIP_SPI_0) {
        R8_SPI0_CTRL_MOD &= (uint8_t)~RB_SPI_FIFO_DIR;
        R16_SPI0_TOTAL_CNT = 1;
        R8_SPI0_INT_FLAG = RB_SPI_IF_CNT_END;
    } else {
        R8_SPI1_CTRL_MOD &= (uint8_t)~RB_SPI_FIFO_DIR;
        R16_SPI1_TOTAL_CNT = 1;
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

static uint8_t spi_read_result(chip_spi_t spi)
{
    return (spi == CHIP_SPI_0) ? R8_SPI0_BUFFER : R8_SPI1_BUFFER;
}

static chip_status_t spi_wait_ready(chip_spi_t spi, uint64_t started_at, uint32_t timeout_us)
{
    while (!spi_ready(spi)) {
        if (ch585_timeout_expired(started_at, timeout_us)) {
            return CHIP_ERROR_TIMEOUT;
        }
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
    size_t index;
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

    for (index = 0; index < size; ++index) {
        status = spi_wait_ready(spi, started_at, timeout_us);
        if (status != CHIP_OK) {
            return status;
        }
        spi_prepare_byte(spi);
        spi_write_fifo(spi, (tx_data == NULL) ? UINT8_C(0xFF) : tx_data[index]);
        status = spi_wait_ready(spi, started_at, timeout_us);
        if (status != CHIP_OK) {
            return status;
        }
        if (rx_data != NULL) {
            rx_data[index] = spi_read_result(spi);
        }
    }
    return CHIP_OK;
}
