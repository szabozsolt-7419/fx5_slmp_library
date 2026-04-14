/**
 * @file fx5_private.h
 * @brief Internal API for FX5 request building and response parsing.
 *
 * This header defines the internal contracts used by the FX5 library
 * implementation and by low-level unit tests.
 *
 * It is intentionally more detailed than the public API in fx5.h and exposes
 * lower-level building blocks for:
 * - request validation and sizing
 * - 3E/4E wire header construction
 * - PDU generation
 * - response header detection and resynchronization
 * - response payload parsing
 *
 * This header is not intended for normal application use.
 */

#ifndef FX5_PRIVATE_H
#define FX5_PRIVATE_H

#include <fx5/fx5.h>

#include <fx5/fx5_ringbuf.h>

#include <stdbool.h>
#include <stdint.h>

/** @defgroup fx5_internal Internal API
 *  @brief Internal helpers used by the implementation and low-level tests.
 *  @{ */

/* --------------------------------------------------------------------------
 * Internal frame constants
 * -------------------------------------------------------------------------- */


/** @brief 4E binary request subheader. */
#define FX5_TCP_REQUEST_SUBHEADER_4E_BINARY  (0x0054u)
/** @brief 4E binary response subheader. */
#define FX5_TCP_RESPONSE_SUBHEADER_4E_BINARY (0x00D4u)
/** @brief 3E binary request subheader. */
#define FX5_TCP_REQUEST_SUBHEADER_3E_BINARY  (0x0050u)
/** @brief 3E binary response subheader. */
#define FX5_TCP_RESPONSE_SUBHEADER_3E_BINARY (0x00D0u)

/** @brief Offset of the response magic number. */
#define FX5_MAGIC_NUMBER_OFFSET              (0u)
/** @brief Offset of the 4E serial field. */
#define FX5_SERIAL_OFFSET_4E                 (2u)
/** @brief Offset of the packet size field in 4E responses. */
#define FX5_PACKET_SIZE_OFFSET_4E            (11u)
/** @brief Offset of the packet size field in 3E responses. */
#define FX5_PACKET_SIZE_OFFSET_3E            (7u)


/** @brief Maximum payload size stored in the internal transaction buffer. */
#define FX5_TX_MAX_PAYLOAD_BYTES             ((uint16_t)(FX5_MAX_VALUE_COUNT * 2u))
/** @brief Internal transaction buffer size. */
#define FX5_IOBUF_SIZE     ((uint16_t)(FX5_REQ_TCP_HEADER_SIZE_4E + FX5_REQ_PDU_HEADER_SIZE + FX5_TX_MAX_PAYLOAD_BYTES + FX5_IOBUF_GUARD_BYTES))

/* --------------------------------------------------------------------------
 * Internal context definition
 * -------------------------------------------------------------------------- */

/**
 * @brief Internal FX5 context representation.
 *
 * The public header exposes this type as opaque. The implementation keeps the
 * full layout private so that request state, response state, routing settings,
 * and the internal receive buffer can evolve without changing the public API.
 */
struct fx5_context {
    bool in_use;                            /**< True if the context is acquired from the internal pool. */

    fx5_network_settings_t network_settings;/**< Outgoing request routing/header settings. */

    uint16_t next_serial;                   /**< Next 4E serial number to assign. */
    uint16_t serial;                        /**< Last transmitted 4E serial number. */
    uint16_t resync_counter;                /**< Number of bytes discarded during resynchronization. */

    fx5_device_t device;                    /**< Active request device type. */
    fx5_cmd_t command;                      /**< Active request command. */
    uint32_t address;                       /**< Active request base address. */
    uint16_t count;                         /**< Logical request or response value count. */

    uint16_t response_code;                 /**< Last parsed PLC response code. */
    uint16_t values[FX5_MAX_VALUE_COUNT];   /**< Shared logical value array for request and response data. */

    uint8_t buffer[FX5_IOBUF_SIZE];         /**< Internal receive buffer storage. */
    fx5_ringbuf_t rx_cbuffer;                     /**< Circular buffer view over @ref buffer. */
};

/* --------------------------------------------------------------------------
 * Internal context helpers
 * -------------------------------------------------------------------------- */

/**
 * @ingroup fx5_internal
 * @brief Initialize network settings to library defaults.
 *
 * The defaults currently select 4E binary requests and local CPU routing.
 *
 * @param[out] settings Settings structure to initialize.
 */
void fx5_network_settings_init_internal(
    fx5_network_settings_t *settings
    );

/**
 * @ingroup fx5_internal
 * @brief Reset an internal context to its default state.
 *
 * This clears transaction state, response state, logical values, and resets
 * the circular receive buffer while preserving the acquisition flag.
 *
 * @param[in,out] ctx Context to reset.
 */
void fx5_context_reset_internal(
    fx5_context_t *ctx
    );

/* --------------------------------------------------------------------------
 * Internal request validation and sizing
 * -------------------------------------------------------------------------- */

/** @ingroup fx5_internal @brief Return whether the command is supported by the library. */
bool fx5_is_supported_command(
    fx5_cmd_t command
    );

/** @ingroup fx5_internal @brief Return whether the device is supported by the library. */
bool fx5_is_supported_device(
    fx5_device_t device
    );

/** @ingroup fx5_internal @brief Return whether the device is bit-oriented. */
bool fx5_is_bit_device(
    fx5_device_t device
    );

/** @ingroup fx5_internal @brief Return the request wire header size for the selected header type. */
uint16_t fx5_get_request_header_size(
    fx5_header_t header_type
    );

