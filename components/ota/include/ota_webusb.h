#ifndef PLATFORM_OTA_WEBUSB_H
#define PLATFORM_OTA_WEBUSB_H

#include <stdbool.h>
#include <stdint.h>

#include <ota_staging.h>

#define OTA_WEBUSB_COMMAND_BEGIN  UINT8_C(1)
#define OTA_WEBUSB_COMMAND_DATA   UINT8_C(2)
#define OTA_WEBUSB_COMMAND_END    UINT8_C(3)
#define OTA_WEBUSB_COMMAND_ABORT  UINT8_C(4)
#define OTA_WEBUSB_COMMAND_STATUS UINT8_C(5)
#define OTA_WEBUSB_COMMAND_GET_CONFIG UINT8_C(6)
#define OTA_WEBUSB_COMMAND_SET_CONFIG UINT8_C(7)

#define OTA_WEBUSB_STATUS_OK          UINT8_C(0)
#define OTA_WEBUSB_STATUS_BAD_COMMAND UINT8_C(1)
#define OTA_WEBUSB_STATUS_BAD_STATE   UINT8_C(2)
#define OTA_WEBUSB_STATUS_BAD_SIZE    UINT8_C(3)
#define OTA_WEBUSB_STATUS_BAD_OFFSET  UINT8_C(4)
#define OTA_WEBUSB_STATUS_FLASH_ERROR UINT8_C(5)
#define OTA_WEBUSB_STATUS_IMAGE_ERROR UINT8_C(6)
#define OTA_WEBUSB_STATUS_CONFIG_ERROR UINT8_C(7)

#define OTA_WEBUSB_CONFIG_DATA_SIZE UINT16_C(44)

#ifdef OTA_WEBUSB_CONFIG_ENABLED
typedef uint8_t (*ota_webusb_config_read_t)(uint8_t *data, uint16_t capacity,
                                            uint16_t *size, void *context);
typedef uint8_t (*ota_webusb_config_write_t)(const uint8_t *data, uint16_t size,
                                             void *context);
#endif

void ota_webusb_init(ota_staging_t *staging);
void ota_webusb_set_reboot_delay(uint32_t delay_ms);
#ifdef OTA_WEBUSB_CONFIG_ENABLED
void ota_webusb_set_config_handlers(ota_webusb_config_read_t read_config,
                                    ota_webusb_config_write_t write_config,
                                    void *handler_context);
#endif
void ota_webusb_task(void);
bool ota_webusb_is_busy(void);
bool ota_webusb_should_reboot(uint32_t now_ms);

#endif
