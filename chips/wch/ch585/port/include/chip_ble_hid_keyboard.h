#ifndef CHIP_API_BLE_HID_KEYBOARD_H
#define CHIP_API_BLE_HID_KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHIP_BLE_HID_KEYBOARD_KEY_COUNT 6U

typedef void (*chip_ble_hid_led_callback_t)(uint8_t leds, void *context);

typedef struct {
    chip_ble_hid_led_callback_t led_callback;
    void *led_context;
} chip_ble_hid_keyboard_config_t;

chip_status_t chip_ble_hid_keyboard_init(
    const chip_ble_hid_keyboard_config_t *config);
chip_status_t chip_ble_hid_keyboard_send(
    uint8_t modifiers,
    const uint8_t keycodes[CHIP_BLE_HID_KEYBOARD_KEY_COUNT]);
bool chip_ble_hid_keyboard_ready(void);

#ifdef __cplusplus
}
#endif

#endif
