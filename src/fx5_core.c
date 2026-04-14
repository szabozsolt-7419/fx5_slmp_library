/**
 * @file fx5_core.c
 * @brief Implementation of the FX5 SLMP request builder and response parser.
 *
 * The source file is structured into these logical layers:
 * - public API and internal context pool handling
 * - request validation and request builders
 * - response header detection and parsing
 * - response payload parsing helpers
 *
 * The implementation is allocation-free and intended for embedded targets.
 */

#include <fx5/fx5.h>

#include <fx5/internal/fx5_utility.h>
#include <fx5/internal/fx5_private.h>

#include <stdbool.h>
#include <stddef.h>

/* --------------------------------------------------------------------------
 * Public API and context pool
 * -------------------------------------------------------------------------- */

static fx5_context_t s_context_pool[FX5_CONTEXT_POOL_SIZE];

static bool fx5_is_acquired(const fx5_context_t *context)
{
    return (context != NULL) && context->in_use;
}

static fx5_status_t fx5_require_acquired(const fx5_context_t *context)
{
    if (context == NULL) return FX5_ST_ERR_NULL;
    if (!context->in_use) return FX5_ST_ERR_NOT_ACQUIRED;
    return FX5_ST_OK;
}

fx5_status_t fx5_acquire(fx5_context_t **out_ctx)
{
    if (out_ctx == NULL) return FX5_ST_ERR_NULL;

    for (uint16_t k = 0u; k < FX5_CONTEXT_POOL_SIZE; ++k) {
        if (!s_context_pool[k].in_use) {        
            fx5_context_reset_internal(&s_context_pool[k]);
            s_context_pool[k].in_use = true;
            *out_ctx = &s_context_pool[k];
            return FX5_ST_OK;
        }
    }

    *out_ctx = NULL;
    return FX5_ST_ERR_NO_CONTEXT;
}

void fx5_release(fx5_context_t **ctx)
{
    if (ctx == NULL) return;
    if (*ctx == NULL) return;

    fx5_context_t *c = *ctx;

    if (!c->in_use) {
        *ctx = NULL;   /* defensive: ne maradjon stale pointer */
        return;
    }

    fx5_context_reset_internal(c);
    c->in_use = false;

    *ctx = NULL;       /* ez a lényeg: caller pointer nullázása */
}


fx5_status_t fx5_reset(fx5_context_t *ctx)
{
    fx5_status_t status = fx5_require_acquired(ctx);
    if (status != FX5_ST_OK) return status;

    fx5_context_reset_internal(ctx);

    return FX5_ST_OK;
}

void fx5_context_reset_internal(
    fx5_context_t *context
    )
{
    if (!context) return;    

    fx5_network_settings_init(&context->network_settings);
    context->next_serial = 0u;
    context->serial = 0u;
    context->device = FX5_DEV_D;
    context->command = FX5_CMD_BATCH_READ;
    context->resync_counter = 0u;
    context->count = 0u;
    context->response_code = FX5_RESPONSE_OK;
    context->address = 0x000000u;

    //just to spare some cpu cycles
    /*for (uint16_t k = 0; k < FX5_MAX_VALUE_COUNT; ++k) {
        context->values[k] = 0u;
    }*/

    fx5_ringbuf_init(&context->rx_cbuffer, context->buffer, FX5_IOBUF_SIZE);    
}


void fx5_network_settings_init(fx5_network_settings_t *settings)
{
    if (settings == NULL) return;

    settings->header_type = FX5_DEFAULT_HEADER_TYPE;
    settings->network_no = FX5_DEFAULT_NETWORK_NO;
    settings->pc_no = FX5_DEFAULT_PC_NO;
    settings->module_io_no = FX5_DEFAULT_MODULE_IO_NO;
    settings->module_station_no = FX5_DEFAULT_MODULE_STATION_NO;
    settings->monitoring_timer = FX5_DEFAULT_MONITORING_TIMER;
}

fx5_status_t fx5_set_network_settings(
    fx5_context_t *ctx,
    const fx5_network_settings_t *settings
    )
{
    fx5_status_t status = fx5_require_acquired(ctx);
    if (status != FX5_ST_OK) return status;
    if (settings == NULL) return FX5_ST_ERR_NULL;
    if (settings->header_type != FX5_3E_HEADER && settings->header_type != FX5_4E_HEADER) {
        return FX5_ST_ERR_UNSUPPORTED;
    }

    ctx->network_settings = *settings;
    return FX5_ST_OK;
}

