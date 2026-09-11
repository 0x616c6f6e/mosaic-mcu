#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <chip_flash.h>
#include <chip_program_flash.h>
#include <chip_system.h>
#include <chip_time.h>
#include <ota_image.h>
#include <ota_spi_disk.h>
#include <platform_fatfs.h>
#include <platform_log.h>
#include <spi_flash.h>

#include "tusb.h"

#include "keyboard_board.h"
#include "keyboard_log.h"
#include <firmware_version.h>
#include "ota_staging.h"
#include "ota_usb_msc.h"
#include "ota_webusb.h"

static spi_flash_t external_flash;
static ota_spi_disk_t update_disk;
static ota_staging_t update_staging;
static uint8_t disk_cache[OTA_SPI_FLASH_SECTOR_SIZE];
static uint8_t io_buffer[512] __attribute__((aligned(4)));
static uint8_t format_buffer[512];
static FATFS filesystem;

#define OTA_INSTALL_PROGRESS_LOG_INTERVAL UINT32_C(32768)

uint32_t tusb_time_millis_api(void)
{
    return chip_time_millis();
}

static bool install_is_in_progress(void)
{
    uint8_t marker[OTA_INSTALL_MARKER_SIZE];
    size_t index;
    chip_status_t status = chip_flash_read(OTA_INSTALL_MARKER_OFFSET, marker,
                                           sizeof(marker));

    if (status != CHIP_OK) {
        LOG_ERROR("boot", "install marker read failed status=%u",
                  (unsigned int)status);
        return true;
    }
    for (index = 0U; index < sizeof(marker); ++index) {
        if (marker[index] != UINT8_C(0xFF)) {
            LOG_DEBUG("boot", "install marker set offset=%lu value=0x%02X",
                      (unsigned long)index, (unsigned int)marker[index]);
            return true;
        }
    }
    return false;
}

static bool mark_install_in_progress(void)
{
    const uint32_t marker = 0U;
    uint32_t verify = UINT32_MAX;

    return (chip_flash_erase(OTA_INSTALL_MARKER_OFFSET,
                             OTA_INSTALL_MARKER_SIZE) == CHIP_OK) &&
           (chip_flash_write(OTA_INSTALL_MARKER_OFFSET, &marker,
                             sizeof(marker)) == CHIP_OK) &&
           (chip_flash_read(OTA_INSTALL_MARKER_OFFSET, &verify,
                            sizeof(verify)) == CHIP_OK) &&
           (verify == marker);
}

static bool mark_install_complete(void)
{
    return chip_flash_erase(OTA_INSTALL_MARKER_OFFSET,
                            OTA_INSTALL_MARKER_SIZE) == CHIP_OK;
}

static bool application_looks_present(void)
{
    uint32_t first_word = UINT32_MAX;
    chip_status_t status = chip_program_flash_read(
        OTA_APPLICATION_ADDRESS, &first_word, sizeof(first_word));

    LOG_DEBUG("boot", "application probe status=%u first_word=0x%08lX",
              (unsigned int)status, (unsigned long)first_word);
    return (status == CHIP_OK) &&
           ((first_word & UINT32_C(0x7F)) == UINT32_C(0x6F));
}

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8U));
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) | ((uint32_t)data[3] << 24U);
}

static bool volume_size_matches_disk(void)
{
    uint32_t sectors;

    if ((disk_read(0U, format_buffer, 0U, 1U) != RES_OK) ||
        (read_u16_le(&format_buffer[11]) != OTA_DISK_SECTOR_SIZE) ||
        (format_buffer[510] != 0x55U) || (format_buffer[511] != 0xAAU)) {
        return false;
    }
    sectors = read_u16_le(&format_buffer[19]);
    if (sectors == 0U) {
        sectors = read_u32_le(&format_buffer[32]);
    }
    return sectors == (OTA_MSC_FLASH_CAPACITY / OTA_DISK_SECTOR_SIZE);
}

