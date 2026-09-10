#include <ota_webusb_staging.h>

#include <stdio.h>
#include <string.h>

#include "ota_board.h"

static uint8_t flash_memory[OTA_SPI_FLASH_CAPACITY];

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,          \
                    __LINE__, #condition);                                     \
            return 1;                                                          \
        }                                                                      \
    } while (0)

chip_status_t spi_flash_read(spi_flash_t *flash, uint32_t address,
                             void *data, size_t size)
{
    (void)flash;
    if ((address > sizeof(flash_memory)) ||
        (size > (sizeof(flash_memory) - address))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    memcpy(data, &flash_memory[address], size);
    return CHIP_OK;
}

chip_status_t spi_flash_program(spi_flash_t *flash, uint32_t address,
                                const void *data, size_t size)
{
    const uint8_t *source = data;
    size_t index;

    (void)flash;
    if ((address > sizeof(flash_memory)) ||
        (size > (sizeof(flash_memory) - address))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    for (index = 0U; index < size; ++index) {
        flash_memory[address + index] &= source[index];
    }
    return CHIP_OK;
}

chip_status_t spi_flash_erase_sector(spi_flash_t *flash, uint32_t address)
{
    (void)flash;
    if (((address % OTA_SPI_FLASH_SECTOR_SIZE) != 0U) ||
        (address > (sizeof(flash_memory) - OTA_SPI_FLASH_SECTOR_SIZE))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    memset(&flash_memory[address], 0xFF, OTA_SPI_FLASH_SECTOR_SIZE);
    return CHIP_OK;
}

int main(void)
{
    static const uint8_t payload[] = "validated WebUSB OTA payload";
    ota_image_header_t source_header = {
        .magic = OTA_IMAGE_MAGIC,
        .format_version = OTA_IMAGE_FORMAT_VERSION,
        .header_size = OTA_IMAGE_HEADER_SIZE,
        .target_id = OTA_TARGET_ID,
        .load_address = OTA_APPLICATION_ADDRESS,
        .image_size = sizeof(payload),
        .image_crc32 = 0U,
        .firmware_version = 9U,
        .header_crc32 = 0U,
    };
    ota_image_header_t result_header;
    spi_flash_t flash = {0};
    uint8_t scratch[11];
    uint32_t package_size = OTA_IMAGE_HEADER_SIZE + sizeof(payload);

    memset(flash_memory, 0xFF, sizeof(flash_memory));
    source_header.image_crc32 = ota_crc32(payload, sizeof(payload));
    source_header.header_crc32 = ota_crc32(
        &source_header, offsetof(ota_image_header_t, header_crc32));
    memcpy(&flash_memory[OTA_WEBUSB_IMAGE_OFFSET], &source_header,
           sizeof(source_header));
    memcpy(&flash_memory[OTA_WEBUSB_IMAGE_OFFSET + OTA_IMAGE_HEADER_SIZE],
           payload, sizeof(payload));

    CHECK(ota_webusb_staging_validate_committed(
              &flash, &result_header, scratch, sizeof(scratch)) ==
          OTA_IMAGE_NOT_FOUND);
    CHECK(ota_webusb_staging_validate_package(
              &flash, package_size, &result_header, scratch,
              sizeof(scratch)) == OTA_IMAGE_OK);
    CHECK(result_header.firmware_version == 9U);
    CHECK(ota_webusb_staging_commit(&flash, package_size) == CHIP_OK);
    CHECK(ota_webusb_staging_validate_committed(
              &flash, &result_header, scratch, sizeof(scratch)) ==
          OTA_IMAGE_OK);

    flash_memory[OTA_WEBUSB_IMAGE_OFFSET + OTA_IMAGE_HEADER_SIZE] ^= 1U;
    CHECK(ota_webusb_staging_validate_committed(
              &flash, NULL, scratch, sizeof(scratch)) ==
          OTA_IMAGE_INVALID_CRC);
    CHECK(ota_webusb_staging_clear(&flash) == CHIP_OK);
    CHECK(ota_webusb_staging_validate_committed(
              &flash, NULL, scratch, sizeof(scratch)) ==
          OTA_IMAGE_NOT_FOUND);
    return 0;
}
