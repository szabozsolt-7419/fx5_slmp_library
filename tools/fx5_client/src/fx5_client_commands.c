#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "fx5_client_commands.h"

#include <stdio.h>

static const char *fx5_client_device_name(fx5_device_t device)
{
    switch (device) {
        case FX5_DEV_D: return "D";
        case FX5_DEV_M: return "M";
        case FX5_DEV_X: return "X";
        case FX5_DEV_Y: return "Y";
        case FX5_DEV_L: return "L";
        case FX5_DEV_F: return "F";
        case FX5_DEV_S: return "S";
        case FX5_DEV_B: return "B";
        case FX5_DEV_W: return "W";
        case FX5_DEV_SM: return "SM";
        case FX5_DEV_SD: return "SD";
        case FX5_DEV_SB: return "SB";
        case FX5_DEV_SW: return "SW";
        default: return "?";
    }
}

static unsigned fx5_client_device_address_base(fx5_device_t device)
{
    switch (device) {
        case FX5_DEV_X:
        case FX5_DEV_Y:
            return 8u;
        case FX5_DEV_B:
        case FX5_DEV_W:
        case FX5_DEV_SB:
        case FX5_DEV_SW:
            return 16u;
        default:
            return 10u;
    }
}

static bool fx5_client_is_bit_device(fx5_device_t device)
{
    switch (device) {
        case FX5_DEV_M:
        case FX5_DEV_SM:
        case FX5_DEV_L:
        case FX5_DEV_F:
        case FX5_DEV_S:
        case FX5_DEV_X:
        case FX5_DEV_Y:
        case FX5_DEV_B:
        case FX5_DEV_SB:
            return true;
        case FX5_DEV_D:
        case FX5_DEV_SD:
        case FX5_DEV_W:
        case FX5_DEV_SW:
        default:
            return false;
    }
}

static bool fx5_client_device_allows_write(fx5_device_t device)
{
    switch (device) {
        case FX5_DEV_SM:
        case FX5_DEV_SD:
        case FX5_DEV_SB:
        case FX5_DEV_SW:
            return false;
        default:
            return true;
    }
}

static void fx5_client_print_device_address(FILE *stream, fx5_device_t device, uint32_t address)
{
    const unsigned base = fx5_client_device_address_base(device);

    if (base == 16u) {
        fprintf(stream, "%s%lX", fx5_client_device_name(device), (unsigned long)address);
    } else if (base == 8u) {
        fprintf(stream, "%s%lo", fx5_client_device_name(device), (unsigned long)address);
    } else {
        fprintf(stream, "%s%lu", fx5_client_device_name(device), (unsigned long)address);
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

    if (cmd->type == FX5_CLIENT_CMD_SET && !fx5_client_device_allows_write(cmd->device)) {
        return FX5_ST_ERR_UNSUPPORTED;
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
            fx5_client_print_device_address(stream, cmd->device, cmd->start_address + i);
            fprintf(stream, "=%s\n", (value != 0u) ? "true" : "false");
        } else {
            fx5_client_print_device_address(stream, cmd->device, cmd->start_address + i);
            fprintf(stream, "=%u\n", (unsigned)value);
        }
    }
}
