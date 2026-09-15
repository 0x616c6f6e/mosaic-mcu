#ifndef CHIP_API_I2C_H
#define CHIP_API_I2C_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Available I2C controllers. */
typedef enum {
    CHIP_I2C_0 = 0, /**< I2C0 controller. */
    CHIP_I2C_COUNT, /**< Number of I2C controllers. */
} chip_i2c_t;

/** @brief I2C bus frequency. */
typedef struct {
    uint32_t clock_hz; /**< Requested bus frequency in Hz. */
} chip_i2c_config_t;

/** @brief Configure an I2C controller.
 * @param i2c Controller instance.
 * @param config Bus frequency in hertz.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_i2c_init(chip_i2c_t i2c, const chip_i2c_config_t *config);
/** @brief Release an I2C controller.
 * @param i2c Controller instance.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_i2c_deinit(chip_i2c_t i2c);
/** @brief Check whether a device acknowledges its address.
 * @param i2c Controller instance.
 * @param address_7bit Unshifted 7-bit device address.
 * @param timeout_us Timeout in microseconds.
 * @return CHIP_OK if acknowledged, or a CHIP_ERROR_* status.
 */
chip_status_t chip_i2c_probe(chip_i2c_t i2c,
                             uint8_t address_7bit,
                             uint32_t timeout_us);

/** @brief Write and/or read data from an I2C device.
 * @note Uses a repeated START when both lengths are nonzero.
 * @param i2c Controller instance.
 * @param address_7bit Unshifted 7-bit device address.
 * @param write_data Bytes to send when write_size is nonzero.
 * @param write_size Number of bytes to write.
 * @param[out] read_data Receives read_size bytes when nonzero.
 * @param read_size Number of bytes to read.
 * @param timeout_us Timeout in microseconds.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_i2c_transfer(chip_i2c_t i2c,
                                uint8_t address_7bit,
                                const uint8_t *write_data,
                                size_t write_size,
                                uint8_t *read_data,
                                size_t read_size,
                                uint32_t timeout_us);

#ifdef __cplusplus
}
#endif

#endif
