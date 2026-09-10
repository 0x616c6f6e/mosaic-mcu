#include "ota_system_config.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <jsmn.h>
#include <platform_fatfs.h>

#define OTA_SYSTEM_CONFIG_TEMP_PATH "0:/SYSTEM.TMP"
#define OTA_SYSTEM_CONFIG_MAX_FILE_SIZE 1023U
#define OTA_SYSTEM_CONFIG_TOKEN_COUNT 16U

#define CONFIG_FIELD_SCHEMA  UINT8_C(0x01)
#define CONFIG_FIELD_NAME    UINT8_C(0x02)
#define CONFIG_FIELD_LOG     UINT8_C(0x04)
#define CONFIG_FIELD_IDLE    UINT8_C(0x08)
#define CONFIG_FIELD_REBOOT  UINT8_C(0x10)
#define CONFIG_FIELDS_ALL    UINT8_C(0x1F)

static const char default_json[] =
    "{\r\n"
    "  \"schema_version\": 1,\r\n"
    "  \"device_name\": \"CH585 OTA\",\r\n"
    "  \"log_level\": \"info\",\r\n"
    "  \"usb_idle_timeout_ms\": 2000,\r\n"
    "  \"webusb_reboot_delay_ms\": 500\r\n"
    "}\r\n";

static FATFS config_filesystem;
static char json_buffer[OTA_SYSTEM_CONFIG_MAX_FILE_SIZE + 1U];

void ota_system_config_defaults(ota_system_config_t *config)
{
    if (config == NULL) {
        return;
    }
    memset(config, 0, sizeof(*config));
    config->schema_version = OTA_SYSTEM_CONFIG_SCHEMA_VERSION;
    memcpy(config->device_name, "CH585 OTA", sizeof("CH585 OTA"));
    config->log_level = PLATFORM_LOG_LEVEL_INFO;
    config->usb_idle_timeout_ms = UINT32_C(2000);
    config->webusb_reboot_delay_ms = UINT32_C(500);
}

static bool token_equals(const char *json, const jsmntok_t *token,
                         const char *expected)
{
    size_t length;

    if ((token->start < 0) || (token->end < token->start)) {
        return false;
    }
    length = (size_t)(token->end - token->start);
    return (strlen(expected) == length) &&
           (memcmp(&json[token->start], expected, length) == 0);
}

static bool parse_u32(const char *json, const jsmntok_t *token,
                      uint32_t *value)
{
    uint32_t result = 0U;
    int position;

    if ((token->type != JSMN_PRIMITIVE) || (token->start < 0) ||
        (token->end <= token->start)) {
        return false;
    }
    for (position = token->start; position < token->end; ++position) {
        uint32_t digit;

        if ((json[position] < '0') || (json[position] > '9')) {
            return false;
        }
        digit = (uint32_t)(json[position] - '0');
        if (result > ((UINT32_MAX - digit) / 10U)) {
            return false;
        }
        result = (result * 10U) + digit;
    }
    *value = result;
    return true;
}

static bool parse_device_name(const char *json, const jsmntok_t *token,
                              char *name)
{
    size_t length;
    size_t index;

    if ((token->type != JSMN_STRING) || (token->start < 0) ||
        (token->end <= token->start)) {
        return false;
    }
    length = (size_t)(token->end - token->start);
    if (length >= OTA_SYSTEM_CONFIG_DEVICE_NAME_SIZE) {
        return false;
    }
    for (index = 0U; index < length; ++index) {
        char character = json[token->start + (int)index];

        if (!(((character >= 'a') && (character <= 'z')) ||
              ((character >= 'A') && (character <= 'Z')) ||
              ((character >= '0') && (character <= '9')) ||
              (character == ' ') || (character == '-') ||
              (character == '_') || (character == '.'))) {
            return false;
        }
    }
    memset(name, 0, OTA_SYSTEM_CONFIG_DEVICE_NAME_SIZE);
    memcpy(name, &json[token->start], length);
    return true;
}

