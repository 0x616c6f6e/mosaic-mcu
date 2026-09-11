#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <chip_time.h>
#include <ota_spi_disk.h>
#include <platform_fatfs.h>
#include <platform_log.h>
#include <platform_lua.h>
#include <spi_flash.h>

#include "tusb.h"

#include "demo_support.h"
#include "usb_msc.h"

#define LUA_MEMORY_LIMIT       (48U * 1024U)
#define LUA_SCRIPT_MAX_SIZE    (8U * 1024U)
#define LUA_EXECUTION_LIMIT_MS UINT32_C(5000)
#define USB_WRITE_IDLE_MS      UINT32_C(2000)
#define FATFS_ERASE_BLOCK_SECTORS \
    (DEMO_SPI_FLASH_SECTOR_SIZE / OTA_DISK_SECTOR_SIZE)
#define LUA_SCRIPT_PATH "0:/MAIN.LUA"

_Static_assert((DEMO_SPI_FLASH_SECTOR_SIZE % OTA_DISK_SECTOR_SIZE) == 0U,
               "SPI Flash erase size must be a multiple of 512 bytes");
_Static_assert((DEMO_FATFS_OFFSET_BYTES % DEMO_SPI_FLASH_SECTOR_SIZE) == 0U,
               "FatFs offset must be erase-block aligned");
_Static_assert((DEMO_FATFS_SIZE_BYTES % DEMO_SPI_FLASH_SECTOR_SIZE) == 0U,
               "FatFs size must be erase-block aligned");
_Static_assert(DEMO_FATFS_SIZE_BYTES <= DEMO_SPI_FLASH_CAPACITY_BYTES,
               "FatFs partition is larger than the flash");
_Static_assert(DEMO_FATFS_OFFSET_BYTES <=
               (DEMO_SPI_FLASH_CAPACITY_BYTES - DEMO_FATFS_SIZE_BYTES),
               "FatFs partition exceeds flash capacity");

typedef struct {
    char data[160];
    size_t size;
    bool truncated;
} lua_output_buffer_t;

static const char default_script[] =
    "print('running MAIN.LUA', _VERSION)\n"
    "for i = 1, 6 do\n"
    "  mcu.led(i % 2 == 1)\n"
    "  print('step', i, 'millis', mcu.millis())\n"
    "  mcu.delay(150)\n"
    "end\n"
    "mcu.led(true)\n";

static spi_flash_t external_flash;
static ota_spi_disk_t script_disk;
static uint8_t disk_cache[DEMO_SPI_FLASH_SECTOR_SIZE];
static uint8_t format_buffer[OTA_DISK_SECTOR_SIZE];
static uint8_t script_buffer[LUA_SCRIPT_MAX_SIZE];
static FATFS filesystem;
static FIL script_file;
static lua_output_buffer_t output_buffer;
static uint32_t lua_deadline_ms;
static uint32_t last_script_hash;
static bool script_hash_valid;

uint32_t tusb_time_millis_api(void)
{
    return chip_time_millis();
}

static void require_status(chip_status_t status, const char *operation)
{
    if (status != CHIP_OK) {
        LOG_ERROR("lua-usb", "%s failed, status=%u", operation,
                  (unsigned int)status);
        demo_halt();
    }
}

static void require_fatfs(FRESULT result, const char *operation)
{
    if (result != FR_OK) {
        LOG_ERROR("lua-usb", "%s failed, error=%u", operation,
                  (unsigned int)result);
        demo_halt();
    }
}

static void flush_lua_output(void)
{
    if ((output_buffer.size == 0U) && !output_buffer.truncated) {
        return;
    }
    output_buffer.data[output_buffer.size] = '\0';
    LOG_INFO("lua", "%s%s", output_buffer.data,
             output_buffer.truncated ? "..." : "");
    output_buffer.size = 0U;
    output_buffer.truncated = false;
}

static void lua_log_output(const char *data, size_t size, void *context)
{
    size_t index;

    (void)context;
    for (index = 0U; index < size; ++index) {
        if (data[index] == '\n') {
            flush_lua_output();
        } else if (output_buffer.size < (sizeof(output_buffer.data) - 1U)) {
            output_buffer.data[output_buffer.size++] = data[index];
        } else {
            output_buffer.truncated = true;
        }
    }
}

static int lua_mcu_led(lua_State *state)
{
    if (demo_led_write(lua_toboolean(state, 1) != 0) != CHIP_OK) {
        return luaL_error(state, "LED write failed");
    }
    return 0;
}

