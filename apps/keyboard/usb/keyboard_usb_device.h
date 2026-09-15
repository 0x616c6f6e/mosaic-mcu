#ifndef CH585_KEYBOARD_USB_DEVICE_H
#define CH585_KEYBOARD_USB_DEVICE_H

#include <stdbool.h>

/** @brief Set the product string used in USB descriptors.
 * @param name NUL-terminated product name.
 */
void keyboard_usb_set_product_name(const char *name);
/** @brief Control whether the USB device exposes its MSC interface.
 * @param visible true to expose the update disk.
 */
void keyboard_usb_set_disk_visible(bool visible);

#endif