fx5_status_t fx5_get_network_settings(
    const fx5_context_t *ctx,
    fx5_network_settings_t *out_settings
    )
{
    fx5_status_t status = fx5_require_acquired(ctx);
    if (status != FX5_ST_OK) return status;
    if (out_settings == NULL) return FX5_ST_ERR_NULL;

    *out_settings = ctx->network_settings;
    return FX5_ST_OK;
}

fx5_status_t fx5_set_header_mode(
    fx5_context_t *ctx,
    fx5_header_t mode
    )
{
    fx5_status_t status = fx5_require_acquired(ctx);
    if (status != FX5_ST_OK) return status;
    if (mode != FX5_3E_HEADER && mode != FX5_4E_HEADER) return FX5_ST_ERR_UNSUPPORTED;

    ctx->network_settings.header_type = mode;
    return FX5_ST_OK;
}


fx5_status_t fx5_set_monitoring_timer(
    fx5_context_t *ctx,
    uint16_t timer
    )
{
    fx5_status_t status = fx5_require_acquired(ctx);
    if (status != FX5_ST_OK) return status;

    ctx->network_settings.monitoring_timer = timer;
    return FX5_ST_OK;
}

fx5_status_t fx5_set_request(
    fx5_context_t *ctx,
    fx5_cmd_t command,
    fx5_device_t device,
    uint32_t address,
    uint16_t count
    )
{
    fx5_status_t status = fx5_require_acquired(ctx);
    if (status != FX5_ST_OK) return status;
    if (count == 0u || count > FX5_MAX_VALUE_COUNT) return FX5_ST_ERR_INVALID_COUNT;
    if (address > FX5_MAX_ADDRESS) return FX5_ST_ERR_INVALID_ADDRESS;
    if (!fx5_is_supported_command(command)) return FX5_ST_ERR_UNSUPPORTED;
    if (!fx5_is_supported_device(device)) return FX5_ST_ERR_UNSUPPORTED;

    ctx->command = command;
    ctx->device = device;
    ctx->address = address;
    ctx->count = count;
    ctx->response_code = FX5_RESPONSE_OK;
    ctx->resync_counter = 0u;

    for (uint16_t k = 0u; k < FX5_MAX_VALUE_COUNT; ++k) {
        ctx->values[k] = 0u;
    }

    return FX5_ST_OK;
}

fx5_status_t fx5_set_write_value(
    fx5_context_t *ctx,
    uint16_t index,
    uint16_t value
    )
{
    fx5_status_t status = fx5_require_acquired(ctx);
    if (status != FX5_ST_OK) return status;
    if (ctx->count == 0u) return FX5_ST_ERR_STATE;
    if (index >= ctx->count || index >= FX5_MAX_VALUE_COUNT) return FX5_ST_ERR_INVALID_INDEX;

    ctx->values[index] = value;
    return FX5_ST_OK;
}




fx5_status_t fx5_build_request(
    fx5_context_t *context,
    uint8_t *out_buf,
    uint16_t out_buf_size,
    uint16_t *actual_size
    )
{
    if (out_buf == NULL || actual_size == NULL) return FX5_ST_ERR_NULL;

    fx5_status_t status = fx5_require_acquired(context);
    if (status != FX5_ST_OK) return status;

    status = fx5_validate_request(context);
    if (status != FX5_ST_OK) return status;

    *actual_size = fx5_get_required_request_size(context);
    if (out_buf_size < *actual_size) return FX5_ST_ERR_TOO_SMALL;

    const uint16_t header_size = fx5_get_request_header_size(context->network_settings.header_type);
    const uint16_t pdu_size = (uint16_t)(*actual_size - header_size);

    uint16_t written_header_size = 0u;
    if (context->network_settings.header_type == FX5_4E_HEADER) {
        status = fx5_build_wire_header_4e(
            context,
            out_buf,
            out_buf_size,
            pdu_size,
            &written_header_size
            );
    } else {
        status = fx5_build_wire_header_3e(
            context,
            out_buf,
            out_buf_size,
            pdu_size,
            &written_header_size
            );
    }
    if (status != FX5_ST_OK) return status;

    uint16_t written_pdu_size = 0u;
    if (context->command == FX5_CMD_BATCH_READ) {
        status = fx5_build_pdu_read_request(
            context,
            &out_buf[written_header_size],
            (uint16_t)(out_buf_size - written_header_size),
            &written_pdu_size
            );
    } else {
        status = fx5_build_pdu_write_request(
            context,
            &out_buf[written_header_size],
            (uint16_t)(out_buf_size - written_header_size),
            &written_pdu_size
            );
    }
    if (status != FX5_ST_OK) return status;

    if ((uint16_t)(written_header_size + written_pdu_size) != *actual_size) {
        return FX5_ST_ERR_INTERNAL;
    }

    return FX5_ST_OK;
}

