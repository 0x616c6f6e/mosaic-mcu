#include <ota_spi_disk.h>

#include <string.h>

static ota_spi_disk_t *active_disk;

static bool disk_range_valid(const ota_spi_disk_t *disk, uint32_t offset,
                             size_t size)
{
    return (disk != NULL) && disk->initialized && (offset <= disk->size) &&
           (size <= (size_t)(disk->size - offset));
}

static chip_status_t flush_cache(ota_spi_disk_t *disk)
{
    uint32_t address;
    chip_status_t status;

    if (!disk->cache_valid || !disk->cache_dirty) {
        return CHIP_OK;
    }
    address = disk->offset + disk->cached_block;
    status = spi_flash_erase_sector(disk->flash, address);
    if (status == CHIP_OK) {
        status = spi_flash_program(disk->flash, address, disk->cache,
                                   disk->cache_size);
    }
    if (status == CHIP_OK) {
        disk->cache_dirty = false;
    }
    return status;
}

static chip_status_t load_cache(ota_spi_disk_t *disk, uint32_t block)
{
    chip_status_t status;

    if (disk->cache_valid && (disk->cached_block == block)) {
        return CHIP_OK;
    }
    status = flush_cache(disk);
    if (status != CHIP_OK) {
        return status;
    }
    status = spi_flash_read(disk->flash, disk->offset + block, disk->cache,
                            disk->cache_size);
    if (status == CHIP_OK) {
        disk->cached_block = block;
        disk->cache_valid = true;
        disk->cache_dirty = false;
    }
    return status;
}

chip_status_t ota_spi_disk_init(ota_spi_disk_t *disk, spi_flash_t *flash,
                                uint32_t offset, uint32_t size,
                                uint8_t *cache, size_t cache_size)
{
    if ((disk == NULL) || (flash == NULL) || !flash->initialized ||
        (cache == NULL) || (cache_size != flash->config.sector_size) ||
        (cache_size == 0U) || (cache_size > UINT32_MAX) || (size == 0U) ||
        ((offset % flash->config.sector_size) != 0U) ||
        ((size % flash->config.sector_size) != 0U) ||
        ((size % OTA_DISK_SECTOR_SIZE) != 0U) ||
        (offset > flash->config.capacity_bytes) ||
        (size > (flash->config.capacity_bytes - offset))) {
        return CHIP_ERROR_INVALID_ARG;
    }
    memset(disk, 0, sizeof(*disk));
    disk->flash = flash;
    disk->offset = offset;
    disk->size = size;
    disk->cache = cache;
    disk->cache_size = cache_size;
    disk->initialized = true;
    return CHIP_OK;
}

