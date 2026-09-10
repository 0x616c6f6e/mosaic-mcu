#include <stddef.h>
#include <stdint.h>

#include "tusb_option.h"

#if CFG_TUD_ENABLED && (CFG_TUSB_MCU == OPT_MCU_NONE)

#include "device/dcd.h"

#include <chip_usb_device.h>

static chip_usb_endpoint_type_t endpoint_type(tusb_xfer_type_t type) {
  switch (type) {
    case TUSB_XFER_CONTROL: return CHIP_USB_ENDPOINT_CONTROL;
    case TUSB_XFER_ISOCHRONOUS: return CHIP_USB_ENDPOINT_ISOCHRONOUS;
    case TUSB_XFER_BULK: return CHIP_USB_ENDPOINT_BULK;
    case TUSB_XFER_INTERRUPT: return CHIP_USB_ENDPOINT_INTERRUPT;
    default: return CHIP_USB_ENDPOINT_CONTROL;
  }
}

static void chip_usb_event(const chip_usb_event_t* event, void* context) {
  (void) context;

  switch (event->type) {
    case CHIP_USB_EVENT_BUS_RESET:
      dcd_event_bus_reset(0, TUSB_SPEED_FULL, true);
      break;
    case CHIP_USB_EVENT_SUSPEND:
      dcd_event_bus_signal(0, DCD_EVENT_SUSPEND, true);
      break;
    case CHIP_USB_EVENT_RESUME:
      dcd_event_bus_signal(0, DCD_EVENT_RESUME, true);
      break;
    case CHIP_USB_EVENT_SETUP_RECEIVED:
      dcd_event_setup_received(0, event->data.setup.bytes, true);
      break;
    case CHIP_USB_EVENT_TRANSFER_COMPLETE:
      dcd_event_xfer_complete(0, event->data.transfer.endpoint_address,
                              (uint32_t) event->data.transfer.transferred,
                              XFER_RESULT_SUCCESS, true);
      break;
    case CHIP_USB_EVENT_ERROR:
      dcd_event_xfer_complete(0, event->data.error.endpoint_address, 0,
                              XFER_RESULT_FAILED, true);
      break;
    default:
      break;
  }
}

bool dcd_init(uint8_t rhport, const tusb_rhport_init_t* rh_init) {
  const chip_usb_device_config_t config = {
    .endpoint0_max_packet_size = CFG_TUD_ENDPOINT0_SIZE,
    .callback = chip_usb_event,
    .callback_context = NULL,
  };

  if (rhport != 0 || rh_init == NULL || rh_init->role != TUSB_ROLE_DEVICE ||
      (rh_init->speed != TUSB_SPEED_AUTO && rh_init->speed != TUSB_SPEED_FULL)) {
    return false;
  }
  if (chip_usb_device_init(&config) != CHIP_OK) {
    return false;
  }
  return chip_usb_device_connect() == CHIP_OK;
}

bool dcd_deinit(uint8_t rhport) {
  return rhport == 0 && chip_usb_device_deinit() == CHIP_OK;
}

void dcd_int_handler(uint8_t rhport) {
  (void) rhport;
}

void dcd_int_enable(uint8_t rhport) {
  if (rhport == 0) {
    (void) chip_usb_device_set_interrupt_enabled(true);
  }
}

void dcd_int_disable(uint8_t rhport) {
  if (rhport == 0) {
    (void) chip_usb_device_set_interrupt_enabled(false);
  }
}

void dcd_set_address(uint8_t rhport, uint8_t dev_addr) {
  if (rhport == 0 && chip_usb_device_set_address(dev_addr) == CHIP_OK) {
    (void) chip_usb_device_submit_in(0, NULL, 0);
  }
}

void dcd_remote_wakeup(uint8_t rhport) {
  if (rhport == 0) {
    (void) chip_usb_device_remote_wakeup();
  }
}

void dcd_connect(uint8_t rhport) {
  if (rhport == 0) {
    (void) chip_usb_device_connect();
  }
}

void dcd_disconnect(uint8_t rhport) {
  if (rhport == 0) {
    (void) chip_usb_device_disconnect();
  }
}

void dcd_sof_enable(uint8_t rhport, bool en) {
  (void) rhport;
  (void) en;
}

bool dcd_edpt_open(uint8_t rhport, const tusb_desc_endpoint_t* desc_edpt) {
  if (rhport != 0 || desc_edpt == NULL) {
    return false;
  }

  return chip_usb_device_endpoint_open(desc_edpt->bEndpointAddress,
                                       endpoint_type(desc_edpt->bmAttributes.xfer),
                                       tu_edpt_packet_size(desc_edpt)) == CHIP_OK;
}

void dcd_edpt_close_all(uint8_t rhport) {
  if (rhport != 0) {
    return;
  }

  for (uint8_t endpoint = 1; endpoint < CHIP_USB_DEVICE_ENDPOINT_COUNT; ++endpoint) {
    (void) chip_usb_device_endpoint_close(endpoint);
    (void) chip_usb_device_endpoint_close((uint8_t) (endpoint | TUSB_DIR_IN_MASK));
  }
}

void dcd_edpt_close(uint8_t rhport, uint8_t ep_addr) {
  if (rhport == 0) {
    (void) chip_usb_device_endpoint_close(ep_addr);
  }
}

bool dcd_edpt_xfer(uint8_t rhport, uint8_t ep_addr, uint8_t* buffer,
                   uint16_t total_bytes, bool is_isr) {
  (void) is_isr;

  if (rhport != 0) {
    return false;
  }
  if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN) {
    return chip_usb_device_submit_in(tu_edpt_number(ep_addr), buffer,
                                     total_bytes) == CHIP_OK;
  }
  return chip_usb_device_submit_out(tu_edpt_number(ep_addr), buffer,
                                    total_bytes) == CHIP_OK;
}

void dcd_edpt_stall(uint8_t rhport, uint8_t ep_addr) {
  if (rhport == 0) {
    (void) chip_usb_device_endpoint_stall(ep_addr);
  }
}

void dcd_edpt_clear_stall(uint8_t rhport, uint8_t ep_addr) {
  if (rhport == 0) {
    (void) chip_usb_device_endpoint_clear_stall(ep_addr);
  }
}

#endif
