#ifndef PLATFORM_OTA_STAGING_H
#define PLATFORM_OTA_STAGING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <ota_image.h>
#include <spi_flash.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Raw flash offsets and image limits for an OTA staging partition. */
typedef struct {
    spi_flash_t *flash;           /**< Flash backing the staging partition. */
    uint32_t metadata_offset;     /**< Commit-marker address in flash bytes. */
    uint32_t image_offset;        /**< Package start address in flash bytes. */
    uint32_t image_capacity;      /**< Maximum package size in bytes. */
    uint32_t erase_size;          /**< Erase-block size in bytes. */
    uint32_t target_id;           /**< Expected firmware product identifier. */
    uint32_t application_address; /**< Expected Program-Flash load address. */
    uint32_t application_size;    /**< Maximum payload size in bytes. */
} ota_staging_config_t;

/** @brief Initialized OTA staging area. */
typedef struct {
    ota_staging_config_t config; /**< Validated partition configuration. */
    bool initialized;            /**< true after successful init. */
} ota_staging_t;

/** @brief Configure a raw OTA staging area outside the filesystem.
 * @param[out] staging Instance to initialize.
 * @param config Flash partition, target, and application limits.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t ota_staging_init(ota_staging_t *staging,
                               const ota_staging_config_t *config);
/** @brief Remove the staging area's commit marker.
 * @param staging Initialized staging area.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t ota_staging_clear(ota_staging_t *staging);
/** @brief Mark a completely written package as committed.
 * @param staging Initialized staging area.
 * @param package_size Total package size including header, in bytes.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t ota_staging_commit(ota_staging_t *staging,
                                 uint32_t package_size);
/** @brief Validate a raw staged package's header and payload CRC.
 * @param staging Initialized staging area.
 * @param package_size Total package size including header, in bytes.
 * @param[out] header Validated header, or NULL if unused.
 * @param[out] scratch Temporary read buffer.
 * @param scratch_size Scratch buffer capacity in bytes.
 * @return OTA_IMAGE_OK if valid, or an OTA_IMAGE_* status.
 */
ota_image_status_t ota_staging_validate_package(
    ota_staging_t *staging, uint32_t package_size,
    ota_image_header_t *header, uint8_t *scratch, size_t scratch_size);
/** @brief Validate the last committed staging package.
 * @param staging Initialized staging area.
 * @param[out] header Validated header, or NULL if unused.
 * @param[out] scratch Temporary read buffer.
 * @param scratch_size Scratch buffer capacity in bytes.
 * @return OTA_IMAGE_OK if valid, or an OTA_IMAGE_* status.
 */
ota_image_status_t ota_staging_validate_committed(
    ota_staging_t *staging, ota_image_header_t *header,
    uint8_t *scratch, size_t scratch_size);
/** @brief Read bytes relative to the staged image payload.
 * @param staging Initialized staging area.
 * @param payload_offset Byte offset after the OTA header.
 * @param[out] data Destination buffer.
 * @param size Number of bytes to read.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t ota_staging_read_payload(ota_staging_t *staging,
                                       uint32_t payload_offset, void *data,
                                       size_t size);

#ifdef __cplusplus
}
#endif

#endif
