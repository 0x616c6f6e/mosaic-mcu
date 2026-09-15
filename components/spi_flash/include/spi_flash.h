#ifndef PLATFORM_SPI_FLASH_H
#define PLATFORM_SPI_FLASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <chip_gpio.h>
#include <chip_spi.h>
#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum capacity reachable with three-byte SPI NOR addresses. */
#define SPI_FLASH_MAX_CAPACITY_BYTES UINT32_C(0x01000000)

/** @brief SPI NOR geometry and transfer/program/erase timeouts. */
typedef struct {
    chip_spi_t spi;                 /**< SPI controller owned by this flash. */
    chip_pin_t cs_pin;              /**< Active-low chip select. */
    uint32_t clock_hz;             /**< SPI clock frequency in Hz. */
    uint32_t capacity_bytes;       /**< Flash size; 0 detects from JEDEC ID. */
    uint32_t page_size;            /**< Program page size in bytes. */
    uint32_t sector_size;          /**< Erase sector size in bytes. */
    uint32_t transfer_timeout_us;  /**< SPI transfer timeout in microseconds. */
    uint32_t program_timeout_us;   /**< Page program timeout in microseconds. */
    uint32_t sector_erase_timeout_us; /**< Sector erase timeout in microseconds. */
    uint32_t chip_erase_timeout_us; /**< Whole-chip erase timeout in microseconds. */
} spi_flash_config_t;

/** @brief Initialized SPI NOR instance and its resolved configuration. */
typedef struct {
    spi_flash_config_t config; /**< Copy with detected capacity resolved. */
    bool initialized;          /**< true after successful initialization. */
} spi_flash_t;

/** @brief Three-byte JEDEC identification returned by command 0x9F. */
typedef struct {
    uint8_t manufacturer; /**< JEDEC manufacturer code. */
    uint8_t memory_type;  /**< JEDEC memory family code. */
    uint8_t capacity;     /**< JEDEC capacity exponent (2^code bytes). */
} spi_flash_jedec_id_t;

/** @brief Initialize a SPI NOR device and optionally detect its capacity.
 * @param[out] flash Instance to initialize; config.capacity_bytes is resolved.
 * @param config Bus, geometry, and timeout settings in microseconds.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t spi_flash_init(spi_flash_t *flash,
                             const spi_flash_config_t *config);
/** @brief Release the flash device's SPI controller and chip-select pin.
 * @param flash Initialized flash instance.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t spi_flash_deinit(spi_flash_t *flash);

/** @brief Read manufacturer, memory type, and capacity code.
 * @param flash Initialized flash instance.
 * @param[out] id Receives the JEDEC identification bytes.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t spi_flash_read_jedec_id(spi_flash_t *flash,
                                      spi_flash_jedec_id_t *id);
/** @brief Read the SPI NOR status register.
 * @param flash Initialized flash instance.
 * @param[out] status Receives the status register byte.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t spi_flash_read_status(spi_flash_t *flash, uint8_t *status);
/** @brief Poll until the SPI NOR write-in-progress bit clears.
 * @param flash Initialized flash instance.
 * @param timeout_us Maximum wait in microseconds.
 * @return CHIP_OK when ready, or a CHIP_ERROR_* status.
 */
chip_status_t spi_flash_wait_ready(spi_flash_t *flash, uint32_t timeout_us);

/** @brief Read bytes from SPI NOR.
 * @param flash Initialized flash instance.
 * @param address Zero-based byte address.
 * @param[out] data Receives size bytes.
 * @param size Number of bytes to read.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t spi_flash_read(spi_flash_t *flash, uint32_t address,
                             void *data, size_t size);
/** @brief Page-program bytes, splitting writes across page boundaries.
 * @note Programming changes bits only from 1 to 0; erase before rewriting.
 * @param flash Initialized flash instance.
 * @param address Zero-based byte address.
 * @param data Bytes to program.
 * @param size Number of bytes to write.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t spi_flash_program(spi_flash_t *flash, uint32_t address,
                                const void *data, size_t size);
/** @brief Erase one SPI NOR sector.
 * @param flash Initialized flash instance.
 * @param address Byte address aligned to config.sector_size.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t spi_flash_erase_sector(spi_flash_t *flash, uint32_t address);
/** @brief Erase the entire SPI NOR device.
 * @param flash Initialized flash instance.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t spi_flash_erase_chip(spi_flash_t *flash);

#ifdef __cplusplus
}
#endif

#endif