static int lua_mcu_delay(lua_State *state)
{
    lua_Integer delay_ms = luaL_checkinteger(state, 1);

    if ((delay_ms < 0) || (delay_ms > 1000)) {
        return luaL_error(state, "delay must be between 0 and 1000 ms");
    }
    chip_delay_ms((uint32_t)delay_ms);
    return 0;
}

static int lua_mcu_millis(lua_State *state)
{
    lua_pushinteger(state, (lua_Integer)chip_time_millis());
    return 1;
}

static void register_mcu_module(lua_State *state)
{
    static const luaL_Reg functions[] = {
        {"led", lua_mcu_led},
        {"delay", lua_mcu_delay},
        {"millis", lua_mcu_millis},
        {NULL, NULL},
    };

    luaL_newlib(state, functions);
    lua_setglobal(state, "mcu");
}

static void lua_timeout_hook(lua_State *state, lua_Debug *debug)
{
    (void)debug;
    if ((int32_t)(chip_time_millis() - lua_deadline_ms) >= 0) {
        (void)luaL_error(state, "execution time limit exceeded");
    }
}

static void create_default_script(void)
{
    UINT written;

    require_fatfs(f_open(&script_file, LUA_SCRIPT_PATH,
                         FA_WRITE | FA_CREATE_ALWAYS),
                  "create MAIN.LUA");
    require_fatfs(f_write(&script_file, default_script,
                          sizeof(default_script) - 1U, &written),
                  "write MAIN.LUA");
    if (written != (sizeof(default_script) - 1U)) {
        LOG_ERROR("lua-usb", "short MAIN.LUA write=%u", written);
        demo_halt();
    }
    require_fatfs(f_close(&script_file), "close MAIN.LUA");
    LOG_INFO("lua-usb", "created default MAIN.LUA");
}

static void prepare_volume(void)
{
    const MKFS_PARM format_options = {
        .fmt = FM_FAT | FM_SFD,
        .n_fat = 1U,
        .align = FATFS_ERASE_BLOCK_SECTORS,
        .n_root = 64U,
        .au_size = DEMO_SPI_FLASH_SECTOR_SIZE,
    };
    FILINFO info;
    FRESULT result = f_mount(&filesystem, "0:", 1U);

    if (result == FR_NO_FILESYSTEM) {
        LOG_WARN("lua-usb", "no filesystem, formatting disk");
        require_fatfs(f_mkfs("0:", &format_options, format_buffer,
                             sizeof(format_buffer)),
                      "format disk");
        require_fatfs(f_mount(&filesystem, "0:", 1U), "mount formatted disk");
        create_default_script();
    } else {
        require_fatfs(result, "mount disk");
        result = f_stat(LUA_SCRIPT_PATH, &info);
        if (result == FR_NO_FILE) {
            create_default_script();
        } else {
            require_fatfs(result, "stat MAIN.LUA");
        }
    }
    require_fatfs(f_unmount("0:"), "unmount disk");
    require_status(ota_spi_disk_sync(&script_disk), "sync disk");
}

static uint32_t hash_script(const uint8_t *data, size_t size)
{
    uint32_t hash = UINT32_C(2166136261);
    size_t index;

    for (index = 0U; index < size; ++index) {
        hash ^= data[index];
        hash *= UINT32_C(16777619);
    }
    return hash;
}

