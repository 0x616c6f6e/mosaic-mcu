#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <chip_time.h>
#include <platform_log.h>
#include <spi_flash.h>

#include "demo_support.h"

#define SPI_FLASH_TEST_DATA_SIZE 64U

#if DEMO_SPI_FLASH_ENABLE_WRITE_TEST
_Static_assert((DEMO_SPI_FLASH_TEST_ADDRESS % DEMO_SPI_FLASH_SECTOR_SIZE) == 0U,
               "SPI Flash test address must be sector aligned");
_Static_assert(DEMO_SPI_FLASH_TEST_ADDRESS <=
               (DEMO_SPI_FLASH_CAPACITY_BYTES - DEMO_SPI_FLASH_SECTOR_SIZE),
               "SPI Flash test sector must fit in the configured capacity");
#endif

static void require_status(chip_status_t status, const char *operation)
{
    if (status != CHIP_OK) {
        LOG_ERROR("spi-flash", "%s failed, status=%u", operation,
                  (unsigned int)status);
        demo_halt();
    }
}

#if DEMO_SPI_FLASH_ENABLE_WRITE_TEST
static void run_write_test(spi_flash_t *flash)
{
    uint8_t expected[SPI_FLASH_TEST_DATA_SIZE];
    uint8_t actual[SPI_FLASH_TEST_DATA_SIZE];
    size_t index;

    for (index = 0U; index < sizeof(expected); ++index) {
        expected[index] = (uint8_t)(UINT8_C(0xA5) ^ (uint8_t)index);
    }

    LOG_WARN("spi-flash", "destructive test sector=0x%06lX",
             (unsigned long)DEMO_SPI_FLASH_TEST_ADDRESS);
    require_status(spi_flash_erase_sector(flash,
                                          DEMO_SPI_FLASH_TEST_ADDRESS),
                   "sector erase");
    require_status(spi_flash_read(flash, DEMO_SPI_FLASH_TEST_ADDRESS,
                                  actual, sizeof(actual)),
                   "erased data read");
    for (index = 0U; index < sizeof(actual); ++index) {
        if (actual[index] != UINT8_C(0xFF)) {
            LOG_ERROR("spi-flash", "erase verify failed, offset=%u value=0x%02X",
                      (unsigned int)index, (unsigned int)actual[index]);
            demo_halt();
        }
    }

    require_status(spi_flash_program(flash, DEMO_SPI_FLASH_TEST_ADDRESS,
                                     expected, sizeof(expected)),
                   "page program");
    memset(actual, 0, sizeof(actual));
    require_status(spi_flash_read(flash, DEMO_SPI_FLASH_TEST_ADDRESS,
                                  actual, sizeof(actual)),
                   "programmed data read");
    if (memcmp(actual, expected, sizeof(expected)) != 0) {
        LOG_ERROR("spi-flash", "program verify failed");
        demo_halt();
    }

    require_status(spi_flash_erase_sector(flash,
                                          DEMO_SPI_FLASH_TEST_ADDRESS),
                   "cleanup erase");
    LOG_INFO("spi-flash", "erase/program/read verification passed");
}
#endif

int main(void)
{
    const spi_flash_config_t config = {
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
    spi_flash_jedec_id_t jedec_id;
    spi_flash_t flash;

    DEMO_REQUIRE(demo_platform_init());
    DEMO_REQUIRE(demo_led_init());
    DEMO_REQUIRE(demo_log_init());
    require_status(demo_spi_flash_pins_init(), "pin init");
    require_status(spi_flash_init(&flash, &config), "flash init");
    require_status(spi_flash_read_jedec_id(&flash, &jedec_id),
                   "JEDEC ID read");

    LOG_INFO("spi-flash", "JEDEC ID=%02X %02X %02X",
             (unsigned int)jedec_id.manufacturer,
             (unsigned int)jedec_id.memory_type,
             (unsigned int)jedec_id.capacity);
    if (((jedec_id.manufacturer == 0U) && (jedec_id.memory_type == 0U) &&
         (jedec_id.capacity == 0U)) ||
        ((jedec_id.manufacturer == UINT8_C(0xFF)) &&
         (jedec_id.memory_type == UINT8_C(0xFF)) &&
         (jedec_id.capacity == UINT8_C(0xFF)))) {
        LOG_ERROR("spi-flash", "no valid JEDEC device response");
        demo_halt();
    }

#if DEMO_SPI_FLASH_ENABLE_WRITE_TEST
    if ((jedec_id.capacity >= 32U) ||
        ((UINT32_C(1) << jedec_id.capacity) !=
         DEMO_SPI_FLASH_CAPACITY_BYTES)) {
        LOG_ERROR("spi-flash", "capacity mismatch, code=0x%02X configured=%lu",
                  (unsigned int)jedec_id.capacity,
                  (unsigned long)DEMO_SPI_FLASH_CAPACITY_BYTES);
        demo_halt();
    }
    run_write_test(&flash);
#else
    LOG_INFO("spi-flash", "write test disabled");
#endif

    require_status(demo_led_write(true), "success LED");
    for (;;) {
        chip_delay_ms(1000U);
    }
}
