#include <fx5/fx5.h>
#include <fx5/internal/fx5_private.h>
#include <fx5/internal/fx5_utility.h>


/* --------------------------------------------------------------------------
 * Response parsing
 * -------------------------------------------------------------------------- */


uint16_t fx5_get_response_header_size(
    fx5_header_t header_type
    )
{
    return (header_type == FX5_4E_HEADER) ? FX5_RESP_TCP_HEADER_SIZE_4E : FX5_RESP_TCP_HEADER_SIZE_3E;
}


fx5_header_t fx5_detect_response_header(
    fx5_ringbuf_t *rx_buffer
    )
{
    if (rx_buffer == NULL) return FX5_UNKNOWN_HEADER;
    if (fx5_ringbuf_size(rx_buffer) < 2u) return FX5_UNKNOWN_HEADER;

    const uint16_t magic = pdu_peekc_u16_le(rx_buffer, FX5_MAGIC_NUMBER_OFFSET);
    if (magic == FX5_TCP_RESPONSE_SUBHEADER_4E_BINARY) return FX5_4E_HEADER;
    if (magic == FX5_TCP_RESPONSE_SUBHEADER_3E_BINARY) return FX5_3E_HEADER;

    return FX5_UNKNOWN_HEADER;
}

fx5_header_t fx5_try_resync(
    fx5_ringbuf_t *rx_buffer,
    uint16_t *discarded_bytes
    )
{
    if (rx_buffer == NULL || discarded_bytes == NULL) return FX5_UNKNOWN_HEADER;

    while (fx5_ringbuf_size(rx_buffer) > 1u) {
        const fx5_header_t header_type = fx5_detect_response_header(rx_buffer);
        if (header_type != FX5_UNKNOWN_HEADER) return header_type;

        fx5_ringbuf_drop_front(rx_buffer, 1u);
        (*discarded_bytes)++;
    }

    if (fx5_ringbuf_size(rx_buffer) == 1u) {
        uint8_t byte0 = 0u;
        if (!fx5_ringbuf_peek(rx_buffer, 0u, &byte0)) {
            return FX5_UNKNOWN_HEADER;
        }

        const uint8_t low3e = (uint8_t)(FX5_TCP_RESPONSE_SUBHEADER_3E_BINARY & 0xFFu);
        const uint8_t low4e = (uint8_t)(FX5_TCP_RESPONSE_SUBHEADER_4E_BINARY & 0xFFu);
        if (byte0 != low3e && byte0 != low4e) {
            fx5_ringbuf_drop_front(rx_buffer, 1u);
            (*discarded_bytes)++;
        }
    }

    return FX5_UNKNOWN_HEADER;
}


fx5_status_t fx5_parse_response_header_3e(
    fx5_context_t *context,
    fx5_ringbuf_t *rx_buffer,
    uint16_t *packet_size
    )
{
    (void)context;
    if (rx_buffer == NULL || packet_size == NULL) return FX5_ST_ERR_NULL;

    if (fx5_ringbuf_size(rx_buffer) < FX5_RESP_TCP_HEADER_SIZE_3E) return FX5_ST_ERR_NEED_MORE;

    *packet_size = pdu_peekc_u16_le(rx_buffer, FX5_PACKET_SIZE_OFFSET_3E);
    if (*packet_size < FX5_RESP_PDU_HEADER_SIZE || *packet_size > (uint16_t)FX5_IOBUF_SIZE) {
        return FX5_ST_ERR_HEADER_MISMATCH;
    }

    return FX5_ST_OK;
}

fx5_status_t fx5_parse_response_header_4e(
    fx5_context_t *context,
    fx5_ringbuf_t *rx_buffer,
    uint16_t *packet_size
    )
{
    if (context == NULL || rx_buffer == NULL || packet_size == NULL) return FX5_ST_ERR_NULL;

    if (fx5_ringbuf_size(rx_buffer) < FX5_RESP_TCP_HEADER_SIZE_4E) return FX5_ST_ERR_NEED_MORE;

    const uint16_t serial = pdu_peekc_u16_le(rx_buffer, FX5_SERIAL_OFFSET_4E);
    if (serial != context->serial) {
        return FX5_ST_ERR_HEADER_MISMATCH;
    }

    *packet_size = pdu_peekc_u16_le(rx_buffer, FX5_PACKET_SIZE_OFFSET_4E);
    if (*packet_size < FX5_RESP_PDU_HEADER_SIZE || *packet_size > (uint16_t)FX5_IOBUF_SIZE) {
        return FX5_ST_ERR_HEADER_MISMATCH;
    }

    return FX5_ST_OK;
}


