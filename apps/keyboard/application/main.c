#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <chip_system.h>
#include <chip_time.h>
#include <ota_image.h>
#include <ota_spi_disk.h>
#include <platform_fatfs.h>
#include <platform_log.h>
#include <spi_flash.h>

#include "tusb.h"

#include "keyboard_board.h"
#include "keyboard_hid.h"
#include "keyboard_log.h"
#include "keyboard_usb_device.h"
#include "build_info.h"
#include "ota_system_config.h"
#include "ota_usb_msc.h"
#include "ota_webusb.h"

static spi_flash_t external_flash;
static ota_spi_disk_t update_disk;
static ota_staging_t update_staging;
static uint8_t disk_cache[OTA_SPI_FLASH_SECTOR_SIZE];
static uint8_t validation_buffer[512];
static FATFS filesystem;
static ota_system_config_t system_config;
static bool webusb_initialized;

#define OTA_WEB_CONFIG_VERSION UINT8_C(1)
#define OTA_WEB_CONFIG_JSON_MAX_SIZE UINT16_C(512)
#define OTA_WEB_CONFIG_APPLY_DELAY_MS UINT32_C(100)

static ota_system_config_t pending_system_config;
static char pending_config_json[OTA_WEB_CONFIG_JSON_MAX_SIZE];
static uint16_t pending_config_size;
static uint32_t pending_config_deadline;
static bool config_update_pending;

uint32_t tusb_time_millis_api(void)
{
    return chip_time_millis();
}

static void write_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
    data[2] = (uint8_t)(value >> 16U);
    data[3] = (uint8_t)(value >> 24U);
}

static uint8_t read_web_config(uint8_t *data, uint16_t capacity,
                               uint16_t *size, void *context)
{
    size_t name_size = strlen(system_config.device_name);

    (void)context;
    if ((data == NULL) || (size == NULL) ||
        (capacity < OTA_WEBUSB_CONFIG_DATA_SIZE) ||
        (name_size >= OTA_SYSTEM_CONFIG_DEVICE_NAME_SIZE)) {
        return OTA_WEBUSB_STATUS_CONFIG_ERROR;
    }
    memset(data, 0, OTA_WEBUSB_CONFIG_DATA_SIZE);
    data[0] = OTA_WEB_CONFIG_VERSION;
    data[1] = system_config.usb_disk_visible ? 1U : 0U;
    data[2] = (uint8_t)system_config.log_level;
    data[3] = (uint8_t)name_size;
    write_u32_le(&data[4], system_config.usb_idle_timeout_ms);
    write_u32_le(&data[8], system_config.webusb_reboot_delay_ms);
    memcpy(&data[12], system_config.device_name, name_size);
    *size = OTA_WEBUSB_CONFIG_DATA_SIZE;
    return OTA_WEBUSB_STATUS_OK;
}

static uint8_t stage_web_config(const uint8_t *data, uint16_t size,
                                void *context)
{
    ota_system_config_t parsed;
    ota_system_config_status_t status;

    (void)context;
    if ((data == NULL) || (size == 0U) ||
        (size > sizeof(pending_config_json))) {
        return OTA_WEBUSB_STATUS_BAD_SIZE;
    }
    if (config_update_pending || ota_webusb_is_busy()) {
        return OTA_WEBUSB_STATUS_BAD_STATE;
    }
    status = ota_system_config_parse((const char *)data, size, &parsed);
    if (status != OTA_SYSTEM_CONFIG_OK) {
        LOG_WARN("config", "WebUSB config rejected status=%s",
                 ota_system_config_status_name(status));
        return OTA_WEBUSB_STATUS_CONFIG_ERROR;
    }
    memcpy(pending_config_json, data, size);
    pending_system_config = parsed;
    pending_config_size = size;
    pending_config_deadline = chip_time_millis() +
                              OTA_WEB_CONFIG_APPLY_DELAY_MS;
    config_update_pending = true;
    LOG_INFO("config", "WebUSB config accepted bytes=%u",
             (unsigned int)size);
    return OTA_WEBUSB_STATUS_OK;
}

