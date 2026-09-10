#include "ota_webusb_staging.h"

#include <limits.h>
#include <stddef.h>

#include "ota_board.h"

#define OTA_WEBUSB_METADATA_MAGIC UINT32_C(0x4255574D)

typedef struct {
    uint32_t magic;
    uint32_t package_size;
    uint32_t crc32;
} ota_webusb_metadata_t;

_Static_assert(sizeof(ota_webusb_metadata_t) <= OTA_SPI_FLASH_SECTOR_SIZE,
               "WebUSB metadata must fit in one sector");

chip_status_t ota_webusb_staging_clear(spi_flash_t *flash)
{
    return spi_flash_erase_sector(flash, OTA_WEBUSB_METADATA_OFFSET);
}

chip_status_t ota_webusb_staging_commit(spi_flash_t *flash,
                                        uint32_t package_size)
{
    ota_webusb_metadata_t metadata = {
        .magic = OTA_WEBUSB_METADATA_MAGIC,
        .package_size = package_size,
        .crc32 = 0U,
    };

    metadata.crc32 = ota_crc32(&metadata,
                               offsetof(ota_webusb_metadata_t, crc32));
    return spi_flash_program(flash, OTA_WEBUSB_METADATA_OFFSET,
                             &metadata, sizeof(metadata));
}

ota_image_status_t ota_webusb_staging_validate_package(
    spi_flash_t *flash, uint32_t package_size, ota_image_header_t *header,
    uint8_t *scratch, size_t scratch_size)
{
    ota_image_header_t local_header;
    ota_image_status_t status;
    uint32_t crc = ota_crc32_begin();
    uint32_t offset = 0U;
    uint32_t remaining;

    if ((flash == NULL) || (scratch == NULL) || (scratch_size == 0U) ||
        (package_size > OTA_WEBUSB_IMAGE_CAPACITY)) {
        return OTA_IMAGE_IO_ERROR;
    }
    if (spi_flash_read(flash, OTA_WEBUSB_IMAGE_OFFSET, &local_header,
                       sizeof(local_header)) != CHIP_OK) {
        return OTA_IMAGE_IO_ERROR;
    }
    status = ota_image_validate_header(&local_header, OTA_TARGET_ID,
                                       OTA_APPLICATION_ADDRESS,
                                       OTA_APPLICATION_SIZE, package_size);
    if (status != OTA_IMAGE_OK) {
        return status;
    }

    remaining = local_header.image_size;
    while (remaining > 0U) {
        size_t chunk = (remaining < scratch_size) ?
                       (size_t)remaining : scratch_size;

        if (spi_flash_read(flash,
                           OTA_WEBUSB_IMAGE_OFFSET + local_header.header_size +
                               offset,
                           scratch, chunk) != CHIP_OK) {
            return OTA_IMAGE_IO_ERROR;
        }
        crc = ota_crc32_update(crc, scratch, chunk);
        offset += (uint32_t)chunk;
        remaining -= (uint32_t)chunk;
    }
    if (ota_crc32_end(crc) != local_header.image_crc32) {
        return OTA_IMAGE_INVALID_CRC;
    }
    if (header != NULL) {
        *header = local_header;
    }
    return OTA_IMAGE_OK;
}

ota_image_status_t ota_webusb_staging_validate_committed(
    spi_flash_t *flash, ota_image_header_t *header,
    uint8_t *scratch, size_t scratch_size)
{
    ota_webusb_metadata_t metadata;
    uint32_t expected_crc;

    if ((flash == NULL) ||
        (spi_flash_read(flash, OTA_WEBUSB_METADATA_OFFSET, &metadata,
                        sizeof(metadata)) != CHIP_OK)) {
        return OTA_IMAGE_IO_ERROR;
    }
    if (metadata.magic != OTA_WEBUSB_METADATA_MAGIC) {
        return OTA_IMAGE_NOT_FOUND;
    }
    expected_crc = ota_crc32(&metadata,
                             offsetof(ota_webusb_metadata_t, crc32));
    if (expected_crc != metadata.crc32) {
        return OTA_IMAGE_INVALID_HEADER;
    }
    return ota_webusb_staging_validate_package(flash, metadata.package_size,
                                                header, scratch,
                                                scratch_size);
}

chip_status_t ota_webusb_staging_read_payload(
    spi_flash_t *flash, uint32_t payload_offset, void *data, size_t size)
{
    if ((payload_offset > OTA_APPLICATION_SIZE) ||
        (size > (size_t)(OTA_APPLICATION_SIZE - payload_offset))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    return spi_flash_read(flash,
                          OTA_WEBUSB_IMAGE_OFFSET + OTA_IMAGE_HEADER_SIZE +
                              payload_offset,
                          data, size);
}
