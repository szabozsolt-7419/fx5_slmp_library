#include <fx5/fx5.h>
#include <fx5/internal/fx5_private.h>
#include <fx5/internal/fx5_utility.h>

/* --------------------------------------------------------------------------
 * Request validation and request building
 * -------------------------------------------------------------------------- */

bool fx5_is_supported_command(
    fx5_cmd_t command
    )
{
    switch (command) {
        case FX5_CMD_BATCH_READ:
        case FX5_CMD_BATCH_WRITE:
            return true;
    }

    return false;
}

bool fx5_is_supported_device(
    fx5_device_t device
    )
{
    switch (device) {
        case FX5_DEV_D:
        case FX5_DEV_W:
        case FX5_DEV_SD:
        case FX5_DEV_SW:
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
    }

    return false;
}

bool fx5_is_bit_device(
    fx5_device_t device
    )
{
    switch (device) {
        case FX5_DEV_D:
        case FX5_DEV_W:
        case FX5_DEV_SD:
        case FX5_DEV_SW:
            return false;
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
    }

    return false;
}

static bool fx5_device_allows_write(
    fx5_device_t device
    )
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

static uint16_t fx5_get_max_value_count(
    fx5_device_t device
    )
{
    return fx5_is_bit_device(device) ? FX5_MAX_BIT_VALUE_COUNT : FX5_MAX_WORD_VALUE_COUNT;
}

static bool fx5_get_packed_bit_value(
    const uint16_t *values,
    uint16_t index
    )
{
    const uint16_t slot = (uint16_t)(index / 16u);
    const uint16_t mask = (uint16_t)(1u << (index % 16u));

    return (values[slot] & mask) != 0u;
}


uint16_t fx5_get_request_header_size(
    fx5_header_t header_type
    )
{
    return (header_type == FX5_4E_HEADER) ? FX5_REQ_TCP_HEADER_SIZE_4E : FX5_REQ_TCP_HEADER_SIZE_3E;
}

static uint16_t fx5_get_write_payload_size(
    fx5_device_t device,
    uint16_t count
    )
{
    if (fx5_is_bit_device(device)) {
        return (uint16_t)((count + 1u) / 2u);   /* 2 points / byte */
    }

    return (uint16_t)(count * 2u);              /* 1 word / value */
}

uint16_t fx5_get_required_request_size(
    const fx5_context_t *context
    )
{
    if (context == NULL) return 0u;

    uint16_t size = 0u;
    size += fx5_get_request_header_size(context->network_settings.header_type);
    size += FX5_REQ_PDU_HEADER_SIZE;

    if (context->command == FX5_CMD_BATCH_WRITE) {
        size = (uint16_t)(size + fx5_get_write_payload_size(context->device, context->count));
    }

    return size;
}


fx5_status_t fx5_validate_request(
    const fx5_context_t *context
    )
{
    if (context == NULL) return FX5_ST_ERR_NULL;
    if (context->count == 0u) return FX5_ST_ERR_INVALID_COUNT;
    if (context->address > FX5_MAX_ADDRESS) return FX5_ST_ERR_INVALID_ADDRESS;
    if (!fx5_is_supported_command(context->command)) return FX5_ST_ERR_UNSUPPORTED;
    if (!fx5_is_supported_device(context->device)) return FX5_ST_ERR_UNSUPPORTED;
    if (context->count > fx5_get_max_value_count(context->device)) return FX5_ST_ERR_INVALID_COUNT;
    if (context->command == FX5_CMD_BATCH_WRITE && !fx5_device_allows_write(context->device)) {
        return FX5_ST_ERR_UNSUPPORTED;
    }
    if (context->network_settings.header_type != FX5_3E_HEADER && context->network_settings.header_type != FX5_4E_HEADER) {
        return FX5_ST_ERR_UNSUPPORTED;
    }

    return FX5_ST_OK;
}

fx5_status_t fx5_build_wire_header_3e(
    fx5_context_t *context,
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t payload_size,
    uint16_t *written_size
    )
{
    if (context == NULL || buffer == NULL || written_size == NULL) return FX5_ST_ERR_NULL;
    if (buffer_size < FX5_REQ_TCP_HEADER_SIZE_3E) return FX5_ST_ERR_TOO_SMALL;

    uint16_t index = 0u;
    index += pdu_wr_u16_le(&buffer[index], FX5_TCP_REQUEST_SUBHEADER_3E_BINARY);
    buffer[index++] = (uint8_t)context->network_settings.network_no;
    buffer[index++] = (uint8_t)context->network_settings.pc_no;
    index += pdu_wr_u16_le(&buffer[index], context->network_settings.module_io_no);
    buffer[index++] = (uint8_t)context->network_settings.module_station_no;
    index += pdu_wr_u16_le(&buffer[index], payload_size + 2u);
    index += pdu_wr_u16_le(&buffer[index], context->network_settings.monitoring_timer);

    context->serial = 0u;
    *written_size = index;
    return FX5_ST_OK;
}

