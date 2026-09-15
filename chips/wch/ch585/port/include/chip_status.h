#ifndef CHIP_API_STATUS_H
#define CHIP_API_STATUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Shared result codes for Chip API operations. */
typedef enum {
    CHIP_OK = 0,             /**< Operation completed successfully. */
    CHIP_ERROR_INVALID_ARG,  /**< Invalid argument or out-of-range value. */
    CHIP_ERROR_UNSUPPORTED,  /**< Feature or configuration not supported. */
    CHIP_ERROR_NOT_READY,    /**< Peripheral has not been initialized. */
    CHIP_ERROR_BUSY,         /**< Peripheral cannot accept another operation. */
    CHIP_ERROR_TIMEOUT,      /**< Operation exceeded its time limit. */
    CHIP_ERROR_IO,           /**< Hardware transfer or access failed. */
} chip_status_t;

/** @brief Do not wait; other timeout values use microseconds. */
#define CHIP_TIMEOUT_NONE       UINT32_C(0)
/** @brief Wait indefinitely; other timeout values use microseconds. */
#define CHIP_TIMEOUT_FOREVER    UINT32_MAX

#ifdef __cplusplus
}
#endif

#endif
