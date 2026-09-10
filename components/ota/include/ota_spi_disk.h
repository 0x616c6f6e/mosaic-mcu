#ifndef PLATFORM_OTA_SPI_DISK_H
#define PLATFORM_OTA_SPI_DISK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <platform_fatfs.h>
#include <spi_flash.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_DISK_SECTOR_SIZE UINT32_C(512)

typedef struct {
    spi_flash_t *flash;
    uint32_t offset;
    uint32_t size;
    uint8_t *cache;
    size_t cache_size;
    uint32_t cached_block;
    bool cache_valid;
    bool cache_dirty;
    bool initialized;
} ota_spi_disk_t;

chip_status_t ota_spi_disk_init(ota_spi_disk_t *disk, spi_flash_t *flash,
                                uint32_t offset, uint32_t size,
                                uint8_t *cache, size_t cache_size);
chip_status_t ota_spi_disk_bind(ota_spi_disk_t *disk);
chip_status_t ota_spi_disk_sync(ota_spi_disk_t *disk);
chip_status_t ota_spi_disk_read(ota_spi_disk_t *disk, uint32_t offset,
                                void *buffer, size_t size);
chip_status_t ota_spi_disk_write(ota_spi_disk_t *disk, uint32_t offset,
                                 const void *buffer, size_t size);
uint32_t ota_spi_disk_sector_count(const ota_spi_disk_t *disk);

#ifdef __cplusplus
}
#endif

#endif
