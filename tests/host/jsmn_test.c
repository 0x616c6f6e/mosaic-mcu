#include <jsmn.h>

#include <assert.h>
#include <stddef.h>
#include <string.h>

size_t jsmn_token_size_from_second_translation_unit(void);

static int token_equals(const char *json, const jsmntok_t *token,
                        const char *expected)
{
    size_t length = (size_t)(token->end - token->start);

    return (strlen(expected) == length) &&
           (memcmp(&json[token->start], expected, length) == 0);
}

int main(void)
{
    static const char json[] =
        "{\"name\":\"ch585\",\"enabled\":true,\"pins\":[8,9]}";
    jsmn_parser parser;
    jsmntok_t tokens[12];
    jsmntok_t too_few_tokens[2];
    int count;

    assert(jsmn_token_size_from_second_translation_unit() == sizeof(jsmntok_t));

    jsmn_init(&parser);
    count = jsmn_parse(&parser, json, sizeof(json) - 1U,
                       tokens, sizeof(tokens) / sizeof(tokens[0]));
    assert(count == 9);
    assert(tokens[0].type == JSMN_OBJECT);
    assert(tokens[0].size == 3);
    assert(token_equals(json, &tokens[1], "name"));
    assert(token_equals(json, &tokens[2], "ch585"));
    assert(token_equals(json, &tokens[3], "enabled"));
    assert(token_equals(json, &tokens[4], "true"));
    assert(tokens[6].type == JSMN_ARRAY);
    assert(tokens[6].size == 2);
    assert(token_equals(json, &tokens[7], "8"));
    assert(token_equals(json, &tokens[8], "9"));

    jsmn_init(&parser);
    assert(jsmn_parse(&parser, json, sizeof(json) - 1U,
                      too_few_tokens,
                      sizeof(too_few_tokens) / sizeof(too_few_tokens[0])) ==
           JSMN_ERROR_NOMEM);

    jsmn_init(&parser);
    assert(jsmn_parse(&parser, "{\"value\":", 9U, tokens,
                      sizeof(tokens) / sizeof(tokens[0])) == JSMN_ERROR_PART);

    jsmn_init(&parser);
    assert(jsmn_parse(&parser, "{\"x\":\"\\q\"}", 10U, tokens,
                      sizeof(tokens) / sizeof(tokens[0])) == JSMN_ERROR_INVAL);
    return 0;
}
