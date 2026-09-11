#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "tusb.h"

#include <keyboard_usb_device.h>

#define USB_PID_DISK_VISIBLE UINT16_C(0x4113)
#define USB_PID_DISK_HIDDEN  UINT16_C(0x4114)
#define MS_OS_20_FUNCTION_INTERFACE_OFFSET 22U

static void *control_buffer;
static uint16_t control_length;

bool tud_control_xfer(uint8_t rhport, const tusb_control_request_t *request,
                      void *buffer, uint16_t length)
{
    (void)rhport;
    (void)request;
    control_buffer = buffer;
    control_length = length;
    return true;
}

static void check_ms_os_interface(uint8_t expected_interface)
{
    const tusb_control_request_t request = {
        .bmRequestType_bit = {
            .recipient = TUSB_REQ_RCPT_DEVICE,
            .type = TUSB_REQ_TYPE_VENDOR,
            .direction = TUSB_DIR_IN,
        },
        .bRequest = 2U,
        .wIndex = 7U,
    };

    control_buffer = NULL;
    control_length = 0U;
    assert(tud_vendor_control_xfer_cb(0U, CONTROL_STAGE_SETUP, &request));
    assert(control_buffer != NULL);
    assert(control_length == 0xB2U);
    assert(((const uint8_t *)control_buffer)
               [MS_OS_20_FUNCTION_INTERFACE_OFFSET] == expected_interface);
}

static void check_disk_visible(void)
{
    const tusb_desc_device_t *device;
    const tusb_desc_configuration_t *configuration;
    const tusb_desc_interface_t *first_interface;

    keyboard_usb_set_disk_visible(true);
    device = (const tusb_desc_device_t *)tud_descriptor_device_cb();
    configuration = (const tusb_desc_configuration_t *)
        tud_descriptor_configuration_cb(0U);
    first_interface = (const tusb_desc_interface_t *)
        ((const uint8_t *)configuration + sizeof(*configuration));

    assert(device->idProduct == USB_PID_DISK_VISIBLE);
#ifdef KEYBOARD_USB_HID_ENABLED
    assert(configuration->bNumInterfaces == 3U);
    assert(configuration->wTotalLength ==
           (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN + TUD_HID_DESC_LEN +
            TUD_VENDOR_DESC_LEN));
    check_ms_os_interface(2U);
#else
    assert(configuration->bNumInterfaces == 2U);
    assert(configuration->wTotalLength ==
           (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN + TUD_VENDOR_DESC_LEN));
    check_ms_os_interface(1U);
#endif
    assert(first_interface->bInterfaceNumber == 0U);
    assert(first_interface->bInterfaceClass == TUSB_CLASS_MSC);
}

static void check_webusb_only(void)
{
    const tusb_desc_device_t *device;
    const tusb_desc_configuration_t *configuration;
    const tusb_desc_interface_t *first_interface;

    keyboard_usb_set_disk_visible(false);
    device = (const tusb_desc_device_t *)tud_descriptor_device_cb();
    configuration = (const tusb_desc_configuration_t *)
        tud_descriptor_configuration_cb(0U);
    first_interface = (const tusb_desc_interface_t *)
        ((const uint8_t *)configuration + sizeof(*configuration));

    assert(device->idProduct == USB_PID_DISK_HIDDEN);
#ifdef KEYBOARD_USB_HID_ENABLED
    assert(configuration->bNumInterfaces == 2U);
    assert(configuration->wTotalLength ==
           (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_VENDOR_DESC_LEN));
    assert(first_interface->bInterfaceClass == TUSB_CLASS_HID);
    assert(tud_hid_descriptor_report_cb(0U) != NULL);
    check_ms_os_interface(1U);
#else
    assert(configuration->bNumInterfaces == 1U);
    assert(configuration->wTotalLength ==
           (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN));
    assert(first_interface->bInterfaceClass == TUSB_CLASS_VENDOR_SPECIFIC);
    check_ms_os_interface(0U);
#endif
    assert(first_interface->bInterfaceNumber == 0U);
}

int main(void)
{
    check_disk_visible();
    check_webusb_only();
    return 0;
}
