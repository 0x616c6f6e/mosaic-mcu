#ifndef CHIP_API_BLE_H
#define CHIP_API_BLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHIP_BLE_DEVICE_NAME_MAX_SIZE 21U
#define CHIP_BLE_LEGACY_ADV_MAX_SIZE  31U
#define CHIP_BLE_CONN_HANDLE_INVALID  UINT16_MAX

typedef enum {
    CHIP_BLE_STATE_UNINITIALIZED = 0,
    CHIP_BLE_STATE_READY,
    CHIP_BLE_STATE_ADVERTISING,
    CHIP_BLE_STATE_CONNECTED,
    CHIP_BLE_STATE_ERROR,
} chip_ble_state_t;

typedef enum {
    CHIP_BLE_EVENT_STATE_CHANGED = 0,
    CHIP_BLE_EVENT_CONNECTED,
    CHIP_BLE_EVENT_DISCONNECTED,
    CHIP_BLE_EVENT_ERROR,
} chip_ble_event_type_t;

typedef struct {
    chip_ble_event_type_t type;
    chip_ble_state_t state;
    uint16_t connection_handle;
    uint8_t reason;
    uint8_t status;
} chip_ble_event_t;

typedef void (*chip_ble_event_callback_t)(const chip_ble_event_t *event,
                                          void *context);

typedef struct {
    const char *device_name;
    const uint8_t *advertising_data;
    size_t advertising_data_size;
    const uint8_t *scan_response_data;
    size_t scan_response_data_size;
    uint16_t advertising_interval; /* 0.625 ms units. */
    uint16_t min_connection_interval; /* 1.25 ms units. */
    uint16_t max_connection_interval; /* 1.25 ms units. */
    int8_t tx_power_dbm;
    bool advertise_on_start;
    chip_ble_event_callback_t event_callback;
    void *event_context;
} chip_ble_peripheral_config_t;

void chip_ble_peripheral_config_default(chip_ble_peripheral_config_t *config);
chip_status_t chip_ble_peripheral_init(const chip_ble_peripheral_config_t *config);
void chip_ble_process(void);
chip_status_t chip_ble_set_advertising(bool enabled);
chip_ble_state_t chip_ble_state(void);
uint16_t chip_ble_connection_handle(void);

#ifdef __cplusplus
}
#endif

#endif
