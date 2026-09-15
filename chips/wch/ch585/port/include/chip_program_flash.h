#ifndef CHIP_API_PROGRAM_FLASH_H
#define CHIP_API_PROGRAM_FLASH_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Program-Flash size and erase/write granularity in bytes. */
typedef struct {
    uint32_t size;                 /**< Total Program-Flash capacity in bytes. */
    uint32_t erase_size;           /**< Erase alignment in bytes. */
    uint32_t write_size;           /**< Required write alignment in bytes. */
    uint32_t preferred_write_size; /**< Efficient write chunk size in bytes. */
} chip_program_flash_info_t;

/** @brief Query Program-Flash geometry.
 * @return Size and operation granularity in bytes.
 */
chip_program_flash_info_t chip_program_flash_info(void);
/** @brief Read Program-Flash at a mapped address.
 * @param address Program-Flash address.
 * @param[out] buffer Destination buffer.
 * @param size Number of bytes to read.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_program_flash_read(uint32_t address, void *buffer,
                                      size_t size);
/** @brief Erase Program-Flash blocks.
 * @param address Program-Flash address aligned to erase_size.
 * @param size Number of bytes, a multiple of erase_size.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_program_flash_erase(uint32_t address, size_t size);
/** @brief Program bytes at a Program-Flash address.
 * @param address Program-Flash address aligned to write_size.
 * @param data Bytes to program.
 * @param size Number of bytes, a multiple of write_size.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_program_flash_write(uint32_t address, const void *data,
                                       size_t size);
/** @brief Compare Program-Flash with an expected byte sequence.
 * @param address Program-Flash address aligned to write_size.
 * @param data Expected bytes.
 * @param size Number of bytes, a multiple of write_size.
 * @return CHIP_OK if identical, or a CHIP_ERROR_* status.
 */
chip_status_t chip_program_flash_verify(uint32_t address, const void *data,
                                        size_t size);

#ifdef __cplusplus
}
#endif

#endif
