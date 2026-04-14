#include "fx5_client_commands.h"

#include <stdio.h>

static const char *fx5_client_device_name(fx5_device_t device)
{
    switch (device) {
        case FX5_DEV_D: return "D";
        case FX5_DEV_M: return "M";
        case FX5_DEV_X: return "X";
        case FX5_DEV_Y: return "Y";
        default: return "?";
    }
}

static bool fx5_client_is_bit_device(fx5_device_t device)
{
    switch (device) {
        case FX5_DEV_M:
        case FX5_DEV_X:
        case FX5_DEV_Y:
            return true;
        case FX5_DEV_D:
        default:
            return false;
    }
}

bool fx5_client_command_needs_plc_io(const fx5_client_command_t *cmd)
{
    if (cmd == NULL) {
        return false;
    }

    return cmd->type == FX5_CLIENT_CMD_GET || cmd->type == FX5_CLIENT_CMD_SET;
}

fx5_status_t fx5_client_apply_command_to_context(
    fx5_context_t *ctx,
    const fx5_client_command_t *cmd,
    const fx5_client_config_t *config
    )
{
    fx5_status_t st;
    fx5_network_settings_t net;
    uint16_t i = 0u;

    if (ctx == NULL || cmd == NULL || config == NULL) {
        return FX5_ST_ERR_NULL;
    }

    if (!fx5_client_command_needs_plc_io(cmd)) {
        return FX5_ST_ERR_STATE;
    }

    st = fx5_reset(ctx);
    if (st != FX5_ST_OK) {
        return st;
    }

    net = config->network_settings;

    st = fx5_set_network_settings(ctx, &net);
    if (st != FX5_ST_OK) {
        return st;
    }

    if (cmd->type == FX5_CLIENT_CMD_GET) {
        return fx5_set_request(
            ctx,
            FX5_CMD_BATCH_READ,
            cmd->device,
            cmd->start_address,
            cmd->count
        );
    }

    st = fx5_set_request(
        ctx,
        FX5_CMD_BATCH_WRITE,
        cmd->device,
        cmd->start_address,
        cmd->count
    );
    if (st != FX5_ST_OK) {
        return st;
    }

    for (i = 0u; i < cmd->value_count; ++i) {
        st = fx5_set_write_value(ctx, i, cmd->values[i]);
        if (st != FX5_ST_OK) {
            return st;
        }
    }

    return FX5_ST_OK;
}

void fx5_client_print_response(
    FILE *stream,
    const fx5_client_command_t *cmd,
    const fx5_context_t *ctx
    )
{
    uint16_t count = 0u;
    uint16_t i = 0u;

    if (stream == NULL || cmd == NULL || ctx == NULL) {
        return;
    }

    uint16_t end_code = fx5_get_response_code(ctx);

    if (end_code != 0u) {
      fprintf(stream, "PLC ERROR: 0x%04X\n", end_code);
      return;
  }

    if (cmd->type == FX5_CLIENT_CMD_SET) {
        fprintf(stream, "OK\n");
        return;
    }

    if (cmd->type != FX5_CLIENT_CMD_GET) {
        return;
    }

    count = fx5_get_response_count(ctx);

    for (i = 0u; i < count; ++i) {
        uint16_t value = 0u;
        const fx5_status_t st = fx5_get_response_value(ctx, i, &value);
        if (st != FX5_ST_OK) {
            fprintf(stream, "ERR: failed to read response value %u\n", (unsigned)i);
            return;
        }

        if (fx5_client_is_bit_device(cmd->device)) {
            fprintf(stream,
                    "%s%lu=%s\n",
                    fx5_client_device_name(cmd->device),
                    (unsigned long)(cmd->start_address + i),
                    (value != 0u) ? "true" : "false");
        } else {
            fprintf(stream,
                    "%s%lu=%u\n",
                    fx5_client_device_name(cmd->device),
                    (unsigned long)(cmd->start_address + i),
                    (unsigned)value);
        }
    }
}