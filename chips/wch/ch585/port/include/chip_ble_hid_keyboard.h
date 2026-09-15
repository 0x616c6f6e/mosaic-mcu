#ifndef CHIP_API_BLE_HID_KEYBOARD_H
#define CHIP_API_BLE_HID_KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Number of usage-code slots in a BLE Boot Keyboard report. */
#define CHIP_BLE_HID_KEYBOARD_KEY_COUNT 6U

/** @brief Receive keyboard LED state from the connected BLE host. */
typedef void (*chip_ble_hid_led_callback_t)(uint8_t leds, void *context);

/** @brief BLE keyboard LED handler configuration. */
typedef struct {
    chip_ble_hid_led_callback_t led_callback; /**< Optional host LED handler. */
    void *led_context;                        /**< Handler context. */
} chip_ble_hid_keyboard_config_t;

/** @brief Register the BLE HID keyboard service.
 * @param config Required configuration; callback may be NULL if unused.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_ble_hid_keyboard_init(
    const chip_ble_hid_keyboard_config_t *config);
/** @brief Send a six-key BLE HID keyboard report.
 * @param modifiers HID modifier bitmask.
 * @param keycodes Six HID usage codes; unused slots are zero.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_ble_hid_keyboard_send(
    uint8_t modifiers,
    const uint8_t keycodes[CHIP_BLE_HID_KEYBOARD_KEY_COUNT]);
/** @brief Check whether the BLE HID report can be sent.
 * @return true when the service is connected and ready.
 */
bool chip_ble_hid_keyboard_ready(void);

#ifdef __cplusplus
}
#endif

#endif
