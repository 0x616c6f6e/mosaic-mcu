#ifndef PLATFORM_OTA_IMAGE_H
#define PLATFORM_OTA_IMAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <platform_fatfs.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Signature at the start of an OTA package header. */
#define OTA_IMAGE_MAGIC          UINT32_C(0x41544F4D)
/** @brief Version of the OTA package header format. */
#define OTA_IMAGE_FORMAT_VERSION UINT32_C(1)
/** @brief Serialized OTA package header length in bytes. */
#define OTA_IMAGE_HEADER_SIZE    UINT32_C(36)

/** @brief Fixed-size OTA package header preceding the firmware payload. */
typedef struct {
    uint32_t magic;            /**< OTA_IMAGE_MAGIC signature. */
    uint32_t format_version;   /**< OTA_IMAGE_FORMAT_VERSION. */
    uint32_t header_size;      /**< OTA_IMAGE_HEADER_SIZE in bytes. */
    uint32_t target_id;        /**< Firmware product identifier. */
    uint32_t load_address;     /**< Destination Program-Flash address. */
    uint32_t image_size;       /**< Payload length in bytes. */
    uint32_t image_crc32;      /**< CRC32 of the firmware payload. */
    uint32_t firmware_version; /**< Package version number. */
    uint32_t header_crc32;     /**< CRC32 of preceding header fields. */
} ota_image_header_t;

/** @brief OTA image validation results. */
typedef enum {
    OTA_IMAGE_OK = 0,        /**< Header and payload validated. */
    OTA_IMAGE_NOT_FOUND,     /**< No update file or commit marker exists. */
    OTA_IMAGE_IO_ERROR,      /**< Filesystem or flash access failed. */
    OTA_IMAGE_INVALID_HEADER, /**< Header signature, version, or CRC invalid. */
    OTA_IMAGE_WRONG_TARGET,  /**< Package targets another product. */
    OTA_IMAGE_WRONG_ADDRESS, /**< Package load address differs. */
    OTA_IMAGE_INVALID_SIZE,  /**< Package or payload size invalid. */
    OTA_IMAGE_INVALID_CRC,   /**< Payload CRC differs from header. */
} ota_image_status_t;

/** @brief Initialize CRC32 accumulation.
 * @return Initial CRC32 accumulator.
 */
uint32_t ota_crc32_begin(void);
/** @brief Accumulate CRC32 over a byte sequence.
 * @param crc Accumulator from ota_crc32_begin() or a prior update.
 * @param data Input bytes.
 * @param size Input length in bytes.
 * @return Updated accumulator.
 */
uint32_t ota_crc32_update(uint32_t crc, const void *data, size_t size);
/** @brief Finalize a CRC32 accumulator.
 * @param crc Accumulator from ota_crc32_update().
 * @return Final CRC32 value.
 */
uint32_t ota_crc32_end(uint32_t crc);
/** @brief Compute CRC32 over a byte sequence.
 * @param data Input bytes.
 * @param size Input length in bytes.
 * @return Final CRC32 value.
 */
uint32_t ota_crc32(const void *data, size_t size);

/** @brief Validate header fields against an expected firmware target.
 * @param header OTA package header.
 * @param expected_target_id Product target identifier.
 * @param expected_load_address Application load address.
 * @param maximum_image_size Maximum firmware payload size in bytes.
 * @param file_size Total package size including header, in bytes.
 * @return OTA_IMAGE_OK if valid, or an OTA_IMAGE_* status.
 */
ota_image_status_t ota_image_validate_header(
    const ota_image_header_t *header, uint32_t expected_target_id,
    uint32_t expected_load_address, uint32_t maximum_image_size,
    uint32_t file_size);

/** @brief Read and validate a FAT-hosted OTA file and payload CRC.
 * @param path FatFs path of the OTA package.
 * @param expected_target_id Product target identifier.
 * @param expected_load_address Application load address.
 * @param maximum_image_size Maximum firmware payload size in bytes.
 * @param[out] header Validated header, or NULL if unused.
 * @param[out] scratch Temporary read buffer.
 * @param scratch_size Scratch buffer capacity in bytes.
 * @return OTA_IMAGE_OK if valid, or an OTA_IMAGE_* status.
 */
ota_image_status_t ota_image_validate_file(
    const char *path, uint32_t expected_target_id,
    uint32_t expected_load_address, uint32_t maximum_image_size,
    ota_image_header_t *header, uint8_t *scratch, size_t scratch_size);

#ifdef __cplusplus
}
#endif

#endif
