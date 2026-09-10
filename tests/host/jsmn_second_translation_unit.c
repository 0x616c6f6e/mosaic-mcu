#include <jsmn.h>

#include <stddef.h>

size_t jsmn_token_size_from_second_translation_unit(void)
{
    return sizeof(jsmntok_t);
}