chip_status_t ota_spi_disk_bind(ota_spi_disk_t *disk)
{
    if ((disk == NULL) || !disk->initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    active_disk = disk;
    return CHIP_OK;
}

chip_status_t ota_spi_disk_sync(ota_spi_disk_t *disk)
{
    chip_status_t status;

    if ((disk == NULL) || !disk->initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    status = flush_cache(disk);
    if (status != CHIP_OK) {
        return status;
    }
    return spi_flash_wait_ready(disk->flash,
                                disk->flash->config.sector_erase_timeout_us);
}

chip_status_t ota_spi_disk_read(ota_spi_disk_t *disk, uint32_t offset,
                                void *buffer, size_t size)
{
    uint8_t *output = (uint8_t *)buffer;

    if (((buffer == NULL) && (size != 0U)) ||
        !disk_range_valid(disk, offset, size)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    while (size > 0U) {
        uint32_t block = offset - (offset % (uint32_t)disk->cache_size);
        size_t in_block = offset - block;
        size_t chunk = disk->cache_size - in_block;
        chip_status_t status;

        if (chunk > size) {
            chunk = size;
        }
        status = load_cache(disk, block);
        if (status != CHIP_OK) {
            return status;
        }
        memcpy(output, &disk->cache[in_block], chunk);
        output += chunk;
        offset += (uint32_t)chunk;
        size -= chunk;
    }
    return CHIP_OK;
}

chip_status_t ota_spi_disk_write(ota_spi_disk_t *disk, uint32_t offset,
                                 const void *buffer, size_t size)
{
    const uint8_t *input = (const uint8_t *)buffer;

    if (((buffer == NULL) && (size != 0U)) ||
        !disk_range_valid(disk, offset, size)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    while (size > 0U) {
        uint32_t block = offset - (offset % (uint32_t)disk->cache_size);
        size_t in_block = offset - block;
        size_t chunk = disk->cache_size - in_block;
        chip_status_t status;

        if (chunk > size) {
            chunk = size;
        }
        status = load_cache(disk, block);
        if (status != CHIP_OK) {
            return status;
        }
        if (memcmp(&disk->cache[in_block], input, chunk) != 0) {
            memcpy(&disk->cache[in_block], input, chunk);
            disk->cache_dirty = true;
        }
        input += chunk;
        offset += (uint32_t)chunk;
        size -= chunk;
    }
    return CHIP_OK;
}

uint32_t ota_spi_disk_sector_count(const ota_spi_disk_t *disk)
{
    return ((disk != NULL) && disk->initialized) ?
           (disk->size / OTA_DISK_SECTOR_SIZE) : 0U;
}

DSTATUS disk_initialize(BYTE drive)
{
    return (DSTATUS)(((drive == 0U) && (active_disk != NULL) &&
                      active_disk->initialized) ? 0U : STA_NOINIT);
}

DSTATUS disk_status(BYTE drive)
{
    return disk_initialize(drive);
}

DRESULT disk_read(BYTE drive, BYTE *buffer, LBA_t sector, UINT count)
{
    uint64_t offset = (uint64_t)sector * OTA_DISK_SECTOR_SIZE;
    uint64_t size = (uint64_t)count * OTA_DISK_SECTOR_SIZE;

    if ((drive != 0U) || (count == 0U) || (offset > UINT32_MAX) ||
        (size > SIZE_MAX)) {
        return RES_PARERR;
    }
    return (ota_spi_disk_read(active_disk, (uint32_t)offset, buffer,
                              (size_t)size) == CHIP_OK) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE drive, const BYTE *buffer, LBA_t sector, UINT count)
{
    uint64_t offset = (uint64_t)sector * OTA_DISK_SECTOR_SIZE;
    uint64_t size = (uint64_t)count * OTA_DISK_SECTOR_SIZE;

    if ((drive != 0U) || (count == 0U) || (offset > UINT32_MAX) ||
        (size > SIZE_MAX)) {
        return RES_PARERR;
    }
    return (ota_spi_disk_write(active_disk, (uint32_t)offset, buffer,
                               (size_t)size) == CHIP_OK) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE drive, BYTE command, void *buffer)
{
    if ((drive != 0U) || (active_disk == NULL) || !active_disk->initialized) {
        return RES_NOTRDY;
    }
    switch (command) {
    case CTRL_SYNC:
        return (ota_spi_disk_sync(active_disk) == CHIP_OK) ? RES_OK : RES_ERROR;
    case GET_SECTOR_COUNT:
        if (buffer == NULL) {
            return RES_PARERR;
        }
        *(LBA_t *)buffer = ota_spi_disk_sector_count(active_disk);
        return RES_OK;
    case GET_SECTOR_SIZE:
        if (buffer == NULL) {
            return RES_PARERR;
        }
        *(WORD *)buffer = (WORD)OTA_DISK_SECTOR_SIZE;
        return RES_OK;
    case GET_BLOCK_SIZE:
        if (buffer == NULL) {
            return RES_PARERR;
        }
        *(DWORD *)buffer = (DWORD)(active_disk->cache_size /
                                    OTA_DISK_SECTOR_SIZE);
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
