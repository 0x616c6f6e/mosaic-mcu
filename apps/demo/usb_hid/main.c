#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <chip_time.h>
#include <chip_usb_device.h>

#include "demo_support.h"

#define USB_REQUEST_DIRECTION_IN  UINT8_C(0x80)
#define USB_REQUEST_TYPE_MASK     UINT8_C(0x60)
#define USB_REQUEST_TYPE_STANDARD UINT8_C(0x00)
#define USB_REQUEST_TYPE_CLASS    UINT8_C(0x20)
#define USB_REQUEST_RECIPIENT_MASK UINT8_C(0x1F)
#define USB_REQUEST_RECIPIENT_DEVICE UINT8_C(0x00)
#define USB_REQUEST_RECIPIENT_ENDPOINT UINT8_C(0x02)

#define USB_REQUEST_GET_STATUS        UINT8_C(0x00)
#define USB_REQUEST_CLEAR_FEATURE     UINT8_C(0x01)
#define USB_REQUEST_SET_FEATURE       UINT8_C(0x03)
#define USB_REQUEST_SET_ADDRESS       UINT8_C(0x05)
#define USB_REQUEST_GET_DESCRIPTOR    UINT8_C(0x06)
#define USB_REQUEST_GET_CONFIGURATION UINT8_C(0x08)
#define USB_REQUEST_SET_CONFIGURATION UINT8_C(0x09)
#define USB_REQUEST_GET_INTERFACE     UINT8_C(0x0A)
#define USB_REQUEST_SET_INTERFACE     UINT8_C(0x0B)

#define HID_REQUEST_GET_IDLE      UINT8_C(0x02)
#define HID_REQUEST_GET_PROTOCOL  UINT8_C(0x03)
#define HID_REQUEST_SET_REPORT    UINT8_C(0x09)
#define HID_REQUEST_SET_IDLE      UINT8_C(0x0A)
#define HID_REQUEST_SET_PROTOCOL  UINT8_C(0x0B)

#define USB_DESCRIPTOR_DEVICE       UINT8_C(0x01)
#define USB_DESCRIPTOR_CONFIGURATION UINT8_C(0x02)
#define USB_DESCRIPTOR_STRING       UINT8_C(0x03)
#define USB_DESCRIPTOR_HID          UINT8_C(0x21)
#define USB_DESCRIPTOR_HID_REPORT   UINT8_C(0x22)

#define USB_FEATURE_ENDPOINT_HALT UINT16_C(0x0000)
#define USB_FEATURE_REMOTE_WAKEUP UINT16_C(0x0001)

typedef enum {
    CONTROL_IDLE,
    CONTROL_DATA_IN,
    CONTROL_DATA_OUT,
    CONTROL_STATUS_IN,
    CONTROL_STATUS_OUT,
} control_state_t;

static const uint8_t hid_report_descriptor[] = {
    0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x05, 0x07,
    0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01,
    0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01,
    0x75, 0x08, 0x81, 0x01, 0x95, 0x05, 0x75, 0x01,
    0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02,
    0x95, 0x01, 0x75, 0x03, 0x91, 0x01, 0x95, 0x06,
    0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07,
    0x19, 0x00, 0x29, 0x65, 0x81, 0x00, 0xC0,
};

static const uint8_t device_descriptor[] = {
    18, USB_DESCRIPTOR_DEVICE, 0x00, 0x02, 0x00, 0x00, 0x00, 64,
    0x09, 0x12, 0x50, 0x58, 0x00, 0x01, 1, 2, 0, 1,
};

static const uint8_t configuration_descriptor[] = {
    9, USB_DESCRIPTOR_CONFIGURATION, 34, 0, 1, 1, 0, 0xA0, 50,
    9, 0x04, 0, 0, 1, 0x03, 0x01, 0x01, 0,
    9, USB_DESCRIPTOR_HID, 0x11, 0x01, 0, 1, USB_DESCRIPTOR_HID_REPORT,
    (uint8_t)sizeof(hid_report_descriptor), 0,
    7, 0x05, 0x81, 0x03, 8, 0, 10,
};

static const uint8_t language_descriptor[] = {4, USB_DESCRIPTOR_STRING, 0x09, 0x04};
static const uint8_t manufacturer_descriptor[] = {
    28, USB_DESCRIPTOR_STRING,
    'O', 0, 'p', 0, 'e', 0, 'n', 0, ' ', 0, 'P', 0, 'l', 0,
    'a', 0, 't', 0, 'f', 0, 'o', 0, 'r', 0, 'm', 0,
};
static const uint8_t product_descriptor[] = {
    38, USB_DESCRIPTOR_STRING,
    'C', 0, 'H', 0, '5', 0, '8', 0, '5', 0, ' ', 0, 'U', 0,
    'S', 0, 'B', 0, ' ', 0, 'H', 0, 'I', 0, 'D', 0, ' ', 0,
    'D', 0, 'e', 0, 'm', 0, 'o', 0,
};

