#ifndef PLATFORM_OTA_USB_MSC_H
#define PLATFORM_OTA_USB_MSC_H

#include <stdbool.h>
#include <stdint.h>

#include <ota_spi_disk.h>

void ota_usb_msc_init(ota_spi_disk_t *disk);
chip_status_t ota_usb_msc_set_identity(const char *vendor_id,
                                       const char *product_id,
                                       const char *revision);
bool ota_usb_msc_should_check(uint32_t now_ms, uint32_t idle_timeout_ms);
void ota_usb_msc_mark_checked(void);
void ota_usb_msc_resume(void);

#endif