fx5_status_t fx5_parse_word_response_payload(
    fx5_context_t *context,
    fx5_ringbuf_t *rx_buffer,
    uint16_t payload_size
    )
{
    if (context == NULL || rx_buffer == NULL) return FX5_ST_ERR_NULL;
    if ((payload_size & 1u) != 0u) return FX5_ST_ERR_INVALID_COUNT;

    const uint16_t value_count = (uint16_t)(payload_size / 2u);
    const uint16_t requested_count = context->count;
    const uint16_t expected_payload_size = (uint16_t)(requested_count * 2u);

    if (requested_count == 0u || requested_count > FX5_MAX_VALUE_COUNT) {
        if (payload_size > 0u) {
            fx5_ringbuf_drop_front(rx_buffer, payload_size);
        }
        return FX5_ST_ERR_INVALID_COUNT;
    }

    if (payload_size != expected_payload_size) {
        if (payload_size > 0u) {
            fx5_ringbuf_drop_front(rx_buffer, payload_size);
        }
        return FX5_ST_ERR_INVALID_COUNT;
    }

    context->count = value_count;
    for (uint16_t k = 0u; k < value_count; ++k) {
        context->values[k] = pdu_rdc_u16_le(rx_buffer);
    }

    return FX5_ST_OK;
}




static uint16_t fx5_unpack_bitunit_bytes(
    fx5_ringbuf_t *buffer,
    uint16_t payload_size,
    uint16_t *values,
    uint16_t max_count,
    uint16_t *remaining_bytes
    )
{
    uint16_t bit_count = (uint16_t)(payload_size * 2u);
    if (bit_count > max_count) {
        bit_count = max_count;
    }

    const uint16_t bytes_needed = (uint16_t)((bit_count + 1u) / 2u);
    *remaining_bytes = (uint16_t)(payload_size - bytes_needed);

    uint16_t bit_index = 0u;
    for (uint16_t i = 0u; i < bytes_needed; ++i) {
        uint8_t b = 0u;
        (void)fx5_ringbuf_peek(buffer, 0u, &b);
        fx5_ringbuf_drop_front(buffer, 1u);

        const uint8_t hi = (uint8_t)((b >> 4) & 0x0Fu);
        const uint8_t lo = (uint8_t)(b & 0x0Fu);

        if (bit_index < bit_count) {
            values[bit_index++] = (hi == 0x01u) ? FX5_BIT_TRUE : FX5_BIT_FALSE;
        }
        if (bit_index < bit_count) {
            values[bit_index++] = (lo == 0x01u) ? FX5_BIT_TRUE : FX5_BIT_FALSE;
        }
    }

    return bit_count;
}



fx5_status_t fx5_parse_bit_response_payload(
    fx5_context_t *context,
    fx5_ringbuf_t *rx_buffer,
    uint16_t payload_size
    )
{
    if (context == NULL || rx_buffer == NULL) return FX5_ST_ERR_NULL;

    uint16_t remaining_bytes = 0u;
    const uint16_t requested_count = context->count;
    const uint16_t expected_payload_size = (uint16_t)((requested_count + 1u) / 2u);

    if (requested_count == 0u || requested_count > FX5_MAX_VALUE_COUNT) {
        if (payload_size > 0u) {
            fx5_ringbuf_drop_front(rx_buffer, payload_size);
        }
        return FX5_ST_ERR_INVALID_COUNT;
    }

    if (payload_size < expected_payload_size) {
        if (payload_size > 0u) {
            fx5_ringbuf_drop_front(rx_buffer, payload_size);
        }
        return FX5_ST_ERR_INVALID_COUNT;
    }

    context->count = fx5_unpack_bitunit_bytes(
        rx_buffer,
        payload_size,
        context->values,
        requested_count,
        &remaining_bytes
        );

    if (remaining_bytes > 0u) {
        fx5_ringbuf_drop_front(rx_buffer, remaining_bytes);
    }

    return FX5_ST_OK;
}


fx5_status_t fx5_parse_response_pdu(
    fx5_context_t *context,
    fx5_ringbuf_t *rx_buffer,
    uint16_t packet_size
    )
{
    if (context == NULL || rx_buffer == NULL) return FX5_ST_ERR_NULL;
    if (packet_size < FX5_RESP_PDU_HEADER_SIZE) return FX5_ST_ERR_INVALID_COUNT;

    context->response_code = pdu_rdc_u16_le(rx_buffer);
    const uint16_t payload_size = (uint16_t)(packet_size - FX5_RESP_PDU_HEADER_SIZE);

    if (context->response_code != FX5_RESPONSE_OK) {
        if (payload_size > 0u) {
            fx5_ringbuf_drop_front(rx_buffer, payload_size);
        }
        context->count = 0u;
        return FX5_ST_RESPONSE_ERROR;
    }

    if (context->command == FX5_CMD_BATCH_WRITE) {
        if (payload_size > 0u) {
            fx5_ringbuf_drop_front(rx_buffer, payload_size);
            return FX5_ST_ERR_INVALID_COUNT;
        }

        context->count = 0u;
        return FX5_ST_OK;
    }

    if (fx5_is_bit_device(context->device)) {
        return fx5_parse_bit_response_payload(context, rx_buffer, payload_size);
    }

    return fx5_parse_word_response_payload(context, rx_buffer, payload_size);
}




