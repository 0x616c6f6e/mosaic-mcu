#ifndef CH585_OTA_BOARD_H
#define CH585_OTA_BOARD_H

#include <stddef.h>
#include <stdint.h>

#include <ota_image.h>
#include <ota_spi_disk.h>
#include <spi_flash.h>

#define OTA_TARGET_ID              UINT32_C(0x00000585)
#define OTA_BOOTLOADER_SIZE        UINT32_C(0x00010000)
#define OTA_APPLICATION_ADDRESS    OTA_BOOTLOADER_SIZE
#define OTA_APPLICATION_SIZE       UINT32_C(0x00060000)
#define OTA_UPDATE_PATH            "0:/FIRMWARE.OTA"
#define OTA_INSTALL_MARKER_OFFSET  UINT32_C(0x00007F00)
#define OTA_INSTALL_MARKER_SIZE    UINT32_C(256)
#define OTA_USB_IDLE_TIMEOUT_MS    UINT32_C(2000)

#define OTA_SPI_FLASH_CAPACITY     UINT32_C(0x00800000)
#define OTA_SPI_FLASH_SECTOR_SIZE  UINT32_C(4096)

/* Keep WebUSB staging outside the FAT/MSC address range. */
#define OTA_WEBUSB_STAGING_SIZE    UINT32_C(0x00080000)
#define OTA_MSC_FLASH_CAPACITY     \
    (OTA_SPI_FLASH_CAPACITY - OTA_WEBUSB_STAGING_SIZE)
#define OTA_WEBUSB_METADATA_OFFSET OTA_MSC_FLASH_CAPACITY
#define OTA_WEBUSB_IMAGE_OFFSET    \
    (OTA_WEBUSB_METADATA_OFFSET + OTA_SPI_FLASH_SECTOR_SIZE)
#define OTA_WEBUSB_IMAGE_CAPACITY  \
    (OTA_WEBUSB_STAGING_SIZE - OTA_SPI_FLASH_SECTOR_SIZE)

_Static_assert((OTA_MSC_FLASH_CAPACITY % OTA_SPI_FLASH_SECTOR_SIZE) == 0U,
               "MSC capacity must be sector aligned");
_Static_assert((OTA_IMAGE_HEADER_SIZE + OTA_APPLICATION_SIZE) <=
                   OTA_WEBUSB_IMAGE_CAPACITY,
               "WebUSB staging area is too small for an OTA package");

chip_status_t ota_board_init(spi_flash_t *flash, ota_spi_disk_t *disk,
                             uint8_t *disk_cache, size_t disk_cache_size);

#endif
