#ifndef PLATFORM_LOG_H
#define PLATFORM_LOG_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLATFORM_LOG_LEVEL_DEBUG_VALUE 0
#define PLATFORM_LOG_LEVEL_INFO_VALUE  1
#define PLATFORM_LOG_LEVEL_WARN_VALUE  2
#define PLATFORM_LOG_LEVEL_ERROR_VALUE 3
#define PLATFORM_LOG_LEVEL_NONE_VALUE  4

#ifndef PLATFORM_LOG_COMPILE_LEVEL
#define PLATFORM_LOG_COMPILE_LEVEL PLATFORM_LOG_LEVEL_DEBUG_VALUE
#endif

typedef enum {
    PLATFORM_LOG_LEVEL_DEBUG = PLATFORM_LOG_LEVEL_DEBUG_VALUE,
    PLATFORM_LOG_LEVEL_INFO = PLATFORM_LOG_LEVEL_INFO_VALUE,
    PLATFORM_LOG_LEVEL_WARN = PLATFORM_LOG_LEVEL_WARN_VALUE,
    PLATFORM_LOG_LEVEL_ERROR = PLATFORM_LOG_LEVEL_ERROR_VALUE,
    PLATFORM_LOG_LEVEL_NONE = PLATFORM_LOG_LEVEL_NONE_VALUE,
} platform_log_level_t;

typedef int (*platform_log_backend_t)(const char *data, size_t size,
                                      void *context);
typedef uint32_t (*platform_log_timestamp_t)(void *context);

typedef struct {
    platform_log_backend_t backend;
    void *backend_context;
    platform_log_timestamp_t timestamp;
    void *timestamp_context;
    platform_log_level_t level;
} platform_log_config_t;

enum {
    PLATFORM_LOG_FILTERED = 0,
    PLATFORM_LOG_ERROR_INVALID_ARGUMENT = -1,
    PLATFORM_LOG_ERROR_NOT_READY = -2,
    PLATFORM_LOG_ERROR_FORMAT = -3,
    PLATFORM_LOG_ERROR_BACKEND = -4,
    PLATFORM_LOG_ERROR_TRUNCATED = -5,
};

int platform_log_init(const platform_log_config_t *config);
int platform_log_set_level(platform_log_level_t level);
platform_log_level_t platform_log_get_level(void);
int platform_log_vprintf(platform_log_level_t level, const char *tag,
                         const char *format, va_list arguments);
int platform_log_printf(platform_log_level_t level, const char *tag,
                        const char *format, ...);

#if PLATFORM_LOG_COMPILE_LEVEL <= PLATFORM_LOG_LEVEL_DEBUG_VALUE
#define LOG_DEBUG(tag, ...) \
    ((void)platform_log_printf(PLATFORM_LOG_LEVEL_DEBUG, (tag), __VA_ARGS__))
#else
#define LOG_DEBUG(tag, ...) ((void)0)
#endif

#if PLATFORM_LOG_COMPILE_LEVEL <= PLATFORM_LOG_LEVEL_INFO_VALUE
#define LOG_INFO(tag, ...) \
    ((void)platform_log_printf(PLATFORM_LOG_LEVEL_INFO, (tag), __VA_ARGS__))
#else
#define LOG_INFO(tag, ...) ((void)0)
#endif

#if PLATFORM_LOG_COMPILE_LEVEL <= PLATFORM_LOG_LEVEL_WARN_VALUE
#define LOG_WARN(tag, ...) \
    ((void)platform_log_printf(PLATFORM_LOG_LEVEL_WARN, (tag), __VA_ARGS__))
#else
#define LOG_WARN(tag, ...) ((void)0)
#endif

#if PLATFORM_LOG_COMPILE_LEVEL <= PLATFORM_LOG_LEVEL_ERROR_VALUE
#define LOG_ERROR(tag, ...) \
    ((void)platform_log_printf(PLATFORM_LOG_LEVEL_ERROR, (tag), __VA_ARGS__))
#else
#define LOG_ERROR(tag, ...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif
