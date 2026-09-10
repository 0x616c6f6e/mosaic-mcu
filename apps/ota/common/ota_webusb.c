#include "ota_webusb.h"

#include <stddef.h>
#include <string.h>

#include <chip_time.h>
#include <ota_image.h>
#include <platform_log.h>

#include "tusb.h"

#include "ota_board.h"
#include "ota_webusb_staging.h"

#define OTA_WEBUSB_REQUEST_SIZE UINT32_C(12)
#define OTA_WEBUSB_RESPONSE_SIZE UINT32_C(16)
#define OTA_WEBUSB_DATA_SIZE UINT16_C(512)
#define OTA_WEBUSB_REBOOT_DELAY_MS UINT32_C(500)

typedef struct {
    spi_flash_t *flash;
    uint8_t request[OTA_WEBUSB_REQUEST_SIZE];
    uint8_t data[OTA_WEBUSB_DATA_SIZE];
    uint8_t validation_buffer[512];
    uint32_t request_used;
    uint32_t data_used;
    uint32_t total_size;
    uint32_t received_size;
    uint32_t erased_size;
    uint32_t reboot_delay_ms;
    uint32_t reboot_deadline;
    uint16_t data_expected;
    bool receiving;
    bool reboot_pending;
} ota_webusb_context_t;

static ota_webusb_context_t context;

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8U));
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) | ((uint32_t)data[3] << 24U);
}

static void write_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
    data[2] = (uint8_t)(value >> 16U);
    data[3] = (uint8_t)(value >> 24U);
}

static void reset_parser(void)
{
    context.request_used = 0U;
    context.data_used = 0U;
    context.data_expected = 0U;
}

static void send_response(uint8_t command, uint8_t status)
{
    uint8_t response[OTA_WEBUSB_RESPONSE_SIZE] = {
        'R', 'O', 'T', 'A', command, status, 0, 0,
    };

    write_u32_le(&response[8], context.received_size);
    write_u32_le(&response[12], context.total_size);
    if (tud_vendor_write(response, sizeof(response)) == sizeof(response)) {
        (void)tud_vendor_write_flush();
    }
}

static uint8_t begin_update(uint32_t package_size, uint16_t payload_size)
{
    if ((payload_size != 0U) || (package_size < OTA_IMAGE_HEADER_SIZE) ||
        (package_size > OTA_WEBUSB_IMAGE_CAPACITY)) {
        return OTA_WEBUSB_STATUS_BAD_SIZE;
    }
    if (ota_webusb_staging_clear(context.flash) != CHIP_OK) {
        return OTA_WEBUSB_STATUS_FLASH_ERROR;
    }
    context.total_size = package_size;
    context.received_size = 0U;
    context.erased_size = 0U;
    context.receiving = true;
    context.reboot_pending = false;
    LOG_INFO("webusb", "receiving package bytes=%lu",
             (unsigned long)package_size);
    return OTA_WEBUSB_STATUS_OK;
}

static uint8_t write_data(uint32_t offset, uint16_t payload_size)
{
    uint32_t end_offset;

    if (!context.receiving) {
        return OTA_WEBUSB_STATUS_BAD_STATE;
    }
    if ((payload_size == 0U) || (offset != context.received_size)) {
        return OTA_WEBUSB_STATUS_BAD_OFFSET;
    }
    end_offset = offset + payload_size;
    if ((end_offset < offset) || (end_offset > context.total_size)) {
        return OTA_WEBUSB_STATUS_BAD_SIZE;
    }
    while (context.erased_size < end_offset) {
        if (spi_flash_erase_sector(
                context.flash,
                OTA_WEBUSB_IMAGE_OFFSET + context.erased_size) != CHIP_OK) {
            context.receiving = false;
            return OTA_WEBUSB_STATUS_FLASH_ERROR;
        }
        context.erased_size += OTA_SPI_FLASH_SECTOR_SIZE;
    }
    if (spi_flash_program(context.flash, OTA_WEBUSB_IMAGE_OFFSET + offset,
                          context.data, payload_size) != CHIP_OK) {
        context.receiving = false;
        return OTA_WEBUSB_STATUS_FLASH_ERROR;
    }
    context.received_size = end_offset;
    return OTA_WEBUSB_STATUS_OK;
}

