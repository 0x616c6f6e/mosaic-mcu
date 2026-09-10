#include <ota_system_config.h>

#include <assert.h>
#include <string.h>

static const char valid_json[] =
    "{"
    "\"schema_version\":1,"
    "\"device_name\":\"Controller 01\","
    "\"log_level\":\"warn\","
    "\"usb_idle_timeout_ms\":3500,"
    "\"webusb_reboot_delay_ms\":750"
    "}";

static void test_defaults(void)
{
    ota_system_config_t config;

    ota_system_config_defaults(&config);
    assert(config.schema_version == 1U);
    assert(strcmp(config.device_name, "CH585 OTA") == 0);
    assert(config.log_level == PLATFORM_LOG_LEVEL_INFO);
    assert(config.usb_idle_timeout_ms == 2000U);
    assert(config.webusb_reboot_delay_ms == 500U);
}

static void test_valid_config(void)
{
    ota_system_config_t config;

    assert(ota_system_config_parse(valid_json, sizeof(valid_json) - 1U,
                                   &config) == OTA_SYSTEM_CONFIG_OK);
    assert(config.schema_version == 1U);
    assert(strcmp(config.device_name, "Controller 01") == 0);
    assert(config.log_level == PLATFORM_LOG_LEVEL_WARN);
    assert(config.usb_idle_timeout_ms == 3500U);
    assert(config.webusb_reboot_delay_ms == 750U);
}

static void expect_invalid(const char *json,
                           ota_system_config_status_t expected)
{
    ota_system_config_t config;
    ota_system_config_t before;

    memset(&config, 0xA5, sizeof(config));
    before = config;
    assert(ota_system_config_parse(json, strlen(json), &config) == expected);
    assert(memcmp(&config, &before, sizeof(config)) == 0);
}

static void test_invalid_configs(void)
{
    expect_invalid("{\"schema_version\":1}",
                   OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":1,\"schema_version\":1,"
        "\"device_name\":\"x\",\"log_level\":\"info\","
        "\"usb_idle_timeout_ms\":2000,"
        "\"webusb_reboot_delay_ms\":500}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":1,\"device_name\":\"x\","
        "\"log_level\":\"verbose\",\"usb_idle_timeout_ms\":2000,"
        "\"webusb_reboot_delay_ms\":500}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":1,\"device_name\":\"bad/name\","
        "\"log_level\":\"info\",\"usb_idle_timeout_ms\":2000,"
        "\"webusb_reboot_delay_ms\":500}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":1,\"device_name\":\"x\","
        "\"log_level\":\"info\",\"usb_idle_timeout_ms\":499,"
        "\"webusb_reboot_delay_ms\":500}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":1,\"device_name\":\"x\","
        "\"log_level\":\"info\",\"usb_idle_timeout_ms\":2000,"
        "\"webusb_reboot_delay_ms\":4294967296}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":1,\"device_name\":\"x\","
        "\"log_level\":\"info\",\"usb_idle_timeout_ms\":2000,"
        "\"webusb_reboot_delay_ms\":500,\"extra\":1}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid("{\"schema_version\":1",
                   OTA_SYSTEM_CONFIG_INVALID_JSON);
}

int main(void)
{
    test_defaults();
    test_valid_config();
    test_invalid_configs();
    return 0;
}
