#include <spi_flash.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <chip_time.h>

#define FAKE_FLASH_SIZE   512U
#define FAKE_PAGE_SIZE    16U
#define FAKE_SECTOR_SIZE 128U
#define MAX_TRANSACTIONS 64U

typedef struct {
    uint8_t command;
    uint32_t address;
    size_t data_size;
} fake_transaction_t;

static uint8_t fake_memory[FAKE_FLASH_SIZE];
static fake_transaction_t fake_transactions[MAX_TRANSACTIONS];
static size_t fake_transaction_count;
static uint8_t fake_command;
static uint32_t fake_address;
static size_t fake_position;
static size_t fake_data_size;
static bool fake_cs_high;
static bool fake_spi_initialized;
static bool fake_gpio_initialized;
static bool fake_time_initialized;
static bool fake_write_enabled;
static bool fake_busy_forever;
static unsigned int fake_busy_reads;
static uint64_t fake_now_us;
static chip_status_t fake_next_transfer_status;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,          \
                    __LINE__, #condition);                                     \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void fake_reset(void)
{
    memset(fake_memory, 0xFF, sizeof(fake_memory));
    memset(fake_transactions, 0, sizeof(fake_transactions));
    fake_transaction_count = 0U;
    fake_command = 0U;
    fake_address = 0U;
    fake_position = 0U;
    fake_data_size = 0U;
    fake_cs_high = true;
    fake_spi_initialized = false;
    fake_gpio_initialized = false;
    fake_time_initialized = true;
    fake_write_enabled = false;
    fake_busy_forever = false;
    fake_busy_reads = 0U;
    fake_now_us = 0U;
    fake_next_transfer_status = CHIP_OK;
}

static void fake_clear_transactions(void)
{
    memset(fake_transactions, 0, sizeof(fake_transactions));
    fake_transaction_count = 0U;
}

chip_status_t chip_gpio_init(chip_pin_t pin, const chip_gpio_config_t *config)
{
    (void)pin;
    if ((config == NULL) || (config->mode != CHIP_GPIO_OUTPUT_PUSH_PULL) ||
        !config->initial_level) {
        return CHIP_ERROR_INVALID_ARG;
    }
    fake_gpio_initialized = true;
    fake_cs_high = true;
    return CHIP_OK;
}

chip_status_t chip_gpio_deinit(chip_pin_t pin)
{
    (void)pin;
    fake_gpio_initialized = false;
    return CHIP_OK;
}

static void fake_finish_transaction(void)
{
    fake_transaction_t *transaction;

    if ((fake_position == 0U) ||
        (fake_transaction_count >= MAX_TRANSACTIONS)) {
        return;
    }
    transaction = &fake_transactions[fake_transaction_count++];
    transaction->command = fake_command;
    transaction->address = fake_address;
    transaction->data_size = fake_data_size;

    if (fake_command == UINT8_C(0x06)) {
        fake_write_enabled = true;
    } else if ((fake_command == UINT8_C(0x02)) && fake_write_enabled) {
        fake_write_enabled = false;
        fake_busy_reads = 1U;
    } else if ((fake_command == UINT8_C(0x20)) && fake_write_enabled) {
        size_t index;

        for (index = (size_t)fake_address;
             index < ((size_t)fake_address + FAKE_SECTOR_SIZE); ++index) {
            fake_memory[index] = UINT8_C(0xFF);
        }
        fake_write_enabled = false;
        fake_busy_reads = 1U;
    } else if ((fake_command == UINT8_C(0xC7)) && fake_write_enabled) {
        memset(fake_memory, 0xFF, sizeof(fake_memory));
        fake_write_enabled = false;
        fake_busy_reads = 1U;
    }
}

chip_status_t chip_gpio_write(chip_pin_t pin, bool level)
{
    (void)pin;
    if (!fake_gpio_initialized) {
        return CHIP_ERROR_NOT_READY;
    }
    if (!level) {
        fake_command = 0U;
        fake_address = 0U;
        fake_position = 0U;
        fake_data_size = 0U;
    } else if (!fake_cs_high) {
        fake_finish_transaction();
    }
    fake_cs_high = level;
    return CHIP_OK;
}

chip_status_t chip_spi_init(chip_spi_t spi, const chip_spi_config_t *config)
{
    if ((spi != CHIP_SPI_0) || (config == NULL) ||
        (config->clock_hz != UINT32_C(8000000)) ||
        (config->mode != CHIP_SPI_MODE_0) ||
        (config->bit_order != CHIP_SPI_MSB_FIRST)) {
        return CHIP_ERROR_INVALID_ARG;
    }
    fake_spi_initialized = true;
    return CHIP_OK;
}

