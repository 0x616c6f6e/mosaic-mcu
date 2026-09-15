#ifndef CHIP_API_SYSTEM_H
#define CHIP_API_SYSTEM_H

#include <stddef.h>
#include <stdint.h>

#include <chip_status.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Hardware unique ID length in bytes. */
#define CHIP_UNIQUE_ID_SIZE 8U

/** @brief Internal oscillator or external crystal clock. */
typedef enum {
    CHIP_CLOCK_INTERNAL, /**< Internal clock source. */
    CHIP_CLOCK_EXTERNAL, /**< External crystal source. */
} chip_clock_source_t;

/** @brief Clock source, core frequency, and optional crystal load. */
typedef struct {
    chip_clock_source_t source; /**< Clock source to select. */
    uint32_t core_clock_hz;     /**< Requested core frequency in Hz. */
    uint8_t external_crystal_load_pf; /**< Crystal load in pF; 0 uses default. */
} chip_system_config_t;

/** @brief Configure the SoC clock and core system.
 * @param config Clock settings.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_system_init(const chip_system_config_t *config);
/** @brief Read the configured core clock frequency.
 * @return Current core clock in hertz.
 */
uint32_t chip_system_clock_hz(void);
/** @brief Read the selected core clock source.
 * @return Active clock source.
 */
chip_clock_source_t chip_system_clock_source(void);
/** @brief Read the chip identification byte.
 * @return SoC chip ID.
 */
uint8_t chip_system_chip_id(void);
/** @brief Read the hardware unique ID.
 * @param[out] buffer Receives CHIP_UNIQUE_ID_SIZE bytes.
 * @param size Capacity of buffer in bytes.
 * @return CHIP_OK on success, or a CHIP_ERROR_* status.
 */
chip_status_t chip_system_unique_id(uint8_t *buffer, size_t size);

/** @brief Enter a critical section and save interrupt state.
 * @return Opaque state token for the matching exit call.
 */
uint32_t chip_system_critical_enter(void);
/** @brief Leave a critical section and restore interrupt state.
 * @param state Token returned by the matching enter call.
 */
void chip_system_critical_exit(uint32_t state);

/** @brief Reset the SoC; does not return. */
void chip_system_reset(void);
/** @brief Jump to a firmware entry address; does not return.
 * @param address Destination code address.
 */
void chip_system_jump(uint32_t address);

#ifdef __cplusplus
}
#endif

#endif
