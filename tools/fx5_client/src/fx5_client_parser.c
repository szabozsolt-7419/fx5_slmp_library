#include "fx5_client_parser.h"
#include "fx5_client_utility.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static void fx5_client_command_init(fx5_client_command_t *cmd)
{
    if (cmd == NULL) {
        return;
    }

    memset(cmd, 0, sizeof(*cmd));
    cmd->type = FX5_CLIENT_CMD_INVALID;
    cmd->device = 0;
    cmd->header_type = FX5_4E_HEADER;
}

static bool fx5_client_is_token_boundary(char c)
{
    return c == '\0' || isspace((unsigned char)c) || c == ',' || c == '=';
}

static bool fx5_client_parse_bool_word(const char *text, bool *out_value, size_t *out_len)
{
    if (text == NULL || out_value == NULL || out_len == NULL) {
        return false;
    }

    if (strncmp(text, "true", 4) == 0 &&
        fx5_client_is_token_boundary(text[4])) {
        *out_value = true;
        *out_len = 4u;
        return true;
    }

    if (strncmp(text, "false", 5) == 0 &&
        fx5_client_is_token_boundary(text[5])) {
        *out_value = false;
        *out_len = 5u;
        return true;
    }

    return false;
}

static bool fx5_client_parse_device_and_range(
    const char **p,
    fx5_device_t *out_device,
    uint32_t *out_start,
    uint16_t *out_count
    )
{
    const char *s = NULL;
    uint32_t start = 0u;
    uint32_t end = 0u;
    fx5_device_t device = 0;

    if (p == NULL || *p == NULL || out_device == NULL || out_start == NULL || out_count == NULL) {
        return false;
    }

    s = fx5_client_skip_spaces(*p);
    if (s == NULL || *s == '\0') {
        return false;
    }

    switch (*s) {
        case 'D': device = FX5_DEV_D; break;
        case 'M': device = FX5_DEV_M; break;
        case 'X': device = FX5_DEV_X; break;
        case 'Y': device = FX5_DEV_Y; break;
        default:
            return false;
    }

    ++s;

    if (!fx5_client_parse_uint32_cursor(&s, &start)) {
        return false;
    }

    s = fx5_client_skip_spaces(s);

    if (*s == '-') {
        ++s;
        s = fx5_client_skip_spaces(s);

        if (!fx5_client_parse_uint32_cursor(&s, &end)) {
            return false;
        }

        if (end < start) {
            return false;
        }

        if ((end - start + 1u) > 0xFFFFu) {
            return false;
        }

        *out_count = (uint16_t)(end - start + 1u);
    } else {
        *out_count = 1u;
    }

    *out_device = device;
    *out_start = start;
    *p = s;
    return true;
}

static bool fx5_client_parse_scalar_or_list(
    const char *p,
    fx5_device_t device,
    uint16_t expected_count,
    fx5_client_command_t *cmd
    )
{
    uint16_t count = 0u;

    if (p == NULL || cmd == NULL) {
        return false;
    }

    p = fx5_client_skip_spaces(p);
    if (*p == '\0') {
        return false;
    }

    if (device == FX5_DEV_M || device == FX5_DEV_X || device == FX5_DEV_Y) {
        size_t len = 0u;
        bool b = false;

        if (fx5_client_parse_bool_word(p, &b, &len)) {
            p += len;
            p = fx5_client_skip_spaces(p);
            if (*p != '\0') {
                return false;
            }

            cmd->has_scalar_value = true;
            cmd->bool_value = b;
            cmd->value_count = expected_count;

            for (count = 0u; count < expected_count; ++count) {
                cmd->values[count] = b ? 1u : 0u;
            }

            return true;
        }
    }

    while (*p != '\0') {
        bool b = false;
        size_t bool_len = 0u;
        uint32_t number = 0u;

        if (count >= FX5_CLIENT_MAX_VALUES) {
            return false;
        }

        if ((device == FX5_DEV_M || device == FX5_DEV_X || device == FX5_DEV_Y) &&
            fx5_client_parse_bool_word(p, &b, &bool_len)) {
            cmd->values[count++] = b ? 1u : 0u;
            p += bool_len;
        } else {
            if (!fx5_client_parse_uint32_cursor(&p, &number)) {
                return false;
            }
            if (number > 0xFFFFu) {
                return false;
            }
            cmd->values[count++] = (uint16_t)number;
        }

        p = fx5_client_skip_spaces(p);

        if (*p == ',') {
            ++p;
            p = fx5_client_skip_spaces(p);
            continue;
        }

        if (*p == '\0') {
            break;
        }

        return false;
    }

    if (count == 0u) {
        return false;
    }

    if (count == 1u && expected_count > 1u) {
        cmd->has_scalar_value = true;
        cmd->bool_value = (cmd->values[0] != 0u);
        cmd->value_count = expected_count;

        for (uint16_t i = 1u; i < expected_count; ++i) {
            cmd->values[i] = cmd->values[0];
        }

        return true;
    }

    if (count != expected_count) {
        return false;
    }

    cmd->value_count = count;
    return true;
}

