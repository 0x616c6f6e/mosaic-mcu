#ifndef MOSAIC_KEYBOARD_LAYOUT_H
#define MOSAIC_KEYBOARD_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

#include <keyboard_engine.h>

/** @brief Initialize board GPIOs and direct keys for the keyboard layout.
 * @return true when the layout is ready.
 */
bool keyboard_layout_init(void);
/** @brief Get the static key bindings and scan callback.
 * @return Configuration valid for the initialized layout.
 */
const keyboard_engine_config_t *keyboard_layout_config(void);
/** @brief Receive USB keyboard LED state (currently ignored by this layout).
 * @param leds HID keyboard LED bitmask.
 */
void keyboard_layout_set_leds(uint8_t leds);

#endif
