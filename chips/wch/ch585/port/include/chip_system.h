#ifndef CHIP_API_SYSTEM_H
#define CHIP_API_SYSTEM_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHIP_UNIQUE_ID_SIZE 8U

typedef enum {
    CHIP_CLOCK_INTERNAL,
    CHIP_CLOCK_EXTERNAL,
} chip_clock_source_t;

typedef struct {
    chip_clock_source_t source;
    uint32_t core_clock_hz;
    uint8_t external_crystal_load_pf; /* 0 keeps the chip default. */
} chip_system_config_t;

chip_status_t chip_system_init(const chip_system_config_t *config);
uint32_t chip_system_clock_hz(void);
chip_clock_source_t chip_system_clock_source(void);
uint8_t chip_system_chip_id(void);
chip_status_t chip_system_unique_id(uint8_t *buffer, size_t size);

/* The returned state is an opaque token for the matching exit call. */
uint32_t chip_system_critical_enter(void);
void chip_system_critical_exit(uint32_t state);

void chip_system_reset(void);

#ifdef __cplusplus
}
#endif

#endif