static bool fx5_client_parse_get(const char *p, fx5_client_command_t *cmd)
{
    if (cmd == NULL) {
        return false;
    }

    cmd->type = FX5_CLIENT_CMD_GET;

    if (!fx5_client_parse_device_and_range(&p, &cmd->device, &cmd->start_address, &cmd->count)) {
        return false;
    }

    p = fx5_client_skip_spaces(p);
    return *p == '\0';
}

static bool fx5_client_parse_set(const char *p, fx5_client_command_t *cmd)
{
    if (cmd == NULL) {
        return false;
    }

    cmd->type = FX5_CLIENT_CMD_SET;

    if (!fx5_client_parse_device_and_range(&p, &cmd->device, &cmd->start_address, &cmd->count)) {
        return false;
    }

    p = fx5_client_skip_spaces(p);
    if (*p != '=') {
        return false;
    }

    ++p;
    return fx5_client_parse_scalar_or_list(p, cmd->device, cmd->count, cmd);
}

bool fx5_client_parse_command(const char *line, fx5_client_command_t *out_cmd)
{
    const char *p = NULL;

    if (line == NULL || out_cmd == NULL) {
        return false;
    }

    fx5_client_command_init(out_cmd);

    p = fx5_client_skip_spaces(line);
    if (*p == '\0') {
        return false;
    }

    if (strcmp(p, "help") == 0) {
        out_cmd->type = FX5_CLIENT_CMD_HELP;
        return true;
    }

    if (strcmp(p, "quit") == 0 || strcmp(p, "exit") == 0) {
        out_cmd->type = FX5_CLIENT_CMD_QUIT;
        return true;
    }

    if (strcmp(p, "trace on") == 0) {
        out_cmd->type = FX5_CLIENT_CMD_TRACE;
        out_cmd->trace_enabled = true;
        return true;
    }

    if (strcmp(p, "trace off") == 0) {
        out_cmd->type = FX5_CLIENT_CMD_TRACE;
        out_cmd->trace_enabled = false;
        return true;
    }

    if (strcmp(p, "header 3e") == 0) {
        out_cmd->type = FX5_CLIENT_CMD_HEADER;
        out_cmd->header_type = FX5_3E_HEADER;
        return true;
    }

    if (strcmp(p, "header 4e") == 0) {
        out_cmd->type = FX5_CLIENT_CMD_HEADER;
        out_cmd->header_type = FX5_4E_HEADER;
        return true;
    }

    if (strncmp(p, "get", 3) == 0 && isspace((unsigned char)p[3])) {
        return fx5_client_parse_get(p + 3, out_cmd);
    }

    if (strncmp(p, "set", 3) == 0 && isspace((unsigned char)p[3])) {
        return fx5_client_parse_set(p + 3, out_cmd);
    }

    return false;
}