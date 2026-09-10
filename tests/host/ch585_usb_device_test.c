#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <chip_usb_device.h>

#include "CH58x_common.h"

void USB_IRQHandler(void);

static chip_usb_event_t events[16];
static size_t event_count;

static void event_callback(const chip_usb_event_t *event, void *context)
{
    (void)context;
    assert(event_count < (sizeof(events) / sizeof(events[0])));
    events[event_count++] = *event;
}

static void fire_transfer(uint8_t status, uint8_t received)
{
    fake_usb_int_st = status;
    fake_usb_rx_len = received;
    fake_usb_int_fg = RB_UIF_TRANSFER;
    USB_IRQHandler();
}

static void test_in_and_out_transfers(void)
{
    uint8_t tx[100];
    uint8_t rx[100] = {0};
    uint8_t expected[100];
    uint8_t *endpoint_buffer = (uint8_t *)(uintptr_t)fake_uep_dma[1];
    size_t index;

    assert(chip_usb_device_endpoint_open(0x81, CHIP_USB_ENDPOINT_INTERRUPT, 64) == CHIP_OK);
    assert(chip_usb_device_endpoint_open(0x01, CHIP_USB_ENDPOINT_INTERRUPT, 64) == CHIP_OK);

    for (index = 0; index < sizeof(tx); ++index) {
        tx[index] = (uint8_t)index;
        expected[index] = (uint8_t)(255U - index);
    }

    event_count = 0;
    assert(chip_usb_device_submit_in(1, tx, sizeof(tx)) == CHIP_OK);
    assert(fake_uep_tx_len[1] == 64U);
    assert(memcmp(&endpoint_buffer[64], tx, 64) == 0);
    assert((fake_uep_ctrl[1] & RB_UEP_AUTO_TOG) == 0U);
    assert((fake_uep_ctrl[1] & RB_UEP_T_TOG) == 0U);
    fire_transfer((uint8_t)(UIS_TOKEN_IN | 1U), 0);
    assert(event_count == 0U);
    assert((fake_uep_ctrl[1] & RB_UEP_T_TOG) != 0U);
    assert(fake_uep_tx_len[1] == 36U);
    assert(memcmp(&endpoint_buffer[64], &tx[64], 36) == 0);
    fire_transfer((uint8_t)(UIS_TOKEN_IN | 1U), 0);
    assert((fake_uep_ctrl[1] & RB_UEP_T_TOG) == 0U);
    assert(event_count == 1U);
    assert(events[0].type == CHIP_USB_EVENT_TRANSFER_COMPLETE);
    assert(events[0].data.transfer.endpoint_address == 0x81U);
    assert(events[0].data.transfer.transferred == sizeof(tx));

    event_count = 0;
    assert(chip_usb_device_submit_out(1, rx, sizeof(rx)) == CHIP_OK);
    memcpy(endpoint_buffer, expected, 64);
    fire_transfer((uint8_t)(RB_UIS_TOG_OK | UIS_TOKEN_OUT | 1U), 64);
    assert(event_count == 0U);
    assert((fake_uep_ctrl[1] & RB_UEP_R_TOG) != 0U);
    memcpy(endpoint_buffer, &expected[64], 36);
    fire_transfer((uint8_t)(RB_UIS_TOG_OK | UIS_TOKEN_OUT | 1U), 36);
    assert((fake_uep_ctrl[1] & RB_UEP_R_TOG) == 0U);
    assert(event_count == 1U);
    assert(events[0].data.transfer.endpoint_address == 0x01U);
    assert(events[0].data.transfer.transferred == sizeof(rx));
    assert(memcmp(rx, expected, sizeof(rx)) == 0);
}

