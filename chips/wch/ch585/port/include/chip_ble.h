#ifndef CHIP_API_BLE_H
#define CHIP_API_BLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum supported BLE device-name bytes. */
#define CHIP_BLE_DEVICE_NAME_MAX_SIZE 21U
/** @brief Maximum legacy advertising or scan-response bytes. */
#define CHIP_BLE_LEGACY_ADV_MAX_SIZE  31U
/** @brief Sentinel used when no BLE peer is connected. */
#define CHIP_BLE_CONN_HANDLE_INVALID  UINT16_MAX

/** @brief Lifecycle states of a BLE peripheral. */
typedef enum {
    CHIP_BLE_STATE_UNINITIALIZED = 0,
    CHIP_BLE_STATE_READY,
    CHIP_BLE_STATE_ADVERTISING,
    CHIP_BLE_STATE_CONNECTED,
    CHIP_BLE_STATE_ERROR,
} chip_ble_state_t;

/** @brief Event categories dispatched to the application callback. */
typedef enum {
    CHIP_BLE_EVENT_STATE_CHANGED = 0,
    CHIP_BLE_EVENT_CONNECTED,
    CHIP_BLE_EVENT_DISCONNECTED,
    CHIP_BLE_EVENT_ERROR,
} chip_ble_event_type_t;

/** @brief BLE state transition or connection event. */
typedef struct {
    chip_ble_event_type_t type;   /**< Event category. */
    chip_ble_state_t state;       /**< Resulting peripheral state. */
    uint16_t connection_handle;   /**< Peer handle, or invalid sentinel. */
    uint8_t reason;               /**< Disconnect or error reason code. */
    uint8_t status;               /**< Stack status code. */
} chip_ble_event_t;

/** @brief BLE event handler called by the BLE process loop. */
typedef void (*chip_ble_event_callback_t)(const chip_ble_event_t *event,
                                          void *context);

/** @brief BLE peripheral identity, advertising data, and connection settings. */
typedef struct {
    const char *device_name;          /**< NUL-terminated BLE identity. */
    const uint8_t *advertising_data;  /**< Raw legacy advertising data. */
    size_t advertising_data_size;     /**< Advertising data length in bytes. */
    const uint8_t *scan_response_data; /**< Raw scan-response data. */
    size_t scan_response_data_size;   /**< Scan-response length in bytes. */
    uint16_t advertising_interval;    /**< Interval in 0.625 ms units. */
    uint16_t min_connection_interval; /**< Minimum interval in 1.25 ms units. */
    uint16_t max_connection_interval; /**< Maximum interval in 1.25 ms units. */
    int8_t tx_power_dbm;              /**< Requested TX power in dBm. */
    bool advertise_on_start;          /**< Start advertising during init. */
    chip_ble_event_callback_t event_callback; /**< Optional event handler. */
    void *event_context;              /**< Value passed to event_callback. */
} chip_ble_peripheral_config_t;

/** @brief Fill a BLE peripheral configuration with default values.
 * @param[out] config Configuration to initialize.
 */
void chip_ble_peripheral_config_default(chip_ble_peripheral_config_t *config);
/** @brief Initialize the BLE peripheral stack.
 * @param config Advertising, connection, and callback settings.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_ble_peripheral_init(const chip_ble_peripheral_config_t *config);
/** @brief Process pending BLE events; call regularly from the main loop. */
void chip_ble_process(void);
/** @brief Start or stop BLE advertising.
 * @param enabled true to advertise, false to stop.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_ble_set_advertising(bool enabled);
/** @brief Query the current BLE peripheral state.
 * @return Current state.
 */
chip_ble_state_t chip_ble_state(void);
/** @brief Query the connected peer's handle.
 * @return Connection handle, or CHIP_BLE_CONN_HANDLE_INVALID.
 */
uint16_t chip_ble_connection_handle(void);

#ifdef __cplusplus
}
#endif

#endif
