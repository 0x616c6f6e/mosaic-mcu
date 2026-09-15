#ifndef CH585_DEMO_LUA_USB_FATFS_USB_MSC_H
#define CH585_DEMO_LUA_USB_FATFS_USB_MSC_H

#include <stdbool.h>
#include <stdint.h>

#include <ota_spi_disk.h>

/** @brief Bind the Lua demo's USB MSC interface to a flash-backed disk.
 * @param disk Initialized disk, retained by the MSC adapter.
 */
void lua_usb_msc_init(ota_spi_disk_t *disk);
/** @brief Check whether the script disk is idle enough to process a new script.
 * @param now_ms Current monotonic time in milliseconds.
 * @param idle_timeout_ms Required inactivity in milliseconds.
 * @return true when script processing is pending.
 */
bool lua_usb_msc_script_pending(uint32_t now_ms, uint32_t idle_timeout_ms);
/** @brief Mark the pending script as processed. */
void lua_usb_msc_mark_processed(void);
/** @brief Resume checking the disk for newly written scripts. */
void lua_usb_msc_resume(void);

#endif
