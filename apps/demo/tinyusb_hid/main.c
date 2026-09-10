#include <stdbool.h>
#include <stdint.h>

#include <chip_time.h>

#include "tusb.h"

#include "demo_support.h"

uint32_t tusb_time_millis_api(void)
{
    return chip_time_millis();
}

void tud_mount_cb(void)
{
    (void)demo_led_write(true);
}

void tud_umount_cb(void)
{
    (void)demo_led_write(false);
}

void tud_suspend_cb(bool remote_wakeup_enabled)
{
    (void)remote_wakeup_enabled;
    (void)demo_led_write(false);
}

void tud_resume_cb(void)
{
    (void)demo_led_write(true);
}

uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer,
                               uint16_t requested_length)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)requested_length;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           const uint8_t *buffer,
                           uint16_t size)
{
    (void)instance;
    (void)report_id;

    if ((report_type == HID_REPORT_TYPE_OUTPUT) && (size > 0U)) {
        (void)demo_led_write((buffer[0] & KEYBOARD_LED_CAPSLOCK) != 0U);
    }
}

int main(void)
{
    const tusb_rhport_init_t usb_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };
    uint32_t report_time = 0;
    // bool key_down = false;

    DEMO_REQUIRE(demo_platform_init());
    DEMO_REQUIRE(demo_led_init());
    if (!tusb_init(0, &usb_init)) {
        demo_halt();
    }

    for (;;) {
        uint32_t now;

        tud_task();
        now = chip_time_millis();
        if (tud_mounted() && tud_hid_ready() && ((now - report_time) >= 500U)) {
            // uint8_t keycodes[6] = {0};

            // report_time = now;
            // if (!key_down) {
            //     keycodes[0] = HID_KEY_A;
            // }
            // if (tud_hid_keyboard_report(0, 0, key_down ? NULL : keycodes)) {
            //     key_down = !key_down;
            // }
        }
    }
}
