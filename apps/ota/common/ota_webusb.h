#ifndef CH585_OTA_WEBUSB_H
#define CH585_OTA_WEBUSB_H

#include <stdbool.h>
#include <stdint.h>

#include <spi_flash.h>

#define OTA_WEBUSB_COMMAND_BEGIN  UINT8_C(1)
#define OTA_WEBUSB_COMMAND_DATA   UINT8_C(2)
#define OTA_WEBUSB_COMMAND_END    UINT8_C(3)
#define OTA_WEBUSB_COMMAND_ABORT  UINT8_C(4)
#define OTA_WEBUSB_COMMAND_STATUS UINT8_C(5)

#define OTA_WEBUSB_STATUS_OK          UINT8_C(0)
#define OTA_WEBUSB_STATUS_BAD_COMMAND UINT8_C(1)
#define OTA_WEBUSB_STATUS_BAD_STATE   UINT8_C(2)
#define OTA_WEBUSB_STATUS_BAD_SIZE    UINT8_C(3)
#define OTA_WEBUSB_STATUS_BAD_OFFSET  UINT8_C(4)
#define OTA_WEBUSB_STATUS_FLASH_ERROR UINT8_C(5)
#define OTA_WEBUSB_STATUS_IMAGE_ERROR UINT8_C(6)

void ota_webusb_init(spi_flash_t *flash);
void ota_webusb_set_reboot_delay(uint32_t delay_ms);
void ota_webusb_task(void);
bool ota_webusb_is_busy(void);
bool ota_webusb_should_reboot(uint32_t now_ms);

#endif
