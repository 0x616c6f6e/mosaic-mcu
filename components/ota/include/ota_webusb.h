#ifndef PLATFORM_OTA_WEBUSB_H
#define PLATFORM_OTA_WEBUSB_H

#include <stdbool.h>
#include <stdint.h>

#include <ota_staging.h>

/** @brief Begin a WebUSB OTA transfer. */
#define OTA_WEBUSB_COMMAND_BEGIN  UINT8_C(1)
/** @brief Append a firmware data block. */
#define OTA_WEBUSB_COMMAND_DATA   UINT8_C(2)
/** @brief Validate and commit the transferred image. */
#define OTA_WEBUSB_COMMAND_END    UINT8_C(3)
/** @brief Discard an incomplete transfer. */
#define OTA_WEBUSB_COMMAND_ABORT  UINT8_C(4)
/** @brief Query OTA transaction state. */
#define OTA_WEBUSB_COMMAND_STATUS UINT8_C(5)
/** @brief Read the optional device configuration. */
#define OTA_WEBUSB_COMMAND_GET_CONFIG UINT8_C(6)
/** @brief Update the optional device configuration. */
#define OTA_WEBUSB_COMMAND_SET_CONFIG UINT8_C(7)

/** @brief WebUSB command completed. */
#define OTA_WEBUSB_STATUS_OK          UINT8_C(0)
/** @brief Request command code not recognized. */
#define OTA_WEBUSB_STATUS_BAD_COMMAND UINT8_C(1)
/** @brief Request invalid for the current transfer state. */
#define OTA_WEBUSB_STATUS_BAD_STATE   UINT8_C(2)
/** @brief Request payload length invalid. */
#define OTA_WEBUSB_STATUS_BAD_SIZE    UINT8_C(3)
/** @brief Data block offset differs from expected offset. */
#define OTA_WEBUSB_STATUS_BAD_OFFSET  UINT8_C(4)
/** @brief Flash access or programming failed. */
#define OTA_WEBUSB_STATUS_FLASH_ERROR UINT8_C(5)
/** @brief OTA image validation failed. */
#define OTA_WEBUSB_STATUS_IMAGE_ERROR UINT8_C(6)
/** @brief Device configuration handler rejected the request. */
#define OTA_WEBUSB_STATUS_CONFIG_ERROR UINT8_C(7)

/** @brief Serialized device configuration payload size in bytes. */
#define OTA_WEBUSB_CONFIG_DATA_SIZE UINT16_C(44)

#ifdef OTA_WEBUSB_CONFIG_ENABLED
/** @brief Read device configuration into a WebUSB reply buffer. */
typedef uint8_t (*ota_webusb_config_read_t)(uint8_t *data, uint16_t capacity,
                                            uint16_t *size, void *context);
/** @brief Apply device configuration received over WebUSB. */
typedef uint8_t (*ota_webusb_config_write_t)(const uint8_t *data, uint16_t size,
                                             void *context);
#endif

/** @brief Bind WebUSB OTA requests to a flash staging area.
 * @param staging Initialized staging area.
 */
void ota_webusb_init(ota_staging_t *staging);
/** @brief Set the delay after a committed update before requesting reboot.
 * @param delay_ms Reboot delay in milliseconds.
 */
void ota_webusb_set_reboot_delay(uint32_t delay_ms);
#ifdef OTA_WEBUSB_CONFIG_ENABLED
/** @brief Register optional WebUSB device-configuration handlers.
 * @param read_config Handler for GET_CONFIG.
 * @param write_config Handler for SET_CONFIG.
 * @param handler_context Opaque context passed to each handler.
 */
void ota_webusb_set_config_handlers(ota_webusb_config_read_t read_config,
                                    ota_webusb_config_write_t write_config,
                                    void *handler_context);
#endif
/** @brief Process pending WebUSB OTA commands; call from the main loop. */
void ota_webusb_task(void);
/** @brief Check whether a WebUSB OTA transaction is in progress.
 * @return true while an update is active.
 */
bool ota_webusb_is_busy(void);
/** @brief Check whether a committed OTA package is ready for reboot.
 * @param now_ms Current monotonic time in milliseconds.
 * @return true when the reboot delay has elapsed.
 */
bool ota_webusb_should_reboot(uint32_t now_ms);

#endif
