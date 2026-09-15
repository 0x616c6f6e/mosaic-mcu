#ifndef CHIP_API_USB_DEVICE_H
#define CHIP_API_USB_DEVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Number of hardware endpoint numbers, including endpoint zero. */
#define CHIP_USB_DEVICE_ENDPOINT_COUNT     8U
/** @brief Largest full-speed endpoint packet in bytes. */
#define CHIP_USB_DEVICE_MAX_PACKET_SIZE    64U
/** @brief Direction bit for device-to-host endpoint addresses. */
#define CHIP_USB_ENDPOINT_DIRECTION_IN     UINT8_C(0x80)
/** @brief Endpoint-number bits in a USB endpoint address. */
#define CHIP_USB_ENDPOINT_NUMBER_MASK      UINT8_C(0x0F)

/** @brief Combine endpoint number and direction into a USB address. */
#define CHIP_USB_ENDPOINT_ADDRESS(number, direction) \
    ((uint8_t)(((uint8_t)(number) & CHIP_USB_ENDPOINT_NUMBER_MASK) | (uint8_t)(direction)))
/** @brief Extract the endpoint number from a USB address. */
#define CHIP_USB_ENDPOINT_NUMBER(address) \
    ((uint8_t)((uint8_t)(address) & CHIP_USB_ENDPOINT_NUMBER_MASK))
/** @brief Check whether a USB endpoint address points device-to-host. */
#define CHIP_USB_ENDPOINT_IS_IN(address) \
    ((((uint8_t)(address)) & CHIP_USB_ENDPOINT_DIRECTION_IN) != 0U)

/** @brief USB endpoint transfer types. */
typedef enum {
    CHIP_USB_ENDPOINT_CONTROL,
    CHIP_USB_ENDPOINT_ISOCHRONOUS,
    CHIP_USB_ENDPOINT_BULK,
    CHIP_USB_ENDPOINT_INTERRUPT,
} chip_usb_endpoint_type_t;

/** @brief USB bus and endpoint event categories. */
typedef enum {
    CHIP_USB_EVENT_BUS_RESET,
    CHIP_USB_EVENT_SUSPEND,
    CHIP_USB_EVENT_RESUME,
    CHIP_USB_EVENT_SETUP_RECEIVED,
    CHIP_USB_EVENT_TRANSFER_COMPLETE,
    CHIP_USB_EVENT_ERROR,
} chip_usb_event_type_t;

/** @brief USB controller error categories. */
typedef enum {
    CHIP_USB_ERROR_FIFO_OVERFLOW,
    CHIP_USB_ERROR_INVALID_TRANSACTION,
} chip_usb_error_t;

/** @brief USB bus, setup, transfer, or error event. */
typedef struct {
    chip_usb_event_type_t type; /**< Discriminator for the data union. */
    union {
        struct {
            uint8_t speed_mbps; /**< Bus speed in Mb/s after reset. */
        } bus_reset;
        struct {
            uint8_t bytes[8]; /**< Eight-byte setup request. */
        } setup;
        struct {
            uint8_t endpoint_address; /**< Completed endpoint and direction. */
            size_t transferred;       /**< Transferred bytes. */
        } transfer;
        struct {
            chip_usb_error_t code;    /**< Hardware error category. */
            uint8_t endpoint_address; /**< Endpoint associated with the error. */
        } error;
    } data; /**< Payload selected by type. */
} chip_usb_event_t;

/** @brief USB event callback; runs in interrupt context and must not block. */
typedef void (*chip_usb_event_callback_t)(const chip_usb_event_t *event, void *context);

/** @brief USB endpoint-zero geometry and interrupt event callback. */
typedef struct {
    uint8_t endpoint0_max_packet_size; /**< EP0 max packet size in bytes. */
    chip_usb_event_callback_t callback; /**< Nonblocking USB event handler. */
    void *callback_context;             /**< Opaque handler context. */
} chip_usb_device_config_t;

/** @brief Initialize the USB device controller.
 * @param config Endpoint-zero size and nonblocking event handler.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_init(const chip_usb_device_config_t *config);
/** @brief Release the USB device controller.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_deinit(void);
/** @brief Connect the device to the USB bus.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_connect(void);
/** @brief Disconnect the device from the USB bus.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_disconnect(void);
/** @brief Enable or disable USB device interrupts.
 * @param enabled true to enable interrupts.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_set_interrupt_enabled(bool enabled);
/** @brief Check whether the USB bus is suspended.
 * @return true when suspended.
 */
bool chip_usb_device_is_suspended(void);

/** @brief Set the USB device address.
 * @note Takes effect after the next successful endpoint-zero IN status packet.
 * @param address New USB device address.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_set_address(uint8_t address);
/** @brief Signal remote wakeup to a suspended USB host.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_remote_wakeup(void);

/** @brief Configure a USB endpoint.
 * @param endpoint_address Number and direction encoded in the USB address.
 * @param type Control, bulk, interrupt, or isochronous endpoint.
 * @param max_packet_size Maximum packet size in bytes.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_endpoint_open(uint8_t endpoint_address,
                                            chip_usb_endpoint_type_t type,
                                            uint16_t max_packet_size);
/** @brief Close a USB endpoint.
 * @param endpoint_address Number and direction.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_endpoint_close(uint8_t endpoint_address);
/** @brief Stall a USB endpoint.
 * @param endpoint_address Number and direction.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_endpoint_stall(uint8_t endpoint_address);
/** @brief Clear a USB endpoint's stall condition.
 * @param endpoint_address Number and direction.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_endpoint_clear_stall(uint8_t endpoint_address);
/** @brief Cancel an endpoint's pending transfer.
 * @param endpoint_address Number and direction.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_endpoint_cancel(uint8_t endpoint_address);

/** @brief Queue an IN transfer to the host.
 * @note data must remain valid until the transfer-complete callback.
 * @param endpoint_number Endpoint number without direction bit.
 * @param data Bytes to send.
 * @param size Transfer length in bytes.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_submit_in(uint8_t endpoint_number,
                                        const void *data,
                                        size_t size);
/** @brief Queue an OUT transfer from the host.
 * @note data must remain valid until the transfer-complete callback.
 * @param endpoint_number Endpoint number without direction bit.
 * @param[out] data Destination buffer.
 * @param size Buffer capacity in bytes.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_usb_device_submit_out(uint8_t endpoint_number,
                                         void *data,
                                         size_t size);

#ifdef __cplusplus
}
#endif

#endif
