#include <chip_ble.h>
#include <chip_system.h>
#include <chip_time.h>
#include <platform_log.h>

#include "demo_support.h"

static const uint8_t advertising_data[] = {
    0x02, 0x01, 0x06,
    0x0B, 0x09, 'C', 'H', '5', '8', '5', ' ', 'D', 'e', 'm', 'o',
};

static void ble_event(const chip_ble_event_t *event, void *context)
{
    (void)context;

    switch (event->type) {
    case CHIP_BLE_EVENT_CONNECTED:
        LOG_INFO("ble", "connected, handle=0x%04X",
                 (unsigned int)event->connection_handle);
        break;
    case CHIP_BLE_EVENT_DISCONNECTED:
        LOG_WARN("ble", "disconnected, handle=0x%04X reason=0x%02X",
                 (unsigned int)event->connection_handle,
                 (unsigned int)event->reason);
        break;
    case CHIP_BLE_EVENT_ERROR:
        LOG_ERROR("ble", "stack error=0x%02X", (unsigned int)event->status);
        break;
    case CHIP_BLE_EVENT_STATE_CHANGED:
        LOG_DEBUG("ble", "state=%u", (unsigned int)event->state);
        break;
    default:
        break;
    }
}

int main(void)
{
    chip_ble_peripheral_config_t config;
    chip_system_config_t system_config = {
        .source = CHIP_CLOCK_EXTERNAL,
        .core_clock_hz = DEMO_SYSTEM_CLOCK_HZ,
        .external_crystal_load_pf = 18U,
    };
    chip_status_t status;

    DEMO_REQUIRE(chip_system_init(&system_config));
    DEMO_REQUIRE(chip_time_init());
    DEMO_REQUIRE(demo_log_init());
    LOG_INFO("ble", "CH585 peripheral starting, chip_id=0x%02X",
             (unsigned int)chip_system_chip_id());

    chip_ble_peripheral_config_default(&config);
    config.device_name = "CH585 Demo";
    config.advertising_data = advertising_data;
    config.advertising_data_size = sizeof(advertising_data);
    config.event_callback = ble_event;
    status = chip_ble_peripheral_init(&config);
    if (status != CHIP_OK) {
        LOG_ERROR("ble", "init failed, status=%u", (unsigned int)status);
        demo_halt();
    }
    LOG_INFO("ble", "ready");

    for (;;) {
        chip_ble_process();
    }
}
