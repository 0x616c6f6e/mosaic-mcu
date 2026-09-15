#ifndef CH585_KEYBOARD_BOARD_H
#define CH585_KEYBOARD_BOARD_H

#include <stddef.h>
#include <stdint.h>

#include <ota_image.h>
#include <ota_spi_disk.h>
#include <ota_staging.h>
#include <spi_flash.h>

/** @brief Target identifier embedded in GH64 OTA headers. */
#define OTA_TARGET_ID              UINT32_C(0x00000585)
/** @brief Program-Flash area reserved for the recovery bootloader in bytes. */
#define OTA_BOOTLOADER_SIZE        UINT32_C(0x00010000)
/** @brief Program-Flash start address of the application. */
#define OTA_APPLICATION_ADDRESS    OTA_BOOTLOADER_SIZE
/** @brief Maximum application image size in bytes. */
#define OTA_APPLICATION_SIZE       UINT32_C(0x00060000)
/** @brief FAT path of a disk-hosted update package. */
#define OTA_UPDATE_PATH            "0:/FIRMWARE.OTA"
/** @brief Data-Flash offset of the in-progress install marker. */
#define OTA_INSTALL_MARKER_OFFSET  UINT32_C(0x00007F00)
/** @brief Install marker length in bytes. */
#define OTA_INSTALL_MARKER_SIZE    UINT32_C(256)
/** @brief MSC inactivity required before checking for an update, in ms. */
#define OTA_USB_IDLE_TIMEOUT_MS    UINT32_C(2000)

/** @brief SPI NOR sector erase size in bytes. */
#define OTA_SPI_FLASH_SECTOR_SIZE  UINT32_C(4096)
/** @brief Conservative default SPI NOR clock frequency in Hz. */
#define OTA_SPI_FLASH_CLOCK_HZ      UINT32_C(500000)

/** @brief Reserve this many bytes at the end of detected SPI NOR for WebUSB.
 * @note The remaining capacity is exposed as the FAT/MSC disk.
 */
#define OTA_WEBUSB_STAGING_SIZE    UINT32_C(0x00080000)
/** @brief WebUSB package capacity after reserving one metadata sector. */
#define OTA_WEBUSB_IMAGE_CAPACITY  \
    (OTA_WEBUSB_STAGING_SIZE - OTA_SPI_FLASH_SECTOR_SIZE)

_Static_assert((OTA_IMAGE_HEADER_SIZE + OTA_APPLICATION_SIZE) <=
                   OTA_WEBUSB_IMAGE_CAPACITY,
               "WebUSB staging area is too small for an OTA package");

/** @brief Initialize the board, detect SPI NOR capacity, and bind the FAT disk.
 * @param[out] flash Board SPI NOR instance.
 * @param[out] disk FAT/MSC disk occupying flash outside WebUSB staging.
 * @param[out] disk_cache Persistent erase-block cache storage.
 * @param disk_cache_size Cache capacity in bytes.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t keyboard_board_init(spi_flash_t *flash, ota_spi_disk_t *disk,
                                  uint8_t *disk_cache,
                                  size_t disk_cache_size);
/** @brief Confirm the connected flash matches its resolved JEDEC capacity.
 * @param flash Initialized flash instance.
 * @param[out] id Receives the JEDEC identification bytes.
 * @return CHIP_OK if matched, or a CHIP_ERROR_* status.
 */
chip_status_t keyboard_board_probe_flash(spi_flash_t *flash,
                                         spi_flash_jedec_id_t *id);
/** @brief Place WebUSB staging at the end of detected SPI NOR capacity.
 * @param[out] staging Staging area to initialize.
 * @param flash Initialized flash instance with resolved capacity.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t keyboard_board_init_staging(ota_staging_t *staging,
                                          spi_flash_t *flash);

#endif
