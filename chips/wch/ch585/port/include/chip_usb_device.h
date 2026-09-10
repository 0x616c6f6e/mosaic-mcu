#ifndef CHIP_API_USB_DEVICE_H
#define CHIP_API_USB_DEVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHIP_USB_DEVICE_ENDPOINT_COUNT     8U
#define CHIP_USB_DEVICE_MAX_PACKET_SIZE    64U
#define CHIP_USB_ENDPOINT_DIRECTION_IN     UINT8_C(0x80)
#define CHIP_USB_ENDPOINT_NUMBER_MASK      UINT8_C(0x0F)

#define CHIP_USB_ENDPOINT_ADDRESS(number, direction) \
    ((uint8_t)(((uint8_t)(number) & CHIP_USB_ENDPOINT_NUMBER_MASK) | (uint8_t)(direction)))
#define CHIP_USB_ENDPOINT_NUMBER(address) \
    ((uint8_t)((uint8_t)(address) & CHIP_USB_ENDPOINT_NUMBER_MASK))
#define CHIP_USB_ENDPOINT_IS_IN(address) \
    ((((uint8_t)(address)) & CHIP_USB_ENDPOINT_DIRECTION_IN) != 0U)

typedef enum {
    CHIP_USB_ENDPOINT_CONTROL,
    CHIP_USB_ENDPOINT_ISOCHRONOUS,
    CHIP_USB_ENDPOINT_BULK,
    CHIP_USB_ENDPOINT_INTERRUPT,
} chip_usb_endpoint_type_t;

typedef enum {
    CHIP_USB_EVENT_BUS_RESET,
    CHIP_USB_EVENT_SUSPEND,
    CHIP_USB_EVENT_RESUME,
    CHIP_USB_EVENT_SETUP_RECEIVED,
    CHIP_USB_EVENT_TRANSFER_COMPLETE,
    CHIP_USB_EVENT_ERROR,
} chip_usb_event_type_t;

typedef enum {
    CHIP_USB_ERROR_FIFO_OVERFLOW,
    CHIP_USB_ERROR_INVALID_TRANSACTION,
} chip_usb_error_t;

typedef struct {
    chip_usb_event_type_t type;
    union {
        struct {
            uint8_t speed_mbps;
        } bus_reset;
        struct {
            uint8_t bytes[8];
        } setup;
        struct {
            uint8_t endpoint_address;
            size_t transferred;
        } transfer;
        struct {
            chip_usb_error_t code;
            uint8_t endpoint_address;
        } error;
    } data;
} chip_usb_event_t;

/* The callback executes in USB interrupt context and must not block. */
typedef void (*chip_usb_event_callback_t)(const chip_usb_event_t *event, void *context);

typedef struct {
    uint8_t endpoint0_max_packet_size;
    chip_usb_event_callback_t callback;
    void *callback_context;
} chip_usb_device_config_t;

chip_status_t chip_usb_device_init(const chip_usb_device_config_t *config);
chip_status_t chip_usb_device_deinit(void);
chip_status_t chip_usb_device_connect(void);
chip_status_t chip_usb_device_disconnect(void);
chip_status_t chip_usb_device_set_interrupt_enabled(bool enabled);
bool chip_usb_device_is_suspended(void);

/* The address takes effect after the next successful endpoint-zero IN status packet. */
chip_status_t chip_usb_device_set_address(uint8_t address);
chip_status_t chip_usb_device_remote_wakeup(void);

chip_status_t chip_usb_device_endpoint_open(uint8_t endpoint_address,
                                            chip_usb_endpoint_type_t type,
                                            uint16_t max_packet_size);
chip_status_t chip_usb_device_endpoint_close(uint8_t endpoint_address);
chip_status_t chip_usb_device_endpoint_stall(uint8_t endpoint_address);
chip_status_t chip_usb_device_endpoint_clear_stall(uint8_t endpoint_address);
chip_status_t chip_usb_device_endpoint_cancel(uint8_t endpoint_address);

/* Buffers must remain valid until the transfer-complete callback. */
chip_status_t chip_usb_device_submit_in(uint8_t endpoint_number,
                                        const void *data,
                                        size_t size);
chip_status_t chip_usb_device_submit_out(uint8_t endpoint_number,
                                         void *data,
                                         size_t size);

#ifdef __cplusplus
}
#endif

#endif
