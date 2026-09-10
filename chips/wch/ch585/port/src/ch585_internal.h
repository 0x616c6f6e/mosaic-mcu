#ifndef CH585_PORT_INTERNAL_H
#define CH585_PORT_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>
#include <chip_time.h>

static inline chip_status_t ch585_timeout_start(uint32_t timeout_us, uint64_t *started_at)
{
    if (started_at == NULL) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if ((timeout_us != CHIP_TIMEOUT_NONE) &&
        (timeout_us != CHIP_TIMEOUT_FOREVER) &&
        !chip_time_is_initialized()) {
        return CHIP_ERROR_NOT_READY;
    }

    *started_at = chip_time_micros();
    return CHIP_OK;
}

static inline bool ch585_timeout_expired(uint64_t started_at, uint32_t timeout_us)
{
    if (timeout_us == CHIP_TIMEOUT_FOREVER) {
        return false;
    }
    return (chip_time_micros() - started_at) >= timeout_us;
}

#endif