static FRESULT mount_update_volume(bool format_if_missing)
{
    FRESULT result = f_mount(&filesystem, "0:", 1U);

    LOG_DEBUG("boot", "update volume mount result=%u format=%u",
              (unsigned int)result, format_if_missing ? 1U : 0U);
    if ((result == FR_OK) && format_if_missing &&
        !volume_size_matches_disk()) {
        LOG_WARN("boot", "FAT volume size changed, reformatting SPI Flash");
        (void)f_unmount("0:");
        result = FR_NO_FILESYSTEM;
    }
    if ((result == FR_NO_FILESYSTEM) && format_if_missing) {
        const MKFS_PARM options = {
            .fmt = FM_FAT | FM_SFD,
            .n_fat = 1U,
            .align = OTA_SPI_FLASH_SECTOR_SIZE / OTA_DISK_SECTOR_SIZE,
            .n_root = 64U,
            .au_size = OTA_SPI_FLASH_SECTOR_SIZE,
        };

        LOG_WARN("boot", "FAT volume missing, formatting SPI Flash");
        result = f_mkfs("0:", &options, format_buffer,
                        sizeof(format_buffer));
        LOG_DEBUG("boot", "FAT format result=%u", (unsigned int)result);
        if (result == FR_OK) {
            result = f_mount(&filesystem, "0:", 1U);
            LOG_DEBUG("boot", "formatted volume remount result=%u",
                      (unsigned int)result);
        }
    }
    return result;
}

static ota_image_status_t validate_update(ota_image_header_t *header)
{
    return ota_image_validate_file(OTA_UPDATE_PATH, OTA_TARGET_ID,
                                   OTA_APPLICATION_ADDRESS,
                                   OTA_APPLICATION_SIZE, header,
                                   io_buffer, sizeof(io_buffer));
}

typedef chip_status_t (*payload_reader_t)(void *context,
                                          uint32_t payload_offset,
                                          void *data, size_t size);

typedef struct {
    FIL *file;
    uint32_t header_size;
} file_reader_context_t;

static chip_status_t read_file_payload(void *reader_context,
                                       uint32_t payload_offset,
                                       void *data, size_t size)
{
    file_reader_context_t *context = reader_context;
    UINT transferred = 0U;

    if ((size > UINT_MAX) ||
        (f_lseek(context->file, context->header_size + payload_offset) !=
         FR_OK) ||
        (f_read(context->file, data, (UINT)size, &transferred) != FR_OK) ||
        (transferred != size)) {
        return CHIP_ERROR_IO;
    }
    return CHIP_OK;
}

static chip_status_t read_staged_payload(void *reader_context,
                                         uint32_t payload_offset,
                                         void *data, size_t size)
{
    return ota_staging_read_payload((ota_staging_t *)reader_context,
                                    payload_offset, data, size);
}

static bool program_update(const ota_image_header_t *header,
                           payload_reader_t read_payload,
                           void *reader_context)
{
    const chip_program_flash_info_t info = chip_program_flash_info();
    uint32_t erase_size = header->image_size;
    uint32_t address = header->load_address;
    uint32_t payload_offset = 0U;
    uint32_t remaining = header->image_size;

    if ((erase_size % info.erase_size) != 0U) {
        erase_size += info.erase_size - (erase_size % info.erase_size);
    }
    LOG_DEBUG("boot", "program geometry erase=%lu write=%lu load=0x%08lX",
              (unsigned long)info.erase_size,
              (unsigned long)info.write_size,
              (unsigned long)header->load_address);
    LOG_INFO("boot", "installing version=%lu size=%lu crc=0x%08lX",
             (unsigned long)header->firmware_version,
             (unsigned long)header->image_size,
             (unsigned long)header->image_crc32);
    if (erase_size > OTA_APPLICATION_SIZE) {
        LOG_ERROR("boot", "erase size exceeds application partition");
        return false;
    }
    if (!mark_install_in_progress()) {
        LOG_ERROR("boot", "failed to write install marker");
        return false;
    }
    LOG_INFO("boot", "erasing application bytes=%lu",
             (unsigned long)erase_size);
    if (chip_program_flash_erase(header->load_address, erase_size) != CHIP_OK) {
        LOG_ERROR("boot", "application erase failed");
        return false;
    }

    while (remaining > 0U) {
        size_t requested = (remaining < sizeof(io_buffer)) ?
                           (size_t)remaining : sizeof(io_buffer);
        size_t write_size = requested;

        if (read_payload(reader_context, payload_offset, io_buffer,
                         requested) != CHIP_OK) {
            LOG_ERROR("boot", "update source read failed address=0x%08lX",
                      (unsigned long)address);
            return false;
        }
        if ((write_size % info.write_size) != 0U) {
            size_t padded = write_size +
                            (info.write_size - (write_size % info.write_size));

            memset(&io_buffer[write_size], 0xFF, padded - write_size);
            write_size = padded;
        }
        if ((chip_program_flash_write(address, io_buffer, write_size) !=
             CHIP_OK) ||
            (chip_program_flash_verify(address, io_buffer, write_size) !=
             CHIP_OK)) {
            LOG_ERROR("boot", "program/verify failed address=0x%08lX",
                      (unsigned long)address);
            return false;
        }
        address += (uint32_t)write_size;
        payload_offset += (uint32_t)requested;
        remaining -= (uint32_t)requested;
        if ((remaining == 0U) ||
            ((payload_offset % OTA_INSTALL_PROGRESS_LOG_INTERVAL) == 0U)) {
            LOG_DEBUG("boot", "install progress=%lu/%lu",
                      (unsigned long)payload_offset,
                      (unsigned long)header->image_size);
        }
    }
    return true;
}

