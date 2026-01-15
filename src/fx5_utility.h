#ifndef FX5_UTILITY_H
#define FX5_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "../include/buffer.h"

// Command codes
#define PDU_CMD_BATCH_READ   0x0401
#define PDU_CMD_BATCH_WRITE  0x1401

// Subcommand codes
#define PDU_SUB_WORD_UNIT    0x0000
#define PDU_SUB_BIT_UNIT     0x0001   // majd a biteknél



// Fixed sizes
//#define PDU_READ_D_BASE_SIZE   10   // read D words
//#define PDU_WRITE_D_BASE_SIZE  10   // write D words (adat nélkül)


static inline uint16_t pdu_wr_u16_le(uint8_t *dst, uint16_t v)
{
    dst[0] = (uint8_t)(v & 0xFFu);
    dst[1] = (uint8_t)((v >> 8) & 0xFFu);
    return 2u;
}

static inline uint16_t pdu_wr_u24_le(uint8_t *dst, uint32_t v)
{
    dst[0] = (uint8_t)(v & 0xFFu);
    dst[1] = (uint8_t)((v >> 8) & 0xFFu);
    dst[2] = (uint8_t)((v >> 16) & 0xFFu);
    return 3u;
}

static inline uint16_t pdu_wr_u32_le(uint8_t *dst, uint32_t v)
{
    dst[0] = (uint8_t)(v & 0xFFu);
    dst[1] = (uint8_t)((v >> 8) & 0xFFu);
    dst[2] = (uint8_t)((v >> 16) & 0xFFu);
    dst[3] = (uint8_t)((v >> 24) & 0xFFu);

    return 4u;
}

static inline uint32_t pdu_peekc_u32_le(cBuffer *buffer,uint16_t offset) {
    uint8_t b0 = bufferGetAtIndex(buffer,offset + 0);
    uint8_t b1 = bufferGetAtIndex(buffer,offset + 1);
    uint8_t b2 = bufferGetAtIndex(buffer,offset + 2);
    uint8_t b3 = bufferGetAtIndex(buffer,offset + 3);

    return (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 24);
}

static inline uint16_t pdu_peekc_u16_le(cBuffer *buffer,uint16_t offset) {
    uint8_t b0 = bufferGetAtIndex(buffer,offset + 0);
    uint8_t b1 = bufferGetAtIndex(buffer,offset + 1);

    return (uint16_t)b0 | ((uint16_t)b1 << 8);
}

static inline uint16_t pdu_rdc_u16_le(cBuffer *buffer) {
    uint8_t b0 = bufferGetAtIndex(buffer,0);
    uint8_t b1 = bufferGetAtIndex(buffer,1);

    bufferDumpFromFront(buffer,2);
    return (uint16_t)b0 | ((uint16_t)b1 << 8);
}

//ez egy 24 bites egészt ad vissza (3 byte)
static inline uint32_t pdu_rdc_u24_le(cBuffer *buffer)
{
    uint8_t b0 = bufferGetAtIndex(buffer,0);
    uint8_t b1 = bufferGetAtIndex(buffer,1);
    uint8_t b2 = bufferGetAtIndex(buffer,2);

    bufferDumpFromFront(buffer,3);

    return (uint32_t)b0 | ((uint32_t)b1 << 8) | ((uint32_t)b2 << 16);
}

static inline uint16_t pdu_rd_u16_le(const uint8_t *src)
{
    return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}



static inline uint32_t pdu_rd_u24_le(const uint8_t *src)
{
    return (uint32_t)src[0]
           | ((uint32_t)src[1] << 8)
           | ((uint32_t)src[2] << 16);
}




#ifdef __cplusplus
}
#endif

#endif // FX5_UTILITY_H


