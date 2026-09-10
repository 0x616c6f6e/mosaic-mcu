#include <chip_i2c.h>

#include <stdbool.h>

#include <chip_system.h>

#include "CH58x_common.h"
#include "ch585_internal.h"

#define CH585_I2C_ERROR_FLAGS \
    (I2C_FLAG_TIMEOUT | I2C_FLAG_OVR | I2C_FLAG_AF | I2C_FLAG_ARLO | I2C_FLAG_BERR)

static bool i2c_initialized;

static chip_status_t i2c_check_error(void)
{
    if (I2C_GetFlagStatus(I2C_FLAG_TIMEOUT) == SET) {
        return CHIP_ERROR_TIMEOUT;
    }
    if (I2C_GetFlagStatus(I2C_FLAG_ARLO) == SET) {
        return CHIP_ERROR_BUSY;
    }
    if ((I2C_GetFlagStatus(I2C_FLAG_OVR) == SET) ||
        (I2C_GetFlagStatus(I2C_FLAG_AF) == SET) ||
        (I2C_GetFlagStatus(I2C_FLAG_BERR) == SET)) {
        return CHIP_ERROR_IO;
    }
    return CHIP_OK;
}

static chip_status_t i2c_wait_event(uint32_t event,
                                    uint64_t started_at,
                                    uint32_t timeout_us)
{
    for (;;) {
        chip_status_t status = i2c_check_error();
        if (status != CHIP_OK) {
            return status;
        }
        if (I2C_CheckEvent(event) != 0U) {
            return CHIP_OK;
        }
        if (ch585_timeout_expired(started_at, timeout_us)) {
            return CHIP_ERROR_TIMEOUT;
        }
    }
}

static chip_status_t i2c_wait_flag(uint32_t flag,
                                   uint64_t started_at,
                                   uint32_t timeout_us)
{
    for (;;) {
        chip_status_t status = i2c_check_error();
        if (status != CHIP_OK) {
            return status;
        }
        if (I2C_GetFlagStatus(flag) == SET) {
            return CHIP_OK;
        }
        if (ch585_timeout_expired(started_at, timeout_us)) {
            return CHIP_ERROR_TIMEOUT;
        }
    }
}

static void i2c_recover(void)
{
    I2C_GenerateSTOP(ENABLE);
    I2C_ClearFlag(CH585_I2C_ERROR_FLAGS);
    I2C_AcknowledgeConfig(ENABLE);
    I2C_NACKPositionConfig(I2C_NACKPosition_Current);
}

