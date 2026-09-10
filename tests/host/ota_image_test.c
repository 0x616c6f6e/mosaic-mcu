#include <ota_image.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,          \
                    __LINE__, #condition);                                     \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static ota_image_header_t valid_header(void)
{
    ota_image_header_t header = {
        .magic = OTA_IMAGE_MAGIC,
        .format_version = OTA_IMAGE_FORMAT_VERSION,
        .header_size = OTA_IMAGE_HEADER_SIZE,
        .target_id = UINT32_C(0x585),
        .load_address = UINT32_C(0x10000),
        .image_size = UINT32_C(1024),
        .image_crc32 = UINT32_C(0x12345678),
        .firmware_version = UINT32_C(7),
        .header_crc32 = 0U,
    };

    header.header_crc32 = ota_crc32(
        &header, offsetof(ota_image_header_t, header_crc32));
    return header;
}

int main(void)
{
    static const char crc_input[] = "123456789";
    ota_image_header_t header = valid_header();
    uint32_t file_size = header.header_size + header.image_size;

    CHECK(ota_crc32(crc_input, strlen(crc_input)) == UINT32_C(0xCBF43926));
    CHECK(ota_image_validate_header(&header, UINT32_C(0x585),
                                    UINT32_C(0x10000), UINT32_C(0x60000),
                                    file_size) == OTA_IMAGE_OK);

    header.target_id = UINT32_C(0x584);
    header.header_crc32 = ota_crc32(
        &header, offsetof(ota_image_header_t, header_crc32));
    CHECK(ota_image_validate_header(&header, UINT32_C(0x585),
                                    UINT32_C(0x10000), UINT32_C(0x60000),
                                    file_size) == OTA_IMAGE_WRONG_TARGET);

    header = valid_header();
    header.load_address = UINT32_C(0x20000);
    header.header_crc32 = ota_crc32(
        &header, offsetof(ota_image_header_t, header_crc32));
    CHECK(ota_image_validate_header(&header, UINT32_C(0x585),
                                    UINT32_C(0x10000), UINT32_C(0x60000),
                                    file_size) == OTA_IMAGE_WRONG_ADDRESS);

    header = valid_header();
    CHECK(ota_image_validate_header(&header, UINT32_C(0x585),
                                    UINT32_C(0x10000), UINT32_C(0x60000),
                                    file_size - 1U) == OTA_IMAGE_INVALID_SIZE);

    header = valid_header();
    header.header_crc32 ^= UINT32_C(1);
    CHECK(ota_image_validate_header(&header, UINT32_C(0x585),
                                    UINT32_C(0x10000), UINT32_C(0x60000),
                                    file_size) == OTA_IMAGE_INVALID_HEADER);
    return 0;
}
