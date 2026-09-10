#include <stdbool.h>
#include <stdint.h>

#include <FreeRTOS.h>
#include <task.h>

#include <chip_ble.h>
#include <chip_ble_hid_keyboard.h>
#include <chip_system.h>
#include <platform_log.h>

#include "tusb.h"

#include "demo_support.h"

#define USB_TASK_STACK_WORDS  256U
#define APP_TASK_STACK_WORDS  256U
#define BLE_TASK_STACK_WORDS  256U

static volatile chip_ble_state_t current_ble_state;
static volatile bool usb_initialized;
static volatile bool usb_mounted;
static volatile bool usb_init_failed;
static volatile uint8_t ble_keyboard_leds;

static const uint8_t ble_advertising_data[] = {
    0x02, 0x01, 0x06,
    0x03, 0x19, 0xC1, 0x03,
    0x03, 0x03, 0x12, 0x18,
};

static const uint8_t ble_scan_response_data[] = {
    0x0C, 0x09, 'C', 'H', '5', '8', '5', ' ', 'C', 'o', 'm', 'b', 'o',
};

static uint32_t freertos_log_timestamp(void *context)
{
    (void)context;
    return (uint32_t)pdTICKS_TO_MS(xTaskGetTickCount());
}

static void ble_event(const chip_ble_event_t *event, void *context)
{
    (void)context;
    current_ble_state = event->state;
}

static void ble_led_report(uint8_t leds, void *context)
{
    (void)context;
    ble_keyboard_leds = leds;
}

void tud_mount_cb(void)
{
    usb_mounted = true;
    (void)demo_led_write(true);
}

void tud_umount_cb(void)
{
    usb_mounted = false;
    (void)demo_led_write(false);
}

void tud_suspend_cb(bool remote_wakeup_enabled)
{
    (void)remote_wakeup_enabled;
    (void)demo_led_write(false);
}

void tud_resume_cb(void)
{
    (void)demo_led_write(usb_mounted);
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
    (void)report_type;
    (void)buffer;
    (void)size;
}

static void usb_task(void *context)
{
    const tusb_rhport_init_t usb_config = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };

    (void)context;
    if (!tusb_init(0U, &usb_config)) {
        usb_init_failed = true;
        vTaskDelete(NULL);
    }
    usb_initialized = true;
    for (;;) {
        tud_task();
    }
}

static void ble_task(void *context)
{
    (void)context;
    for (;;) {
        chip_ble_process();
    }
}

static void app_task(void *context)
{
    TickType_t wake_time = xTaskGetTickCount();
    bool ble_key_down = false;
    uint32_t iteration = 0U;

    (void)context;
    for (;;) {
        uint8_t ble_keycodes[CHIP_BLE_HID_KEYBOARD_KEY_COUNT] = {0U};

        if (!ble_key_down) {
            ble_keycodes[0] = HID_KEY_A;
        }
        (void)chip_ble_hid_keyboard_send(0U, ble_keycodes);
        ble_key_down = !ble_key_down;

        if (usb_mounted && tud_hid_ready()) {
            // uint8_t keycodes[6] = {0U};

            // if (!key_down) {
            //     keycodes[0] = HID_KEY_A;
            // }
            // if (tud_hid_keyboard_report(0U, 0U,
            //                             key_down ? NULL : keycodes)) {
            //     key_down = !key_down;
            // }
        }

        if ((iteration % 2U) == 0U) {
            LOG_INFO("combo", "usb=%u mounted=%u ble=%u hid=%u leds=0x%02X",
                     usb_initialized ? 1U : 0U,
                     usb_mounted ? 1U : 0U,
                     (unsigned int)current_ble_state,
                     chip_ble_hid_keyboard_ready() ? 1U : 0U,
                     (unsigned int)ble_keyboard_leds);
            if (usb_init_failed) {
                LOG_ERROR("combo", "TinyUSB initialization failed");
            }
        }
        ++iteration;
        vTaskDelayUntil(&wake_time, pdMS_TO_TICKS(500U));
    }
}

int main(void)
{
    chip_system_config_t system_config = {
        .source = CHIP_CLOCK_EXTERNAL,
        .core_clock_hz = DEMO_SYSTEM_CLOCK_HZ,
        .external_crystal_load_pf = 18U,
    };
    chip_ble_peripheral_config_t ble_config;
    chip_ble_hid_keyboard_config_t ble_hid_config = {
        .led_callback = ble_led_report,
        .led_context = NULL,
    };
    chip_status_t status;
    uint32_t interrupt_state;

    DEMO_REQUIRE(chip_system_init(&system_config));
    DEMO_REQUIRE(demo_led_init());
    DEMO_REQUIRE(demo_log_init_with_timestamp(freertos_log_timestamp, NULL));

    chip_ble_peripheral_config_default(&ble_config);
    ble_config.device_name = "CH585 Combo";
    ble_config.advertising_data = ble_advertising_data;
    ble_config.advertising_data_size = sizeof(ble_advertising_data);
    ble_config.scan_response_data = ble_scan_response_data;
    ble_config.scan_response_data_size = sizeof(ble_scan_response_data);
    ble_config.event_callback = ble_event;
    interrupt_state = chip_system_critical_enter();
    status = chip_ble_peripheral_init(&ble_config);
    chip_system_critical_exit(interrupt_state);
    if (status != CHIP_OK) {
        LOG_ERROR("combo", "BLE initialization failed, status=%u",
                  (unsigned int)status);
        demo_halt();
    }
    status = chip_ble_hid_keyboard_init(&ble_hid_config);
    if (status != CHIP_OK) {
        LOG_ERROR("combo", "BLE HID initialization failed, status=%u",
                  (unsigned int)status);
        demo_halt();
    }
    current_ble_state = chip_ble_state();

    if ((xTaskCreate(usb_task, "usb", USB_TASK_STACK_WORDS, NULL, 3U, NULL) !=
         pdPASS) ||
        (xTaskCreate(app_task, "app", APP_TASK_STACK_WORDS, NULL, 2U, NULL) !=
         pdPASS) ||
        (xTaskCreate(ble_task, "ble", BLE_TASK_STACK_WORDS, NULL, 1U, NULL) !=
         pdPASS)) {
        LOG_ERROR("combo", "task creation failed");
        demo_halt();
    }

    LOG_INFO("combo", "FreeRTOS + TinyUSB + BLE starting");
    vTaskStartScheduler();
    LOG_ERROR("combo", "scheduler returned unexpectedly");
    demo_halt();
}
