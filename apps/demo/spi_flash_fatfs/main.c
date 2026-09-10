#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <chip_time.h>
#include <platform_fatfs.h>
#include <platform_log.h>
#include <spi_flash.h>

#include "demo_support.h"

#define FATFS_DRIVE                0U
#define FATFS_SECTOR_SIZE        512U
#define FATFS_ERASE_BLOCK_SECTORS \
    (DEMO_SPI_FLASH_SECTOR_SIZE / FATFS_SECTOR_SIZE)

_Static_assert((DEMO_SPI_FLASH_SECTOR_SIZE % FATFS_SECTOR_SIZE) == 0U,
               "SPI Flash erase size must be a multiple of 512 bytes");
_Static_assert((DEMO_FATFS_OFFSET_BYTES % DEMO_SPI_FLASH_SECTOR_SIZE) == 0U,
               "FatFs offset must be erase-block aligned");
_Static_assert((DEMO_FATFS_SIZE_BYTES % DEMO_SPI_FLASH_SECTOR_SIZE) == 0U,
               "FatFs size must be a whole number of erase blocks");
_Static_assert(DEMO_FATFS_SIZE_BYTES <= DEMO_SPI_FLASH_CAPACITY_BYTES,
               "FatFs partition is larger than the flash");
_Static_assert(DEMO_FATFS_OFFSET_BYTES <=
               (DEMO_SPI_FLASH_CAPACITY_BYTES - DEMO_FATFS_SIZE_BYTES),
               "FatFs partition exceeds flash capacity");

static spi_flash_t *disk_flash;
static uint8_t erase_block_buffer[DEMO_SPI_FLASH_SECTOR_SIZE];
static uint8_t format_buffer[FATFS_SECTOR_SIZE];
static FATFS filesystem;
static FIL boot_count_file;

static void require_status(chip_status_t status, const char *operation)
{
    if (status != CHIP_OK) {
        LOG_ERROR("fatfs", "%s failed, status=%u", operation,
                  (unsigned int)status);
        demo_halt();
    }
}

static void require_fatfs(FRESULT result, const char *operation)
{
    if (result != FR_OK) {
        LOG_ERROR("fatfs", "%s failed, error=%u", operation,
                  (unsigned int)result);
        demo_halt();
    }
}

static bool disk_range_valid(LBA_t sector, UINT count)
{
    uint64_t end_sector = (uint64_t)sector + count;
    uint64_t sector_count = DEMO_FATFS_SIZE_BYTES / FATFS_SECTOR_SIZE;

    return (count != 0U) && (end_sector <= sector_count);
}

DSTATUS disk_initialize(BYTE drive)
{
    return (DSTATUS)(((drive == FATFS_DRIVE) && (disk_flash != NULL) &&
                      disk_flash->initialized) ? 0U : STA_NOINIT);
}

DSTATUS disk_status(BYTE drive)
{
    return disk_initialize(drive);
}