static bool parse_log_level(const char *json, const jsmntok_t *token,
                            platform_log_level_t *level)
{
    if (token->type != JSMN_STRING) {
        return false;
    }
    if (token_equals(json, token, "debug")) {
        *level = PLATFORM_LOG_LEVEL_DEBUG;
    } else if (token_equals(json, token, "info")) {
        *level = PLATFORM_LOG_LEVEL_INFO;
    } else if (token_equals(json, token, "warn")) {
        *level = PLATFORM_LOG_LEVEL_WARN;
    } else if (token_equals(json, token, "error")) {
        *level = PLATFORM_LOG_LEVEL_ERROR;
    } else if (token_equals(json, token, "none")) {
        *level = PLATFORM_LOG_LEVEL_NONE;
    } else {
        return false;
    }
    return true;
}

static ota_system_config_status_t parse_field(
    const char *json, const jsmntok_t *key, const jsmntok_t *value,
    ota_system_config_t *config, uint8_t *fields)
{
    uint32_t number;
    uint8_t field;
    bool valid;

    if (key->type != JSMN_STRING) {
        return OTA_SYSTEM_CONFIG_INVALID_JSON;
    }
    if (token_equals(json, key, "schema_version")) {
        field = CONFIG_FIELD_SCHEMA;
        valid = parse_u32(json, value, &number) &&
                (number == OTA_SYSTEM_CONFIG_SCHEMA_VERSION);
        if (valid) {
            config->schema_version = number;
        }
    } else if (token_equals(json, key, "device_name")) {
        field = CONFIG_FIELD_NAME;
        valid = parse_device_name(json, value, config->device_name);
    } else if (token_equals(json, key, "log_level")) {
        field = CONFIG_FIELD_LOG;
        valid = parse_log_level(json, value, &config->log_level);
    } else if (token_equals(json, key, "usb_idle_timeout_ms")) {
        field = CONFIG_FIELD_IDLE;
        valid = parse_u32(json, value, &number) &&
                (number >= OTA_SYSTEM_CONFIG_MIN_IDLE_MS) &&
                (number <= OTA_SYSTEM_CONFIG_MAX_IDLE_MS);
        if (valid) {
            config->usb_idle_timeout_ms = number;
        }
    } else if (token_equals(json, key, "webusb_reboot_delay_ms")) {
        field = CONFIG_FIELD_REBOOT;
        valid = parse_u32(json, value, &number) &&
                (number >= OTA_SYSTEM_CONFIG_MIN_REBOOT_MS) &&
                (number <= OTA_SYSTEM_CONFIG_MAX_REBOOT_MS);
        if (valid) {
            config->webusb_reboot_delay_ms = number;
        }
    } else {
        return OTA_SYSTEM_CONFIG_INVALID_VALUE;
    }
    if (((*fields & field) != 0U) || !valid) {
        return OTA_SYSTEM_CONFIG_INVALID_VALUE;
    }
    *fields |= field;
    return OTA_SYSTEM_CONFIG_OK;
}

ota_system_config_status_t ota_system_config_parse(
    const char *json, size_t size, ota_system_config_t *config)
{
    jsmn_parser parser;
    jsmntok_t tokens[OTA_SYSTEM_CONFIG_TOKEN_COUNT];
    ota_system_config_t parsed;
    uint8_t fields = 0U;
    int token_count;
    int index;

    if ((json == NULL) || (config == NULL) || (size == 0U)) {
        return OTA_SYSTEM_CONFIG_INVALID_JSON;
    }
    jsmn_init(&parser);
    token_count = jsmn_parse(&parser, json, size, tokens,
                             OTA_SYSTEM_CONFIG_TOKEN_COUNT);
    if ((token_count < 1) || (tokens[0].type != JSMN_OBJECT) ||
        (token_count != (1 + (tokens[0].size * 2)))) {
        return OTA_SYSTEM_CONFIG_INVALID_JSON;
    }
    ota_system_config_defaults(&parsed);
    for (index = 1; index < token_count; index += 2) {
        ota_system_config_status_t status = parse_field(
            json, &tokens[index], &tokens[index + 1], &parsed, &fields);

        if (status != OTA_SYSTEM_CONFIG_OK) {
            return status;
        }
    }
    if (fields != CONFIG_FIELDS_ALL) {
        return OTA_SYSTEM_CONFIG_INVALID_VALUE;
    }
    *config = parsed;
    return OTA_SYSTEM_CONFIG_OK;
}