static void execute_script(bool force)
{
    platform_lua_allocator_t allocator;
    lua_State *state = NULL;
    FSIZE_t file_size;
    UINT bytes_read;
    FRESULT result;
    int lua_status;
    uint32_t current_hash;

    result = f_mount(&filesystem, "0:", 1U);
    if (result != FR_OK) {
        LOG_ERROR("lua-usb", "script mount failed, error=%u",
                  (unsigned int)result);
        return;
    }
    result = f_open(&script_file, LUA_SCRIPT_PATH, FA_READ);
    if (result != FR_OK) {
        LOG_WARN("lua-usb", "MAIN.LUA unavailable, error=%u",
                 (unsigned int)result);
        (void)f_unmount("0:");
        return;
    }
    file_size = f_size(&script_file);
    if (file_size > sizeof(script_buffer)) {
        LOG_ERROR("lua-usb", "MAIN.LUA too large, bytes=%lu limit=%u",
                  (unsigned long)file_size, (unsigned int)sizeof(script_buffer));
        (void)f_close(&script_file);
        (void)f_unmount("0:");
        return;
    }
    result = f_read(&script_file, script_buffer, (UINT)file_size, &bytes_read);
    (void)f_close(&script_file);
    (void)f_unmount("0:");
    if ((result != FR_OK) || (bytes_read != (UINT)file_size)) {
        LOG_ERROR("lua-usb", "MAIN.LUA read failed, error=%u bytes=%u",
                  (unsigned int)result, bytes_read);
        return;
    }
    current_hash = hash_script(script_buffer, bytes_read);
    if (!force && script_hash_valid && (current_hash == last_script_hash)) {
        LOG_INFO("lua-usb", "MAIN.LUA unchanged, skipping execution");
        return;
    }
    last_script_hash = current_hash;
    script_hash_valid = true;

    platform_lua_allocator_init(&allocator, LUA_MEMORY_LIMIT);
    state = platform_lua_newstate(&allocator, chip_time_millis());
    if (state == NULL) {
        LOG_ERROR("lua-usb", "Lua state allocation failed");
        return;
    }
    platform_lua_openlibs(state);
    register_mcu_module(state);
    lua_deadline_ms = chip_time_millis() + LUA_EXECUTION_LIMIT_MS;
    lua_sethook(state, lua_timeout_hook, LUA_MASKCOUNT, 1000);
    lua_status = luaL_loadbufferx(state, (const char *)script_buffer,
                                  bytes_read, LUA_SCRIPT_PATH, "t");
    if (lua_status == LUA_OK) {
        lua_status = lua_pcall(state, 0, 0, 0);
    }
    if (lua_status != LUA_OK) {
        const char *message = lua_tostring(state, -1);

        LOG_ERROR("lua-usb", "MAIN.LUA failed: %s",
                  (message != NULL) ? message : "unknown error");
    } else {
        LOG_INFO("lua-usb", "MAIN.LUA completed, peak=%lu bytes",
                 (unsigned long)platform_lua_memory_peak(&allocator));
    }
    lua_close(state);
    flush_lua_output();
}

int main(void)
{
    const spi_flash_config_t flash_config = {
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
    const tusb_rhport_init_t usb_config = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };
    spi_flash_jedec_id_t flash_id;

    DEMO_REQUIRE(demo_platform_init());
    DEMO_REQUIRE(demo_led_init());
    DEMO_REQUIRE(demo_log_init());
    require_status(demo_spi_flash_pins_init(), "SPI pin init");
    require_status(spi_flash_init(&external_flash, &flash_config),
                   "SPI Flash init");
    require_status(spi_flash_read_jedec_id(&external_flash, &flash_id),
                   "JEDEC ID read");
    LOG_INFO("lua-usb", "SPI Flash JEDEC=%02X:%02X:%02X disk=%lu bytes",
             (unsigned int)flash_id.manufacturer,
             (unsigned int)flash_id.memory_type,
             (unsigned int)flash_id.capacity,
             (unsigned long)DEMO_FATFS_SIZE_BYTES);
    if ((flash_id.capacity >= 32U) ||
        ((UINT32_C(1) << flash_id.capacity) !=
         DEMO_SPI_FLASH_CAPACITY_BYTES)) {
        LOG_ERROR("lua-usb", "Flash capacity mismatch, code=0x%02X",
                  (unsigned int)flash_id.capacity);
        demo_halt();
    }
    require_status(ota_spi_disk_init(&script_disk, &external_flash,
                                     DEMO_FATFS_OFFSET_BYTES,
                                     DEMO_FATFS_SIZE_BYTES, disk_cache,
                                     sizeof(disk_cache)),
                   "disk init");
    require_status(ota_spi_disk_bind(&script_disk), "disk bind");
    platform_lua_set_output(lua_log_output, NULL);
    prepare_volume();
    execute_script(true);
    lua_usb_msc_init(&script_disk);
    if (!tusb_init(0U, &usb_config)) {
        LOG_ERROR("lua-usb", "TinyUSB initialization failed");
        demo_halt();
    }
    LOG_INFO("lua-usb", "USB disk ready; edit and eject MAIN.LUA");

    for (;;) {
        tud_task();
        if (lua_usb_msc_script_pending(chip_time_millis(),
                                       USB_WRITE_IDLE_MS)) {
            LOG_INFO("lua-usb", "host writes complete, loading MAIN.LUA");
            tud_disconnect();
            chip_delay_ms(20U);
            if (ota_spi_disk_sync(&script_disk) == CHIP_OK) {
                execute_script(false);
            } else {
                LOG_ERROR("lua-usb", "disk synchronization failed");
            }
            lua_usb_msc_mark_processed();
            lua_usb_msc_resume();
            tud_connect();
        }
    }
}
