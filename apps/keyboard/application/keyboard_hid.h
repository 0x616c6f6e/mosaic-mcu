#ifndef MOSAIC_KEYBOARD_HID_H
#define MOSAIC_KEYBOARD_HID_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Initialize the keyboard HID report engine.
 * @return true when keyboard layout and report engine are ready.
 */
bool keyboard_hid_init(void);
/** @brief Scan, debounce, and send HID reports when due.
 * @param now_ms Current monotonic time in milliseconds.
 */
void keyboard_hid_task(uint32_t now_ms);

#endif
