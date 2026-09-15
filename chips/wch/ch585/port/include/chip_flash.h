#ifndef CHIP_API_FLASH_H
#define CHIP_API_FLASH_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Data-Flash size and erase/write granularity in bytes. */
typedef struct {
    uint32_t size;       /**< Total Data-Flash capacity in bytes. */
    uint32_t erase_size; /**< Required erase alignment in bytes. */
    uint32_t write_size; /**< Reported hardware write unit in bytes. */
} chip_flash_info_t;

/** @brief Query Data-Flash geometry.
 * @return Size and operation granularity in bytes.
 */
chip_flash_info_t chip_flash_info(void);
/** @brief Read from Data-Flash using a zero-based offset.
 * @param offset Byte offset from the start of Data-Flash.
 * @param[out] buffer Destination buffer.
 * @param size Number of bytes to read.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_flash_read(uint32_t offset, void *buffer, size_t size);
/** @brief Write to Data-Flash using a zero-based offset.
 * @param offset Byte offset from the start of Data-Flash.
 * @param data Bytes to program.
 * @param size Number of bytes to write.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_flash_write(uint32_t offset, const void *data, size_t size);
/** @brief Erase a region of Data-Flash.
 * @param offset Zero-based byte offset, aligned to erase_size.
 * @param size Number of bytes, a multiple of erase_size.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_flash_erase(uint32_t offset, size_t size);

#ifdef __cplusplus
}
#endif

#endif
