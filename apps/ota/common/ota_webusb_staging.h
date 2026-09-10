#ifndef CH585_OTA_WEBUSB_STAGING_H
#define CH585_OTA_WEBUSB_STAGING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ota_image.h>
#include <spi_flash.h>

chip_status_t ota_webusb_staging_clear(spi_flash_t *flash);
chip_status_t ota_webusb_staging_commit(spi_flash_t *flash,
                                        uint32_t package_size);

ota_image_status_t ota_webusb_staging_validate_package(
    spi_flash_t *flash, uint32_t package_size, ota_image_header_t *header,
    uint8_t *scratch, size_t scratch_size);
ota_image_status_t ota_webusb_staging_validate_committed(
    spi_flash_t *flash, ota_image_header_t *header,
    uint8_t *scratch, size_t scratch_size);

chip_status_t ota_webusb_staging_read_payload(
    spi_flash_t *flash, uint32_t payload_offset, void *data, size_t size);

#endif