fx5_status_t fx5_feed_response_bytes(
    fx5_context_t *ctx,
    const uint8_t *data,
    uint16_t size
    )
{
    fx5_status_t status = fx5_require_acquired(ctx);
    if (status != FX5_ST_OK) return status;
    if (data == NULL) return FX5_ST_ERR_NULL;

    if ((uint32_t)fx5_ringbuf_size(&ctx->rx_cbuffer) + (uint32_t)size > (uint32_t)FX5_IOBUF_SIZE) {
        return FX5_ST_ERR_TOO_SMALL;
    }

    for (uint16_t k = 0u; k < size; ++k) {
        if (!fx5_ringbuf_push_back(&ctx->rx_cbuffer, data[k])) {
            return FX5_ST_ERR_TOO_SMALL;
        }
    }

    return FX5_ST_OK;
}





fx5_status_t fx5_parse_response(fx5_context_t *context)
{
    fx5_status_t status = fx5_require_acquired(context);
    if (status != FX5_ST_OK) return status;

    fx5_ringbuf_t *rx_buffer = &context->rx_cbuffer;
    context->count = 0u;
    context->resync_counter = 0u;

    while (true) {
        fx5_header_t header_type = fx5_try_resync(rx_buffer, &context->resync_counter);
        if (context->resync_counter > FX5_RESYNC_LIMIT) return FX5_ST_ERR_RESYNC;
        if (header_type == FX5_UNKNOWN_HEADER) return FX5_ST_ERR_NEED_MORE;

        uint16_t packet_size = 0u;
        uint16_t header_size = 0u;

        if (header_type == FX5_4E_HEADER) {
            status = fx5_parse_response_header_4e(context, rx_buffer, &packet_size);
            header_size = FX5_RESP_TCP_HEADER_SIZE_4E;
        } else {
            status = fx5_parse_response_header_3e(context, rx_buffer, &packet_size);
            header_size = FX5_RESP_TCP_HEADER_SIZE_3E;
        }

        if (status == FX5_ST_ERR_NEED_MORE) return status;
        if (status == FX5_ST_ERR_HEADER_MISMATCH) {
            fx5_ringbuf_drop_front(rx_buffer, 1u);
            context->resync_counter++;
            if (context->resync_counter > FX5_RESYNC_LIMIT) return FX5_ST_ERR_RESYNC;
            continue;
        }
        if (status != FX5_ST_OK) return status;

        const uint32_t total_needed = (uint32_t)header_size + (uint32_t)packet_size;
        if ((uint32_t)fx5_ringbuf_size(rx_buffer) < total_needed) return FX5_ST_ERR_NEED_MORE;

        fx5_ringbuf_drop_front(rx_buffer, header_size);
        return fx5_parse_response_pdu(context, rx_buffer, packet_size);
    }
}

uint16_t fx5_get_response_code(const fx5_context_t *ctx)
{
    if (!fx5_is_acquired(ctx)) return 0u;
    return ctx->response_code;
}

uint16_t fx5_get_response_count(const fx5_context_t *ctx)
{
    if (!fx5_is_acquired(ctx)) return 0u;
    return ctx->count;
}

fx5_status_t fx5_get_response_value(
    const fx5_context_t *ctx,
    uint16_t index,
    uint16_t *out_value
    )
{
    fx5_status_t status = fx5_require_acquired(ctx);
    if (status != FX5_ST_OK) return status;
    if (out_value == NULL) return FX5_ST_ERR_NULL;
    if (index >= ctx->count) return FX5_ST_ERR_INVALID_INDEX;

    *out_value = ctx->values[index];
    return FX5_ST_OK;
}

uint16_t fx5_get_last_serial(const fx5_context_t *ctx)
{
    if (!fx5_is_acquired(ctx)) return 0u;
    return ctx->serial;
}