static ota_system_config_status_t create_default_file(void)
{
    FIL file;
    UINT written = 0U;
    FRESULT result;

    (void)f_unlink(OTA_SYSTEM_CONFIG_TEMP_PATH);
    result = f_open(&file, OTA_SYSTEM_CONFIG_TEMP_PATH,
                    FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK) {
        return OTA_SYSTEM_CONFIG_IO_ERROR;
    }
    result = f_write(&file, default_json, sizeof(default_json) - 1U,
                     &written);
    if ((result == FR_OK) && (written == (sizeof(default_json) - 1U))) {
        result = f_sync(&file);
    }
    if ((f_close(&file) != FR_OK) && (result == FR_OK)) {
        result = FR_DISK_ERR;
    }
    if (result != FR_OK) {
        (void)f_unlink(OTA_SYSTEM_CONFIG_TEMP_PATH);
        return OTA_SYSTEM_CONFIG_IO_ERROR;
    }
    if (f_rename(OTA_SYSTEM_CONFIG_TEMP_PATH,
                 OTA_SYSTEM_CONFIG_PATH) != FR_OK) {
        (void)f_unlink(OTA_SYSTEM_CONFIG_TEMP_PATH);
        return OTA_SYSTEM_CONFIG_IO_ERROR;
    }
    return OTA_SYSTEM_CONFIG_CREATED;
}

ota_system_config_status_t ota_system_config_load(
    ota_system_config_t *config)
{
    ota_system_config_status_t status;
    FIL file;
    UINT transferred = 0U;
    FRESULT result;
    FSIZE_t file_size;

    if (config == NULL) {
        return OTA_SYSTEM_CONFIG_INVALID_VALUE;
    }
    result = f_mount(&config_filesystem, "0:", 1U);
    if (result != FR_OK) {
        return OTA_SYSTEM_CONFIG_MOUNT_ERROR;
    }
    result = f_open(&file, OTA_SYSTEM_CONFIG_PATH, FA_READ);
    if (result == FR_NO_FILE) {
        ota_system_config_defaults(config);
        status = create_default_file();
        (void)f_unmount("0:");
        return status;
    }
    if (result != FR_OK) {
        (void)f_unmount("0:");
        return OTA_SYSTEM_CONFIG_IO_ERROR;
    }
    file_size = f_size(&file);
    if (file_size > OTA_SYSTEM_CONFIG_MAX_FILE_SIZE) {
        (void)f_close(&file);
        (void)f_unmount("0:");
        return OTA_SYSTEM_CONFIG_TOO_LARGE;
    }
    result = f_read(&file, json_buffer, (UINT)file_size, &transferred);
    if ((f_close(&file) != FR_OK) && (result == FR_OK)) {
        result = FR_DISK_ERR;
    }
    if ((result != FR_OK) || (transferred != file_size)) {
        (void)f_unmount("0:");
        return OTA_SYSTEM_CONFIG_IO_ERROR;
    }
    json_buffer[transferred] = '\0';
    status = ota_system_config_parse(json_buffer, transferred, config);
    (void)f_unmount("0:");
    return status;
}

const char *ota_system_config_status_name(ota_system_config_status_t status)
{
    static const char *const names[] = {
        "ok", "created", "mount_error", "io_error", "too_large",
        "invalid_json", "invalid_value",
    };

    if ((unsigned int)status >= (sizeof(names) / sizeof(names[0]))) {
        return "unknown";
    }
    return names[status];
}