/** @ingroup fx5_internal @brief Return the response wire header size for the selected header type. */
uint16_t fx5_get_response_header_size(
    fx5_header_t header_type
    );

/**
 * @ingroup fx5_internal
 * @brief Compute the total request size for the currently configured transaction.
 *
 * @param[in] ctx Active context.
 * @return Total frame size in bytes, or zero if @p ctx is NULL.
 */
uint16_t fx5_get_required_request_size(
    const fx5_context_t *ctx
    );

/**
 * @ingroup fx5_internal
 * @brief Validate the currently configured request.
 *
 * @param[in] ctx Active context.
 * @return FX5_ST_OK if the request is valid, otherwise an error code.
 */
fx5_status_t fx5_validate_request(
    const fx5_context_t *ctx
    );

/* --------------------------------------------------------------------------
 * Internal request builders
 * -------------------------------------------------------------------------- */

/**
 * @ingroup fx5_internal
 * @brief Build a 3E binary wire header.
 *
 * @param[in,out] ctx Active context.
 * @param[out] buffer Destination buffer.
 * @param[in] buffer_size Size of @p buffer in bytes.
 * @param[in] payload_size Size of the PDU that follows the wire header.
 * @param[out] written_size Receives the number of bytes written.
 * @return FX5_ST_OK on success, otherwise an error code.
 */
fx5_status_t fx5_build_wire_header_3e(
    fx5_context_t *ctx,
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t payload_size,
    uint16_t *written_size
    );

/**
 * @ingroup fx5_internal
 * @brief Build a 4E binary wire header.
 *
 * On success the context serial number is updated to match the generated
 * request frame.
 *
 * @param[in,out] ctx Active context.
 * @param[out] buffer Destination buffer.
 * @param[in] buffer_size Size of @p buffer in bytes.
 * @param[in] payload_size Size of the PDU that follows the wire header.
 * @param[out] written_size Receives the number of bytes written.
 * @return FX5_ST_OK on success, otherwise an error code.
 */
fx5_status_t fx5_build_wire_header_4e(
    fx5_context_t *ctx,
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t payload_size,
    uint16_t *written_size
    );

/**
 * @ingroup fx5_internal
 * @brief Build the request PDU for a batch-read transaction.
 */
fx5_status_t fx5_build_pdu_read_request(
    const fx5_context_t *ctx,
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t *written_size
    );

/**
 * @ingroup fx5_internal
 * @brief Build the request PDU for a batch-write transaction.
 */
fx5_status_t fx5_build_pdu_write_request(
    const fx5_context_t *ctx,
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t *written_size
    );

/* --------------------------------------------------------------------------
 * Internal payload conversion helpers
 * -------------------------------------------------------------------------- */

/**
 * @ingroup fx5_internal
 * @brief Pack logical bit values into little-endian response/request words.
 *
 * The input array uses the library bit representation values in 16-bit slots.
 *
 * @return Number of bytes written on success, or a negative value on error.
 */
/*int fx5_pack_bits_to_words_le(
    const uint16_t *values,
    uint16_t count,
    uint8_t *buffer,
    uint16_t buffer_size
    );*/

/**
 * @ingroup fx5_internal
 * @brief Parse a bit-device response payload into the context value array.
 */
fx5_status_t fx5_parse_bit_response_payload(
    fx5_context_t *ctx,
    fx5_ringbuf_t *rx_buffer,
    uint16_t payload_size
    );

/**
 * @ingroup fx5_internal
 * @brief Parse a word-device response payload into the context value array.
 */
fx5_status_t fx5_parse_word_response_payload(
    fx5_context_t *ctx,
    fx5_ringbuf_t *rx_buffer,
    uint16_t payload_size
    );

/* --------------------------------------------------------------------------
 * Internal response parsing
 * -------------------------------------------------------------------------- */

/**
 * @ingroup fx5_internal
 * @brief Detect the response header type currently visible at the front of the RX buffer.
 *
 * @return FX5_3E_HEADER, FX5_4E_HEADER, or FX5_UNKNOWN_HEADER.
 */
fx5_header_t fx5_detect_response_header(
    fx5_ringbuf_t *rx_buffer
    );

/**
 * @ingroup fx5_internal
 * @brief Resynchronize the RX buffer to the next detectable response frame.
 *
 * @param[in,out] rx_buffer Circular receive buffer.
 * @param[out] discarded_bytes Number of bytes discarded during resynchronization.
 * @return Detected header type or FX5_UNKNOWN_HEADER if more data is required.
 */
fx5_header_t fx5_try_resync(
    fx5_ringbuf_t *rx_buffer,
    uint16_t *discarded_bytes
    );

/**
 * @ingroup fx5_internal
 * @brief Parse a 3E response wire header and extract the packet size.
 */
fx5_status_t fx5_parse_response_header_3e(
    fx5_context_t *ctx,
    fx5_ringbuf_t *rx_buffer,
    uint16_t *packet_size
    );

/**
 * @ingroup fx5_internal
 * @brief Parse a 4E response wire header and extract the packet size.
 */
fx5_status_t fx5_parse_response_header_4e(
    fx5_context_t *ctx,
    fx5_ringbuf_t *rx_buffer,
    uint16_t *packet_size
    );

/**
 * @ingroup fx5_internal
 * @brief Parse a response PDU after the wire header has been validated.
 */
fx5_status_t fx5_parse_response_pdu(
    fx5_context_t *ctx,
    fx5_ringbuf_t *rx_buffer,
    uint16_t packet_size
    );

/** @} */

#endif /* FX5_PRIVATE_H */
