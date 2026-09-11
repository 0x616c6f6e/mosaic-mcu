#ifndef MOSAIC_KEYBOARD_LAYOUT_H
#define MOSAIC_KEYBOARD_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

#include <keyboard_engine.h>

bool keyboard_layout_init(void);
const keyboard_engine_config_t *keyboard_layout_config(void);
void keyboard_layout_set_leds(uint8_t leds);

#endif
