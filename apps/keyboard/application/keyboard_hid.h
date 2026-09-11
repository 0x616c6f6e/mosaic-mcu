#ifndef MOSAIC_KEYBOARD_HID_H
#define MOSAIC_KEYBOARD_HID_H

#include <stdbool.h>
#include <stdint.h>

bool keyboard_hid_init(void);
void keyboard_hid_task(uint32_t now_ms);

#endif
