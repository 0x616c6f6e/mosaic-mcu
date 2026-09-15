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

#define SPI_FLASH_MAX_CAPACITY_BYTES UINT32_C(0x01000000)

typedef struct {
    chip_spi_t spi;
    chip_pin_t cs_pin;
    uint32_t clock_hz;
    /* Set to zero to detect the capacity from the JEDEC ID during init. */
    uint32_t capacity_bytes;
    uint32_t page_size;
    uint32_t sector_size;
    uint32_t transfer_timeout_us;
    uint32_t program_timeout_us;
    uint32_t sector_erase_timeout_us;
    uint32_t chip_erase_timeout_us;
} spi_flash_config_t;

typedef struct {
    spi_flash_config_t config;
    bool initialized;
} spi_flash_t;

typedef struct {
    uint8_t manufacturer;
    uint8_t memory_type;
    uint8_t capacity;
} spi_flash_jedec_id_t;

chip_status_t spi_flash_init(spi_flash_t *flash,
                             const spi_flash_config_t *config);
chip_status_t spi_flash_deinit(spi_flash_t *flash);

chip_status_t spi_flash_read_jedec_id(spi_flash_t *flash,
                                      spi_flash_jedec_id_t *id);
chip_status_t spi_flash_read_status(spi_flash_t *flash, uint8_t *status);
chip_status_t spi_flash_wait_ready(spi_flash_t *flash, uint32_t timeout_us);

chip_status_t spi_flash_read(spi_flash_t *flash, uint32_t address,
                             void *data, size_t size);
chip_status_t spi_flash_program(spi_flash_t *flash, uint32_t address,
                                const void *data, size_t size);
chip_status_t spi_flash_erase_sector(spi_flash_t *flash, uint32_t address);
chip_status_t spi_flash_erase_chip(spi_flash_t *flash);

#ifdef __cplusplus
}
#endif

#endif