static volatile control_state_t control_state;
static volatile bool usb_configured;
static volatile bool report_busy;
static uint8_t configuration_value;
static uint8_t idle_rate;
static uint8_t protocol_value = 1;
static uint8_t control_reply[2];
static uint8_t output_report;

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8U));
}

static void control_stall(void)
{
    (void)chip_usb_device_endpoint_stall(0x00);
    (void)chip_usb_device_endpoint_stall(0x80);
    control_state = CONTROL_IDLE;
}

static void control_status_in(void)
{
    control_state = CONTROL_STATUS_IN;
    if (chip_usb_device_submit_in(0, NULL, 0) != CHIP_OK) {
        control_stall();
    }
}

static void control_send(const uint8_t *data, size_t available, uint16_t requested)
{
    size_t size = (available > requested) ? requested : available;

    control_state = CONTROL_DATA_IN;
    if (chip_usb_device_submit_in(0, data, size) != CHIP_OK) {
        control_stall();
    }
}

static bool descriptor_for_request(uint16_t value,
                                   const uint8_t **descriptor,
                                   size_t *size)
{
    uint8_t type = (uint8_t)(value >> 8U);
    uint8_t index = (uint8_t)value;

    switch (type) {
    case USB_DESCRIPTOR_DEVICE:
        *descriptor = device_descriptor;
        *size = sizeof(device_descriptor);
        return true;
    case USB_DESCRIPTOR_CONFIGURATION:
        *descriptor = configuration_descriptor;
        *size = sizeof(configuration_descriptor);
        return true;
    case USB_DESCRIPTOR_STRING:
        if (index == 0U) {
            *descriptor = language_descriptor;
            *size = sizeof(language_descriptor);
        } else if (index == 1U) {
            *descriptor = manufacturer_descriptor;
            *size = sizeof(manufacturer_descriptor);
        } else if (index == 2U) {
            *descriptor = product_descriptor;
            *size = sizeof(product_descriptor);
        } else {
            return false;
        }
        return true;
    case USB_DESCRIPTOR_HID:
        *descriptor = &configuration_descriptor[18];
        *size = 9;
        return true;
    case USB_DESCRIPTOR_HID_REPORT:
        *descriptor = hid_report_descriptor;
        *size = sizeof(hid_report_descriptor);
        return true;
    default:
        return false;
    }
}

static void set_configuration(uint8_t value)
{
    if (value > 1U) {
        control_stall();
        return;
    }
    if ((value == 1U) && !usb_configured) {
        if (chip_usb_device_endpoint_open(0x81, CHIP_USB_ENDPOINT_INTERRUPT, 8) != CHIP_OK) {
            control_stall();
            return;
        }
        usb_configured = true;
        (void)demo_led_write(true);
    } else if ((value == 0U) && usb_configured) {
        (void)chip_usb_device_endpoint_close(0x81);
        usb_configured = false;
        report_busy = false;
        (void)demo_led_write(false);
    }
    configuration_value = value;
    control_status_in();
}

static void handle_standard_request(const uint8_t *setup)
{
    uint8_t request_type = setup[0];
    uint8_t request = setup[1];
    uint16_t value = read_le16(&setup[2]);
    uint16_t index = read_le16(&setup[4]);
    uint16_t length = read_le16(&setup[6]);
    const uint8_t *descriptor;
    size_t descriptor_size;

    switch (request) {
    case USB_REQUEST_GET_DESCRIPTOR:
        if (((request_type & USB_REQUEST_DIRECTION_IN) == 0U) ||
            !descriptor_for_request(value, &descriptor, &descriptor_size)) {
            control_stall();
            return;
        }
        control_send(descriptor, descriptor_size, length);
        break;
    case USB_REQUEST_SET_ADDRESS:
        if ((value > 127U) || (chip_usb_device_set_address((uint8_t)value) != CHIP_OK)) {
            control_stall();
            return;
        }
        control_status_in();
        break;
    case USB_REQUEST_SET_CONFIGURATION:
        if (value > 1U) {
            control_stall();
        } else {
            set_configuration((uint8_t)value);
        }
        break;
    case USB_REQUEST_GET_CONFIGURATION:
        control_reply[0] = configuration_value;
        control_send(control_reply, 1, length);
        break;
    case USB_REQUEST_GET_STATUS:
        control_reply[0] = 0;
        control_reply[1] = 0;
        control_send(control_reply, sizeof(control_reply), length);
        break;
    case USB_REQUEST_GET_INTERFACE:
        control_reply[0] = 0;
        control_send(control_reply, 1, length);
        break;
    case USB_REQUEST_SET_INTERFACE:
        control_status_in();
        break;
    case USB_REQUEST_CLEAR_FEATURE:
    case USB_REQUEST_SET_FEATURE:
        if (((request_type & USB_REQUEST_RECIPIENT_MASK) == USB_REQUEST_RECIPIENT_ENDPOINT) &&
            (value == USB_FEATURE_ENDPOINT_HALT)) {
            chip_status_t status = (request == USB_REQUEST_SET_FEATURE) ?
                chip_usb_device_endpoint_stall((uint8_t)index) :
                chip_usb_device_endpoint_clear_stall((uint8_t)index);
            if (status == CHIP_OK) {
                control_status_in();
                break;
            }
        } else if (((request_type & USB_REQUEST_RECIPIENT_MASK) == USB_REQUEST_RECIPIENT_DEVICE) &&
                   (value == USB_FEATURE_REMOTE_WAKEUP)) {
            control_status_in();
            break;
        }
        control_stall();
        break;
    default:
        control_stall();
        break;
    }
}

