#include <platform_log.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static char captured[512];
static size_t captured_size;
static bool backend_fails;
static bool backend_writes_partially;

static int capture_backend(const char *data, size_t size, void *context)
{
    (void)context;
    if (backend_fails) {
        return -1;
    }
    assert(size <= sizeof(captured));
    memcpy(captured, data, size);
    captured_size = size;
    if (backend_writes_partially) {
        return (int)(size - 1U);
    }
    return (int)size;
}

static uint32_t fixed_timestamp(void *context)
{
    (void)context;
    return UINT32_C(123);
}

int main(void)
{
    platform_log_config_t config = {
        .backend = capture_backend,
        .backend_context = NULL,
        .timestamp = fixed_timestamp,
        .timestamp_context = NULL,
        .level = PLATFORM_LOG_LEVEL_DEBUG,
    };
    char long_message[400];
    int result;

    assert(platform_log_printf(PLATFORM_LOG_LEVEL_INFO, "test", "early") ==
           PLATFORM_LOG_ERROR_NOT_READY);
    assert(platform_log_init(NULL) == PLATFORM_LOG_ERROR_INVALID_ARGUMENT);
    assert(platform_log_init(&config) == 0);

    result = platform_log_printf(PLATFORM_LOG_LEVEL_INFO, "unit",
                                 "value=%02X", 42U);
    assert(result > 0);
    assert(captured_size == strlen("[0000000123] I/unit: value=2A\r\n"));
    assert(memcmp(captured, "[0000000123] I/unit: value=2A\r\n",
                  captured_size) == 0);

    result = platform_log_printf(PLATFORM_LOG_LEVEL_DEBUG, "format",
                                 "%c %s %d %05d %u %x %X %%",
                                 'A', "ok", -12, -12, 12U, 0x2aU, 0x2aU);
    assert(result > 0);
    assert(memcmp(captured,
                  "[0000000123] D/format: A ok -12 -0012 12 2a 2A %\r\n",
                  captured_size) == 0);

    assert(platform_log_set_level(PLATFORM_LOG_LEVEL_WARN) == 0);
    captured_size = 0U;
    assert(platform_log_printf(PLATFORM_LOG_LEVEL_INFO, "unit", "filtered") ==
           PLATFORM_LOG_FILTERED);
    assert(captured_size == 0U);

    result = platform_log_printf(PLATFORM_LOG_LEVEL_WARN, NULL, "warning\n");
    assert(result > 0);
    assert(memcmp(captured, "[0000000123] W: warning\r\n", captured_size) == 0);

    memset(long_message, 'x', sizeof(long_message));
    long_message[sizeof(long_message) - 1U] = '\0';
    result = platform_log_printf(PLATFORM_LOG_LEVEL_ERROR, "unit", "%s",
                                 long_message);
    assert(result == PLATFORM_LOG_ERROR_TRUNCATED);
    assert(captured[captured_size - 2U] == '\r');
    assert(captured[captured_size - 1U] == '\n');

    captured_size = 0U;
    assert(platform_log_printf(PLATFORM_LOG_LEVEL_ERROR, "unit", "%f", 1.0) ==
           PLATFORM_LOG_ERROR_FORMAT);
    assert(captured_size == 0U);

    backend_fails = true;
    assert(platform_log_printf(PLATFORM_LOG_LEVEL_ERROR, "unit", "failure") ==
           PLATFORM_LOG_ERROR_BACKEND);
    backend_fails = false;
    backend_writes_partially = true;
    assert(platform_log_printf(PLATFORM_LOG_LEVEL_ERROR, "unit", "partial") ==
           PLATFORM_LOG_ERROR_BACKEND);
    assert(platform_log_set_level((platform_log_level_t)99) ==
           PLATFORM_LOG_ERROR_INVALID_ARGUMENT);
    return 0;
}
