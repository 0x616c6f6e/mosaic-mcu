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

/** @brief FAT/MSC logical sector size in bytes. */
#define OTA_DISK_SECTOR_SIZE UINT32_C(512)

/** @brief SPI NOR-backed block device with a write-back erase-block cache. */
typedef struct {
    spi_flash_t *flash; /**< Flash device backing this disk. */
    uint32_t offset;    /**< Partition start address in flash bytes. */
    uint32_t size;      /**< Exposed partition size in bytes. */
    uint8_t *cache;     /**< Caller-owned persistent erase-block buffer. */
    size_t cache_size;  /**< Cache capacity in bytes. */
    uint32_t cached_block; /**< Cached erase-block offset from disk start. */
    bool cache_valid;   /**< Cached block contains data. */
    bool cache_dirty;   /**< Cached data awaits a flash write. */
    bool initialized;   /**< true after successful init. */
} ota_spi_disk_t;

/** @brief Initialize a flash-backed block device.
 * @param[out] disk Block device to initialize.
 * @param flash Initialized SPI NOR instance.
 * @param offset Partition start address in flash bytes.
 * @param size Partition size in bytes.
 * @param[out] cache Persistent erase-block cache storage.
 * @param cache_size Cache capacity in bytes.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t ota_spi_disk_init(ota_spi_disk_t *disk, spi_flash_t *flash,
                                uint32_t offset, uint32_t size,
                                uint8_t *cache, size_t cache_size);
/** @brief Bind this disk to the FatFs block-device adapter.
 * @param disk Initialized block device.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t ota_spi_disk_bind(ota_spi_disk_t *disk);
/** @brief Flush pending erase-block cache writes to flash.
 * @param disk Initialized block device.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t ota_spi_disk_sync(ota_spi_disk_t *disk);
/** @brief Read bytes relative to the disk partition start.
 * @param disk Initialized block device.
 * @param offset Relative byte offset.
 * @param[out] buffer Destination buffer.
 * @param size Number of bytes to read.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t ota_spi_disk_read(ota_spi_disk_t *disk, uint32_t offset,
                                void *buffer, size_t size);
/** @brief Write bytes relative to the disk partition start.
 * @param disk Initialized block device.
 * @param offset Relative byte offset.
 * @param buffer Bytes to write.
 * @param size Number of bytes to write.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t ota_spi_disk_write(ota_spi_disk_t *disk, uint32_t offset,
                                 const void *buffer, size_t size);
/** @brief Query the disk's logical sector count.
 * @param disk Initialized block device.
 * @return Number of OTA_DISK_SECTOR_SIZE-byte sectors.
 */
uint32_t ota_spi_disk_sector_count(const ota_spi_disk_t *disk);

#ifdef __cplusplus
}
#endif

#endif
