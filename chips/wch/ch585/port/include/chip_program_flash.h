#ifndef CHIP_API_PROGRAM_FLASH_H
#define CHIP_API_PROGRAM_FLASH_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t size;
    uint32_t erase_size;
    uint32_t write_size;
    uint32_t preferred_write_size;
} chip_program_flash_info_t;

chip_program_flash_info_t chip_program_flash_info(void);
chip_status_t chip_program_flash_read(uint32_t address, void *buffer,
                                      size_t size);
chip_status_t chip_program_flash_erase(uint32_t address, size_t size);
chip_status_t chip_program_flash_write(uint32_t address, const void *data,
                                       size_t size);
chip_status_t chip_program_flash_verify(uint32_t address, const void *data,
                                        size_t size);

#ifdef __cplusplus
}
#endif

#endif
