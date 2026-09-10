#ifndef CHIP_API_I2C_H
#define CHIP_API_I2C_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHIP_I2C_0 = 0,
    CHIP_I2C_COUNT,
} chip_i2c_t;

typedef struct {
    uint32_t clock_hz;
} chip_i2c_config_t;

chip_status_t chip_i2c_init(chip_i2c_t i2c, const chip_i2c_config_t *config);
chip_status_t chip_i2c_deinit(chip_i2c_t i2c);
chip_status_t chip_i2c_probe(chip_i2c_t i2c,
                             uint8_t address_7bit,
                             uint32_t timeout_us);

/* Uses a repeated START when both write_size and read_size are non-zero. */
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
