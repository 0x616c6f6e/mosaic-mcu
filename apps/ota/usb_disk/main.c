#include <stdbool.h>
#include <stdint.h>

#include <chip_system.h>
#include <chip_time.h>
#include <ota_image.h>
#include <ota_spi_disk.h>
#include <platform_fatfs.h>
#include <platform_log.h>
#include <spi_flash.h>

#include "tusb.h"

#include "ota_board.h"
#include "ota_log.h"
#include "ota_system_config.h"
#include "ota_usb_device.h"
#include "ota_usb_msc.h"
#include "ota_webusb.h"

static spi_flash_t external_flash;
static ota_spi_disk_t update_disk;
static uint8_t disk_cache[OTA_SPI_FLASH_SECTOR_SIZE];
static uint8_t validation_buffer[512];
static FATFS filesystem;
static ota_system_config_t system_config;
static bool webusb_initialized;

uint32_t tusb_time_millis_api(void)
{
    return chip_time_millis();
}

static ota_image_status_t validate_update_file(ota_image_header_t *header)
{
    ota_image_status_t status = OTA_IMAGE_IO_ERROR;

    if (f_mount(&filesystem, "0:", 1U) == FR_OK) {
        status = ota_image_validate_file(OTA_UPDATE_PATH, OTA_TARGET_ID,
                                         OTA_APPLICATION_ADDRESS,
                                         OTA_APPLICATION_SIZE, header,
                                         validation_buffer,
                                         sizeof(validation_buffer));
    }
    (void)f_unmount("0:");
    return status;
}

static void apply_system_config(const ota_system_config_t *config)
{
    system_config = *config;
    ota_usb_set_product_name(system_config.device_name);
    if (webusb_initialized) {
        ota_webusb_set_reboot_delay(system_config.webusb_reboot_delay_ms);
    }
    (void)platform_log_set_level(system_config.log_level);
    LOG_INFO("config", "applied name=%s idle_ms=%lu reboot_ms=%lu",
             system_config.device_name,
             (unsigned long)system_config.usb_idle_timeout_ms,
             (unsigned long)system_config.webusb_reboot_delay_ms);
}

static void load_system_config(void)
{
    ota_system_config_t loaded;
    ota_system_config_status_t config_status =
        ota_system_config_load(&loaded);

    if ((config_status == OTA_SYSTEM_CONFIG_OK) ||
        (config_status == OTA_SYSTEM_CONFIG_CREATED)) {
        if (config_status == OTA_SYSTEM_CONFIG_CREATED) {
            LOG_INFO("config", "created default %s", OTA_SYSTEM_CONFIG_PATH);
        }
        apply_system_config(&loaded);
    } else {
        LOG_ERROR("config", "keeping current settings status=%s",
                  ota_system_config_status_name(config_status));
    }
}

int main(void)
{
    const tusb_rhport_init_t usb_config = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };
    chip_status_t status;
    spi_flash_jedec_id_t flash_id;

    status = ota_log_init();
    if (status != CHIP_OK) {
        for (;;) {
        }
    }
    LOG_INFO("ota", "#################\r\n");
    LOG_INFO("ota", "application entry address=0x%08lX",
             (unsigned long)OTA_APPLICATION_ADDRESS);
    (void)ota_log_flush();
    status = ota_board_init(&external_flash, &update_disk, disk_cache,
                            sizeof(disk_cache));
    if (status != CHIP_OK) {
        LOG_ERROR("ota", "board initialization failed status=%u",
                  (unsigned int)status);
        for (;;) {
        }
    }
    status = spi_flash_read_jedec_id(&external_flash, &flash_id);
    if (status == CHIP_OK) {
        LOG_INFO("ota", "SPI Flash JEDEC=%02X:%02X:%02X",
                 (unsigned int)flash_id.manufacturer,
                 (unsigned int)flash_id.memory_type,
                 (unsigned int)flash_id.capacity);
    } else {
        LOG_ERROR("ota", "SPI Flash JEDEC read failed status=%u",
                  (unsigned int)status);
    }
    ota_system_config_defaults(&system_config);
    load_system_config();
    (void)ota_spi_disk_sync(&update_disk);
    LOG_INFO("ota", "USB update application started");
    ota_usb_msc_init(&update_disk);
    ota_webusb_init(&external_flash);
    webusb_initialized = true;
    ota_webusb_set_reboot_delay(system_config.webusb_reboot_delay_ms);
    if (!tusb_init(0U, &usb_config)) {
        LOG_ERROR("ota", "TinyUSB initialization failed");
        for (;;) {
        }
    }
    LOG_INFO("ota", "waiting for %s", OTA_UPDATE_PATH);

    for (;;) {
        tud_task();
        ota_webusb_task();
        if (ota_webusb_should_reboot(chip_time_millis())) {
            (void)ota_spi_disk_sync(&update_disk);
            LOG_INFO("ota", "WebUSB update ready, resetting");
            (void)ota_log_flush();
            chip_system_reset();
        }
        if (!ota_webusb_is_busy() &&
            ota_usb_msc_should_check(chip_time_millis(),
                                     system_config.usb_idle_timeout_ms)) {
            chip_status_t sync_status;
            ota_system_config_status_t config_status;
            ota_system_config_t loaded_config;
            ota_image_header_t header;
            ota_image_status_t image_status = OTA_IMAGE_IO_ERROR;

            LOG_INFO("ota", "host writes stopped, checking update");
            tud_disconnect();
            chip_delay_ms(20U);
            sync_status = ota_spi_disk_sync(&update_disk);
            ota_usb_msc_mark_checked();
            if (sync_status == CHIP_OK) {
                config_status = ota_system_config_load(&loaded_config);
                if ((config_status == OTA_SYSTEM_CONFIG_OK) ||
                    (config_status == OTA_SYSTEM_CONFIG_CREATED)) {
                    apply_system_config(&loaded_config);
                    (void)ota_spi_disk_sync(&update_disk);
                } else {
                    LOG_ERROR("config", "reload rejected status=%s",
                              ota_system_config_status_name(config_status));
                }
                image_status = validate_update_file(&header);
            } else {
                LOG_ERROR("ota", "SPI Flash synchronization failed status=%u",
                          (unsigned int)sync_status);
            }
            if (image_status == OTA_IMAGE_OK) {
                LOG_INFO("ota", "valid image version=%lu size=%lu, resetting",
                         (unsigned long)header.firmware_version,
                         (unsigned long)header.image_size);
                chip_system_reset();
            }
            if (image_status != OTA_IMAGE_NOT_FOUND) {
                LOG_WARN("ota", "update not accepted image_status=%u",
                         (unsigned int)image_status);
            }
            ota_usb_msc_resume();
            tud_connect();
        }
    }
}
