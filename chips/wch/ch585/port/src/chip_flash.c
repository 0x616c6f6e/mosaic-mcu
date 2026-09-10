#include <chip_flash.h>

#include <stdbool.h>
#include <string.h>

#include <chip_system.h>

#include "CH58x_common.h"

#define CH585_DATA_FLASH_SIZE        UINT32_C(0x8000)
#define CH585_DATA_FLASH_ERASE_SIZE  UINT32_C(256)
#define CH585_DATA_FLASH_WRITE_SIZE  UINT32_C(1)
#define CH585_FLASH_STAGING_SIZE     256U

static bool flash_range_valid(uint32_t offset, size_t size)
{
    return (offset <= CH585_DATA_FLASH_SIZE) &&
           (size <= (size_t)(CH585_DATA_FLASH_SIZE - offset));
}

static chip_status_t flash_result(uint32_t result)
{
    return (result == 0U) ? CHIP_OK : CHIP_ERROR_IO;
}

chip_flash_info_t chip_flash_info(void)
{
    chip_flash_info_t info = {
        .size = CH585_DATA_FLASH_SIZE,
        .erase_size = CH585_DATA_FLASH_ERASE_SIZE,
        .write_size = CH585_DATA_FLASH_WRITE_SIZE,
    };
    return info;
}

chip_status_t chip_flash_read(uint32_t offset, void *buffer, size_t size)
{
    uint8_t *output = (uint8_t *)buffer;
    uint32_t staging[CH585_FLASH_STAGING_SIZE / sizeof(uint32_t)];

    if (((buffer == NULL) && (size != 0U)) || !flash_range_valid(offset, size)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    while (size > 0U) {
        size_t chunk = (size > sizeof(staging)) ? sizeof(staging) : size;
        uint32_t state = chip_system_critical_enter();
        uint32_t result = EEPROM_READ(offset, staging, (uint32_t)chunk);
        chip_system_critical_exit(state);
        if (result != 0U) {
            return CHIP_ERROR_IO;
        }
        memcpy(output, staging, chunk);
        output += chunk;
        offset += (uint32_t)chunk;
        size -= chunk;
    }
    return CHIP_OK;
}

chip_status_t chip_flash_write(uint32_t offset, const void *data, size_t size)
{
    const uint8_t *input = (const uint8_t *)data;
    uint32_t staging[CH585_FLASH_STAGING_SIZE / sizeof(uint32_t)];

    if (((data == NULL) && (size != 0U)) || !flash_range_valid(offset, size)) {
        return CHIP_ERROR_INVALID_ARG;
    }

    while (size > 0U) {
        size_t chunk = (size > sizeof(staging)) ? sizeof(staging) : size;
        uint32_t state;
        uint32_t result;

        memcpy(staging, input, chunk);
        state = chip_system_critical_enter();
        result = EEPROM_WRITE(offset, staging, (uint32_t)chunk);
        chip_system_critical_exit(state);
        if (result != 0U) {
            return CHIP_ERROR_IO;
        }
        input += chunk;
        offset += (uint32_t)chunk;
        size -= chunk;
    }
    return CHIP_OK;
}

chip_status_t chip_flash_erase(uint32_t offset, size_t size)
{
    uint32_t state;
    uint32_t result;

    if (!flash_range_valid(offset, size) ||
        ((offset % CH585_DATA_FLASH_ERASE_SIZE) != 0U) ||
        ((size % CH585_DATA_FLASH_ERASE_SIZE) != 0U)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (size == 0U) {
        return CHIP_OK;
    }

    state = chip_system_critical_enter();
    result = EEPROM_ERASE(offset, (uint32_t)size);
    chip_system_critical_exit(state);
    return flash_result(result);
}
