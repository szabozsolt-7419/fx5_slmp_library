/**
 * @file fx5_utility.h
 * @brief Internal byte-level helpers for FX5 frame encoding and decoding.
 *
 * This header contains small inline helpers for writing and reading
 * little-endian values and for peeking or consuming data from the internal
 * circular receive buffer.
 *
 * These helpers are intentionally lightweight and suitable for embedded use.
 */
#ifndef FX5_UTILITY_H
#define FX5_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


#include <fx5/fx5_ringbuf.h>

/** @defgroup fx5_util Internal byte helpers
 *  @brief Endian and circular-buffer helper functions used by the implementation.
 *  @{ */

/** @brief Batch read command code. */
#define PDU_CMD_BATCH_READ   0x0401
/** @brief Batch write command code. */
#define PDU_CMD_BATCH_WRITE  0x1401

/** @brief Word access subcommand. */
#define PDU_SUB_WORD_UNIT    0x0000
/** @brief Bit access subcommand. */
#define PDU_SUB_BIT_UNIT     0x0001

/** @brief Write a 16-bit unsigned integer in little-endian form. */
static inline uint16_t pdu_wr_u16_le(uint8_t *dst, uint16_t v)
{
    dst[0] = (uint8_t)(v & 0xFFu);
    dst[1] = (uint8_t)((v >> 8) & 0xFFu);
    return 2u;
}

/** @brief Write a 24-bit unsigned integer in little-endian form. */
static inline uint16_t pdu_wr_u24_le(uint8_t *dst, uint32_t v)
{
    dst[0] = (uint8_t)(v & 0xFFu);
    dst[1] = (uint8_t)((v >> 8) & 0xFFu);
    dst[2] = (uint8_t)((v >> 16) & 0xFFu);
    return 3u;
}

/** @brief Write a 32-bit unsigned integer in little-endian form. */
static inline uint16_t pdu_wr_u32_le(uint8_t *dst, uint32_t v)
{
    dst[0] = (uint8_t)(v & 0xFFu);
    dst[1] = (uint8_t)((v >> 8) & 0xFFu);
    dst[2] = (uint8_t)((v >> 16) & 0xFFu);
    dst[3] = (uint8_t)((v >> 24) & 0xFFu);

    return 4u;
}

/** @brief Peek a 32-bit little-endian value from a circular buffer without consuming it. */
static inline uint32_t pdu_peekc_u32_le(fx5_ringbuf_t *buffer, uint16_t offset) {
    uint8_t b0 = 0u;
    uint8_t b1 = 0u;
    uint8_t b2 = 0u;
    uint8_t b3 = 0u;

    (void)fx5_ringbuf_peek(buffer, offset + 0u,&b0);
    (void)fx5_ringbuf_peek(buffer, offset + 1u,&b1);
    (void)fx5_ringbuf_peek(buffer, offset + 2u,&b2);
    (void)fx5_ringbuf_peek(buffer, offset + 3u,&b3);

    return (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
}

/** @brief Peek a 16-bit little-endian value from a circular buffer without consuming it. */
static inline uint16_t pdu_peekc_u16_le(fx5_ringbuf_t *buffer, uint16_t offset) {
    uint8_t b0 = 0u;
    uint8_t b1 = 0u;

    (void)fx5_ringbuf_peek(buffer, offset + 0u,&b0);
    (void)fx5_ringbuf_peek(buffer, offset + 1u,&b1);

    return (uint16_t)b0 | ((uint16_t)b1 << 8);
}

/** @brief Read and consume a 16-bit little-endian value from a circular buffer. */
static inline uint16_t pdu_rdc_u16_le(fx5_ringbuf_t *buffer) {
    uint8_t b0 = 0u;
    uint8_t b1 = 0u;

    (void)fx5_ringbuf_peek(buffer, 0u,&b0);
    (void)fx5_ringbuf_peek(buffer, 1u,&b1);

    fx5_ringbuf_drop_front(buffer, 2u);
    return (uint16_t)b0 | ((uint16_t)b1 << 8);
}

/** @brief Read and consume a 24-bit little-endian value from a circular buffer. */
static inline uint32_t pdu_rdc_u24_le(fx5_ringbuf_t *buffer)
{
    uint8_t b0 = 0u;
    uint8_t b1 = 0u;
    uint8_t b2 = 0u;

    (void)fx5_ringbuf_peek(buffer, 0u,&b0);
    (void)fx5_ringbuf_peek(buffer, 1u,&b1);
    (void)fx5_ringbuf_peek(buffer, 2u,&b2);

    fx5_ringbuf_drop_front(buffer, 3u);

    return (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16);
}

/** @brief Read a 16-bit little-endian value from a raw byte buffer. */
static inline uint16_t pdu_rd_u16_le(const uint8_t *src)
{
    return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

/** @brief Read a 24-bit little-endian value from a raw byte buffer. */
static inline uint32_t pdu_rd_u24_le(const uint8_t *src)
{
    return (uint32_t)src[0]
           | ((uint32_t)src[1] << 8)
           | ((uint32_t)src[2] << 16);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* FX5_UTILITY_H */
