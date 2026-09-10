#ifndef CHIP_API_SPI_H
#define CHIP_API_SPI_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHIP_SPI_0 = 0,
    CHIP_SPI_1,
    CHIP_SPI_COUNT,
} chip_spi_t;

typedef enum {
    CHIP_SPI_MODE_0 = 0,
    CHIP_SPI_MODE_1,
    CHIP_SPI_MODE_2,
    CHIP_SPI_MODE_3,
} chip_spi_mode_t;

typedef enum {
    CHIP_SPI_MSB_FIRST,
    CHIP_SPI_LSB_FIRST,
} chip_spi_bit_order_t;

typedef struct {
    uint32_t clock_hz;
    chip_spi_mode_t mode;
    chip_spi_bit_order_t bit_order;
} chip_spi_config_t;

chip_status_t chip_spi_init(chip_spi_t spi, const chip_spi_config_t *config);
chip_status_t chip_spi_deinit(chip_spi_t spi);

/* Chip select is controlled by the caller through GPIO. */
chip_status_t chip_spi_transfer(chip_spi_t spi,
                                const uint8_t *tx_data,
                                uint8_t *rx_data,
                                size_t size,
                                uint32_t timeout_us);

#ifdef __cplusplus
}
#endif

#endif
