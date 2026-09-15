#ifndef CH585_OTA_SYSTEM_CONFIG_H
#define CH585_OTA_SYSTEM_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <platform_log.h>

/** @brief FAT path containing persisted system settings. */
#define OTA_SYSTEM_CONFIG_PATH "0:/SYSTEM.JSON"
/** @brief Expected SYSTEM.JSON schema revision. */
#define OTA_SYSTEM_CONFIG_SCHEMA_VERSION UINT32_C(2)
/** @brief Device name buffer length including NUL terminator. */
#define OTA_SYSTEM_CONFIG_DEVICE_NAME_SIZE 32U
/** @brief Minimum MSC idle timeout in milliseconds. */
#define OTA_SYSTEM_CONFIG_MIN_IDLE_MS UINT32_C(500)
/** @brief Maximum MSC idle timeout in milliseconds. */
#define OTA_SYSTEM_CONFIG_MAX_IDLE_MS UINT32_C(30000)
/** @brief Minimum WebUSB reboot delay in milliseconds. */
#define OTA_SYSTEM_CONFIG_MIN_REBOOT_MS UINT32_C(100)
/** @brief Maximum WebUSB reboot delay in milliseconds. */
#define OTA_SYSTEM_CONFIG_MAX_REBOOT_MS UINT32_C(5000)

/** @brief Persisted product name, USB disk visibility, and update timings. */
typedef struct {
    uint32_t schema_version; /**< Persisted schema revision. */
    char device_name[OTA_SYSTEM_CONFIG_DEVICE_NAME_SIZE]; /**< USB product name. */
    bool usb_disk_visible;   /**< Whether USB MSC disk is exposed. */
    platform_log_level_t log_level; /**< Runtime log filter. */
    uint32_t usb_idle_timeout_ms; /**< Disk-write inactivity interval in ms. */
    uint32_t webusb_reboot_delay_ms; /**< Delay after commit in ms. */
} ota_system_config_t;

/** @brief System configuration parse and storage results. */
typedef enum {
    OTA_SYSTEM_CONFIG_OK = 0, /**< Loaded, parsed, or saved successfully. */
    OTA_SYSTEM_CONFIG_CREATED, /**< Defaults were written to a new file. */
    OTA_SYSTEM_CONFIG_MOUNT_ERROR, /**< FAT volume could not be mounted. */
    OTA_SYSTEM_CONFIG_IO_ERROR, /**< FAT read or write failed. */
    OTA_SYSTEM_CONFIG_TOO_LARGE, /**< JSON exceeds accepted size. */
    OTA_SYSTEM_CONFIG_INVALID_JSON, /**< Malformed JSON document. */
    OTA_SYSTEM_CONFIG_INVALID_VALUE, /**< Field fails validation. */
} ota_system_config_status_t;

/** @brief Fill system settings with their factory defaults.
 * @param[out] config Settings to initialize.
 */
void ota_system_config_defaults(ota_system_config_t *config);
/** @brief Parse and validate a SYSTEM.JSON buffer.
 * @param json UTF-8 JSON bytes; need not be NUL-terminated.
 * @param size JSON length in bytes.
 * @param[out] config Receives valid system settings.
 * @return OTA_SYSTEM_CONFIG_OK or a configuration error code.
 */
ota_system_config_status_t ota_system_config_parse(
    const char *json, size_t size, ota_system_config_t *config);
/** @brief Load SYSTEM.JSON from the FAT volume.
 * @param[out] config Receives loaded or default settings.
 * @return OTA_SYSTEM_CONFIG_OK, CREATED, or an error code.
 */
ota_system_config_status_t ota_system_config_load(
    ota_system_config_t *config);
/** @brief Validate and persist a SYSTEM.JSON buffer.
 * @param json UTF-8 JSON bytes; need not be NUL-terminated.
 * @param size JSON length in bytes.
 * @param[out] config Receives the saved system settings.
 * @return OTA_SYSTEM_CONFIG_OK or an error code.
 */
ota_system_config_status_t ota_system_config_save_json(
    const char *json, size_t size, ota_system_config_t *config);
/** @brief Map a system configuration status to a readable name.
 * @param status Status code to describe.
 * @return Static status string.
 */
const char *ota_system_config_status_name(ota_system_config_status_t status);

#endif
