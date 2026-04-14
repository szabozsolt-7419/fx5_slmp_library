#ifndef TEST_FX5_VECTORS_H
#define TEST_FX5_VECTORS_H

#include <stddef.h>
#include <stdint.h>

/*
 * Golden vectors derived from:
 * - FX5 User's Manual (SLMP), 3E binary frame examples
 * - Device Read (Batch) examples
 * - Device Write (Batch) examples
 *
 * Notes:
 * - These vectors intentionally focus on unambiguous examples.
 * - Edge cases (partial frames, garbage, malformed lengths, resync stress)
 *   should live in dedicated handwritten tests, not in this table.
 * - All full-frame examples below use 3E binary framing.
 */

typedef struct fx5_test_vector
{
    const char *name;

    const uint8_t *request_pdu;
    size_t request_pdu_size;

    const uint8_t *request_frame_3e;
    size_t request_frame_3e_size;

    const uint8_t *response_frame_3e;
    size_t response_frame_3e_size;
} fx5_test_vector_t;

/* --------------------------------------------------------------------------
 * Common 3E binary framing used by the manual examples
 * -------------------------------------------------------------------------- */

/*
 * Request header fields used by the example frames:
 *   subheader           = 50 00
 *   network no          = 00
 *   pc no               = FF
 *   module I/O no       = FF 03
 *   multidrop station   = 00
 *   request data length = varies
 *   monitoring timer    = 10 00
 *
 * Response header fields used by the example frames:
 *   subheader           = D0 00
 *   network no          = 00
 *   pc no               = FF
 *   module I/O no       = FF 03
 *   multidrop station   = 00
 *   response data len   = varies
 *   end code            = 00 00
 */

/* --------------------------------------------------------------------------
 * 1) Batch Read, binary, bit units
 *    M100 .. M107
 * -------------------------------------------------------------------------- */

static const uint8_t FX5_VEC_READ_M100_107_BIT_REQ_PDU[] = {
    0x01, 0x04,             /* command    = 0401H */
    0x01, 0x00,             /* subcommand = 0001H (bit units) */
    0x64, 0x00, 0x00,       /* head device no = 100 */
    0x90,                   /* device code = M */
    0x08, 0x00              /* number of devices = 8 */
};

static const uint8_t FX5_VEC_READ_M100_107_BIT_REQ_FRAME_3E[] = {
    0x50, 0x00,
    0x00,
    0xFF,
    0xFF, 0x03,
    0x00,
    0x0C, 0x00,             /* request data length = 12 bytes */
    0x10, 0x00,
    0x01, 0x04,
    0x01, 0x00,
    0x64, 0x00, 0x00,
    0x90,
    0x08, 0x00
};

static const uint8_t FX5_VEC_READ_M100_107_BIT_RESP_FRAME_3E[] = {
    0xD0, 0x00,
    0x00,
    0xFF,
    0xFF, 0x03,
    0x00,
    0x06, 0x00,             /* response data length = end code + 4 bytes data */
    0x00, 0x00,
    0x00, 0x01, 0x00, 0x11  /* response data */
};


/* --------------------------------------------------------------------------
 * 4) Batch Write, binary, word units, word device
 *    D100 .. D102
 *    Values: 0x1995, 0x1202, 0x1130
 * -------------------------------------------------------------------------- */

static const uint8_t FX5_VEC_WRITE_D100_102_WORD_REQ_PDU[] = {
    0x01, 0x14,             /* command    = 1401H */
    0x00, 0x00,             /* subcommand = 0000H (word units) */
    0x64, 0x00, 0x00,       /* head device no = 100 */
    0xA8,                   /* device code = D */
    0x03, 0x00,             /* number of devices = 3 */
    0x95, 0x19,             /* D100 = 0x1995 */
    0x02, 0x12,             /* D101 = 0x1202 */
    0x30, 0x11              /* D102 = 0x1130 */
};

static const uint8_t FX5_VEC_WRITE_D100_102_WORD_REQ_FRAME_3E[] = {
    0x50, 0x00,
    0x00,
    0xFF,
    0xFF, 0x03,
    0x00,
    0x12, 0x00,             /* request data length = 18 bytes */
    0x10, 0x00,
    0x01, 0x14,
    0x00, 0x00,
    0x64, 0x00, 0x00,
    0xA8,
    0x03, 0x00,
    0x95, 0x19,
    0x02, 0x12,
    0x30, 0x11
};

static const uint8_t FX5_VEC_WRITE_OK_RESP_FRAME_3E[] = {
    0xD0, 0x00,
    0x00,
    0xFF,
    0xFF, 0x03,
    0x00,
    0x02, 0x00,             /* response data length = end code only */
    0x00, 0x00
};

/* --------------------------------------------------------------------------
 * Vector table
 * -------------------------------------------------------------------------- */

static const fx5_test_vector_t FX5_TEST_VECTORS[] = {
    {
        "read_m100_107_bit_units",
        FX5_VEC_READ_M100_107_BIT_REQ_PDU,
        sizeof(FX5_VEC_READ_M100_107_BIT_REQ_PDU),
        FX5_VEC_READ_M100_107_BIT_REQ_FRAME_3E,
        sizeof(FX5_VEC_READ_M100_107_BIT_REQ_FRAME_3E),
        FX5_VEC_READ_M100_107_BIT_RESP_FRAME_3E,
        sizeof(FX5_VEC_READ_M100_107_BIT_RESP_FRAME_3E)
    },


    {
        "write_d100_102_word_units",
        FX5_VEC_WRITE_D100_102_WORD_REQ_PDU,
        sizeof(FX5_VEC_WRITE_D100_102_WORD_REQ_PDU),
        FX5_VEC_WRITE_D100_102_WORD_REQ_FRAME_3E,
        sizeof(FX5_VEC_WRITE_D100_102_WORD_REQ_FRAME_3E),
        FX5_VEC_WRITE_OK_RESP_FRAME_3E,
        sizeof(FX5_VEC_WRITE_OK_RESP_FRAME_3E)
    }
};

static const size_t FX5_TEST_VECTOR_COUNT =
    sizeof(FX5_TEST_VECTORS) / sizeof(FX5_TEST_VECTORS[0]);

#endif /* TEST_FX5_VECTORS_H */