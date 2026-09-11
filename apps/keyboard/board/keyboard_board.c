#include "keyboard_board.h"

#include <chip_gpio.h>
#include <chip_spi.h>
#include <chip_system.h>
#include <chip_time.h>

#define OTA_SYSTEM_CLOCK_HZ UINT32_C(62400000)
#define OTA_SPI_TIMEOUT_US  UINT32_C(100000)
#define OTA_PROGRAM_TIMEOUT_US UINT32_C(1000000)
#define OTA_ERASE_TIMEOUT_US   UINT32_C(3000000)
#define OTA_CHIP_ERASE_TIMEOUT_US UINT32_C(200000000)

#define OTA_SPI_INSTANCE CHIP_SPI_1
#define OTA_SPI_CS   CHIP_PIN(CHIP_GPIO_PORT_A, 3)
#define OTA_SPI_SCK  CHIP_PIN(CHIP_GPIO_PORT_A, 0)
#define OTA_SPI_MOSI CHIP_PIN(CHIP_GPIO_PORT_A, 1)
#define OTA_SPI_MISO CHIP_PIN(CHIP_GPIO_PORT_A, 2)

static chip_status_t init_spi_pins(void)
{
    const chip_gpio_config_t output = {
        .mode = CHIP_GPIO_OUTPUT_PUSH_PULL,
        .pull = CHIP_GPIO_PULL_NONE,
        .drive = CHIP_GPIO_DRIVE_HIGH,
        .initial_level = false,
    };
    const chip_gpio_config_t input = {
        .mode = CHIP_GPIO_INPUT,
        .pull = CHIP_GPIO_PULL_NONE,
        .drive = CHIP_GPIO_DRIVE_LOW,
        .initial_level = false,
    };
    const chip_gpio_config_t chip_select = {
        .mode = CHIP_GPIO_OUTPUT_PUSH_PULL,
        .pull = CHIP_GPIO_PULL_NONE,
        .drive = CHIP_GPIO_DRIVE_HIGH,
        .initial_level = true,
    };
    chip_status_t status = chip_gpio_init(OTA_SPI_CS, &chip_select);

    if (status == CHIP_OK) {
        status = chip_gpio_init(OTA_SPI_SCK, &output);
    }
    if (status == CHIP_OK) {
        status = chip_gpio_init(OTA_SPI_MOSI, &output);
    }
    if (status == CHIP_OK) {
        status = chip_gpio_init(OTA_SPI_MISO, &input);
    }
    return status;
}

chip_status_t keyboard_board_init(spi_flash_t *flash, ota_spi_disk_t *disk,
                                  uint8_t *disk_cache,
                                  size_t disk_cache_size)
{
    const chip_system_config_t system_config = {
        .source = CHIP_CLOCK_INTERNAL,
        .core_clock_hz = OTA_SYSTEM_CLOCK_HZ,
        .external_crystal_load_pf = 0U,
    };
    const spi_flash_config_t flash_config = {
        .spi = OTA_SPI_INSTANCE,
        .cs_pin = OTA_SPI_CS,
        .clock_hz = OTA_SPI_FLASH_CLOCK_HZ,
        .capacity_bytes = OTA_SPI_FLASH_CAPACITY,
        .page_size = 256U,
        .sector_size = OTA_SPI_FLASH_SECTOR_SIZE,
        .transfer_timeout_us = OTA_SPI_TIMEOUT_US,
        .program_timeout_us = OTA_PROGRAM_TIMEOUT_US,
        .sector_erase_timeout_us = OTA_ERASE_TIMEOUT_US,
        .chip_erase_timeout_us = OTA_CHIP_ERASE_TIMEOUT_US,
    };
    chip_status_t status = chip_system_init(&system_config);

    if (status == CHIP_OK) {
        status = chip_time_init();
    }
    if (status == CHIP_OK) {
        status = init_spi_pins();
    }
    if (status == CHIP_OK) {
        status = spi_flash_init(flash, &flash_config);
    }
    if (status == CHIP_OK) {
        status = ota_spi_disk_init(disk, flash, 0U,
                                   OTA_MSC_FLASH_CAPACITY,
                                   disk_cache, disk_cache_size);
    }
    if (status == CHIP_OK) {
        status = ota_spi_disk_bind(disk);
    }
    return status;
}

chip_status_t keyboard_board_probe_flash(spi_flash_t *flash,
                                         spi_flash_jedec_id_t *id)
{
    chip_status_t status;

    if ((flash == NULL) || (id == NULL)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    status = spi_flash_read_jedec_id(flash, id);
    if (status != CHIP_OK) {
        return status;
    }
    if ((((id->manufacturer == 0U) && (id->memory_type == 0U) &&
          (id->capacity == 0U))) ||
        ((id->manufacturer == UINT8_C(0xFF)) &&
         (id->memory_type == UINT8_C(0xFF)) &&
         (id->capacity == UINT8_C(0xFF))) ||
        (id->capacity >= 32U) ||
        ((UINT32_C(1) << id->capacity) != flash->config.capacity_bytes)) {
        return CHIP_ERROR_IO;
    }
    return CHIP_OK;
}

chip_status_t keyboard_board_init_staging(ota_staging_t *staging,
                                          spi_flash_t *flash)
{
    const ota_staging_config_t config = {
        .flash = flash,
        .metadata_offset = OTA_WEBUSB_METADATA_OFFSET,
        .image_offset = OTA_WEBUSB_IMAGE_OFFSET,
        .image_capacity = OTA_WEBUSB_IMAGE_CAPACITY,
        .erase_size = OTA_SPI_FLASH_SECTOR_SIZE,
        .target_id = OTA_TARGET_ID,
        .application_address = OTA_APPLICATION_ADDRESS,
        .application_size = OTA_APPLICATION_SIZE,
    };

    return ota_staging_init(staging, &config);
}