fx5_status_t fx5_build_wire_header_4e(
    fx5_context_t *context,
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t payload_size,
    uint16_t *written_size
    )
{
    if (context == NULL || buffer == NULL || written_size == NULL) return FX5_ST_ERR_NULL;
    if (buffer_size < FX5_REQ_TCP_HEADER_SIZE_4E) return FX5_ST_ERR_TOO_SMALL;

    uint16_t index = 0u;
    index += pdu_wr_u16_le(&buffer[index], FX5_TCP_REQUEST_SUBHEADER_4E_BINARY);
    context->serial = context->next_serial++;
    index += pdu_wr_u16_le(&buffer[index], context->serial);
    index += pdu_wr_u16_le(&buffer[index], 0u);
    buffer[index++] = (uint8_t)context->network_settings.network_no;
    buffer[index++] = (uint8_t)context->network_settings.pc_no;
    index += pdu_wr_u16_le(&buffer[index], context->network_settings.module_io_no);
    buffer[index++] = (uint8_t)context->network_settings.module_station_no;
    index += pdu_wr_u16_le(&buffer[index], payload_size + 2u);
    index += pdu_wr_u16_le(&buffer[index], context->network_settings.monitoring_timer);

    *written_size = index;
    return FX5_ST_OK;
}


static fx5_status_t fx5_build_pdu_access_prefix(
    const fx5_context_t *context,
    uint16_t sub_command,
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t *written_size
    )
{
    if (context == NULL || buffer == NULL || written_size == NULL) {
        return FX5_ST_ERR_NULL;
    }

    if (buffer_size < FX5_REQ_PDU_HEADER_SIZE) {
        return FX5_ST_ERR_TOO_SMALL;
    }

    uint16_t index = 0u;
    index += pdu_wr_u16_le(&buffer[index], context->command);
    index += pdu_wr_u16_le(&buffer[index], sub_command);
    index += pdu_wr_u24_le(&buffer[index], context->address);
    buffer[index++] = (uint8_t)context->device;
    index += pdu_wr_u16_le(&buffer[index], context->count);

    *written_size = index;
    return FX5_ST_OK;
}




fx5_status_t fx5_build_pdu_read_request(
    const fx5_context_t *context,
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t *written_size
    )
{
    if (context == NULL || buffer == NULL || written_size == NULL) {
        return FX5_ST_ERR_NULL;
    }

    const uint16_t sub_command =
        fx5_is_bit_device(context->device) ? PDU_SUB_BIT_UNIT : PDU_SUB_WORD_UNIT;

    return fx5_build_pdu_access_prefix(
        context,
        sub_command,
        buffer,
        buffer_size,
        written_size
        );
}

static int fx5_pack_bits_to_nibbles(
    const uint16_t *values,
    uint16_t count,
    uint8_t *buffer,
    uint16_t buffer_size
    )
{
    if (values == NULL || buffer == NULL) return -1;

    const uint16_t expected_size = (uint16_t)((count + 1u) / 2u);
    if (buffer_size < expected_size) return -1;

    uint16_t index = 0u;
    uint16_t k = 0u;

    while (k < count) {
        uint8_t hi = fx5_get_packed_bit_value(values, k) ? 0x10u : 0x00u;
        uint8_t lo = 0x00u;

        k++;
        if (k < count) {
            lo = fx5_get_packed_bit_value(values, k) ? 0x01u : 0x00u;
            k++;
        }

        buffer[index++] = (uint8_t)(hi | lo);
    }

    return (int)index;
}

fx5_status_t fx5_build_pdu_write_request(
    const fx5_context_t *context,
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t *written_size
    )
{
    if (context == NULL || buffer == NULL || written_size == NULL) return FX5_ST_ERR_NULL;

    const bool bit_device = fx5_is_bit_device(context->device);
    const uint16_t payload_size = fx5_get_write_payload_size(context->device, context->count);
    const uint16_t required_size = (uint16_t)(FX5_REQ_PDU_HEADER_SIZE + payload_size);

    if (buffer_size < required_size) return FX5_ST_ERR_TOO_SMALL;

    uint16_t index = 0u;
    const uint16_t sub_command = bit_device ? PDU_SUB_BIT_UNIT : PDU_SUB_WORD_UNIT;

    fx5_status_t status = fx5_build_pdu_access_prefix(
        context,
        sub_command,
        buffer,
        buffer_size,
        &index
        );
    if (status != FX5_ST_OK) {
        return status;
    }

    if (bit_device) {
        const int written = fx5_pack_bits_to_nibbles(
            context->values,
            context->count,
            &buffer[index],
            (uint16_t)(buffer_size - index)
            );
        if (written < 0) return FX5_ST_ERR_INTERNAL;
        index = (uint16_t)(index + (uint16_t)written);
    } else {
        for (uint16_t k = 0u; k < context->count; ++k) {
            index += pdu_wr_u16_le(&buffer[index], context->values[k]);
        }
    }

    *written_size = index;
    return FX5_ST_OK;
}

