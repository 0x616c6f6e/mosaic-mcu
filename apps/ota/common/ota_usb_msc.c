#include "ota_usb_msc.h"

#include <string.h>

#include <chip_time.h>
#include <platform_log.h>

#include "tusb.h"

static ota_spi_disk_t *msc_disk;
static uint32_t write_generation;
static uint32_t checked_generation;
static uint32_t last_write_ms;
static bool ejected;

void ota_usb_msc_init(ota_spi_disk_t *disk)
{
    msc_disk = disk;
    write_generation = 0U;
    checked_generation = 0U;
    last_write_ms = chip_time_millis();
    ejected = false;
}

bool ota_usb_msc_should_check(uint32_t now_ms, uint32_t idle_timeout_ms)
{
    return (write_generation != checked_generation) &&
           (ejected || ((now_ms - last_write_ms) >= idle_timeout_ms));
}

void ota_usb_msc_mark_checked(void)
{
    checked_generation = write_generation;
}

void ota_usb_msc_resume(void)
{
    ejected = false;
}

void tud_mount_cb(void)
{
    LOG_INFO("usb", "mass-storage mounted");
}

void tud_umount_cb(void)
{
    LOG_INFO("usb", "mass-storage unmounted");
}

uint32_t tud_msc_inquiry2_cb(uint8_t lun, scsi_inquiry_resp_t *response,
                             uint32_t buffer_size)
{
    static const char vendor[] = "Mosaic";
    static const char product[] = "OTA Disk";
    static const char revision[] = "1.0";

    (void)lun;
    (void)buffer_size;
    memcpy(response->vendor_id, vendor, sizeof(vendor) - 1U);
    memcpy(response->product_id, product, sizeof(product) - 1U);
    memcpy(response->product_rev, revision, sizeof(revision) - 1U);
    return sizeof(*response);
}

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    if ((msc_disk == NULL) || !msc_disk->initialized || ejected) {
        return tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3AU, 0x00U);
    }
    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count,
                         uint16_t *block_size)
{
    (void)lun;
    *block_count = ota_spi_disk_sector_count(msc_disk);
    *block_size = (uint16_t)OTA_DISK_SECTOR_SIZE;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start,
                           bool load_eject)
{
    (void)lun;
    (void)power_condition;
    if (load_eject && !start) {
        ejected = true;
        if (ota_spi_disk_sync(msc_disk) == CHIP_OK) {
            LOG_INFO("usb", "media ejected and synchronized");
        } else {
            LOG_ERROR("usb", "media synchronization failed");
        }
    }
    return true;
}

bool tud_msc_is_writable_cb(uint8_t lun)
{
    (void)lun;
    return true;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                          void *buffer, uint32_t buffer_size)
{
    uint64_t byte_offset = ((uint64_t)lba * OTA_DISK_SECTOR_SIZE) + offset;

    (void)lun;
    if ((byte_offset > UINT32_MAX) ||
        (ota_spi_disk_read(msc_disk, (uint32_t)byte_offset, buffer,
                           buffer_size) != CHIP_OK)) {
        LOG_ERROR("usb", "read failed lba=%lu offset=%lu size=%lu",
                  (unsigned long)lba, (unsigned long)offset,
                  (unsigned long)buffer_size);
        return -1;
    }
    return (int32_t)buffer_size;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           uint8_t *buffer, uint32_t buffer_size)
{
    uint64_t byte_offset = ((uint64_t)lba * OTA_DISK_SECTOR_SIZE) + offset;

    (void)lun;
    if ((byte_offset > UINT32_MAX) ||
        (ota_spi_disk_write(msc_disk, (uint32_t)byte_offset, buffer,
                            buffer_size) != CHIP_OK)) {
        LOG_ERROR("usb", "write failed lba=%lu offset=%lu size=%lu",
                  (unsigned long)lba, (unsigned long)offset,
                  (unsigned long)buffer_size);
        return -1;
    }
    last_write_ms = chip_time_millis();
    ++write_generation;
    return (int32_t)buffer_size;
}

int32_t tud_msc_scsi_cb(uint8_t lun, const uint8_t command[16], void *buffer,
                        uint16_t buffer_size)
{
    (void)buffer;
    (void)buffer_size;
    if (command[0] == 0x35U) {
        if (ota_spi_disk_sync(msc_disk) == CHIP_OK) {
            return 0;
        }
        LOG_ERROR("usb", "SCSI synchronize-cache failed");
        return -1;
    }
    (void)tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20U, 0x00U);
    return -1;
}
