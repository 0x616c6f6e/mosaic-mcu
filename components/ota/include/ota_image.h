#ifndef PLATFORM_OTA_IMAGE_H
#define PLATFORM_OTA_IMAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <platform_fatfs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_IMAGE_MAGIC          UINT32_C(0x41544F4D)
#define OTA_IMAGE_FORMAT_VERSION UINT32_C(1)
#define OTA_IMAGE_HEADER_SIZE    UINT32_C(36)

typedef struct {
    uint32_t magic;
    uint32_t format_version;
    uint32_t header_size;
    uint32_t target_id;
    uint32_t load_address;
    uint32_t image_size;
    uint32_t image_crc32;
    uint32_t firmware_version;
    uint32_t header_crc32;
} ota_image_header_t;

typedef enum {
    OTA_IMAGE_OK = 0,
    OTA_IMAGE_NOT_FOUND,
    OTA_IMAGE_IO_ERROR,
    OTA_IMAGE_INVALID_HEADER,
    OTA_IMAGE_WRONG_TARGET,
    OTA_IMAGE_WRONG_ADDRESS,
    OTA_IMAGE_INVALID_SIZE,
    OTA_IMAGE_INVALID_CRC,
} ota_image_status_t;

uint32_t ota_crc32_begin(void);
uint32_t ota_crc32_update(uint32_t crc, const void *data, size_t size);
uint32_t ota_crc32_end(uint32_t crc);
uint32_t ota_crc32(const void *data, size_t size);

ota_image_status_t ota_image_validate_header(
    const ota_image_header_t *header, uint32_t expected_target_id,
    uint32_t expected_load_address, uint32_t maximum_image_size,
    uint32_t file_size);

ota_image_status_t ota_image_validate_file(
    const char *path, uint32_t expected_target_id,
    uint32_t expected_load_address, uint32_t maximum_image_size,
    ota_image_header_t *header, uint8_t *scratch, size_t scratch_size);

#ifdef __cplusplus
}
#endif

#endif
