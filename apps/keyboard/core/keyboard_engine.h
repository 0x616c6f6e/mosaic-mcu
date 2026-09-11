#ifndef MOSAIC_KEYBOARD_ENGINE_H
#define MOSAIC_KEYBOARD_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEYBOARD_ENGINE_MAX_KEYS 80U
#define KEYBOARD_HID_KEY_SLOTS   6U
#define KEYBOARD_HID_REPORT_SIZE 8U
#define KEYBOARD_HID_ERROR_ROLLOVER UINT8_C(0x01)

typedef struct {
    uint8_t modifier;
    uint8_t usage;
} keyboard_binding_t;

typedef bool (*keyboard_scan_t)(bool *pressed, size_t key_count,
                                void *context);

typedef struct {
    const keyboard_binding_t *bindings;
    size_t key_count;
    uint8_t debounce_scans;
    uint32_t scan_interval_ms;
    keyboard_scan_t scan;
    void *scan_context;
} keyboard_engine_config_t;

typedef struct {
    keyboard_engine_config_t config;
    bool raw[KEYBOARD_ENGINE_MAX_KEYS];
    bool stable[KEYBOARD_ENGINE_MAX_KEYS];
    uint8_t debounce[KEYBOARD_ENGINE_MAX_KEYS];
    uint8_t report[KEYBOARD_HID_REPORT_SIZE];
    uint32_t last_scan_ms;
    bool first_scan;
    bool initialized;
} keyboard_engine_t;

bool keyboard_engine_init(keyboard_engine_t *engine,
                          const keyboard_engine_config_t *config);
bool keyboard_engine_poll(keyboard_engine_t *engine, uint32_t now_ms,
                          uint8_t report[KEYBOARD_HID_REPORT_SIZE],
                          bool *report_changed);

#ifdef __cplusplus
}
#endif

#endif
