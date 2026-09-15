#ifndef PLATFORM_OTA_USB_MSC_H
#define PLATFORM_OTA_USB_MSC_H

#include <stdbool.h>
#include <stdint.h>

#include <ota_spi_disk.h>

/** @brief Attach a flash-backed disk to the USB MSC adapter.
 * @param disk Initialized disk, retained by the adapter.
 */
void ota_usb_msc_init(ota_spi_disk_t *disk);
/** @brief Set USB MSC inquiry strings.
 * @param vendor_id Vendor string.
 * @param product_id Product string.
 * @param revision Product revision string.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t ota_usb_msc_set_identity(const char *vendor_id,
                                       const char *product_id,
                                       const char *revision);
/** @brief Check whether the host disk has been idle long enough for OTA inspection.
 * @param now_ms Current monotonic time in milliseconds.
 * @param idle_timeout_ms Required inactivity in milliseconds.
 * @return true when the update file should be checked.
 */
bool ota_usb_msc_should_check(uint32_t now_ms, uint32_t idle_timeout_ms);
/** @brief Mark the pending MSC update check as complete. */
void ota_usb_msc_mark_checked(void);
/** @brief Resume MSC update checks after a completed check. */
void ota_usb_msc_resume(void);

#endif
