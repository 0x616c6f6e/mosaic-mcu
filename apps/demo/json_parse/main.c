#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <chip_time.h>
#include <platform_log.h>

#include "demo_support.h"
#include <jsmn.h>

static bool token_equals(const char *json, const jsmntok_t *token,
                         const char *expected)
{
    size_t token_length = (size_t)(token->end - token->start);

    return (strlen(expected) == token_length) &&
           (memcmp(&json[token->start], expected, token_length) == 0);
}

static bool token_to_u32(const char *json, const jsmntok_t *token,
                         uint32_t *value)
{
    uint32_t result = 0U;

    if ((token->type != JSMN_PRIMITIVE) || (value == NULL) ||
        (token->start >= token->end)) {
        return false;
    }
    for (int position = token->start; position < token->end; ++position) {
        char character = json[position];

        if ((character < '0') || (character > '9')) {
            return false;
        }
        if (result > ((UINT32_MAX - (uint32_t)(character - '0')) / 10U)) {
            return false;
        }
        result = (result * 10U) + (uint32_t)(character - '0');
    }
    *value = result;
    return true;
}

int main(void)
{
    static const char json[] =
        "{\"enabled\":true,\"led_period_ms\":250}";
    jsmn_parser parser;
    jsmntok_t tokens[8];
    uint32_t led_period_ms = 500U;
    bool enabled = false;
    bool led_on = false;
    int token_count;

    DEMO_REQUIRE(demo_platform_init());
    DEMO_REQUIRE(demo_led_init());
    DEMO_REQUIRE(demo_log_init());

    jsmn_init(&parser);
    token_count = jsmn_parse(&parser, json, sizeof(json) - 1U,
                             tokens, sizeof(tokens) / sizeof(tokens[0]));
    if ((token_count != 5) || (tokens[0].type != JSMN_OBJECT)) {
        LOG_ERROR("json", "parse failed, result=%d", token_count);
        demo_halt();
    }
    if (token_equals(json, &tokens[1], "enabled")) {
        enabled = token_equals(json, &tokens[2], "true");
    }
    if (token_equals(json, &tokens[3], "led_period_ms") &&
        !token_to_u32(json, &tokens[4], &led_period_ms)) {
        LOG_ERROR("json", "invalid led_period_ms");
        demo_halt();
    }
    LOG_INFO("json", "tokens=%d enabled=%u led_period_ms=%lu",
             token_count, enabled ? 1U : 0U, (unsigned long)led_period_ms);

    for (;;) {
        if (enabled) {
            led_on = !led_on;
            DEMO_REQUIRE(demo_led_write(led_on));
        }
        chip_delay_ms(led_period_ms);
    }
}
