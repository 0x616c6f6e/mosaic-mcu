#include <stdint.h>

#include <chip_i2c.h>
#include <chip_time.h>
#include <platform_log.h>

#include "demo_support.h"

int main(void)
{
    chip_i2c_config_t config = {
        .clock_hz = UINT32_C(100000),
    };

    DEMO_REQUIRE(demo_platform_init());
    DEMO_REQUIRE(demo_log_init());
    DEMO_REQUIRE(demo_i2c_pins_init());
    DEMO_REQUIRE(chip_i2c_init(DEMO_I2C_INSTANCE, &config));
    LOG_INFO("i2c", "CH585 scan started");

    for (;;) {
        uint8_t address;
        uint32_t found = 0;

        for (address = UINT8_C(0x08); address <= UINT8_C(0x77); ++address) {
            if (chip_i2c_probe(DEMO_I2C_INSTANCE, address, UINT32_C(2000)) == CHIP_OK) {
                LOG_INFO("i2c", "device=0x%02X", (unsigned int)address);
                ++found;
            }
        }
        LOG_INFO("i2c", "found=%lu", (unsigned long)found);
        chip_delay_ms(2000);
    }
}
