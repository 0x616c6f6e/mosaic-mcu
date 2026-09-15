#ifndef CH585_OTA_TUSB_CONFIG_H
#define CH585_OTA_TUSB_CONFIG_H

/** @brief TinyUSB uses the repository's custom CH585 DCD. */
#define CFG_TUSB_MCU              OPT_MCU_NONE
/** @brief Firmware runs TinyUSB in its bare-metal event loop. */
#define CFG_TUSB_OS               OPT_OS_NONE
/** @brief Disable TinyUSB internal debug logging. */
#define CFG_TUSB_DEBUG            0
/** @brief Rhport zero operates as a full-speed USB device. */
#define CFG_TUSB_RHPORT0_MODE     (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

/** @brief Control endpoint packet length in bytes. */
#define CFG_TUD_ENDPOINT0_SIZE    64
#ifdef KEYBOARD_USB_HID_ENABLED
/** @brief Expose keyboard HID in the application build. */
#define CFG_TUD_HID               1
/** @brief Keyboard HID endpoint buffer size in bytes. */
#define CFG_TUD_HID_EP_BUFSIZE    8
#else
/** @brief Recovery bootloader omits keyboard HID. */
#define CFG_TUD_HID               0
#endif
/** @brief No USB serial interface. */
#define CFG_TUD_CDC               0
/** @brief USB MSC exposes the OTA disk when enabled at runtime. */
#define CFG_TUD_MSC               1
/** @brief No USB MIDI interface. */
#define CFG_TUD_MIDI              0
/** @brief Vendor interface handles WebUSB OTA requests. */
#define CFG_TUD_VENDOR            1
/** @brief MSC endpoint buffer size in bytes. */
#define CFG_TUD_MSC_EP_BUFSIZE    512
/** @brief Vendor OUT receive FIFO in bytes. */
#define CFG_TUD_VENDOR_RX_BUFSIZE 1024
/** @brief Vendor IN transmit FIFO in bytes. */
#define CFG_TUD_VENDOR_TX_BUFSIZE 64
/** @brief Vendor OUT endpoint packet size in bytes. */
#define CFG_TUD_VENDOR_RX_EPSIZE  64
/** @brief Vendor IN endpoint packet size in bytes. */
#define CFG_TUD_VENDOR_TX_EPSIZE  64

#endif
