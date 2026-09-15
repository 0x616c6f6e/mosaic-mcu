#ifndef CHIP_API_SPI_H
#define CHIP_API_SPI_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Available SPI controllers. */
typedef enum {
    CHIP_SPI_0 = 0, /**< SPI0 controller. */
    CHIP_SPI_1,     /**< SPI1 controller. */
    CHIP_SPI_COUNT, /**< Number of SPI controllers. */
} chip_spi_t;

/** @brief SPI clock polarity and phase modes. */
typedef enum {
    CHIP_SPI_MODE_0 = 0, /**< CPOL=0, CPHA=0. */
    CHIP_SPI_MODE_1,     /**< CPOL=0, CPHA=1. */
    CHIP_SPI_MODE_2,     /**< CPOL=1, CPHA=0. */
    CHIP_SPI_MODE_3,     /**< CPOL=1, CPHA=1. */
} chip_spi_mode_t;

/** @brief Bit transmission order within each SPI byte. */
typedef enum {
    CHIP_SPI_MSB_FIRST, /**< Most significant bit first. */
    CHIP_SPI_LSB_FIRST, /**< Least significant bit first. */
} chip_spi_bit_order_t;

/** @brief SPI bus clock, mode, and bit order. */
typedef struct {
    uint32_t clock_hz;         /**< Requested bus frequency in Hz. */
    chip_spi_mode_t mode;      /**< Clock polarity and phase. */
    chip_spi_bit_order_t bit_order; /**< Byte bit order. */
} chip_spi_config_t;

/** @brief Initialize an SPI controller.
 * @param spi Controller instance.
 * @param config Bus clock and wire format.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_spi_init(chip_spi_t spi, const chip_spi_config_t *config);
/** @brief Release an SPI controller.
 * @param spi Controller instance.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_spi_deinit(chip_spi_t spi);

/** @brief Exchange bytes on an SPI bus.
 * @note The caller controls chip select separately through GPIO.
 * @param spi Controller instance.
 * @param tx_data Bytes to send, or NULL to send dummy bytes.
 * @param[out] rx_data Receives bytes, or NULL to discard them.
 * @param size Transfer length in bytes.
 * @param timeout_us Transfer timeout in microseconds.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_spi_transfer(chip_spi_t spi,
                                const uint8_t *tx_data,
                                uint8_t *rx_data,
                                size_t size,
                                uint32_t timeout_us);

#ifdef __cplusplus
}
#endif

#endif
