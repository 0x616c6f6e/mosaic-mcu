#include <ota_image.h>

#include <limits.h>
#include <string.h>

_Static_assert(sizeof(ota_image_header_t) == OTA_IMAGE_HEADER_SIZE,
               "OTA image header layout changed");

uint32_t ota_crc32_begin(void)
{
    return UINT32_C(0xFFFFFFFF);
}

uint32_t ota_crc32_update(uint32_t crc, const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    size_t index;

    for (index = 0U; index < size; ++index) {
        uint32_t bit;

        crc ^= bytes[index];
        for (bit = 0U; bit < 8U; ++bit) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & UINT32_C(1));

            crc = (crc >> 1U) ^ (UINT32_C(0xEDB88320) & mask);
        }
    }
    return crc;
}

uint32_t ota_crc32_end(uint32_t crc)
{
    return crc ^ UINT32_C(0xFFFFFFFF);
}

uint32_t ota_crc32(const void *data, size_t size)
{
    return ota_crc32_end(ota_crc32_update(ota_crc32_begin(), data, size));
}

ota_image_status_t ota_image_validate_header(
    const ota_image_header_t *header, uint32_t expected_target_id,
    uint32_t expected_load_address, uint32_t maximum_image_size,
    uint32_t file_size)
{
    uint32_t header_crc;

    if ((header == NULL) || (header->magic != OTA_IMAGE_MAGIC) ||
        (header->format_version != OTA_IMAGE_FORMAT_VERSION) ||
        (header->header_size != OTA_IMAGE_HEADER_SIZE)) {
        return OTA_IMAGE_INVALID_HEADER;
    }
    header_crc = ota_crc32(header, offsetof(ota_image_header_t, header_crc32));
    if (header_crc != header->header_crc32) {
        return OTA_IMAGE_INVALID_HEADER;
    }
    if (header->target_id != expected_target_id) {
        return OTA_IMAGE_WRONG_TARGET;
    }
    if (header->load_address != expected_load_address) {
        return OTA_IMAGE_WRONG_ADDRESS;
    }
    if ((header->image_size == 0U) ||
        (header->image_size > maximum_image_size) ||
        (header->image_size > (UINT32_MAX - header->header_size)) ||
        (file_size != (header->header_size + header->image_size))) {
        return OTA_IMAGE_INVALID_SIZE;
    }
    return OTA_IMAGE_OK;
}

ota_image_status_t ota_image_validate_file(
    const char *path, uint32_t expected_target_id,
    uint32_t expected_load_address, uint32_t maximum_image_size,
    ota_image_header_t *header, uint8_t *scratch, size_t scratch_size)
{
    ota_image_header_t local_header;
    ota_image_status_t status;
    uint32_t crc = ota_crc32_begin();
    uint32_t remaining;
    FIL file;
    UINT transferred;
    FRESULT result;

    if ((path == NULL) || (scratch == NULL) || (scratch_size == 0U) ||
        (scratch_size > UINT_MAX)) {
        return OTA_IMAGE_IO_ERROR;
    }
    result = f_open(&file, path, FA_READ);
    if (result == FR_NO_FILE || result == FR_NO_PATH) {
        return OTA_IMAGE_NOT_FOUND;
    }
    if (result != FR_OK) {
        return OTA_IMAGE_IO_ERROR;
    }
    result = f_read(&file, &local_header, sizeof(local_header), &transferred);
    if ((result != FR_OK) || (transferred != sizeof(local_header))) {
        (void)f_close(&file);
        return OTA_IMAGE_INVALID_HEADER;
    }
    if (f_size(&file) > UINT32_MAX) {
        (void)f_close(&file);
        return OTA_IMAGE_INVALID_SIZE;
    }
    status = ota_image_validate_header(&local_header, expected_target_id,
                                       expected_load_address,
                                       maximum_image_size,
                                       (uint32_t)f_size(&file));
    if (status != OTA_IMAGE_OK) {
        (void)f_close(&file);
        return status;
    }

    remaining = local_header.image_size;
    while (remaining > 0U) {
        UINT chunk = (remaining < scratch_size) ? (UINT)remaining :
                                                   (UINT)scratch_size;

        result = f_read(&file, scratch, chunk, &transferred);
        if ((result != FR_OK) || (transferred != chunk)) {
            (void)f_close(&file);
            return OTA_IMAGE_IO_ERROR;
        }
        crc = ota_crc32_update(crc, scratch, chunk);
        remaining -= chunk;
    }
    if (f_close(&file) != FR_OK) {
        return OTA_IMAGE_IO_ERROR;
    }
    if (ota_crc32_end(crc) != local_header.image_crc32) {
        return OTA_IMAGE_INVALID_CRC;
    }
    if (header != NULL) {
        *header = local_header;
    }
    return OTA_IMAGE_OK;
}
