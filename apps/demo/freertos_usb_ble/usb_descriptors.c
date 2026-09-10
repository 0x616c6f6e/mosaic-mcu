#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "tusb.h"

#define USB_VID UINT16_C(0xCAFE)
#define USB_PID UINT16_C(0x4011)
#define CONFIG_TOTAL_LENGTH (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)
#define HID_ENDPOINT_ADDRESS UINT8_C(0x81)

static const tusb_desc_device_t device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 0,
    .bNumConfigurations = 1,
};

static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

static const uint8_t configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LENGTH,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(hid_report_descriptor), HID_ENDPOINT_ADDRESS,
                       CFG_TUD_HID_EP_BUFSIZE, 10),
};

static const char *const string_descriptors[] = {
    (const char[]){0x09, 0x04},
    "Embedded Platform",
    "CH585 RTOS USB BLE",
};

static uint16_t string_descriptor[32];

const uint8_t *tud_descriptor_device_cb(void)
{
    return (const uint8_t *)&device_descriptor;
}

const uint8_t *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return configuration_descriptor;
}

const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return hid_report_descriptor;
}

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t language_id)
{
    size_t count;

    (void)language_id;
    if (index >= (sizeof(string_descriptors) / sizeof(string_descriptors[0]))) {
        return NULL;
    }

    if (index == 0U) {
        memcpy(&string_descriptor[1], string_descriptors[0], 2U);
        count = 1U;
    } else {
        const char *text = string_descriptors[index];
        size_t limit = (sizeof(string_descriptor) /
                        sizeof(string_descriptor[0])) - 1U;

        count = strlen(text);
        if (count > limit) {
            count = limit;
        }
        for (size_t position = 0U; position < count; ++position) {
            string_descriptor[position + 1U] = (uint8_t)text[position];
        }
    }

    string_descriptor[0] = (uint16_t)((TUSB_DESC_STRING << 8U) |
                                      (uint16_t)(2U * count + 2U));
    return string_descriptor;
}
