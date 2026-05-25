#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "fx5_client_utility.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *fx5_client_strdup(const char *text)
{
    size_t len;
    char *copy;

    if (text == NULL) {
        return NULL;
    }

    len = strlen(text);
    copy = (char *)malloc(len + 1u);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, len + 1u);
    return copy;
}

char *fx5_client_trim(char *text)
{
    char *end;

    if (text == NULL) {
        return NULL;
    }

    while (*text != '\0' && isspace((unsigned char)*text)) {
        ++text;
    }

    if (*text == '\0') {
        return text;
    }

    end = text + strlen(text) - 1;
    while (end > text && isspace((unsigned char)*end)) {
        --end;
    }

    end[1] = '\0';
    return text;
}

void fx5_client_trim_right(char *text)
{
    size_t len;

    if (text == NULL) {
        return;
    }

    len = strlen(text);
    while (len > 0u) {
        char ch = text[len - 1u];
        if (!isspace((unsigned char)ch)) {
            break;
        }

        text[len - 1u] = '\0';
        --len;
    }
}

const char *fx5_client_skip_spaces(const char *text)
{
    if (text == NULL) {
        return NULL;
    }

    while (*text != '\0' && isspace((unsigned char)*text)) {
        ++text;
    }

    return text;
}

char *fx5_client_skip_spaces_mut(char *text)
{
    if (text == NULL) {
        return NULL;
    }

    while (*text != '\0' && isspace((unsigned char)*text)) {
        ++text;
    }

    return text;
}

bool fx5_client_parse_bool(const char *text, bool *out_value)
{
    if (text == NULL || out_value == NULL) {
        return false;
    }

    if (strcmp(text, "true") == 0) {
        *out_value = true;
        return true;
    }

    if (strcmp(text, "false") == 0) {
        *out_value = false;
        return true;
    }

    return false;
}

bool fx5_client_parse_uint32_text(const char *text, uint32_t *out_value)
{
    char *endptr = NULL;
    unsigned long value;

    if (text == NULL || out_value == NULL) {
        return false;
    }

    if (*text == '\0') {
        return false;
    }

    value = strtoul(text, &endptr, 10);
    if (endptr == text || *endptr != '\0') {
        return false;
    }

    *out_value = (uint32_t)value;
    return true;
}

bool fx5_client_parse_uint16_text(const char *text, uint16_t *out_value)
{
    uint32_t tmp;

    if (text == NULL || out_value == NULL) {
        return false;
    }

    if (!fx5_client_parse_uint32_text(text, &tmp)) {
        return false;
    }

    if (tmp > 0xFFFFu) {
        return false;
    }

    *out_value = (uint16_t)tmp;
    return true;
}

bool fx5_client_parse_uint32_cursor(const char **text, uint32_t *out_value)
{
    const char *s;
    char *endptr = NULL;
    unsigned long value;

    if (text == NULL || *text == NULL || out_value == NULL) {
        return false;
    }

    s = *text;
    if (!isdigit((unsigned char)*s)) {
        return false;
    }

    value = strtoul(s, &endptr, 10);
    if (endptr == s) {
        return false;
    }

    *out_value = (uint32_t)value;
    *text = endptr;
    return true;
}
