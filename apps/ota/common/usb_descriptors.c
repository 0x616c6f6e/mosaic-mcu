#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "tusb.h"

#include "ota_usb_device.h"

#define USB_VID UINT16_C(0xCAFE)
/* A new PID avoids stale host-side descriptors from the former MSC-only device. */
#define USB_PID UINT16_C(0x4113)

#define ITF_NUM_MSC    0
#define ITF_NUM_VENDOR 1
#define ITF_NUM_TOTAL  2

#define VENDOR_REQUEST_WEBUSB    1
#define VENDOR_REQUEST_MICROSOFT 2
#define MS_OS_20_DESC_LEN        0xB2
#define BOS_TOTAL_LENGTH \
    (TUD_BOS_DESC_LEN + TUD_BOS_WEBUSB_DESC_LEN + \
     TUD_BOS_MICROSOFT_OS_DESC_LEN)
#define CONFIG_TOTAL_LENGTH \
    (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN + TUD_VENDOR_DESC_LEN)

static const tusb_desc_device_t device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0210,
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0201,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 0,
    .bNumConfigurations = 1,
};

static const uint8_t configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LENGTH,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 0, 0x01, 0x81, 64),
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 3, 0x02, 0x82, 64),
};

static const uint8_t bos_descriptor[] = {
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LENGTH, 2),
    TUD_BOS_WEBUSB_DESCRIPTOR(VENDOR_REQUEST_WEBUSB, 1),
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN,
                                VENDOR_REQUEST_MICROSOFT),
};

static const uint8_t ms_os_20_descriptor[] = {
    U16_TO_U8S_LE(0x000A),
    U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
    U32_TO_U8S_LE(0x06030000),
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

    U16_TO_U8S_LE(0x0008),
    U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION),
    0, 0,
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),

    U16_TO_U8S_LE(0x0008),
    U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION),
    ITF_NUM_VENDOR, 0,
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08),

    U16_TO_U8S_LE(0x0014),
    U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
    'W', 'I', 'N', 'U', 'S', 'B', 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08 - 0x08 - 0x14),
    U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
    U16_TO_U8S_LE(0x0007),
    U16_TO_U8S_LE(0x002A),
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0,
    'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0,
    'a', 0, 'c', 0, 'e', 0, 'G', 0, 'U', 0, 'I', 0,
    'D', 0, 's', 0, 0, 0,
    U16_TO_U8S_LE(0x0050),
    '{', 0, 'A', 0, '3', 0, '0', 0, 'C', 0, '7', 0, '2', 0,
    'A', 0, '4', 0, '-', 0, '7', 0, '8', 0, '2', 0, 'E', 0,
    '-', 0, '4', 0, 'F', 0, 'A', 0, '2', 0, '-', 0, 'B', 0,
    '6', 0, 'D', 0, '8', 0, '-', 0, '1', 0, 'A', 0, '0', 0,
    'C', 0, '4', 0, 'D', 0, '5', 0, '8', 0, '5', 0, '0', 0,
    '0', 0, '1', 0, '}', 0, 0, 0, 0, 0,
};

static const char landing_page[] = "localhost:8000";
static const struct {
    uint8_t length;
    uint8_t descriptor_type;
    uint8_t scheme;
    char url[sizeof(landing_page) - 1U];
} webusb_url_descriptor = {
    .length = (uint8_t)(3U + sizeof(landing_page) - 1U),
    .descriptor_type = 3,
    .scheme = 0,
    .url = "localhost:8000",
};

static char product_name[32] = "CH585 OTA Disk + WebUSB";
static const char *const string_descriptors[] = {
    (const char[]){0x09, 0x04},
    "Mosaic MCU",
    product_name,
    "WebUSB OTA",
};
static uint16_t string_descriptor[32];

TU_VERIFY_STATIC(sizeof(ms_os_20_descriptor) == MS_OS_20_DESC_LEN,
                 "Incorrect Microsoft OS descriptor size");

void ota_usb_set_product_name(const char *name)
{
    size_t length;

    if (name == NULL) {
        return;
    }
    length = strlen(name);
    if ((length == 0U) || (length >= sizeof(product_name))) {
        return;
    }
    memcpy(product_name, name, length + 1U);
}

const uint8_t *tud_descriptor_device_cb(void)
{
    return (const uint8_t *)&device_descriptor;
}

const uint8_t *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return configuration_descriptor;
}

const uint8_t *tud_descriptor_bos_cb(void)
{
    return bos_descriptor;
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                const tusb_control_request_t *request)
{
    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }
    if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR) {
        return false;
    }
    if ((request->bRequest == VENDOR_REQUEST_WEBUSB) &&
        (request->wIndex == 2U)) {
        return tud_control_xfer(rhport, request,
                                (void *)(uintptr_t)&webusb_url_descriptor,
                                webusb_url_descriptor.length);
    }
    if ((request->bRequest == VENDOR_REQUEST_MICROSOFT) &&
        (request->wIndex == 7U)) {
        return tud_control_xfer(rhport, request,
                                (void *)(uintptr_t)ms_os_20_descriptor,
                                sizeof(ms_os_20_descriptor));
    }
    return false;
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
