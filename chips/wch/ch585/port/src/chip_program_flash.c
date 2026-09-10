#include <chip_program_flash.h>

#include <stdbool.h>
#include <string.h>

#include <chip_system.h>

#include "CH58x_common.h"

#define CH585_PROGRAM_FLASH_SIZE           UINT32_C(0x70000)
#define CH585_PROGRAM_FLASH_ERASE_SIZE     UINT32_C(4096)
#define CH585_PROGRAM_FLASH_WRITE_SIZE     UINT32_C(4)
#define CH585_PROGRAM_FLASH_PREFERRED_SIZE UINT32_C(256)

static bool program_range_valid(uint32_t address, size_t size)
{
    return (address <= CH585_PROGRAM_FLASH_SIZE) &&
           (size <= (size_t)(CH585_PROGRAM_FLASH_SIZE - address));
}

static chip_status_t program_result(uint32_t result)
{
    return (result == 0U) ? CHIP_OK : CHIP_ERROR_IO;
}

chip_program_flash_info_t chip_program_flash_info(void)
{
    const chip_program_flash_info_t info = {
        .size = CH585_PROGRAM_FLASH_SIZE,
        .erase_size = CH585_PROGRAM_FLASH_ERASE_SIZE,
        .write_size = CH585_PROGRAM_FLASH_WRITE_SIZE,
        .preferred_write_size = CH585_PROGRAM_FLASH_PREFERRED_SIZE,
    };

    return info;
}

chip_status_t chip_program_flash_read(uint32_t address, void *buffer,
                                      size_t size)
{
    if (((buffer == NULL) && (size != 0U)) ||
        !program_range_valid(address, size)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (size != 0U) {
        memcpy(buffer, (const void *)(uintptr_t)address, size);
    }
    return CHIP_OK;
}

chip_status_t chip_program_flash_erase(uint32_t address, size_t size)
{
    uint32_t interrupt_state;
    uint32_t result;

    if (!program_range_valid(address, size) ||
        ((address % CH585_PROGRAM_FLASH_ERASE_SIZE) != 0U) ||
        ((size % CH585_PROGRAM_FLASH_ERASE_SIZE) != 0U)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    if (size == 0U) {
        return CHIP_OK;
    }
    interrupt_state = chip_system_critical_enter();
    result = FLASH_ROM_ERASE(address, (uint32_t)size);
    chip_system_critical_exit(interrupt_state);
    return program_result(result);
}

static chip_status_t program_operation(
    uint32_t address, const void *data, size_t size,
    uint32_t (*operation)(uint8_t, uint32_t, void *, uint32_t),
    uint8_t command)
{
    uint32_t staging[CH585_PROGRAM_FLASH_PREFERRED_SIZE / sizeof(uint32_t)];
    const uint8_t *input = (const uint8_t *)data;

    if (((data == NULL) && (size != 0U)) ||
        !program_range_valid(address, size) ||
        ((address % CH585_PROGRAM_FLASH_WRITE_SIZE) != 0U) ||
        ((size % CH585_PROGRAM_FLASH_WRITE_SIZE) != 0U)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    while (size > 0U) {
        size_t chunk = (size > sizeof(staging)) ? sizeof(staging) : size;
        uint32_t interrupt_state;
        uint32_t result;

        memcpy(staging, input, chunk);
        interrupt_state = chip_system_critical_enter();
        result = operation(command, address, staging, (uint32_t)chunk);
        chip_system_critical_exit(interrupt_state);
        if (result != 0U) {
            return CHIP_ERROR_IO;
        }
        input += chunk;
        address += (uint32_t)chunk;
        size -= chunk;
    }
    return CHIP_OK;
}

chip_status_t chip_program_flash_write(uint32_t address, const void *data,
                                       size_t size)
{
    return program_operation(address, data, size, FLASH_EEPROM_CMD,
                             CMD_FLASH_ROM_WRITE);
}

chip_status_t chip_program_flash_verify(uint32_t address, const void *data,
                                        size_t size)
{
    return program_operation(address, data, size, FLASH_EEPROM_CMD,
                             CMD_FLASH_ROM_VERIFY);
}
