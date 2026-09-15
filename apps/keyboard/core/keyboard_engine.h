#ifndef MOSAIC_KEYBOARD_ENGINE_H
#define MOSAIC_KEYBOARD_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum physical key positions supported by the engine. */
#define KEYBOARD_ENGINE_MAX_KEYS 80U
/** @brief HID Boot Keyboard simultaneous usage slots. */
#define KEYBOARD_HID_KEY_SLOTS   6U
/** @brief HID Boot Keyboard report length in bytes. */
#define KEYBOARD_HID_REPORT_SIZE 8U
/** @brief HID usage indicating that too many keys are held. */
#define KEYBOARD_HID_ERROR_ROLLOVER UINT8_C(0x01)

/** @brief Modifier bit and HID usage for one physical key. */
typedef struct {
    uint8_t modifier; /**< HID modifier bit; zero for normal keys. */
    uint8_t usage;    /**< HID usage code; zero for unbound positions. */
} keyboard_binding_t;

/** @brief Fill physical key states; return false on scan failure. */
typedef bool (*keyboard_scan_t)(bool *pressed, size_t key_count,
                                void *context);

/** @brief Physical bindings, debounce threshold, and scan schedule. */
typedef struct {
    const keyboard_binding_t *bindings; /**< Per-key mappings kept by caller. */
    size_t key_count;            /**< Number of physical key positions. */
    uint8_t debounce_scans;      /**< Consecutive scans required for change. */
    uint32_t scan_interval_ms;   /**< Minimum time between scans in ms. */
    keyboard_scan_t scan;        /**< Callback that fills key states. */
    void *scan_context;          /**< Opaque scan callback argument. */
} keyboard_engine_config_t;

/** @brief Debounced physical state and current Boot Keyboard report. */
typedef struct {
    keyboard_engine_config_t config; /**< Shallow configuration copy. */
    bool raw[KEYBOARD_ENGINE_MAX_KEYS]; /**< Last sampled key states. */
    bool stable[KEYBOARD_ENGINE_MAX_KEYS]; /**< Debounced key states. */
    uint8_t debounce[KEYBOARD_ENGINE_MAX_KEYS]; /**< Consecutive-change counts. */
    uint8_t report[KEYBOARD_HID_REPORT_SIZE]; /**< Current HID report. */
    uint32_t last_scan_ms; /**< Timestamp of last scan in ms. */
    bool first_scan;       /**< First poll must trigger a scan. */
    bool initialized;      /**< true after validation succeeds. */
} keyboard_engine_t;

/** @brief Initialize a keyboard scanner and HID report builder.
 * @param[out] engine Keyboard engine to initialize.
 * @param config Bindings, scan callback, and debounce settings.
 * @return true if configuration is valid and initialization succeeds.
 */
bool keyboard_engine_init(keyboard_engine_t *engine,
                          const keyboard_engine_config_t *config);
/** @brief Scan when due and return the current debounced HID report.
 * @param engine Initialized keyboard engine.
 * @param now_ms Current monotonic time in milliseconds.
 * @param[out] report Receives the eight-byte Boot Keyboard report.
 * @param[out] report_changed true if the report differs from the prior poll.
 * @return true on success, false on invalid arguments or scan failure.
 */
bool keyboard_engine_poll(keyboard_engine_t *engine, uint32_t now_ms,
                          uint8_t report[KEYBOARD_HID_REPORT_SIZE],
                          bool *report_changed);

#ifdef __cplusplus
}
#endif

#endif
