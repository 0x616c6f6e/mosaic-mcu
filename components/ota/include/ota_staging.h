#ifndef PLATFORM_OTA_STAGING_H
#define PLATFORM_OTA_STAGING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ota_image.h>
#include <spi_flash.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    spi_flash_t *flash;
    uint32_t metadata_offset;
    uint32_t image_offset;
    uint32_t image_capacity;
    uint32_t erase_size;
    uint32_t target_id;
    uint32_t application_address;
    uint32_t application_size;
} ota_staging_config_t;

typedef struct {
    ota_staging_config_t config;
    bool initialized;
} ota_staging_t;

chip_status_t ota_staging_init(ota_staging_t *staging,
                               const ota_staging_config_t *config);
chip_status_t ota_staging_clear(ota_staging_t *staging);
chip_status_t ota_staging_commit(ota_staging_t *staging,
                                 uint32_t package_size);
ota_image_status_t ota_staging_validate_package(
    ota_staging_t *staging, uint32_t package_size,
    ota_image_header_t *header, uint8_t *scratch, size_t scratch_size);
ota_image_status_t ota_staging_validate_committed(
    ota_staging_t *staging, ota_image_header_t *header,
    uint8_t *scratch, size_t scratch_size);
chip_status_t ota_staging_read_payload(ota_staging_t *staging,
                                       uint32_t payload_offset, void *data,
                                       size_t size);

#ifdef __cplusplus
}
#endif

#endif
