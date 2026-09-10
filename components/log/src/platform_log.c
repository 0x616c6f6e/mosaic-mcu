#include <platform_log.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef PLATFORM_LOG_BUFFER_SIZE
#define PLATFORM_LOG_BUFFER_SIZE 256U
#endif

#if PLATFORM_LOG_BUFFER_SIZE < 32U
#error "PLATFORM_LOG_BUFFER_SIZE must be at least 32 bytes"
#endif

typedef struct {
    char *data;
    size_t capacity;
    size_t size;
    bool truncated;
} log_writer_t;

typedef enum {
    LOG_LENGTH_DEFAULT,
    LOG_LENGTH_CHAR,
    LOG_LENGTH_SHORT,
    LOG_LENGTH_LONG,
    LOG_LENGTH_LONG_LONG,
    LOG_LENGTH_SIZE,
} log_length_t;

static platform_log_config_t log_config = {
    .backend = NULL,
    .backend_context = NULL,
    .timestamp = NULL,
    .timestamp_context = NULL,
    .level = PLATFORM_LOG_LEVEL_DEBUG,
};
static bool log_initialized;
static char log_buffer[PLATFORM_LOG_BUFFER_SIZE];

static bool log_level_valid(platform_log_level_t level)
{
    return (level >= PLATFORM_LOG_LEVEL_DEBUG) &&
           (level <= PLATFORM_LOG_LEVEL_NONE);
}

static char log_level_letter(platform_log_level_t level)
{
    static const char letters[] = {'D', 'I', 'W', 'E'};

    if ((level < PLATFORM_LOG_LEVEL_DEBUG) ||
        (level > PLATFORM_LOG_LEVEL_ERROR)) {
        return '?';
    }
    return letters[(unsigned int)level];
}

static void log_putc(log_writer_t *writer, char value)
{
    if (writer->size < writer->capacity) {
        writer->data[writer->size] = value;
    } else {
        writer->truncated = true;
    }
    ++writer->size;
}

static void log_puts(log_writer_t *writer, const char *text)
{
    const char *value = (text != NULL) ? text : "(null)";

    while (*value != '\0') {
        log_putc(writer, *value++);
    }
}

static void log_put_unsigned(log_writer_t *writer, uint64_t value,
                             unsigned int base, bool uppercase,
                             unsigned int width, char padding,
                             bool negative)
{
    char digits[32];
    const char *alphabet = uppercase ?
        "0123456789ABCDEF" : "0123456789abcdef";
    size_t digit_count = 0U;
    unsigned int total_width;

    do {
        digits[digit_count++] = alphabet[value % base];
        value /= base;
    } while ((value != 0U) && (digit_count < sizeof(digits)));

    total_width = (unsigned int)digit_count + (negative ? 1U : 0U);
    if (negative && (padding == '0')) {
        log_putc(writer, '-');
        negative = false;
    }
    while (width > total_width) {
        log_putc(writer, padding);
        --width;
    }
    if (negative) {
        log_putc(writer, '-');
    }
    while (digit_count > 0U) {
        log_putc(writer, digits[--digit_count]);
    }
}

static uint64_t log_unsigned_argument(va_list *arguments, log_length_t length)
{
    switch (length) {
    case LOG_LENGTH_CHAR:
        return (uint8_t)va_arg(*arguments, int);
    case LOG_LENGTH_SHORT:
        return (uint16_t)va_arg(*arguments, int);
    case LOG_LENGTH_LONG:
        return va_arg(*arguments, unsigned long);
    case LOG_LENGTH_LONG_LONG:
        return va_arg(*arguments, unsigned long long);
    case LOG_LENGTH_SIZE:
        return va_arg(*arguments, size_t);
    case LOG_LENGTH_DEFAULT:
    default:
        return va_arg(*arguments, unsigned int);
    }
}

static int64_t log_signed_argument(va_list *arguments, log_length_t length)
{
    switch (length) {
    case LOG_LENGTH_CHAR:
        return (int8_t)va_arg(*arguments, int);
    case LOG_LENGTH_SHORT:
        return (int16_t)va_arg(*arguments, int);
    case LOG_LENGTH_LONG:
        return va_arg(*arguments, long);
    case LOG_LENGTH_LONG_LONG:
        return va_arg(*arguments, long long);
    case LOG_LENGTH_SIZE:
        return va_arg(*arguments, ptrdiff_t);
    case LOG_LENGTH_DEFAULT:
    default:
        return va_arg(*arguments, int);
    }
}

