#ifndef CH585_DEMO_LUA_USB_FATFS_TUSB_CONFIG_H
#define CH585_DEMO_LUA_USB_FATFS_TUSB_CONFIG_H

/** @brief TinyUSB uses the custom CH585 DCD. */
#define CFG_TUSB_MCU              OPT_MCU_NONE
/** @brief Run the USB stack without an RTOS. */
#define CFG_TUSB_OS               OPT_OS_NONE
/** @brief Disable internal USB debug logging. */
#define CFG_TUSB_DEBUG            0
/** @brief Rhport zero is a full-speed USB device. */
#define CFG_TUSB_RHPORT0_MODE     (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

/** @brief Control endpoint packet size in bytes. */
#define CFG_TUD_ENDPOINT0_SIZE    64
/** @brief Script-disk demo does not use keyboard HID. */
#define CFG_TUD_HID               0
/** @brief USB serial disabled. */
#define CFG_TUD_CDC               0
/** @brief Expose the script disk using USB MSC. */
#define CFG_TUD_MSC               1
/** @brief USB MIDI disabled. */
#define CFG_TUD_MIDI              0
/** @brief Vendor USB interface disabled. */
#define CFG_TUD_VENDOR            0
/** @brief MSC endpoint buffer size in bytes. */
#define CFG_TUD_MSC_EP_BUFSIZE    512

#endif
