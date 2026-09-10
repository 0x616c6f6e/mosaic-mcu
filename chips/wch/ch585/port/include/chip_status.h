#ifndef CHIP_API_STATUS_H
#define CHIP_API_STATUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHIP_OK = 0,
    CHIP_ERROR_INVALID_ARG,
    CHIP_ERROR_UNSUPPORTED,
    CHIP_ERROR_NOT_READY,
    CHIP_ERROR_BUSY,
    CHIP_ERROR_TIMEOUT,
    CHIP_ERROR_IO,
} chip_status_t;

/* Timeout values are expressed in microseconds. */
#define CHIP_TIMEOUT_NONE       UINT32_C(0)
#define CHIP_TIMEOUT_FOREVER    UINT32_MAX

#ifdef __cplusplus
}
#endif

#endif