chip_status_t chip_spi_deinit(chip_spi_t spi)
{
    (void)spi;
    fake_spi_initialized = false;
    return CHIP_OK;
}

static uint8_t fake_response_byte(void)
{
    static const uint8_t jedec_id[] = {UINT8_C(0xEF), UINT8_C(0x40),
                                       UINT8_C(0x13)};

    if ((fake_command == UINT8_C(0x9F)) && (fake_position <= 3U)) {
        return jedec_id[fake_position - 1U];
    }
    if (fake_command == UINT8_C(0x05)) {
        if (fake_busy_forever) {
            return UINT8_C(0x01);
        }
        if (fake_busy_reads > 0U) {
            --fake_busy_reads;
            return UINT8_C(0x01);
        }
        return 0U;
    }
    if ((fake_command == UINT8_C(0x03)) && (fake_position >= 4U)) {
        size_t index = (size_t)fake_address + (fake_position - 4U);

        return fake_memory[index];
    }
    return UINT8_C(0xFF);
}

chip_status_t chip_spi_transfer(chip_spi_t spi, const uint8_t *tx_data,
                                uint8_t *rx_data, size_t size,
                                uint32_t timeout_us)
{
    size_t index;

    (void)spi;
    (void)timeout_us;
    if (!fake_spi_initialized || fake_cs_high) {
        return CHIP_ERROR_NOT_READY;
    }
    if (fake_next_transfer_status != CHIP_OK) {
        chip_status_t status = fake_next_transfer_status;

        fake_next_transfer_status = CHIP_OK;
        return status;
    }

    for (index = 0U; index < size; ++index) {
        uint8_t outgoing = (tx_data == NULL) ? UINT8_C(0xFF) : tx_data[index];

        if (fake_position == 0U) {
            fake_command = outgoing;
        } else if (fake_position <= 3U &&
                   ((fake_command == UINT8_C(0x03)) ||
                    (fake_command == UINT8_C(0x02)) ||
                    (fake_command == UINT8_C(0x20)))) {
            fake_address = (fake_address << 8U) | outgoing;
        } else if ((fake_command == UINT8_C(0x02)) &&
                   (fake_position >= 4U) && (tx_data != NULL) &&
                   fake_write_enabled) {
            size_t memory_index = (size_t)fake_address + fake_data_size;

            fake_memory[memory_index] &= outgoing;
            ++fake_data_size;
        }

        if (rx_data != NULL) {
            rx_data[index] = fake_response_byte();
        }
        ++fake_position;
    }
    return CHIP_OK;
}

bool chip_time_is_initialized(void)
{
    return fake_time_initialized;
}

uint64_t chip_time_micros(void)
{
    fake_now_us += UINT64_C(10);
    return fake_now_us;
}

void chip_delay_us(uint32_t delay_us)
{
    fake_now_us += delay_us;
}

static spi_flash_config_t test_config(void)
{
    spi_flash_config_t config = {
        .spi = CHIP_SPI_0,
        .cs_pin = CHIP_PIN(CHIP_GPIO_PORT_A, 12),
        .clock_hz = UINT32_C(8000000),
        .capacity_bytes = FAKE_FLASH_SIZE,
        .page_size = FAKE_PAGE_SIZE,
        .sector_size = FAKE_SECTOR_SIZE,
        .transfer_timeout_us = UINT32_C(100),
        .program_timeout_us = UINT32_C(1000),
        .sector_erase_timeout_us = UINT32_C(2000),
        .chip_erase_timeout_us = UINT32_C(3000),
    };

    return config;
}

static int test_identify_and_read(spi_flash_t *flash)
{
    spi_flash_jedec_id_t id;
    uint8_t data[4];

    fake_memory[7] = UINT8_C(0x12);
    fake_memory[8] = UINT8_C(0x34);
    fake_memory[9] = UINT8_C(0x56);
    fake_memory[10] = UINT8_C(0x78);

    CHECK(spi_flash_read_jedec_id(flash, &id) == CHIP_OK);
    CHECK(id.manufacturer == UINT8_C(0xEF));
    CHECK(id.memory_type == UINT8_C(0x40));
    CHECK(id.capacity == UINT8_C(0x13));
    CHECK(spi_flash_read(flash, 7U, data, sizeof(data)) == CHIP_OK);
    CHECK(memcmp(data, &fake_memory[7], sizeof(data)) == 0);
    CHECK(spi_flash_read(flash, FAKE_FLASH_SIZE, NULL, 0U) == CHIP_OK);
    CHECK(fake_cs_high);
    return 0;
}