static uint8_t finish_update(uint16_t payload_size)
{
    ota_image_header_t header;
    ota_image_status_t image_status;

    if (!context.receiving) {
        return OTA_WEBUSB_STATUS_BAD_STATE;
    }
    if ((payload_size != 0U) ||
        (context.received_size != context.total_size)) {
        return OTA_WEBUSB_STATUS_BAD_SIZE;
    }
    image_status = ota_webusb_staging_validate_package(
        context.flash, context.total_size, &header,
        context.validation_buffer, sizeof(context.validation_buffer));
    if (image_status != OTA_IMAGE_OK) {
        context.receiving = false;
        LOG_WARN("webusb", "package rejected image_status=%u",
                 (unsigned int)image_status);
        return OTA_WEBUSB_STATUS_IMAGE_ERROR;
    }
    if (ota_webusb_staging_commit(context.flash, context.total_size) !=
        CHIP_OK) {
        context.receiving = false;
        return OTA_WEBUSB_STATUS_FLASH_ERROR;
    }
    context.receiving = false;
    context.reboot_pending = true;
    context.reboot_deadline = chip_time_millis() + context.reboot_delay_ms;
    LOG_INFO("webusb", "package ready version=%lu size=%lu",
             (unsigned long)header.firmware_version,
             (unsigned long)header.image_size);
    return OTA_WEBUSB_STATUS_OK;
}

static uint8_t abort_update(uint16_t payload_size)
{
    if (payload_size != 0U) {
        return OTA_WEBUSB_STATUS_BAD_SIZE;
    }
    context.receiving = false;
    context.total_size = 0U;
    context.received_size = 0U;
    context.reboot_pending = false;
    return (ota_webusb_staging_clear(context.flash) == CHIP_OK) ?
           OTA_WEBUSB_STATUS_OK : OTA_WEBUSB_STATUS_FLASH_ERROR;
}

static void process_request(void)
{
    const uint8_t command = context.request[4];
    const uint32_t argument = read_u32_le(&context.request[8]);
    uint8_t status;

    if ((context.request[0] != 'W') || (context.request[1] != 'O') ||
        (context.request[2] != 'T') || (context.request[3] != 'A')) {
        send_response(command, OTA_WEBUSB_STATUS_BAD_COMMAND);
        return;
    }
    switch (command) {
    case OTA_WEBUSB_COMMAND_BEGIN:
        status = begin_update(argument, context.data_expected);
        break;
    case OTA_WEBUSB_COMMAND_DATA:
        status = write_data(argument, context.data_expected);
        break;
    case OTA_WEBUSB_COMMAND_END:
        status = finish_update(context.data_expected);
        break;
    case OTA_WEBUSB_COMMAND_ABORT:
        status = abort_update(context.data_expected);
        break;
    case OTA_WEBUSB_COMMAND_STATUS:
        status = (context.data_expected == 0U) ?
                 OTA_WEBUSB_STATUS_OK : OTA_WEBUSB_STATUS_BAD_SIZE;
        break;
    default:
        status = OTA_WEBUSB_STATUS_BAD_COMMAND;
        break;
    }
    send_response(command, status);
}

void ota_webusb_init(spi_flash_t *flash)
{
    memset(&context, 0, sizeof(context));
    context.flash = flash;
    context.reboot_delay_ms = OTA_WEBUSB_REBOOT_DELAY_MS;
}

void ota_webusb_set_reboot_delay(uint32_t delay_ms)
{
    if (delay_ms != 0U) {
        context.reboot_delay_ms = delay_ms;
    }
}

void ota_webusb_task(void)
{
    if (context.flash == NULL) {
        return;
    }
    while (tud_vendor_available() > 0U) {
        if (context.request_used < OTA_WEBUSB_REQUEST_SIZE) {
            uint32_t remaining = OTA_WEBUSB_REQUEST_SIZE -
                                 context.request_used;
            uint32_t count = tud_vendor_read(
                &context.request[context.request_used], remaining);

            context.request_used += count;
            if (context.request_used != OTA_WEBUSB_REQUEST_SIZE) {
                return;
            }
            context.data_expected = read_u16_le(&context.request[6]);
            if (context.data_expected > OTA_WEBUSB_DATA_SIZE) {
                send_response(context.request[4],
                              OTA_WEBUSB_STATUS_BAD_SIZE);
                tud_vendor_read_flush();
                reset_parser();
                return;
            }
            if (context.data_expected == 0U) {
                process_request();
                reset_parser();
                continue;
            }
        }

        if (context.data_used < context.data_expected) {
            uint32_t remaining = (uint32_t)context.data_expected -
                                 context.data_used;
            uint32_t count = tud_vendor_read(&context.data[context.data_used],
                                             remaining);

            context.data_used += count;
            if (context.data_used != context.data_expected) {
                return;
            }
        }
        process_request();
        reset_parser();
    }
}

bool ota_webusb_should_reboot(uint32_t now_ms)
{
    return context.reboot_pending &&
           ((int32_t)(now_ms - context.reboot_deadline) >= 0);
}

bool ota_webusb_is_busy(void)
{
    return context.receiving || context.reboot_pending;
}
