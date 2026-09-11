#include <ota_staging.h>

#include <stddef.h>
#include <string.h>

#define OTA_STAGING_METADATA_MAGIC UINT32_C(0x4255574D)

typedef struct {
    uint32_t magic;
    uint32_t package_size;
    uint32_t crc32;
} ota_staging_metadata_t;

static bool range_valid(uint32_t offset, uint32_t size, uint32_t capacity)
{
    return (offset <= capacity) && (size <= (capacity - offset));
}

static bool ranges_overlap(uint32_t first_offset, uint32_t first_size,
                           uint32_t second_offset, uint32_t second_size)
{
    return (first_offset < (second_offset + second_size)) &&
           (second_offset < (first_offset + first_size));
}

chip_status_t ota_staging_init(ota_staging_t *staging,
                               const ota_staging_config_t *config)
{
    uint32_t flash_capacity;

    if ((staging == NULL) || (config == NULL) || (config->flash == NULL) ||
        !config->flash->initialized || (config->erase_size == 0U) ||
        (config->erase_size != config->flash->config.sector_size) ||
        (config->image_capacity < OTA_IMAGE_HEADER_SIZE) ||
        (config->application_size == 0U) ||
        ((config->metadata_offset % config->erase_size) != 0U) ||
        ((config->image_offset % config->erase_size) != 0U) ||
        ((config->image_capacity % config->erase_size) != 0U) ||
        (sizeof(ota_staging_metadata_t) > config->erase_size) ||
        (config->application_size >
         (config->image_capacity - OTA_IMAGE_HEADER_SIZE))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    flash_capacity = config->flash->config.capacity_bytes;
    if (!range_valid(config->metadata_offset, config->erase_size,
                     flash_capacity) ||
        !range_valid(config->image_offset, config->image_capacity,
                     flash_capacity) ||
        ranges_overlap(config->metadata_offset, config->erase_size,
                       config->image_offset, config->image_capacity)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    memset(staging, 0, sizeof(*staging));
    staging->config = *config;
    staging->initialized = true;
    return CHIP_OK;
}

chip_status_t ota_staging_clear(ota_staging_t *staging)
{
    if ((staging == NULL) || !staging->initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    return spi_flash_erase_sector(staging->config.flash,
                                  staging->config.metadata_offset);
}

chip_status_t ota_staging_commit(ota_staging_t *staging,
                                 uint32_t package_size)
{
    ota_staging_metadata_t metadata = {
        .magic = OTA_STAGING_METADATA_MAGIC,
        .package_size = package_size,
        .crc32 = 0U,
    };

    if ((staging == NULL) || !staging->initialized ||
        (package_size < OTA_IMAGE_HEADER_SIZE) ||
        (package_size > staging->config.image_capacity)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    metadata.crc32 = ota_crc32(&metadata,
                               offsetof(ota_staging_metadata_t, crc32));
    return spi_flash_program(staging->config.flash,
                             staging->config.metadata_offset,
                             &metadata, sizeof(metadata));
}

ota_image_status_t ota_staging_validate_package(
    ota_staging_t *staging, uint32_t package_size,
    ota_image_header_t *header, uint8_t *scratch, size_t scratch_size)
{
    ota_image_header_t local_header;
    ota_image_status_t status;
    uint32_t crc = ota_crc32_begin();
    uint32_t offset = 0U;
    uint32_t remaining;

    if ((staging == NULL) || !staging->initialized || (scratch == NULL) ||
        (scratch_size == 0U) || (scratch_size > UINT32_MAX) ||
        (package_size > staging->config.image_capacity)) {
        return OTA_IMAGE_IO_ERROR;
    }
    if (spi_flash_read(staging->config.flash, staging->config.image_offset,
                       &local_header, sizeof(local_header)) != CHIP_OK) {
        return OTA_IMAGE_IO_ERROR;
    }
    status = ota_image_validate_header(
        &local_header, staging->config.target_id,
        staging->config.application_address, staging->config.application_size,
        package_size);
    if (status != OTA_IMAGE_OK) {
        return status;
    }

    remaining = local_header.image_size;
    while (remaining > 0U) {
        size_t chunk = (remaining < scratch_size) ?
                       (size_t)remaining : scratch_size;

        if (spi_flash_read(staging->config.flash,
                           staging->config.image_offset +
                               local_header.header_size + offset,
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

ota_image_status_t ota_staging_validate_committed(
    ota_staging_t *staging, ota_image_header_t *header,
    uint8_t *scratch, size_t scratch_size)
{
    ota_staging_metadata_t metadata;
    uint32_t expected_crc;

    if ((staging == NULL) || !staging->initialized ||
        (spi_flash_read(staging->config.flash,
                        staging->config.metadata_offset, &metadata,
                        sizeof(metadata)) != CHIP_OK)) {
        return OTA_IMAGE_IO_ERROR;
    }
    if (metadata.magic != OTA_STAGING_METADATA_MAGIC) {
        return OTA_IMAGE_NOT_FOUND;
    }
    expected_crc = ota_crc32(&metadata,
                             offsetof(ota_staging_metadata_t, crc32));
    if (expected_crc != metadata.crc32) {
        return OTA_IMAGE_INVALID_HEADER;
    }
    return ota_staging_validate_package(staging, metadata.package_size,
                                        header, scratch, scratch_size);
}

chip_status_t ota_staging_read_payload(ota_staging_t *staging,
                                       uint32_t payload_offset, void *data,
                                       size_t size)
{
    if ((staging == NULL) || !staging->initialized ||
        ((data == NULL) && (size != 0U)) ||
        (payload_offset > staging->config.application_size) ||
        (size > (size_t)(staging->config.application_size - payload_offset))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    return spi_flash_read(staging->config.flash,
                          staging->config.image_offset +
                              OTA_IMAGE_HEADER_SIZE + payload_offset,
                          data, size);
}