static bool install_file_update(const ota_image_header_t *header)
{
    FIL file;
    file_reader_context_t reader_context = {
        .file = &file,
        .header_size = header->header_size,
    };

    if (f_open(&file, OTA_UPDATE_PATH, FA_READ) != FR_OK) {
        LOG_ERROR("boot", "failed to open %s", OTA_UPDATE_PATH);
        return false;
    }
    if (!program_update(header, read_file_payload, &reader_context)) {
        (void)f_close(&file);
        return false;
    }
    if (f_close(&file) != FR_OK) {
        LOG_ERROR("boot", "failed to close update file");
        return false;
    }
    if (!mark_install_complete()) {
        LOG_ERROR("boot", "failed to clear install marker");
        return false;
    }
    if (f_unlink(OTA_UPDATE_PATH) != FR_OK) {
        LOG_ERROR("boot", "failed to remove installed update file");
        return false;
    }
    LOG_INFO("boot", "installation completed");
    return true;
}

static bool install_webusb_update(const ota_image_header_t *header)
{
    if (!program_update(header, read_staged_payload, &update_staging)) {
        return false;
    }
    if (!mark_install_complete()) {
        LOG_ERROR("boot", "failed to clear install marker");
        return false;
    }
    if (ota_staging_clear(&update_staging) != CHIP_OK) {
        LOG_ERROR("boot", "failed to clear WebUSB staging marker");
        return false;
    }
    LOG_INFO("boot", "WebUSB installation completed");
    return true;
}

static void start_usb(void)
{
    const tusb_rhport_init_t config = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };

    ota_usb_msc_init(&update_disk);
    if (ota_usb_msc_set_identity("Mosaic", "Keyboard Rescue", "1.0") !=
        CHIP_OK) {
        LOG_ERROR("boot", "invalid MSC identity");
        for (;;) {
        }
    }
    ota_webusb_init(&update_staging);
    if (!tusb_init(0U, &config)) {
        LOG_ERROR("boot", "TinyUSB initialization failed");
        for (;;) {
        }
    }
    LOG_DEBUG("boot", "TinyUSB recovery interface initialized");
}

static void recovery_mode(void)
{
    LOG_WARN("boot", "entering USB recovery mode");
    start_usb();
    for (;;) {
        tud_task();
        ota_webusb_task();
        if (ota_webusb_should_reboot(chip_time_millis())) {
            (void)ota_spi_disk_sync(&update_disk);
            LOG_INFO("boot", "WebUSB recovery image ready, resetting");
            (void)keyboard_log_flush();
            chip_system_reset();
        }
        if (!ota_webusb_is_busy() &&
            ota_usb_msc_should_check(chip_time_millis(),
                                     OTA_USB_IDLE_TIMEOUT_MS)) {
            ota_image_header_t header;
            ota_image_status_t image_status;
            chip_status_t sync_status;
            FRESULT recovery_mount = FR_NOT_READY;

            LOG_DEBUG("boot", "recovery write idle, validating disk image");
            tud_disconnect();
            chip_delay_ms(20U);
            sync_status = ota_spi_disk_sync(&update_disk);
            if (sync_status == CHIP_OK) {
                recovery_mount = mount_update_volume(false);
            }
            image_status = ((sync_status == CHIP_OK) &&
                            (recovery_mount == FR_OK)) ?
                           validate_update(&header) : OTA_IMAGE_IO_ERROR;
            LOG_DEBUG("boot", "recovery sync=%u mount=%u image=%u",
                      (unsigned int)sync_status,
                      (unsigned int)recovery_mount,
                      (unsigned int)image_status);
            (void)f_unmount("0:");
            ota_usb_msc_mark_checked();
            if (image_status == OTA_IMAGE_OK) {
                LOG_INFO("boot", "recovery image valid, resetting to install");
                chip_system_reset();
            }
            LOG_WARN("boot", "recovery image not accepted status=%u",
                     (unsigned int)image_status);
            ota_usb_msc_resume();
            tud_connect();
        }
    }
}