static void test_control_and_bus_events(void)
{
    const uint8_t setup[8] = {0x00, 0x05, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t *endpoint0_buffer = (uint8_t *)(uintptr_t)fake_uep_dma[0];

    event_count = 0;
    memcpy(endpoint0_buffer, setup, sizeof(setup));
    fire_transfer((uint8_t)(RB_UIS_SETUP_ACT | MASK_UIS_TOKEN), 8);
    assert(event_count == 1U);
    assert(events[0].type == CHIP_USB_EVENT_SETUP_RECEIVED);
    assert(memcmp(events[0].data.setup.bytes, setup, sizeof(setup)) == 0);

    event_count = 0;
    assert(chip_usb_device_set_address(7) == CHIP_OK);
    assert(chip_usb_device_submit_in(0, NULL, 0) == CHIP_OK);
    assert(fake_usb_dev_ad == 0U);
    fire_transfer(UIS_TOKEN_IN, 0);
    assert(fake_usb_dev_ad == 7U);
    assert(event_count == 1U);
    assert(events[0].data.transfer.endpoint_address == 0x80U);

    event_count = 0;
    fake_usb_mis_st = RB_UMS_SUSPEND;
    fake_usb_int_fg = RB_UIF_SUSPEND;
    USB_IRQHandler();
    assert(chip_usb_device_is_suspended());
    assert(events[0].type == CHIP_USB_EVENT_SUSPEND);
    fake_delay_ms = 0;
    assert(chip_usb_device_remote_wakeup() == CHIP_OK);
    assert(fake_delay_ms == 2U);
    assert((fake_udev_ctrl & RB_UD_LOW_SPEED) == 0U);

    event_count = 0;
    fake_usb_mis_st = 0;
    fake_usb_int_fg = RB_UIF_SUSPEND;
    USB_IRQHandler();
    assert(!chip_usb_device_is_suspended());
    assert(events[0].type == CHIP_USB_EVENT_RESUME);

    event_count = 0;
    fake_usb_dev_ad = 42;
    fake_usb_int_fg = RB_UIF_BUS_RST;
    USB_IRQHandler();
    assert(fake_usb_dev_ad == 0U);
    assert(events[0].type == CHIP_USB_EVENT_BUS_RESET);
    assert(events[0].data.bus_reset.speed_mbps == 12U);
    assert(chip_usb_device_submit_in(1, setup, sizeof(setup)) == CHIP_ERROR_NOT_READY);
}

static void test_endpoint_state(void)
{
    const uint8_t report[8] = {0, 0, 4, 0, 0, 0, 0, 0};
    uint8_t *endpoint_buffer = (uint8_t *)(uintptr_t)fake_uep_dma[2];

    assert(chip_usb_device_endpoint_open(0x82, CHIP_USB_ENDPOINT_BULK, 32) == CHIP_OK);
    assert((fake_uep2_3_mod & (RB_UEP2_RX_EN | RB_UEP2_TX_EN)) ==
           (RB_UEP2_RX_EN | RB_UEP2_TX_EN));
    assert(chip_usb_device_submit_in(2, report, sizeof(report)) == CHIP_OK);
    assert(memcmp(&endpoint_buffer[64], report, sizeof(report)) == 0);
    event_count = 0;
    fire_transfer((uint8_t)(UIS_TOKEN_IN | 2U), 0);
    assert(event_count == 1U);
    assert(events[0].data.transfer.endpoint_address == 0x82U);
    assert(chip_usb_device_endpoint_stall(0x82) == CHIP_OK);
    assert((fake_uep_ctrl[2] & MASK_UEP_T_RES) == UEP_T_RES_STALL);
    assert(chip_usb_device_endpoint_clear_stall(0x82) == CHIP_OK);
    assert((fake_uep_ctrl[2] & MASK_UEP_T_RES) == UEP_T_RES_NAK);
    assert(chip_usb_device_endpoint_close(0x82) == CHIP_OK);
    assert(chip_usb_device_endpoint_close(0x80) == CHIP_ERROR_BUSY);
    assert(chip_usb_device_endpoint_open(0x83, CHIP_USB_ENDPOINT_ISOCHRONOUS, 64) ==
           CHIP_ERROR_UNSUPPORTED);
    assert(chip_usb_device_endpoint_open(0x83, CHIP_USB_ENDPOINT_BULK, 12) ==
           CHIP_ERROR_INVALID_ARG);
}

int main(void)
{
    chip_usb_device_config_t config = {
        .endpoint0_max_packet_size = 64,
        .callback = event_callback,
        .callback_context = NULL,
    };

    assert(chip_usb_device_init(&config) == CHIP_OK);
    assert(fake_irq_enabled == 1U);
    assert(chip_usb_device_set_interrupt_enabled(false) == CHIP_OK);
    assert(fake_irq_enabled == 0U);
    assert(chip_usb_device_set_interrupt_enabled(true) == CHIP_OK);
    assert(fake_irq_enabled == 1U);
    assert((fake_power_clock_mask & BIT_SLP_CLK_USB) != 0U);
    assert(chip_usb_device_connect() == CHIP_OK);
    assert((fake_udev_ctrl & RB_UD_PORT_EN) != 0U);

    test_in_and_out_transfers();
    test_endpoint_state();
    test_control_and_bus_events();

    assert(chip_usb_device_disconnect() == CHIP_OK);
    assert((fake_udev_ctrl & RB_UD_PORT_EN) == 0U);
    assert(chip_usb_device_deinit() == CHIP_OK);
    assert(fake_irq_enabled == 0U);
    assert((fake_power_clock_mask & BIT_SLP_CLK_USB) == 0U);
    return 0;
}
