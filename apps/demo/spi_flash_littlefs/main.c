#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <chip_time.h>
#include <lfs.h>
#include <platform_log.h>
#include <spi_flash.h>

#include "demo_support.h"

#define LITTLEFS_READ_SIZE      16U
#define LITTLEFS_PROGRAM_SIZE   16U
#define LITTLEFS_CACHE_SIZE    256U
#define LITTLEFS_LOOKAHEAD_SIZE 32U
#define LITTLEFS_BLOCK_CYCLES  500

_Static_assert((DEMO_LITTLEFS_OFFSET_BYTES % DEMO_SPI_FLASH_SECTOR_SIZE) == 0U,
               "littlefs offset must be sector aligned");
_Static_assert((DEMO_LITTLEFS_SIZE_BYTES % DEMO_SPI_FLASH_SECTOR_SIZE) == 0U,
               "littlefs size must be a whole number of sectors");
_Static_assert(DEMO_LITTLEFS_SIZE_BYTES >=
               (UINT32_C(2) * DEMO_SPI_FLASH_SECTOR_SIZE),
               "littlefs needs at least two sectors");
_Static_assert(DEMO_LITTLEFS_SIZE_BYTES <= DEMO_SPI_FLASH_CAPACITY_BYTES,
               "littlefs partition is larger than the flash");
_Static_assert(DEMO_LITTLEFS_OFFSET_BYTES <=
               (DEMO_SPI_FLASH_CAPACITY_BYTES - DEMO_LITTLEFS_SIZE_BYTES),
               "littlefs partition exceeds flash capacity");

typedef struct {
    spi_flash_t *flash;
    uint32_t offset;
} littlefs_storage_t;

static lfs_t filesystem;
static lfs_file_t boot_count_file;
static uint8_t read_buffer[LITTLEFS_CACHE_SIZE];
static uint8_t program_buffer[LITTLEFS_CACHE_SIZE];
static uint8_t lookahead_buffer[LITTLEFS_LOOKAHEAD_SIZE];
static uint8_t file_buffer[LITTLEFS_CACHE_SIZE];

static void require_status(chip_status_t status, const char *operation)
{
    if (status != CHIP_OK) {
        LOG_ERROR("littlefs", "%s failed, status=%u", operation,
                  (unsigned int)status);
        demo_halt();
    }
}

static void require_littlefs(int result, const char *operation)
{
    if (result < 0) {
        LOG_ERROR("littlefs", "%s failed, error=%d", operation, result);
        demo_halt();
    }
}

static bool storage_address(const struct lfs_config *config,
                            lfs_block_t block, lfs_off_t offset,
                            lfs_size_t size, uint32_t *address)
{
    const littlefs_storage_t *storage = config->context;
    uint64_t relative_address;
    uint64_t absolute_address;

    if ((storage == NULL) || (address == NULL) ||
        (block >= config->block_count) ||
        (offset > config->block_size) ||
        (size > (config->block_size - offset))) {
        return false;
    }
    relative_address = ((uint64_t)block * config->block_size) + offset;
    absolute_address = storage->offset + relative_address;
    if ((absolute_address + size) > DEMO_SPI_FLASH_CAPACITY_BYTES) {
        return false;
    }
    *address = (uint32_t)absolute_address;
    return true;
}

static int storage_read(const struct lfs_config *config, lfs_block_t block,
                        lfs_off_t offset, void *buffer, lfs_size_t size)
{
    const littlefs_storage_t *storage = config->context;
    uint32_t address;

    if (!storage_address(config, block, offset, size, &address)) {
        return LFS_ERR_INVAL;
    }
    return (spi_flash_read(storage->flash, address, buffer, size) == CHIP_OK) ?
           LFS_ERR_OK : LFS_ERR_IO;
}

static int storage_program(const struct lfs_config *config, lfs_block_t block,
                           lfs_off_t offset, const void *buffer,
                           lfs_size_t size)
{
    const littlefs_storage_t *storage = config->context;
    uint32_t address;

    if (!storage_address(config, block, offset, size, &address)) {
        return LFS_ERR_INVAL;
    }
    return (spi_flash_program(storage->flash, address, buffer, size) == CHIP_OK) ?
           LFS_ERR_OK : LFS_ERR_IO;
}

static int storage_erase(const struct lfs_config *config, lfs_block_t block)
{
    const littlefs_storage_t *storage = config->context;
    uint32_t address;

    if (!storage_address(config, block, 0U, config->block_size, &address)) {
        return LFS_ERR_INVAL;
    }
    return (spi_flash_erase_sector(storage->flash, address) == CHIP_OK) ?
           LFS_ERR_OK : LFS_ERR_IO;
}

static int storage_sync(const struct lfs_config *config)
{
    (void)config;
    return LFS_ERR_OK;
}