static ota_image_status_t validate_update_file(ota_image_header_t *header)
{
    ota_image_status_t status = OTA_IMAGE_IO_ERROR;
    FRESULT mount_result = f_mount(&filesystem, "0:", 1U);

    LOG_DEBUG("ota", "update volume mount result=%u",
              (unsigned int)mount_result);
    if (mount_result == FR_OK) {
        status = ota_image_validate_file(OTA_UPDATE_PATH, OTA_TARGET_ID,
                                         OTA_APPLICATION_ADDRESS,
                                         OTA_APPLICATION_SIZE, header,
                                         validation_buffer,
                                         sizeof(validation_buffer));
    }
    (void)f_unmount("0:");
    LOG_DEBUG("ota", "update validation status=%u", (unsigned int)status);
    return status;
}

static void apply_system_config(const ota_system_config_t *config)
{
    system_config = *config;
    keyboard_usb_set_product_name(system_config.device_name);
    keyboard_usb_set_disk_visible(system_config.usb_disk_visible);
    if (webusb_initialized) {
        ota_webusb_set_reboot_delay(system_config.webusb_reboot_delay_ms);
    }
    (void)platform_log_set_level(system_config.log_level);
    LOG_INFO("config", "applied name=%s disk=%u level=%u idle_ms=%lu reboot_ms=%lu",
             system_config.device_name,
             system_config.usb_disk_visible ? 1U : 0U,
             (unsigned int)system_config.log_level,
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

static void apply_pending_web_config(uint32_t now_ms)
{
    ota_system_config_t saved;
    ota_system_config_status_t config_status;
    chip_status_t sync_status;

    if (!config_update_pending ||
        ((int32_t)(now_ms - pending_config_deadline) < 0)) {
        return;
    }
    config_update_pending = false;
    tud_disconnect();
    chip_delay_ms(20U);
    sync_status = ota_spi_disk_sync(&update_disk);
    if (sync_status == CHIP_OK) {
        config_status = ota_system_config_save_json(
            pending_config_json, pending_config_size, &saved);
    } else {
        config_status = OTA_SYSTEM_CONFIG_IO_ERROR;
    }
    if (config_status == OTA_SYSTEM_CONFIG_OK) {
        apply_system_config(&saved);
        (void)ota_spi_disk_sync(&update_disk);
        LOG_INFO("config", "WebUSB config saved, reconnecting disk=%u",
                 saved.usb_disk_visible ? 1U : 0U);
    } else {
        LOG_ERROR("config", "WebUSB config save failed sync=%u status=%s",
                  (unsigned int)sync_status,
                  ota_system_config_status_name(config_status));
    }
    ota_usb_msc_resume();
    tud_connect();
}

int main(void)
{
    const tusb_rhport_init_t usb_config = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };
    chip_status_t status;
    spi_flash_jedec_id_t flash_id = {0U, 0U, 0U};

    status = keyboard_log_init();
    if (status != CHIP_OK) {
        for (;;) {
        }
    }
    LOG_INFO("ota", "application build=%s entry=0x%08lX",
             MOSAIC_BUILD_TIMESTAMP,
             (unsigned long)OTA_APPLICATION_ADDRESS);
    (void)keyboard_log_flush();
    status = keyboard_board_init(&external_flash, &update_disk, disk_cache,
                                 sizeof(disk_cache));
    if (status != CHIP_OK) {
        LOG_ERROR("ota", "board initialization failed status=%u",
                  (unsigned int)status);
        for (;;) {
        }
    }
    status = keyboard_board_init_staging(&update_staging, &external_flash);
    if (status != CHIP_OK) {
        LOG_ERROR("ota", "staging initialization failed status=%u",
                  (unsigned int)status);
        for (;;) {
        }
    }
    LOG_DEBUG("ota", "board ready spi_hz=%lu flash_bytes=%lu disk_bytes=%lu",
              (unsigned long)external_flash.config.clock_hz,
              (unsigned long)external_flash.config.capacity_bytes,
              (unsigned long)update_disk.size);
    status = keyboard_board_probe_flash(&external_flash, &flash_id);
    if (status == CHIP_OK) {
        LOG_INFO("ota", "SPI Flash JEDEC=%02X:%02X:%02X",
                 (unsigned int)flash_id.manufacturer,
                 (unsigned int)flash_id.memory_type,
                 (unsigned int)flash_id.capacity);
    } else {
        LOG_ERROR("ota", "SPI Flash probe failed status=%u ID=%02X:%02X:%02X",
                  (unsigned int)status,
                  (unsigned int)flash_id.manufacturer,
                  (unsigned int)flash_id.memory_type,
                  (unsigned int)flash_id.capacity);
        (void)keyboard_log_flush();
        for (;;) {
        }
    }
    ota_system_config_defaults(&system_config);
    load_system_config();
    status = ota_spi_disk_sync(&update_disk);
    if (status != CHIP_OK) {
        LOG_ERROR("ota", "initial disk synchronization failed status=%u",
                  (unsigned int)status);
    } else {
        LOG_DEBUG("ota", "initial disk synchronization complete");
    }
    LOG_INFO("ota", "USB update application started");
    ota_usb_msc_init(&update_disk);
    if (ota_usb_msc_set_identity("Mosaic", "Keyboard Update", "1.0") !=
        CHIP_OK) {
        LOG_ERROR("ota", "invalid MSC identity");
        for (;;) {
        }
    }
    ota_webusb_init(&update_staging);
    ota_webusb_set_config_handlers(read_web_config, stage_web_config, NULL);
    webusb_initialized = true;
    ota_webusb_set_reboot_delay(system_config.webusb_reboot_delay_ms);
    if (!tusb_init(0U, &usb_config)) {
        LOG_ERROR("ota", "TinyUSB initialization failed");
        for (;;) {
        }
    }
    if (!keyboard_hid_init()) {
        LOG_ERROR("keyboard", "HID engine initialization failed");
        for (;;) {
        }
    }
    LOG_DEBUG("ota", "TinyUSB initialized rhport=0 speed=full");
    LOG_INFO("ota", "waiting for %s", OTA_UPDATE_PATH);

    for (;;) {
        tud_task();
        keyboard_hid_task(chip_time_millis());
        ota_webusb_task();
        apply_pending_web_config(chip_time_millis());
        if (ota_webusb_should_reboot(chip_time_millis())) {
            (void)ota_spi_disk_sync(&update_disk);
            LOG_INFO("ota", "WebUSB update ready, resetting");
            (void)keyboard_log_flush();
            chip_system_reset();
        }
        if (system_config.usb_disk_visible && !ota_webusb_is_busy() &&
            ota_usb_msc_should_check(chip_time_millis(),
                                     system_config.usb_idle_timeout_ms)) {
            chip_status_t sync_status;
            ota_system_config_status_t config_status;
            ota_system_config_t loaded_config;
            ota_image_header_t header;
            ota_image_status_t image_status = OTA_IMAGE_IO_ERROR;

            LOG_INFO("ota", "host writes stopped, checking update");
            tud_disconnect();
            LOG_DEBUG("usb", "device disconnected for filesystem check");
            chip_delay_ms(20U);
            sync_status = ota_spi_disk_sync(&update_disk);
            ota_usb_msc_mark_checked();
            LOG_DEBUG("ota", "disk synchronization status=%u",
                      (unsigned int)sync_status);
            if (sync_status == CHIP_OK) {
                config_status = ota_system_config_load(&loaded_config);
                LOG_DEBUG("config", "reload status=%s",
                          ota_system_config_status_name(config_status));
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
            LOG_DEBUG("usb", "device reconnected after filesystem check");
        }
    }
}
