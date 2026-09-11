#ifndef CH585_DEMO_LUA_USB_FATFS_USB_MSC_H
#define CH585_DEMO_LUA_USB_FATFS_USB_MSC_H

#include <stdbool.h>
#include <stdint.h>

#include <ota_spi_disk.h>

void lua_usb_msc_init(ota_spi_disk_t *disk);
bool lua_usb_msc_script_pending(uint32_t now_ms, uint32_t idle_timeout_ms);
void lua_usb_msc_mark_processed(void);
void lua_usb_msc_resume(void);

#endif