int main(void)
{
    const spi_flash_config_t flash_config = {
        .spi = DEMO_SPI_FLASH_INSTANCE,
        .cs_pin = DEMO_SPI_FLASH_CS_PIN,
        .clock_hz = DEMO_SPI_FLASH_CLOCK_HZ,
        .capacity_bytes = DEMO_SPI_FLASH_CAPACITY_BYTES,
        .page_size = DEMO_SPI_FLASH_PAGE_SIZE,
        .sector_size = DEMO_SPI_FLASH_SECTOR_SIZE,
        .transfer_timeout_us = DEMO_SPI_FLASH_TRANSFER_TIMEOUT_US,
        .program_timeout_us = DEMO_SPI_FLASH_PROGRAM_TIMEOUT_US,
        .sector_erase_timeout_us = DEMO_SPI_FLASH_ERASE_TIMEOUT_US,
        .chip_erase_timeout_us = DEMO_SPI_FLASH_CHIP_ERASE_TIMEOUT_US,
    };
    spi_flash_t flash;
    littlefs_storage_t storage = {
        .flash = &flash,
        .offset = DEMO_LITTLEFS_OFFSET_BYTES,
    };
    const struct lfs_config littlefs_config = {
        .context = &storage,
        .read = storage_read,
        .prog = storage_program,
        .erase = storage_erase,
        .sync = storage_sync,
        .read_size = LITTLEFS_READ_SIZE,
        .prog_size = LITTLEFS_PROGRAM_SIZE,
        .block_size = DEMO_SPI_FLASH_SECTOR_SIZE,
        .block_count = DEMO_LITTLEFS_SIZE_BYTES / DEMO_SPI_FLASH_SECTOR_SIZE,
        .block_cycles = LITTLEFS_BLOCK_CYCLES,
        .cache_size = LITTLEFS_CACHE_SIZE,
        .lookahead_size = LITTLEFS_LOOKAHEAD_SIZE,
        .read_buffer = read_buffer,
        .prog_buffer = program_buffer,
        .lookahead_buffer = lookahead_buffer,
    };
    const struct lfs_file_config file_config = {
        .buffer = file_buffer,
    };
    spi_flash_jedec_id_t jedec_id;
    uint32_t boot_count = 0U;
    uint32_t verified_count = 0U;
    lfs_ssize_t bytes_read;
    int result;
    bool formatted = false;

    DEMO_REQUIRE(demo_platform_init());
    DEMO_REQUIRE(demo_led_init());
    DEMO_REQUIRE(demo_log_init());
    require_status(demo_spi_flash_pins_init(), "pin init");
    require_status(spi_flash_init(&flash, &flash_config), "flash init");
    require_status(spi_flash_read_jedec_id(&flash, &jedec_id),
                   "JEDEC ID read");
    LOG_INFO("littlefs", "JEDEC ID=%02X %02X %02X blocks=%lu",
             (unsigned int)jedec_id.manufacturer,
             (unsigned int)jedec_id.memory_type,
             (unsigned int)jedec_id.capacity,
             (unsigned long)littlefs_config.block_count);

    if ((jedec_id.capacity >= 32U) ||
        ((UINT32_C(1) << jedec_id.capacity) !=
         DEMO_SPI_FLASH_CAPACITY_BYTES)) {
        LOG_ERROR("littlefs", "capacity mismatch, code=0x%02X configured=%lu",
                  (unsigned int)jedec_id.capacity,
                  (unsigned long)DEMO_SPI_FLASH_CAPACITY_BYTES);
        demo_halt();
    }

    result = lfs_mount(&filesystem, &littlefs_config);
    if (result == LFS_ERR_CORRUPT) {
        LOG_WARN("littlefs", "mount failed, formatting, error=%d", result);
        require_littlefs(lfs_format(&filesystem, &littlefs_config), "format");
        require_littlefs(lfs_mount(&filesystem, &littlefs_config), "mount");
        formatted = true;
    } else {
        require_littlefs(result, "mount");
    }

    require_littlefs(lfs_file_opencfg(&filesystem, &boot_count_file,
                                      "boot_count", LFS_O_RDWR | LFS_O_CREAT,
                                      &file_config),
                     "open boot_count");
    bytes_read = lfs_file_read(&filesystem, &boot_count_file, &boot_count,
                               sizeof(boot_count));
    if ((bytes_read != 0) && (bytes_read != (lfs_ssize_t)sizeof(boot_count))) {
        LOG_ERROR("littlefs", "boot_count read failed, result=%ld",
                  (long)bytes_read);
        demo_halt();
    }
    ++boot_count;
    require_littlefs(lfs_file_rewind(&filesystem, &boot_count_file), "rewind");
    result = (int)lfs_file_write(&filesystem, &boot_count_file, &boot_count,
                                 sizeof(boot_count));
    if (result != (int)sizeof(boot_count)) {
        LOG_ERROR("littlefs", "boot_count write failed, result=%d", result);
        demo_halt();
    }
    require_littlefs(lfs_file_truncate(&filesystem, &boot_count_file,
                                       sizeof(boot_count)),
                     "truncate boot_count");
    require_littlefs(lfs_file_close(&filesystem, &boot_count_file),
                     "close boot_count");
    require_littlefs(lfs_unmount(&filesystem), "unmount");

    require_littlefs(lfs_mount(&filesystem, &littlefs_config),
                     "verification mount");
    require_littlefs(lfs_file_opencfg(&filesystem, &boot_count_file,
                                      "boot_count", LFS_O_RDONLY,
                                      &file_config),
                     "verification open");
    bytes_read = lfs_file_read(&filesystem, &boot_count_file, &verified_count,
                               sizeof(verified_count));
    if ((bytes_read != (lfs_ssize_t)sizeof(verified_count)) ||
        (verified_count != boot_count)) {
        LOG_ERROR("littlefs", "verification failed, read=%ld value=%lu",
                  (long)bytes_read, (unsigned long)verified_count);
        demo_halt();
    }
    require_littlefs(lfs_file_close(&filesystem, &boot_count_file),
                     "verification close");
    require_littlefs(lfs_unmount(&filesystem), "verification unmount");

    LOG_INFO("littlefs", "boot_count=%lu formatted=%u",
             (unsigned long)boot_count, formatted ? 1U : 0U);
    require_status(demo_led_write(true), "success LED");
    for (;;) {
        chip_delay_ms(1000U);
    }
}