int main(void)
{
    ota_image_header_t header;
    ota_image_status_t image_status = OTA_IMAGE_IO_ERROR;
    ota_image_status_t webusb_status;
    spi_flash_jedec_id_t flash_id = {0U, 0U, 0U};
    chip_status_t flash_status;
    FRESULT mount_result;
    bool interrupted;

    if (keyboard_board_init(&external_flash, &update_disk, disk_cache,
                            sizeof(disk_cache)) != CHIP_OK) {
        for (;;) {
        }
    }
    if (keyboard_board_init_staging(&update_staging, &external_flash) !=
        CHIP_OK) {
        for (;;) {
        }
    }
    if (keyboard_log_init() != CHIP_OK) {
        for (;;) {
        }
    }
    LOG_INFO("boot", "version=%s type=%s built=%s app=0x%08lX",
             keyboard_firmware_version, keyboard_firmware_build_type,
             keyboard_firmware_build_time,
             (unsigned long)OTA_APPLICATION_ADDRESS);
    LOG_INFO("boot", "git=%s hash=%s branch=%s",
             keyboard_firmware_git_describe, keyboard_firmware_git_hash,
             keyboard_firmware_git_branch);
    LOG_DEBUG("boot", "flash=%lu disk=%lu staging=%lu cache=%lu",
              (unsigned long)external_flash.config.capacity_bytes,
              (unsigned long)update_disk.size,
              (unsigned long)OTA_WEBUSB_STAGING_SIZE,
              (unsigned long)sizeof(disk_cache));
    flash_status = keyboard_board_probe_flash(&external_flash, &flash_id);
    if (flash_status == CHIP_OK) {
        LOG_INFO("boot", "SPI Flash JEDEC=%02X:%02X:%02X",
                 (unsigned int)flash_id.manufacturer,
                 (unsigned int)flash_id.memory_type,
                 (unsigned int)flash_id.capacity);
    } else {
        LOG_ERROR("boot", "SPI Flash probe failed status=%u ID=%02X:%02X:%02X",
                  (unsigned int)flash_status,
                  (unsigned int)flash_id.manufacturer,
                  (unsigned int)flash_id.memory_type,
                  (unsigned int)flash_id.capacity);
        (void)keyboard_log_flush();
        for (;;) {
        }
    }
    interrupted = install_is_in_progress();
    if (interrupted) {
        LOG_WARN("boot", "previous installation was interrupted");
    }
    webusb_status = ota_staging_validate_committed(
        &update_staging, &header, io_buffer, sizeof(io_buffer));
    LOG_DEBUG("boot", "WebUSB staging validation status=%u",
              (unsigned int)webusb_status);
    if (webusb_status == OTA_IMAGE_OK) {
        LOG_INFO("boot", "committed WebUSB update found");
        if (install_webusb_update(&header)) {
            chip_system_reset();
        }
        LOG_ERROR("boot", "WebUSB installation failed; keeping recovery mode");
        interrupted = true;
    } else if (webusb_status != OTA_IMAGE_NOT_FOUND) {
        LOG_WARN("boot", "WebUSB staging image rejected status=%u",
                 (unsigned int)webusb_status);
        (void)ota_staging_clear(&update_staging);
    }
    mount_result = mount_update_volume(true);
    if (mount_result == FR_OK) {
        image_status = validate_update(&header);
        LOG_DEBUG("boot", "disk image validation status=%u",
                  (unsigned int)image_status);
        if (image_status == OTA_IMAGE_OK) {
            if (install_file_update(&header)) {
                (void)f_unmount("0:");
                chip_system_reset();
            }
            LOG_ERROR("boot", "installation failed; keeping recovery mode");
            interrupted = true;
        }
        (void)f_unmount("0:");
    } else {
        LOG_ERROR("boot", "failed to mount update volume result=%u",
                  (unsigned int)mount_result);
    }
    if (!interrupted && application_looks_present()) {
        LOG_INFO("boot", "no update, starting application");
        (void)keyboard_log_flush();
        chip_system_jump(OTA_APPLICATION_ADDRESS);
    }
    recovery_mode();
}
