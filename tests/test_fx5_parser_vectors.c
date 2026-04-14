#include "unity.h"

#include <stdint.h>
#include <string.h>

#include <fx5/fx5.h>
#include <test_support.h>
#include "test_fx5_vectors.h"

static void assert_parse_ok_and_code_zero(fx5_context_t *ctx)
{
    fx5_status_t st;

    st = fx5_parse_response(ctx);
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);
    TEST_ASSERT_EQUAL_HEX16(0x0000u, fx5_get_response_code(ctx));
}

void test_parse_3e_batch_read_m100_107_bit_response(void)
{    
    fx5_status_t st;
    uint16_t value = 0u;

    TEST_ASSERT_NOT_NULL(g_ctx);

    /*
     * Context request metadata matters for payload interpretation.
     * This response corresponds to:
     *   batch read, bit units, M100..M107
     */
    st = fx5_set_request(g_ctx, FX5_CMD_BATCH_READ, FX5_DEV_M, 100u, 8u);
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    st = fx5_feed_response_bytes(
        g_ctx,
        FX5_VEC_READ_M100_107_BIT_RESP_FRAME_3E,
        (uint16_t)sizeof(FX5_VEC_READ_M100_107_BIT_RESP_FRAME_3E)
    );
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    assert_parse_ok_and_code_zero(g_ctx);

    TEST_ASSERT_EQUAL_UINT16(8u, fx5_get_response_count(g_ctx));

    /* Manual example payload bytes: 00 01 00 11 */
    /* Current library logical bit values are represented as 16-bit 0x00FF / 0x0000 */
    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_get_response_value(g_ctx, 0u, &value));
    TEST_ASSERT_TRUE(value == FX5_BIT_FALSE || value == 0x0000u);

    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_get_response_value(g_ctx, 1u, &value));
    TEST_ASSERT_TRUE(value == FX5_BIT_FALSE || value == 0x0000u);

    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_get_response_value(g_ctx, 2u, &value));
    TEST_ASSERT_TRUE(value == FX5_BIT_FALSE || value == 0x0000u);

    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_get_response_value(g_ctx, 3u, &value));
    TEST_ASSERT_TRUE(value == FX5_BIT_TRUE || value == 0x00FFu);

    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_get_response_value(g_ctx, 4u, &value));
    TEST_ASSERT_TRUE(value == FX5_BIT_FALSE || value == 0x0000u);

    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_get_response_value(g_ctx, 5u, &value));
    TEST_ASSERT_TRUE(value == FX5_BIT_FALSE || value == 0x0000u);

    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_get_response_value(g_ctx, 6u, &value));
    TEST_ASSERT_TRUE(value == FX5_BIT_TRUE || value == 0x00FFu);

    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_get_response_value(g_ctx, 7u, &value));
    TEST_ASSERT_TRUE(value == FX5_BIT_TRUE || value == 0x00FFu);

}


void test_parse_3e_batch_write_ok_response(void)
{    
    fx5_status_t st;

    TEST_ASSERT_NOT_NULL(g_ctx);

    /*
     * Context request shape is still useful even if write response has no data payload.
     */
    st = fx5_set_request(g_ctx, FX5_CMD_BATCH_WRITE, FX5_DEV_D, 100u, 3u);
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    st = fx5_feed_response_bytes(
        g_ctx,
        FX5_VEC_WRITE_OK_RESP_FRAME_3E,
        (uint16_t)sizeof(FX5_VEC_WRITE_OK_RESP_FRAME_3E)
    );
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    assert_parse_ok_and_code_zero(g_ctx);

    TEST_ASSERT_EQUAL_UINT16(0u, fx5_get_response_count(g_ctx));

}

void test_parse_response_need_more_for_partial_3e_header(void)
{    
    fx5_status_t st;

    TEST_ASSERT_NOT_NULL(g_ctx);

    st = fx5_set_request(g_ctx, FX5_CMD_BATCH_READ, FX5_DEV_D, 0u, 1u);
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    /* Only a prefix of a valid 3E response */
    st = fx5_feed_response_bytes(
        g_ctx,
        FX5_VEC_WRITE_OK_RESP_FRAME_3E,
        5u
    );
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    st = fx5_parse_response(g_ctx);
    TEST_ASSERT_EQUAL(FX5_ST_ERR_NEED_MORE, st);

}

void test_parse_response_need_more_for_partial_3e_payload(void)
{    
    fx5_status_t st;

    TEST_ASSERT_NOT_NULL(g_ctx);

    st = fx5_set_request(g_ctx, FX5_CMD_BATCH_READ, FX5_DEV_M, 100u, 8u);
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    /* Full header + truncated payload */
    st = fx5_feed_response_bytes(
        g_ctx,
        FX5_VEC_READ_M100_107_BIT_RESP_FRAME_3E,
        (uint16_t)(sizeof(FX5_VEC_READ_M100_107_BIT_RESP_FRAME_3E) - 1u)
    );
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    st = fx5_parse_response(g_ctx);
    TEST_ASSERT_EQUAL(FX5_ST_ERR_NEED_MORE, st);

}


void test_parse_response_resyncs_after_leading_garbage(void)
{
    fx5_status_t st;
    uint16_t value = 0u;
    uint8_t buf[64];
    size_t pos = 0u;

    TEST_ASSERT_NOT_NULL(g_ctx);

    st = fx5_set_request(g_ctx, FX5_CMD_BATCH_READ, FX5_DEV_M, 100u, 8u);
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    buf[pos++] = 0xAAu;
    buf[pos++] = 0x55u;
    buf[pos++] = 0x13u;
    memcpy(&buf[pos],
           FX5_VEC_READ_M100_107_BIT_RESP_FRAME_3E,
           sizeof(FX5_VEC_READ_M100_107_BIT_RESP_FRAME_3E));
    pos += sizeof(FX5_VEC_READ_M100_107_BIT_RESP_FRAME_3E);

    st = fx5_feed_response_bytes(g_ctx, buf, (uint16_t)pos);
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    assert_parse_ok_and_code_zero(g_ctx);

    TEST_ASSERT_EQUAL_UINT16(8u, fx5_get_response_count(g_ctx));

    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_get_response_value(g_ctx, 3u, &value));
    TEST_ASSERT_TRUE(value == FX5_BIT_TRUE || value == 0x00FFu);

    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_get_response_value(g_ctx, 6u, &value));
    TEST_ASSERT_TRUE(value == FX5_BIT_TRUE || value == 0x00FFu);

    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_get_response_value(g_ctx, 7u, &value));
    TEST_ASSERT_TRUE(value == FX5_BIT_TRUE || value == 0x00FFu);
}