#ifndef PLATFORM_LOG_H
#define PLATFORM_LOG_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Compile-time numeric value for debug logs. */
#define PLATFORM_LOG_LEVEL_DEBUG_VALUE 0
/** @brief Compile-time numeric value for informational logs. */
#define PLATFORM_LOG_LEVEL_INFO_VALUE  1
/** @brief Compile-time numeric value for warning logs. */
#define PLATFORM_LOG_LEVEL_WARN_VALUE  2
/** @brief Compile-time numeric value for error logs. */
#define PLATFORM_LOG_LEVEL_ERROR_VALUE 3
/** @brief Compile-time numeric value that disables logging. */
#define PLATFORM_LOG_LEVEL_NONE_VALUE  4

#ifndef PLATFORM_LOG_COMPILE_LEVEL
/** @brief Lowest severity retained in the binary at compile time. */
#define PLATFORM_LOG_COMPILE_LEVEL PLATFORM_LOG_LEVEL_DEBUG_VALUE
#endif

/** @brief Severity used by runtime and compile-time log filters. */
typedef enum {
    PLATFORM_LOG_LEVEL_DEBUG = PLATFORM_LOG_LEVEL_DEBUG_VALUE, /**< Debug. */
    PLATFORM_LOG_LEVEL_INFO = PLATFORM_LOG_LEVEL_INFO_VALUE,   /**< Info. */
    PLATFORM_LOG_LEVEL_WARN = PLATFORM_LOG_LEVEL_WARN_VALUE,   /**< Warning. */
    PLATFORM_LOG_LEVEL_ERROR = PLATFORM_LOG_LEVEL_ERROR_VALUE, /**< Error. */
    PLATFORM_LOG_LEVEL_NONE = PLATFORM_LOG_LEVEL_NONE_VALUE,   /**< Disabled. */
} platform_log_level_t;

/** @brief Write one formatted log message; return bytes written or an error. */
typedef int (*platform_log_backend_t)(const char *data, size_t size,
                                      void *context);
/** @brief Supply an optional monotonic log timestamp. */
typedef uint32_t (*platform_log_timestamp_t)(void *context);

/** @brief Logging backend, optional timestamp provider, and runtime level. */
typedef struct {
    platform_log_backend_t backend; /**< Required message sink. */
    void *backend_context;          /**< Opaque backend argument. */
    platform_log_timestamp_t timestamp; /**< Optional time source. */
    void *timestamp_context;        /**< Opaque timestamp argument. */
    platform_log_level_t level;     /**< Initial runtime severity threshold. */
} platform_log_config_t;

/** @brief Nonpositive logging results and failure codes. */
enum {
    PLATFORM_LOG_FILTERED = 0, /**< Suppressed by runtime severity filter. */
    PLATFORM_LOG_ERROR_INVALID_ARGUMENT = -1, /**< Invalid API argument. */
    PLATFORM_LOG_ERROR_NOT_READY = -2, /**< Logger not initialized. */
    PLATFORM_LOG_ERROR_FORMAT = -3, /**< Formatting failed. */
    PLATFORM_LOG_ERROR_BACKEND = -4, /**< Message sink failed. */
    PLATFORM_LOG_ERROR_TRUNCATED = -5, /**< Message exceeded output buffer. */
};

/** @brief Configure the logging backend and runtime filter.
 * @param config Backend, optional timestamp source, and level.
 * @return 0 on success, or a PLATFORM_LOG_ERROR_* code.
 */
int platform_log_init(const platform_log_config_t *config);
/** @brief Change the minimum runtime log level.
 * @param level New log level.
 * @return 0 on success, or a PLATFORM_LOG_ERROR_* code.
 */
int platform_log_set_level(platform_log_level_t level);
/** @brief Read the current runtime log level.
 * @return Active log level.
 */
platform_log_level_t platform_log_get_level(void);
/** @brief Format and emit a log using a va_list.
 * @param level Message severity.
 * @param tag Source tag.
 * @param format printf-style format string.
 * @param arguments Variadic argument list.
 * @return Bytes written, PLATFORM_LOG_FILTERED, or a negative error code.
 */
int platform_log_vprintf(platform_log_level_t level, const char *tag,
                         const char *format, va_list arguments);
/** @brief Format and emit a tagged log message.
 * @param level Message severity.
 * @param tag Source tag.
 * @param format printf-style format string.
 * @return Bytes written, PLATFORM_LOG_FILTERED, or a negative error code.
 */
int platform_log_printf(platform_log_level_t level, const char *tag,
                        const char *format, ...);

/** @brief Emit a tagged debug message when enabled at compile time. */
#if PLATFORM_LOG_COMPILE_LEVEL <= PLATFORM_LOG_LEVEL_DEBUG_VALUE
#define LOG_DEBUG(tag, ...) \
    ((void)platform_log_printf(PLATFORM_LOG_LEVEL_DEBUG, (tag), __VA_ARGS__))
#else
#define LOG_DEBUG(tag, ...) ((void)0)
#endif

/** @brief Emit a tagged informational message when enabled. */
#if PLATFORM_LOG_COMPILE_LEVEL <= PLATFORM_LOG_LEVEL_INFO_VALUE
#define LOG_INFO(tag, ...) \
    ((void)platform_log_printf(PLATFORM_LOG_LEVEL_INFO, (tag), __VA_ARGS__))
#else
#define LOG_INFO(tag, ...) ((void)0)
#endif

/** @brief Emit a tagged warning message when enabled. */
#if PLATFORM_LOG_COMPILE_LEVEL <= PLATFORM_LOG_LEVEL_WARN_VALUE
#define LOG_WARN(tag, ...) \
    ((void)platform_log_printf(PLATFORM_LOG_LEVEL_WARN, (tag), __VA_ARGS__))
#else
#define LOG_WARN(tag, ...) ((void)0)
#endif

/** @brief Emit a tagged error message when enabled. */
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