chip_status_t chip_i2c_init(chip_i2c_t i2c, const chip_i2c_config_t *config)
{
    uint32_t peripheral_clock_mhz;

    if ((i2c != CHIP_I2C_0) || (config == NULL) ||
        (config->clock_hz == 0U) || (config->clock_hz > UINT32_C(400000))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (chip_system_clock_hz() < UINT32_C(2000000)) {
        return CHIP_ERROR_UNSUPPORTED;
    }

    I2C_Init(I2C_Mode_I2C, config->clock_hz, I2C_DutyCycle_2,
             I2C_Ack_Enable, I2C_AckAddr_7bit, 0);
    peripheral_clock_mhz = chip_system_clock_hz() / UINT32_C(1000000);
    if (peripheral_clock_mhz > UINT32_C(36)) {
        peripheral_clock_mhz = UINT32_C(36);
    }
    R16_I2C_CTRL2 = (uint16_t)((R16_I2C_CTRL2 & UINT16_C(0xFF00)) |
                               (uint16_t)peripheral_clock_mhz);
    I2C_ClearFlag(CH585_I2C_ERROR_FLAGS);
    I2C_Cmd(ENABLE);
    i2c_initialized = true;
    return CHIP_OK;
}

chip_status_t chip_i2c_deinit(chip_i2c_t i2c)
{
    if (i2c != CHIP_I2C_0) {
        return CHIP_ERROR_INVALID_ARG;
    }
    I2C_Cmd(DISABLE);
    i2c_initialized = false;
    return CHIP_OK;
}

static chip_status_t i2c_wait_idle(uint64_t started_at, uint32_t timeout_us)
{
    while (I2C_GetFlagStatus(I2C_FLAG_BUSY) == SET) {
        chip_status_t status = i2c_check_error();
        if (status != CHIP_OK) {
            return status;
        }
        if (ch585_timeout_expired(started_at, timeout_us)) {
            return CHIP_ERROR_TIMEOUT;
        }
    }
    return CHIP_OK;
}

static chip_status_t i2c_start_address(uint8_t address_7bit,
                                       uint8_t direction,
                                       uint64_t started_at,
                                       uint32_t timeout_us)
{
    chip_status_t status;

    I2C_GenerateSTART(ENABLE);
    status = i2c_wait_event(I2C_EVENT_MASTER_MODE_SELECT, started_at, timeout_us);
    if (status != CHIP_OK) {
        return status;
    }

    I2C_Send7bitAddress((uint8_t)(address_7bit << 1U), direction);
    return i2c_wait_event((direction == I2C_Direction_Transmitter) ?
                          I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED :
                          I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED,
                          started_at, timeout_us);
}

chip_status_t chip_i2c_probe(chip_i2c_t i2c,
                             uint8_t address_7bit,
                             uint32_t timeout_us)
{
    uint64_t started_at;
    chip_status_t status;

    if ((i2c != CHIP_I2C_0) || (address_7bit > UINT8_C(0x7F))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!i2c_initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    status = ch585_timeout_start(timeout_us, &started_at);
    if (status != CHIP_OK) {
        return status;
    }
    status = i2c_wait_idle(started_at, timeout_us);
    if (status == CHIP_OK) {
        status = i2c_start_address(address_7bit, I2C_Direction_Transmitter,
                                   started_at, timeout_us);
    }
    I2C_GenerateSTOP(ENABLE);
    if (status != CHIP_OK) {
        i2c_recover();
    }
    return status;
}

static chip_status_t i2c_write(const uint8_t *data,
                               size_t size,
                               uint64_t started_at,
                               uint32_t timeout_us)
{
    size_t index;

    for (index = 0; index < size; ++index) {
        chip_status_t status;
        I2C_SendData(data[index]);
        status = i2c_wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED,
                                started_at, timeout_us);
        if (status != CHIP_OK) {
            return status;
        }
    }
    return CHIP_OK;
}

static chip_status_t i2c_read(uint8_t *data,
                              size_t size,
                              uint64_t started_at,
                              uint32_t timeout_us)
{
    size_t index = 0;
    chip_status_t status;

    if (size == 1U) {
        I2C_GenerateSTOP(ENABLE);
        status = i2c_wait_flag(I2C_FLAG_RXNE, started_at, timeout_us);
        if (status == CHIP_OK) {
            data[0] = I2C_ReceiveData();
        }
        return status;
    }

    if (size == 2U) {
        status = i2c_wait_flag(I2C_FLAG_BTF, started_at, timeout_us);
        if (status != CHIP_OK) {
            return status;
        }
        I2C_GenerateSTOP(ENABLE);
        data[0] = I2C_ReceiveData();
        data[1] = I2C_ReceiveData();
        return CHIP_OK;
    }

    while ((size - index) > 3U) {
        status = i2c_wait_flag(I2C_FLAG_RXNE, started_at, timeout_us);
        if (status != CHIP_OK) {
            return status;
        }
        data[index++] = I2C_ReceiveData();
    }

    status = i2c_wait_flag(I2C_FLAG_BTF, started_at, timeout_us);
    if (status != CHIP_OK) {
        return status;
    }
    I2C_AcknowledgeConfig(DISABLE);
    data[index++] = I2C_ReceiveData();
    I2C_GenerateSTOP(ENABLE);
    data[index++] = I2C_ReceiveData();
    status = i2c_wait_flag(I2C_FLAG_RXNE, started_at, timeout_us);
    if (status == CHIP_OK) {
        data[index] = I2C_ReceiveData();
    }
    return status;
}

chip_status_t chip_i2c_transfer(chip_i2c_t i2c,
                                uint8_t address_7bit,
                                const uint8_t *write_data,
                                size_t write_size,
                                uint8_t *read_data,
                                size_t read_size,
                                uint32_t timeout_us)
{
    uint64_t started_at;
    chip_status_t status;

    if ((i2c != CHIP_I2C_0) || (address_7bit > UINT8_C(0x7F)) ||
        ((write_size == 0U) && (read_size == 0U)) ||
        ((write_data == NULL) && (write_size != 0U)) ||
        ((read_data == NULL) && (read_size != 0U))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (!i2c_initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    status = ch585_timeout_start(timeout_us, &started_at);
    if (status != CHIP_OK) {
        return status;
    }

    status = i2c_wait_idle(started_at, timeout_us);
    if (status != CHIP_OK) {
        return status;
    }

    if (write_size != 0U) {
        status = i2c_start_address(address_7bit, I2C_Direction_Transmitter,
                                   started_at, timeout_us);
        if (status == CHIP_OK) {
            status = i2c_write(write_data, write_size, started_at, timeout_us);
        }
        if (status != CHIP_OK) {
            i2c_recover();
            return status;
        }
    }

    if (read_size != 0U) {
        if (read_size == 1U) {
            I2C_NACKPositionConfig(I2C_NACKPosition_Current);
            I2C_AcknowledgeConfig(DISABLE);
        } else if (read_size == 2U) {
            I2C_NACKPositionConfig(I2C_NACKPosition_Next);
            I2C_AcknowledgeConfig(DISABLE);
        } else {
            I2C_NACKPositionConfig(I2C_NACKPosition_Current);
            I2C_AcknowledgeConfig(ENABLE);
        }

        status = i2c_start_address(address_7bit, I2C_Direction_Receiver,
                                   started_at, timeout_us);
        if (status == CHIP_OK) {
            status = i2c_read(read_data, read_size, started_at, timeout_us);
        }
    } else {
        I2C_GenerateSTOP(ENABLE);
    }

    I2C_AcknowledgeConfig(ENABLE);
    I2C_NACKPositionConfig(I2C_NACKPosition_Current);
    if (status != CHIP_OK) {
        i2c_recover();
    }
    return status;
}