static int log_format(log_writer_t *writer, const char *format,
                      va_list *arguments)
{
    while (*format != '\0') {
        bool zero_padding = false;
        unsigned int width = 0U;
        log_length_t length = LOG_LENGTH_DEFAULT;
        char conversion;

        if (*format != '%') {
            log_putc(writer, *format++);
            continue;
        }
        ++format;
        if (*format == '%') {
            log_putc(writer, *format++);
            continue;
        }
        if (*format == '0') {
            zero_padding = true;
            ++format;
        }
        while ((*format >= '0') && (*format <= '9')) {
            unsigned int digit = (unsigned int)(*format - '0');

            width = (width < 100U) ? ((width * 10U) + digit) : 1000U;
            ++format;
        }
        if (*format == 'h') {
            ++format;
            if (*format == 'h') {
                length = LOG_LENGTH_CHAR;
                ++format;
            } else {
                length = LOG_LENGTH_SHORT;
            }
        } else if (*format == 'l') {
            ++format;
            if (*format == 'l') {
                length = LOG_LENGTH_LONG_LONG;
                ++format;
            } else {
                length = LOG_LENGTH_LONG;
            }
        } else if (*format == 'z') {
            length = LOG_LENGTH_SIZE;
            ++format;
        }

        conversion = *format;
        if (conversion == '\0') {
            return PLATFORM_LOG_ERROR_FORMAT;
        }
        ++format;
        switch (conversion) {
        case 'c':
            log_putc(writer, (char)va_arg(*arguments, int));
            break;
        case 's':
            log_puts(writer, va_arg(*arguments, const char *));
            break;
        case 'd':
        case 'i': {
            int64_t value = log_signed_argument(arguments, length);
            bool negative = value < 0;
            uint64_t magnitude = negative ?
                (uint64_t)(-(value + 1)) + 1U : (uint64_t)value;

            log_put_unsigned(writer, magnitude, 10U, false, width,
                             zero_padding ? '0' : ' ', negative);
            break;
        }
        case 'u':
            log_put_unsigned(writer, log_unsigned_argument(arguments, length),
                             10U, false, width,
                             zero_padding ? '0' : ' ', false);
            break;
        case 'x':
        case 'X':
            log_put_unsigned(writer, log_unsigned_argument(arguments, length),
                             16U, conversion == 'X', width,
                             zero_padding ? '0' : ' ', false);
            break;
        case 'p':
            log_puts(writer, "0x");
            log_put_unsigned(writer,
                             (uint64_t)(uintptr_t)va_arg(*arguments, void *),
                             16U, false, width, '0', false);
            break;
        default:
            return PLATFORM_LOG_ERROR_FORMAT;
        }
    }
    return 0;
}

static void log_append_prefix(log_writer_t *writer,
                              platform_log_level_t level, const char *tag)
{
    if (log_config.timestamp != NULL) {
        log_putc(writer, '[');
        log_put_unsigned(writer,
                         log_config.timestamp(log_config.timestamp_context),
                         10U, false, 10U, '0', false);
        log_puts(writer, "] ");
    }
    log_putc(writer, log_level_letter(level));
    if ((tag != NULL) && (tag[0] != '\0')) {
        log_putc(writer, '/');
        log_puts(writer, tag);
    }
    log_puts(writer, ": ");
}

int platform_log_init(const platform_log_config_t *config)
{
    if ((config == NULL) || (config->backend == NULL) ||
        !log_level_valid(config->level)) {
        return PLATFORM_LOG_ERROR_INVALID_ARGUMENT;
    }
    log_config = *config;
    log_initialized = true;
    return 0;
}

int platform_log_set_level(platform_log_level_t level)
{
    if (!log_level_valid(level)) {
        return PLATFORM_LOG_ERROR_INVALID_ARGUMENT;
    }
    log_config.level = level;
    return 0;
}

platform_log_level_t platform_log_get_level(void)
{
    return log_config.level;
}

int platform_log_vprintf(platform_log_level_t level, const char *tag,
                         const char *format, va_list arguments)
{
    log_writer_t writer = {
        .data = log_buffer,
        .capacity = sizeof(log_buffer) - 1U,
        .size = 0U,
        .truncated = false,
    };
    va_list copy;
    int result;
    size_t output_size;
    int written;

    if (!log_initialized) {
        return PLATFORM_LOG_ERROR_NOT_READY;
    }
    if (!log_level_valid(level) || (level == PLATFORM_LOG_LEVEL_NONE) ||
        (format == NULL)) {
        return PLATFORM_LOG_ERROR_INVALID_ARGUMENT;
    }
    if (level < log_config.level) {
        return PLATFORM_LOG_FILTERED;
    }

    log_append_prefix(&writer, level, tag);
    va_copy(copy, arguments);
    result = log_format(&writer, format, &copy);
    va_end(copy);
    if (result != 0) {
        return result;
    }

    output_size = (writer.size < writer.capacity) ? writer.size : writer.capacity;
    if ((output_size > 0U) && (log_buffer[output_size - 1U] == '\n')) {
        --output_size;
        if ((output_size > 0U) && (log_buffer[output_size - 1U] == '\r')) {
            --output_size;
        }
    }
    if (output_size > (writer.capacity - 2U)) {
        output_size = writer.capacity - 2U;
        writer.truncated = true;
    }
    log_buffer[output_size++] = '\r';
    log_buffer[output_size++] = '\n';
    log_buffer[output_size] = '\0';

    written = log_config.backend(log_buffer, output_size,
                                 log_config.backend_context);
    if ((written < 0) || ((size_t)written != output_size)) {
        return PLATFORM_LOG_ERROR_BACKEND;
    }
    return writer.truncated ? PLATFORM_LOG_ERROR_TRUNCATED : written;
}

int platform_log_printf(platform_log_level_t level, const char *tag,
                        const char *format, ...)
{
    va_list arguments;
    int result;

    va_start(arguments, format);
    result = platform_log_vprintf(level, tag, format, arguments);
    va_end(arguments);
    return result;
}