DRESULT disk_read(BYTE drive, BYTE *buffer, LBA_t sector, UINT count)
{
    uint32_t address;
    size_t size;

    if ((drive != FATFS_DRIVE) || (buffer == NULL) ||
        !disk_range_valid(sector, count)) {
        return RES_PARERR;
    }
    if ((disk_flash == NULL) || !disk_flash->initialized) {
        return RES_NOTRDY;
    }
    address = DEMO_FATFS_OFFSET_BYTES +
              ((uint32_t)sector * FATFS_SECTOR_SIZE);
    size = (size_t)count * FATFS_SECTOR_SIZE;
    return (spi_flash_read(disk_flash, address, buffer, size) == CHIP_OK) ?
           RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE drive, const BYTE *buffer, LBA_t sector, UINT count)
{
    if ((drive != FATFS_DRIVE) || (buffer == NULL) ||
        !disk_range_valid(sector, count)) {
        return RES_PARERR;
    }
    if ((disk_flash == NULL) || !disk_flash->initialized) {
        return RES_NOTRDY;
    }

    while (count > 0U) {
        LBA_t block_sector = sector - (sector % FATFS_ERASE_BLOCK_SECTORS);
        UINT sector_offset = (UINT)(sector - block_sector);
        UINT block_remaining = (UINT)FATFS_ERASE_BLOCK_SECTORS - sector_offset;
        UINT chunk_count = (count < block_remaining) ? count : block_remaining;
        uint32_t block_address = DEMO_FATFS_OFFSET_BYTES +
                                 ((uint32_t)block_sector * FATFS_SECTOR_SIZE);
        size_t byte_offset = (size_t)sector_offset * FATFS_SECTOR_SIZE;
        size_t byte_count = (size_t)chunk_count * FATFS_SECTOR_SIZE;

        if (spi_flash_read(disk_flash, block_address, erase_block_buffer,
                           sizeof(erase_block_buffer)) != CHIP_OK) {
            return RES_ERROR;
        }
        if (memcmp(&erase_block_buffer[byte_offset], buffer, byte_count) != 0) {
            memcpy(&erase_block_buffer[byte_offset], buffer, byte_count);
            if ((spi_flash_erase_sector(disk_flash, block_address) != CHIP_OK) ||
                (spi_flash_program(disk_flash, block_address,
                                   erase_block_buffer,
                                   sizeof(erase_block_buffer)) != CHIP_OK)) {
                return RES_ERROR;
            }
        }

        sector += chunk_count;
        buffer += byte_count;
        count -= chunk_count;
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE drive, BYTE command, void *buffer)
{
    if (drive != FATFS_DRIVE) {
        return RES_PARERR;
    }
    if ((disk_flash == NULL) || !disk_flash->initialized) {
        return RES_NOTRDY;
    }

    switch (command) {
    case CTRL_SYNC:
        return (spi_flash_wait_ready(
                    disk_flash,
                    disk_flash->config.sector_erase_timeout_us) == CHIP_OK) ?
               RES_OK : RES_ERROR;
    case GET_SECTOR_COUNT:
        if (buffer == NULL) {
            return RES_PARERR;
        }
        *(LBA_t *)buffer = DEMO_FATFS_SIZE_BYTES / FATFS_SECTOR_SIZE;
        return RES_OK;
    case GET_SECTOR_SIZE:
        if (buffer == NULL) {
            return RES_PARERR;
        }
        *(WORD *)buffer = FATFS_SECTOR_SIZE;
        return RES_OK;
    case GET_BLOCK_SIZE:
        if (buffer == NULL) {
            return RES_PARERR;
        }
        *(DWORD *)buffer = FATFS_ERASE_BLOCK_SECTORS;
        return RES_OK;
    default:
        return RES_PARERR;
    }
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
    const MKFS_PARM format_options = {
        .fmt = FM_FAT | FM_SFD,
        .n_fat = 1U,
        .align = FATFS_ERASE_BLOCK_SECTORS,
        .n_root = 64U,
        .au_size = DEMO_SPI_FLASH_SECTOR_SIZE,
    };
    spi_flash_jedec_id_t jedec_id;
    spi_flash_t flash;
    uint32_t boot_count = 0U;
    uint32_t verified_count = 0U;
    UINT transferred;
    FRESULT result;
    bool formatted = false;

    DEMO_REQUIRE(demo_platform_init());
    DEMO_REQUIRE(demo_led_init());
    DEMO_REQUIRE(demo_log_init());
    require_status(demo_spi_flash_pins_init(), "pin init");
    require_status(spi_flash_init(&flash, &flash_config), "flash init");
    require_status(spi_flash_read_jedec_id(&flash, &jedec_id),
                   "JEDEC ID read");
    LOG_INFO("fatfs", "JEDEC ID=%02X %02X %02X sectors=%lu",
             (unsigned int)jedec_id.manufacturer,
             (unsigned int)jedec_id.memory_type,
             (unsigned int)jedec_id.capacity,
             (unsigned long)(DEMO_FATFS_SIZE_BYTES / FATFS_SECTOR_SIZE));

    if ((jedec_id.capacity >= 32U) ||
        ((UINT32_C(1) << jedec_id.capacity) !=
         DEMO_SPI_FLASH_CAPACITY_BYTES)) {
        LOG_ERROR("fatfs", "capacity mismatch, code=0x%02X configured=%lu",
                  (unsigned int)jedec_id.capacity,
                  (unsigned long)DEMO_SPI_FLASH_CAPACITY_BYTES);
        demo_halt();
    }
    disk_flash = &flash;

    result = f_mount(&filesystem, "0:", 1U);
    if (result == FR_NO_FILESYSTEM) {
        LOG_WARN("fatfs", "no filesystem, formatting");
        require_fatfs(f_mkfs("0:", &format_options, format_buffer,
                             sizeof(format_buffer)),
                      "format");
        require_fatfs(f_mount(&filesystem, "0:", 1U), "mount");
        formatted = true;
    } else {
        require_fatfs(result, "mount");
    }

    require_fatfs(f_open(&boot_count_file, "0:/BOOTCNT.BIN",
                         FA_READ | FA_WRITE | FA_OPEN_ALWAYS),
                  "open boot count");
    require_fatfs(f_read(&boot_count_file, &boot_count, sizeof(boot_count),
                         &transferred),
                  "read boot count");
    if ((transferred != 0U) && (transferred != sizeof(boot_count))) {
        LOG_ERROR("fatfs", "invalid boot count size=%u", transferred);
        demo_halt();
    }
    ++boot_count;
    require_fatfs(f_lseek(&boot_count_file, 0U), "rewind");
    require_fatfs(f_write(&boot_count_file, &boot_count, sizeof(boot_count),
                          &transferred),
                  "write boot count");
    if (transferred != sizeof(boot_count)) {
        LOG_ERROR("fatfs", "short boot count write=%u", transferred);
        demo_halt();
    }
    require_fatfs(f_truncate(&boot_count_file), "truncate boot count");
    require_fatfs(f_close(&boot_count_file), "close boot count");
    require_fatfs(f_unmount("0:"), "unmount");

    require_fatfs(f_mount(&filesystem, "0:", 1U), "verification mount");
    require_fatfs(f_open(&boot_count_file, "0:/BOOTCNT.BIN", FA_READ),
                  "verification open");
    require_fatfs(f_read(&boot_count_file, &verified_count,
                         sizeof(verified_count), &transferred),
                  "verification read");
    if ((transferred != sizeof(verified_count)) ||
        (verified_count != boot_count)) {
        LOG_ERROR("fatfs", "verification failed, read=%u value=%lu",
                  transferred, (unsigned long)verified_count);
        demo_halt();
    }
    require_fatfs(f_close(&boot_count_file), "verification close");
    require_fatfs(f_unmount("0:"), "verification unmount");

    LOG_INFO("fatfs", "boot_count=%lu formatted=%u",
             (unsigned long)boot_count, formatted ? 1U : 0U);
    require_status(demo_led_write(true), "success LED");
    for (;;) {
        chip_delay_ms(1000U);
    }
}
