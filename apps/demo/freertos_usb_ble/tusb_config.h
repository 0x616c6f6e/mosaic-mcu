#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief TinyUSB uses the custom CH585 DCD. */
#define CFG_TUSB_MCU              OPT_MCU_NONE
/** @brief USB stack runs under FreeRTOS. */
#define CFG_TUSB_OS               OPT_OS_FREERTOS
/** @brief Disable internal USB debug logging. */
#define CFG_TUSB_DEBUG            0
/** @brief Rhport zero is a full-speed USB device. */
#define CFG_TUSB_RHPORT0_MODE     (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

/** @brief Control endpoint packet size in bytes. */
#define CFG_TUD_ENDPOINT0_SIZE    64

/** @brief Enable the USB HID interface. */
#define CFG_TUD_HID               1
/** @brief USB serial disabled. */
#define CFG_TUD_CDC               0
/** @brief USB storage disabled. */
#define CFG_TUD_MSC               0
/** @brief USB MIDI disabled. */
#define CFG_TUD_MIDI              0
/** @brief Vendor USB interface disabled. */
#define CFG_TUD_VENDOR            0

/** @brief HID endpoint buffer size in bytes. */
#define CFG_TUD_HID_EP_BUFSIZE    8

#ifdef __cplusplus
}
#endif

#endif
