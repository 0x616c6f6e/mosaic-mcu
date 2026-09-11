#ifndef CH585_OTA_TUSB_CONFIG_H
#define CH585_OTA_TUSB_CONFIG_H

#define CFG_TUSB_MCU              OPT_MCU_NONE
#define CFG_TUSB_OS               OPT_OS_NONE
#define CFG_TUSB_DEBUG            0
#define CFG_TUSB_RHPORT0_MODE     (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

#define CFG_TUD_ENDPOINT0_SIZE    64
#ifdef KEYBOARD_USB_HID_ENABLED
#define CFG_TUD_HID               1
#define CFG_TUD_HID_EP_BUFSIZE    8
#else
#define CFG_TUD_HID               0
#endif
#define CFG_TUD_CDC               0
#define CFG_TUD_MSC               1
#define CFG_TUD_MIDI              0
#define CFG_TUD_VENDOR            1
#define CFG_TUD_MSC_EP_BUFSIZE    512
#define CFG_TUD_VENDOR_RX_BUFSIZE 1024
#define CFG_TUD_VENDOR_TX_BUFSIZE 64
#define CFG_TUD_VENDOR_RX_EPSIZE  64
#define CFG_TUD_VENDOR_TX_EPSIZE  64

#endif
