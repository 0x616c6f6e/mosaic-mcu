#ifndef CH585_OTA_SYSTEM_CONFIG_H
#define CH585_OTA_SYSTEM_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <platform_log.h>

#define OTA_SYSTEM_CONFIG_PATH "0:/SYSTEM.JSON"
#define OTA_SYSTEM_CONFIG_SCHEMA_VERSION UINT32_C(2)
#define OTA_SYSTEM_CONFIG_DEVICE_NAME_SIZE 32U
#define OTA_SYSTEM_CONFIG_MIN_IDLE_MS UINT32_C(500)
#define OTA_SYSTEM_CONFIG_MAX_IDLE_MS UINT32_C(30000)
#define OTA_SYSTEM_CONFIG_MIN_REBOOT_MS UINT32_C(100)
#define OTA_SYSTEM_CONFIG_MAX_REBOOT_MS UINT32_C(5000)

typedef struct {
    uint32_t schema_version;
    char device_name[OTA_SYSTEM_CONFIG_DEVICE_NAME_SIZE];
    bool usb_disk_visible;
    platform_log_level_t log_level;
    uint32_t usb_idle_timeout_ms;
    uint32_t webusb_reboot_delay_ms;
} ota_system_config_t;

typedef enum {
    OTA_SYSTEM_CONFIG_OK = 0,
    OTA_SYSTEM_CONFIG_CREATED,
    OTA_SYSTEM_CONFIG_MOUNT_ERROR,
    OTA_SYSTEM_CONFIG_IO_ERROR,
    OTA_SYSTEM_CONFIG_TOO_LARGE,
    OTA_SYSTEM_CONFIG_INVALID_JSON,
    OTA_SYSTEM_CONFIG_INVALID_VALUE,
} ota_system_config_status_t;

void ota_system_config_defaults(ota_system_config_t *config);
ota_system_config_status_t ota_system_config_parse(
    const char *json, size_t size, ota_system_config_t *config);
ota_system_config_status_t ota_system_config_load(
    ota_system_config_t *config);
ota_system_config_status_t ota_system_config_save_json(
    const char *json, size_t size, ota_system_config_t *config);
const char *ota_system_config_status_name(ota_system_config_status_t status);

#endif
