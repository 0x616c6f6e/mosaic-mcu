#include <ota_system_config.h>

#include <assert.h>
#include <string.h>

#include <platform_fatfs.h>

static char stored_json[1024];
static size_t stored_size;
static bool renamed;
static bool destination_exists;

FRESULT f_mount(FATFS *filesystem, const TCHAR *path, BYTE immediate)
{
    (void)filesystem;
    (void)path;
    (void)immediate;
    return FR_OK;
}

FRESULT f_unlink(const TCHAR *path)
{
    if (strcmp(path, OTA_SYSTEM_CONFIG_PATH) == 0) {
        destination_exists = false;
    }
    return FR_OK;
}

FRESULT f_open(FIL *file, const TCHAR *path, BYTE mode)
{
    (void)file;
    assert(strcmp(path, "0:/SYSTEM.TMP") == 0);
    assert(mode == (FA_CREATE_ALWAYS | FA_WRITE));
    return FR_OK;
}

FRESULT f_write(FIL *file, const void *data, UINT size, UINT *written)
{
    (void)file;
    assert(size <= sizeof(stored_json));
    memcpy(stored_json, data, size);
    stored_size = size;
    *written = size;
    return FR_OK;
}

FRESULT f_sync(FIL *file)
{
    (void)file;
    return FR_OK;
}

FRESULT f_close(FIL *file)
{
    (void)file;
    return FR_OK;
}

FRESULT f_rename(const TCHAR *old_path, const TCHAR *new_path)
{
    assert(strcmp(old_path, "0:/SYSTEM.TMP") == 0);
    assert(strcmp(new_path, OTA_SYSTEM_CONFIG_PATH) == 0);
    if (destination_exists) {
        return FR_EXIST;
    }
    renamed = true;
    destination_exists = true;
    return FR_OK;
}

static const char valid_json[] =
    "{"
    "\"schema_version\":2,"
    "\"device_name\":\"Controller 01\","
    "\"usb_disk_visible\":false,"
    "\"log_level\":\"warn\","
    "\"usb_idle_timeout_ms\":3500,"
    "\"webusb_reboot_delay_ms\":750"
    "}";

static void test_defaults(void)
{
    ota_system_config_t config;

    ota_system_config_defaults(&config);
    assert(config.schema_version == 2U);
    assert(strcmp(config.device_name, "Mosaic Keyboard") == 0);
    assert(config.usb_disk_visible);
    assert(config.log_level == PLATFORM_LOG_LEVEL_DEBUG);
    assert(config.usb_idle_timeout_ms == 2000U);
    assert(config.webusb_reboot_delay_ms == 500U);
}

static void test_valid_config(void)
{
    ota_system_config_t config;

    assert(ota_system_config_parse(valid_json, sizeof(valid_json) - 1U,
                                   &config) == OTA_SYSTEM_CONFIG_OK);
    assert(config.schema_version == 2U);
    assert(strcmp(config.device_name, "Controller 01") == 0);
    assert(!config.usb_disk_visible);
    assert(config.log_level == PLATFORM_LOG_LEVEL_WARN);
    assert(config.usb_idle_timeout_ms == 3500U);
    assert(config.webusb_reboot_delay_ms == 750U);
}

static void test_save_json(void)
{
    ota_system_config_t config;

    stored_size = 0U;
    renamed = false;
    destination_exists = true;
    assert(ota_system_config_save_json(valid_json, sizeof(valid_json) - 1U,
                                       &config) == OTA_SYSTEM_CONFIG_OK);
    assert(stored_size == (sizeof(valid_json) - 1U));
    assert(memcmp(stored_json, valid_json, stored_size) == 0);
    assert(renamed);
    assert(strcmp(config.device_name, "Controller 01") == 0);
    assert(!config.usb_disk_visible);
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
    expect_invalid("{\"schema_version\":2}",
                   OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":2,\"schema_version\":2,"
        "\"device_name\":\"x\",\"log_level\":\"info\","
        "\"usb_idle_timeout_ms\":2000,"
        "\"webusb_reboot_delay_ms\":500}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":2,\"device_name\":\"x\","
        "\"log_level\":\"verbose\",\"usb_idle_timeout_ms\":2000,"
        "\"webusb_reboot_delay_ms\":500}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":2,\"device_name\":\"bad/name\","
        "\"log_level\":\"info\",\"usb_idle_timeout_ms\":2000,"
        "\"webusb_reboot_delay_ms\":500}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":2,\"device_name\":\"x\","
        "\"log_level\":\"info\",\"usb_idle_timeout_ms\":499,"
        "\"webusb_reboot_delay_ms\":500}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":2,\"device_name\":\"x\","
        "\"log_level\":\"info\",\"usb_idle_timeout_ms\":2000,"
        "\"webusb_reboot_delay_ms\":4294967296}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":2,\"device_name\":\"x\","
        "\"log_level\":\"info\",\"usb_idle_timeout_ms\":2000,"
        "\"webusb_reboot_delay_ms\":500,\"extra\":1}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":2,\"device_name\":\"x\","
        "\"usb_disk_visible\":\"true\",\"log_level\":\"info\","
        "\"usb_idle_timeout_ms\":2000,"
        "\"webusb_reboot_delay_ms\":500}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid(
        "{\"schema_version\":1,\"device_name\":\"x\","
        "\"usb_disk_visible\":true,\"log_level\":\"info\","
        "\"usb_idle_timeout_ms\":2000,"
        "\"webusb_reboot_delay_ms\":500}",
        OTA_SYSTEM_CONFIG_INVALID_VALUE);
    expect_invalid("{\"schema_version\":2",
                   OTA_SYSTEM_CONFIG_INVALID_JSON);
}

int main(void)
{
    test_defaults();
    test_valid_config();
    test_save_json();
    test_invalid_configs();
    return 0;
}