static void handle_class_request(const uint8_t *setup)
{
    uint8_t request = setup[1];
    uint16_t value = read_le16(&setup[2]);
    uint16_t length = read_le16(&setup[6]);

    switch (request) {
    case HID_REQUEST_SET_IDLE:
        idle_rate = (uint8_t)(value >> 8U);
        control_status_in();
        break;
    case HID_REQUEST_GET_IDLE:
        control_reply[0] = idle_rate;
        control_send(control_reply, 1, length);
        break;
    case HID_REQUEST_SET_PROTOCOL:
        if (value > 1U) {
            control_stall();
        } else {
            protocol_value = (uint8_t)value;
            control_status_in();
        }
        break;
    case HID_REQUEST_GET_PROTOCOL:
        control_reply[0] = protocol_value;
        control_send(control_reply, 1, length);
        break;
    case HID_REQUEST_SET_REPORT:
        if (length > 1U) {
            control_stall();
            return;
        }
        if (length == 0U) {
            control_status_in();
        } else {
            control_state = CONTROL_DATA_OUT;
            if (chip_usb_device_submit_out(0, &output_report, 1) != CHIP_OK) {
                control_stall();
            }
        }
        break;
    default:
        control_stall();
        break;
    }
}

static void handle_setup(const uint8_t *setup)
{
    uint8_t type = setup[0] & USB_REQUEST_TYPE_MASK;

    control_state = CONTROL_IDLE;
    if (type == USB_REQUEST_TYPE_STANDARD) {
        handle_standard_request(setup);
    } else if (type == USB_REQUEST_TYPE_CLASS) {
        handle_class_request(setup);
    } else {
        control_stall();
    }
}

static void usb_event_callback(const chip_usb_event_t *event, void *context)
{
    (void)context;

    switch (event->type) {
    case CHIP_USB_EVENT_BUS_RESET:
        control_state = CONTROL_IDLE;
        configuration_value = 0;
        usb_configured = false;
        report_busy = false;
        (void)demo_led_write(false);
        break;
    case CHIP_USB_EVENT_SETUP_RECEIVED:
        handle_setup(event->data.setup.bytes);
        break;
    case CHIP_USB_EVENT_TRANSFER_COMPLETE:
        if (event->data.transfer.endpoint_address == 0x80U) {
            if (control_state == CONTROL_DATA_IN) {
                control_state = CONTROL_STATUS_OUT;
                if (chip_usb_device_submit_out(0, NULL, 0) != CHIP_OK) {
                    control_stall();
                }
            } else {
                control_state = CONTROL_IDLE;
            }
        } else if (event->data.transfer.endpoint_address == 0x00U) {
            if (control_state == CONTROL_DATA_OUT) {
                (void)demo_led_write((output_report & UINT8_C(0x02)) != 0U);
                control_status_in();
            } else {
                control_state = CONTROL_IDLE;
            }
        } else if (event->data.transfer.endpoint_address == 0x81U) {
            report_busy = false;
        }
        break;
    case CHIP_USB_EVENT_SUSPEND:
    case CHIP_USB_EVENT_RESUME:
    case CHIP_USB_EVENT_ERROR:
    default:
        break;
    }
}

int main(void)
{
    chip_usb_device_config_t usb_config = {
        .endpoint0_max_packet_size = 64,
        .callback = usb_event_callback,
        .callback_context = NULL,
    };
    uint8_t report[8] = {0};
    bool key_down = false;

    DEMO_REQUIRE(demo_platform_init());
    DEMO_REQUIRE(demo_led_init());
    DEMO_REQUIRE(chip_usb_device_init(&usb_config));
    DEMO_REQUIRE(chip_usb_device_connect());

    for (;;) {
        if (usb_configured && !report_busy && !chip_usb_device_is_suspended()) {
            report[2] = key_down ? 0U : UINT8_C(0x04);
            if (chip_usb_device_submit_in(1, report, sizeof(report)) == CHIP_OK) {
                report_busy = true;
                key_down = !key_down;
            }
        }
        chip_delay_ms(500);
    }
}
