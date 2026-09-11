#include "keyboard_hid.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "tusb.h"

#include "keyboard_engine.h"
#include "keyboard_layout.h"

static keyboard_engine_t keyboard_engine;
static uint8_t pending_report[KEYBOARD_HID_REPORT_SIZE];
static bool report_pending;

bool keyboard_hid_init(void)
{
    report_pending = false;
    memset(pending_report, 0, sizeof(pending_report));
    return keyboard_layout_init() &&
           keyboard_engine_init(&keyboard_engine, keyboard_layout_config());
}

void keyboard_hid_task(uint32_t now_ms)
{
    bool changed;

    if (!keyboard_engine_poll(&keyboard_engine, now_ms, pending_report,
                              &changed)) {
        return;
    }
    report_pending = report_pending || changed;
    if (report_pending && tud_hid_ready() &&
        tud_hid_keyboard_report(0U, pending_report[0], &pending_report[2])) {
        report_pending = false;
    }
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t requested_length)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)requested_length;
    return 0U;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           const uint8_t *buffer, uint16_t size)
{
    (void)instance;
    (void)report_id;
    if ((report_type == HID_REPORT_TYPE_OUTPUT) && (buffer != NULL) &&
        (size > 0U)) {
        keyboard_layout_set_leds(buffer[0]);
    }
}
