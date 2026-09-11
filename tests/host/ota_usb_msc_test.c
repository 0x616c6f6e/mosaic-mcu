#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <chip_time.h>
#include <ota_spi_disk.h>

#include "tusb.h"

#include "ota_usb_msc.h"

static uint32_t fake_now_ms;
static uint8_t sense_key;
static uint8_t sense_code;
static uint8_t sense_qualifier;
static unsigned int sync_count;

uint32_t chip_time_millis(void)
{
    return fake_now_ms;
}

chip_status_t ota_spi_disk_sync(ota_spi_disk_t *disk)
{
    assert(disk != NULL);
    ++sync_count;
    return CHIP_OK;
}

chip_status_t ota_spi_disk_read(ota_spi_disk_t *disk, uint32_t offset,
                                void *buffer, size_t size)
{
    (void)offset;
    (void)buffer;
    (void)size;
    return ((disk != NULL) && disk->initialized) ? CHIP_OK :
                                                  CHIP_ERROR_NOT_READY;
}

chip_status_t ota_spi_disk_write(ota_spi_disk_t *disk, uint32_t offset,
                                 const void *buffer, size_t size)
{
    (void)offset;
    (void)buffer;
    (void)size;
    return ((disk != NULL) && disk->initialized) ? CHIP_OK :
                                                  CHIP_ERROR_NOT_READY;
}

uint32_t ota_spi_disk_sector_count(const ota_spi_disk_t *disk)
{
    return ((disk != NULL) && disk->initialized) ? 32U : 0U;
}

bool tud_msc_set_sense(uint8_t lun, uint8_t key, uint8_t code,
                       uint8_t qualifier)
{
    assert(lun == 0U);
    sense_key = key;
    sense_code = code;
    sense_qualifier = qualifier;
    return true;
}

int main(void)
{
    ota_spi_disk_t disk = {.initialized = true};
    uint8_t buffer[OTA_DISK_SECTOR_SIZE] = {0U};
    scsi_inquiry_resp_t inquiry = {0};

    assert(!tud_msc_test_unit_ready_cb(0U));
    assert(sense_key == SCSI_SENSE_NOT_READY);
    assert(sense_code == 0x3AU);
    assert(sense_qualifier == 0U);

    fake_now_ms = 100U;
    ota_usb_msc_init(&disk);
    assert(ota_usb_msc_set_identity("Mosaic", "Keyboard OTA", "2") ==
           CHIP_OK);
    assert(ota_usb_msc_set_identity("vendor-too-long", "Keyboard", "1") ==
           CHIP_ERROR_INVALID_ARG);
    assert(tud_msc_inquiry2_cb(0U, &inquiry, sizeof(inquiry)) ==
           sizeof(inquiry));
    assert(memcmp(inquiry.vendor_id, "Mosaic  ", sizeof(inquiry.vendor_id)) ==
           0);
    assert(memcmp(inquiry.product_id, "Keyboard OTA    ",
                  sizeof(inquiry.product_id)) == 0);
    assert(memcmp(inquiry.product_rev, "2   ", sizeof(inquiry.product_rev)) ==
           0);
    assert(tud_msc_test_unit_ready_cb(0U));
    assert(!ota_usb_msc_should_check(fake_now_ms, 2000U));

    assert(tud_msc_write10_cb(0U, 1U, 0U, buffer, sizeof(buffer)) ==
           (int32_t)sizeof(buffer));
    assert(!ota_usb_msc_should_check(2099U, 2000U));
    assert(ota_usb_msc_should_check(2100U, 2000U));
    ota_usb_msc_mark_checked();
    assert(!ota_usb_msc_should_check(2100U, 2000U));

    fake_now_ms = 3000U;
    assert(tud_msc_write10_cb(0U, 2U, 0U, buffer, sizeof(buffer)) ==
           (int32_t)sizeof(buffer));
    assert(tud_msc_start_stop_cb(0U, 0U, false, true));
    assert(sync_count == 1U);
    assert(!tud_msc_test_unit_ready_cb(0U));
    assert(ota_usb_msc_should_check(fake_now_ms, 2000U));

    assert(tud_msc_start_stop_cb(0U, 0U, true, true));
    assert(tud_msc_test_unit_ready_cb(0U));
    return 0;
}
