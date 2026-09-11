#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <ota_image.h>
#include <ota_webusb.h>

#include "tusb.h"

static uint8_t rx_data[600];
static uint32_t rx_size;
static uint32_t rx_offset;
static uint8_t tx_data[64];
static uint32_t tx_size;
static uint8_t written_config[64];
static uint16_t written_config_size;

uint32_t chip_time_millis(void)
{
    return 0U;
}

uint32_t tud_vendor_n_available(uint8_t index)
{
    (void)index;
    return rx_size - rx_offset;
}

uint32_t tud_vendor_n_read(uint8_t index, void *buffer, uint32_t size)
{
    uint32_t available = rx_size - rx_offset;
    uint32_t count = (size < available) ? size : available;

    (void)index;
    memcpy(buffer, &rx_data[rx_offset], count);
    rx_offset += count;
    return count;
}

void tud_vendor_n_read_flush(uint8_t index)
{
    (void)index;
    rx_offset = rx_size;
}

uint32_t tud_vendor_n_write(uint8_t index, const void *buffer, uint32_t size)
{
    (void)index;
    assert(size <= sizeof(tx_data));
    memcpy(tx_data, buffer, size);
    tx_size = size;
    return size;
}

uint32_t tud_vendor_n_write_flush(uint8_t index)
{
    (void)index;
    return tx_size;
}

chip_status_t spi_flash_erase_sector(spi_flash_t *flash, uint32_t address)
{
    (void)flash;
    (void)address;
    return CHIP_OK;
}

chip_status_t spi_flash_program(spi_flash_t *flash, uint32_t address,
                                const void *data, size_t size)
{
    (void)flash;
    (void)address;
    (void)data;
    (void)size;
    return CHIP_OK;
}

chip_status_t ota_staging_clear(ota_staging_t *staging)
{
    (void)staging;
    return CHIP_OK;
}

chip_status_t ota_staging_commit(ota_staging_t *staging,
                                 uint32_t package_size)
{
    (void)staging;
    (void)package_size;
    return CHIP_OK;
}

ota_image_status_t ota_staging_validate_package(
    ota_staging_t *staging, uint32_t package_size, ota_image_header_t *header,
    uint8_t *scratch, size_t scratch_size)
{
    (void)staging;
    (void)package_size;
    (void)header;
    (void)scratch;
    (void)scratch_size;
    return OTA_IMAGE_OK;
}

static uint8_t read_config(uint8_t *data, uint16_t capacity, uint16_t *size,
                           void *context)
{
    uint16_t index;

    (void)context;
    assert(capacity == OTA_WEBUSB_CONFIG_DATA_SIZE);
    for (index = 0U; index < capacity; ++index) {
        data[index] = (uint8_t)index;
    }
    *size = capacity;
    return OTA_WEBUSB_STATUS_OK;
}

static uint8_t write_config(const uint8_t *data, uint16_t size, void *context)
{
    (void)context;
    assert(size <= sizeof(written_config));
    memcpy(written_config, data, size);
    written_config_size = size;
    return OTA_WEBUSB_STATUS_OK;
}

static void prepare_request(uint8_t command, const void *payload,
                            uint16_t payload_size)
{
    memset(rx_data, 0, sizeof(rx_data));
    memcpy(rx_data, "WOTA", 4U);
    rx_data[4] = command;
    rx_data[6] = (uint8_t)payload_size;
    rx_data[7] = (uint8_t)(payload_size >> 8U);
    if (payload_size > 0U) {
        memcpy(&rx_data[12], payload, payload_size);
    }
    rx_size = 12U + payload_size;
    rx_offset = 0U;
    tx_size = 0U;
}

static void check_response(uint8_t command, uint32_t expected_size)
{
    assert(tx_size == expected_size);
    assert(memcmp(tx_data, "ROTA", 4U) == 0);
    assert(tx_data[4] == command);
    assert(tx_data[5] == OTA_WEBUSB_STATUS_OK);
}

int main(void)
{
    static const uint8_t json[] = "{\"schema_version\":2}";
    ota_staging_t staging = {.initialized = true};
    uint16_t index;

    ota_webusb_init(&staging);
    ota_webusb_set_config_handlers(read_config, write_config, NULL);

    prepare_request(OTA_WEBUSB_COMMAND_GET_CONFIG, NULL, 0U);
    ota_webusb_task();
    check_response(OTA_WEBUSB_COMMAND_GET_CONFIG,
                   16U + OTA_WEBUSB_CONFIG_DATA_SIZE);
    for (index = 0U; index < OTA_WEBUSB_CONFIG_DATA_SIZE; ++index) {
        assert(tx_data[16U + index] == (uint8_t)index);
    }

    prepare_request(OTA_WEBUSB_COMMAND_SET_CONFIG, json, sizeof(json) - 1U);
    ota_webusb_task();
    check_response(OTA_WEBUSB_COMMAND_SET_CONFIG, 16U);
    assert(written_config_size == (sizeof(json) - 1U));
    assert(memcmp(written_config, json, written_config_size) == 0);
    return 0;
}
