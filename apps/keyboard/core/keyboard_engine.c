#include "keyboard_engine.h"

#include <string.h>

static void build_report(const keyboard_engine_t *engine,
                         uint8_t report[KEYBOARD_HID_REPORT_SIZE])
{
    size_t index;
    size_t usage_count = 0U;
    bool rollover = false;

    memset(report, 0, KEYBOARD_HID_REPORT_SIZE);
    for (index = 0U; index < engine->config.key_count; ++index) {
        const keyboard_binding_t *binding = &engine->config.bindings[index];
        size_t usage_index;
        bool duplicate = false;

        if (!engine->stable[index]) {
            continue;
        }
        report[0] |= binding->modifier;
        if (binding->usage == 0U) {
            continue;
        }
        for (usage_index = 0U; usage_index < usage_count; ++usage_index) {
            if (report[2U + usage_index] == binding->usage) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            continue;
        }
        if (usage_count >= KEYBOARD_HID_KEY_SLOTS) {
            rollover = true;
            break;
        }
        report[2U + usage_count] = binding->usage;
        ++usage_count;
    }
    if (rollover) {
        memset(&report[2], KEYBOARD_HID_ERROR_ROLLOVER,
               KEYBOARD_HID_KEY_SLOTS);
    }
}

bool keyboard_engine_init(keyboard_engine_t *engine,
                          const keyboard_engine_config_t *config)
{
    if ((engine == NULL) || (config == NULL) ||
        (config->key_count > KEYBOARD_ENGINE_MAX_KEYS) ||
        ((config->key_count > 0U) &&
         ((config->bindings == NULL) || (config->scan == NULL) ||
          (config->debounce_scans == 0U) ||
          (config->scan_interval_ms == 0U)))) {
        return false;
    }
    memset(engine, 0, sizeof(*engine));
    engine->config = *config;
    engine->first_scan = true;
    engine->initialized = true;
    return true;
}

bool keyboard_engine_poll(keyboard_engine_t *engine, uint32_t now_ms,
                          uint8_t report[KEYBOARD_HID_REPORT_SIZE],
                          bool *report_changed)
{
    uint8_t next_report[KEYBOARD_HID_REPORT_SIZE];
    size_t index;
    bool stable_changed = false;

    if ((engine == NULL) || !engine->initialized || (report == NULL) ||
        (report_changed == NULL)) {
        return false;
    }
    *report_changed = false;
    memcpy(report, engine->report, sizeof(engine->report));
    if (engine->config.key_count == 0U) {
        return true;
    }
    if (!engine->first_scan &&
        ((now_ms - engine->last_scan_ms) < engine->config.scan_interval_ms)) {
        return true;
    }
    engine->first_scan = false;
    engine->last_scan_ms = now_ms;
    if (!engine->config.scan(engine->raw, engine->config.key_count,
                             engine->config.scan_context)) {
        return false;
    }

    for (index = 0U; index < engine->config.key_count; ++index) {
        if (engine->raw[index] == engine->stable[index]) {
            engine->debounce[index] = 0U;
        } else {
            ++engine->debounce[index];
            if (engine->debounce[index] >= engine->config.debounce_scans) {
                engine->stable[index] = engine->raw[index];
                engine->debounce[index] = 0U;
                stable_changed = true;
            }
        }
    }
    if (stable_changed) {
        build_report(engine, next_report);
        if (memcmp(next_report, engine->report, sizeof(next_report)) != 0) {
            memcpy(engine->report, next_report, sizeof(engine->report));
            *report_changed = true;
        }
    }
    memcpy(report, engine->report, sizeof(engine->report));
    return true;
}
