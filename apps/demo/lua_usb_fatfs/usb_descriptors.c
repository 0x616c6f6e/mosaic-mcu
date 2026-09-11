#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "tusb.h"

#define USB_VID UINT16_C(0xCAFE)
#define USB_PID UINT16_C(0x4020)
#define MSC_ENDPOINT_OUT UINT8_C(0x01)
#define MSC_ENDPOINT_IN  UINT8_C(0x81)
#define CONFIG_TOTAL_LENGTH (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

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

static const uint8_t configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LENGTH,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_MSC_DESCRIPTOR(0, 0, MSC_ENDPOINT_OUT, MSC_ENDPOINT_IN, 64),
};

TU_VERIFY_STATIC(sizeof(configuration_descriptor) == CONFIG_TOTAL_LENGTH,
                 "Incorrect MSC configuration descriptor size");

static const char *const string_descriptors[] = {
    (const char[]){0x09, 0x04},
    "Mosaic MCU",
    "CH585 Lua Script Disk",
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
        size_t position;

        count = strlen(text);
        if (count > limit) {
            count = limit;
        }
        for (position = 0U; position < count; ++position) {
            string_descriptor[position + 1U] = (uint8_t)text[position];
        }
    }
    string_descriptor[0] = (uint16_t)((TUSB_DESC_STRING << 8U) |
                                      (uint16_t)(2U * count + 2U));
    return string_descriptor;
}