static int test_cross_page_program(spi_flash_t *flash)
{
    uint8_t source[20];
    uint8_t result[20];
    size_t index;

    for (index = 0U; index < sizeof(source); ++index) {
        source[index] = (uint8_t)(index + 1U);
    }
    memset(&fake_memory[10], 0xFF, sizeof(source));
    fake_clear_transactions();
    CHECK(spi_flash_program(flash, 10U, source, sizeof(source)) == CHIP_OK);
    CHECK(memcmp(&fake_memory[10], source, sizeof(source)) == 0);
    CHECK(fake_transaction_count == 9U);
    CHECK(fake_transactions[0].command == UINT8_C(0x05));
    CHECK(fake_transactions[1].command == UINT8_C(0x06));
    CHECK(fake_transactions[2].command == UINT8_C(0x02));
    CHECK(fake_transactions[2].address == 10U);
    CHECK(fake_transactions[2].data_size == 6U);
    CHECK(fake_transactions[5].command == UINT8_C(0x06));
    CHECK(fake_transactions[6].command == UINT8_C(0x02));
    CHECK(fake_transactions[6].address == 16U);
    CHECK(fake_transactions[6].data_size == 14U);
    CHECK(spi_flash_read(flash, 10U, result, sizeof(result)) == CHIP_OK);
    CHECK(memcmp(result, source, sizeof(result)) == 0);
    return 0;
}

static int test_erase(spi_flash_t *flash)
{
    memset(&fake_memory[FAKE_SECTOR_SIZE], 0U, FAKE_SECTOR_SIZE);
    CHECK(spi_flash_erase_sector(flash, FAKE_SECTOR_SIZE) == CHIP_OK);
    CHECK(fake_memory[FAKE_SECTOR_SIZE] == UINT8_C(0xFF));
    CHECK(fake_memory[(2U * FAKE_SECTOR_SIZE) - 1U] == UINT8_C(0xFF));
    CHECK(spi_flash_erase_sector(flash, 1U) == CHIP_ERROR_INVALID_ARG);

    fake_memory[0] = 0U;
    CHECK(spi_flash_erase_chip(flash) == CHIP_OK);
    CHECK(fake_memory[0] == UINT8_C(0xFF));
    return 0;
}

static int test_errors_and_timeout(spi_flash_t *flash)
{
    uint8_t byte;

    CHECK(spi_flash_read(flash, FAKE_FLASH_SIZE, &byte, 1U) ==
          CHIP_ERROR_INVALID_ARG);
    CHECK(spi_flash_read(flash, 0U, &byte, SIZE_MAX) ==
          CHIP_ERROR_INVALID_ARG);
    CHECK(spi_flash_program(flash, 0U, NULL, 1U) ==
          CHIP_ERROR_INVALID_ARG);

    fake_next_transfer_status = CHIP_ERROR_IO;
    CHECK(spi_flash_read_status(flash, &byte) == CHIP_ERROR_IO);
    CHECK(fake_cs_high);

    fake_busy_forever = true;
    fake_now_us = 0U;
    CHECK(spi_flash_wait_ready(flash, 100U) == CHIP_ERROR_TIMEOUT);
    CHECK(fake_cs_high);
    fake_busy_forever = false;

    fake_time_initialized = false;
    fake_busy_forever = true;
    CHECK(spi_flash_wait_ready(flash, CHIP_TIMEOUT_NONE) ==
          CHIP_ERROR_TIMEOUT);
    fake_busy_forever = false;
    CHECK(spi_flash_program(flash, 0U, &byte, 1U) == CHIP_ERROR_NOT_READY);
    fake_time_initialized = true;
    return 0;
}

int main(void)
{
    spi_flash_t flash;
    spi_flash_config_t config = test_config();

    fake_reset();
    CHECK(spi_flash_init(&flash, &config) == CHIP_OK);
    CHECK(flash.initialized);
    CHECK(fake_gpio_initialized);
    CHECK(fake_spi_initialized);
    CHECK(fake_cs_high);

    CHECK(test_identify_and_read(&flash) == 0);
    CHECK(test_cross_page_program(&flash) == 0);
    CHECK(test_erase(&flash) == 0);
    CHECK(test_errors_and_timeout(&flash) == 0);

    CHECK(spi_flash_deinit(&flash) == CHIP_OK);
    CHECK(!flash.initialized);
    CHECK(!fake_gpio_initialized);
    CHECK(!fake_spi_initialized);
    CHECK(spi_flash_read_status(&flash, NULL) == CHIP_ERROR_INVALID_ARG);
    puts("spi_flash_test: all tests passed");
    return 0;
}
