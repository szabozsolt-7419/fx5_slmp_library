#include "unity.h"

#include <stdint.h>
#include <string.h>

#include <fx5/fx5.h>
#include <test_support.h>

static void make_default_3e_settings(fx5_network_settings_t *net)
{
    fx5_network_settings_init(net);
    net->header_type = FX5_3E_HEADER;
}

void test_build_request_rejects_invalid_address(void)
{    
    fx5_status_t st;
    uint8_t buf[FX5_MAX_REQUEST_SIZE];
    uint16_t actual_size = 0u;
    fx5_network_settings_t net;

    TEST_ASSERT_NOT_NULL(g_ctx);

    make_default_3e_settings(&net);
    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_set_network_settings(g_ctx, &net));

    st = fx5_set_request(g_ctx, FX5_CMD_BATCH_READ, FX5_DEV_D, 0x01000000u, 1u);
    TEST_ASSERT_EQUAL(FX5_ST_ERR_INVALID_ADDRESS, st);

    /* Defensive: even if someone ignores set_request result, build must not succeed */
    st = fx5_build_request(g_ctx, buf, (uint16_t)sizeof(buf), &actual_size);
    TEST_ASSERT_TRUE(st != FX5_ST_OK);

}

void test_build_request_rejects_zero_count(void)
{    
    fx5_status_t st;

    TEST_ASSERT_NOT_NULL(g_ctx);

    st = fx5_set_request(g_ctx, FX5_CMD_BATCH_READ, FX5_DEV_D, 0u, 0u);
    TEST_ASSERT_EQUAL(FX5_ST_ERR_INVALID_COUNT, st);

}

void test_set_write_value_rejects_out_of_range_index(void)
{    
    fx5_status_t st;
    fx5_network_settings_t net;

    TEST_ASSERT_NOT_NULL(g_ctx);

    make_default_3e_settings(&net);
    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_set_network_settings(g_ctx, &net));
    TEST_ASSERT_EQUAL(FX5_ST_OK,
                      fx5_set_request(g_ctx, FX5_CMD_BATCH_WRITE, FX5_DEV_D, 0u, 1u));

    st = fx5_set_write_value(g_ctx, 1u, 0x1234u);
    TEST_ASSERT_EQUAL(FX5_ST_ERR_INVALID_INDEX, st);
}

void test_build_request_rejects_too_small_output_buffer(void)
{    
    fx5_status_t st;
    uint8_t buf[4];
    uint16_t actual_size = 0u;
    fx5_network_settings_t net;

    TEST_ASSERT_NOT_NULL(g_ctx);

    make_default_3e_settings(&net);
    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_set_network_settings(g_ctx, &net));
    TEST_ASSERT_EQUAL(FX5_ST_OK,
                      fx5_set_request(g_ctx, FX5_CMD_BATCH_READ, FX5_DEV_D, 0u, 1u));

    st = fx5_build_request(g_ctx, buf, (uint16_t)sizeof(buf), &actual_size);
    TEST_ASSERT_EQUAL(FX5_ST_ERR_TOO_SMALL, st); 
}

void test_build_request_accepts_extended_fx5_devices(void)
{
    static const fx5_device_t devices[] = {
        FX5_DEV_L,
        FX5_DEV_F,
        FX5_DEV_S,
        FX5_DEV_B,
        FX5_DEV_W,
        FX5_DEV_SM,
        FX5_DEV_SD,
        FX5_DEV_SB,
        FX5_DEV_SW
    };
    uint8_t buf[FX5_MAX_REQUEST_SIZE];
    uint16_t actual_size = 0u;
    fx5_network_settings_t net;

    TEST_ASSERT_NOT_NULL(g_ctx);

    make_default_3e_settings(&net);
    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_set_network_settings(g_ctx, &net));

    for (size_t i = 0u; i < sizeof(devices) / sizeof(devices[0]); ++i) {
        TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_reset(g_ctx));
        TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_set_network_settings(g_ctx, &net));
        TEST_ASSERT_EQUAL(FX5_ST_OK,
                          fx5_set_request(g_ctx, FX5_CMD_BATCH_READ, devices[i], 0u, 1u));
        TEST_ASSERT_EQUAL(FX5_ST_OK,
                          fx5_build_request(g_ctx, buf, (uint16_t)sizeof(buf), &actual_size));
    }
}

void test_parse_response_returns_error_on_nonzero_end_code(void)
{    
    fx5_status_t st;

    /*
     * 3E binary response:
     *   D0 00 00 FF FF 03 00 02 00 51 C0
     * response data length = 2 bytes
     * end code = C051h (non-zero -> error response)
     */
    static const uint8_t error_response_3e[] = {
        0xD0, 0x00,
        0x00,
        0xFF,
        0xFF, 0x03,
        0x00,
        0x02, 0x00,
        0x51, 0xC0
    };

    TEST_ASSERT_NOT_NULL(g_ctx);

    TEST_ASSERT_EQUAL(FX5_ST_OK,
                      fx5_set_request(g_ctx, FX5_CMD_BATCH_READ, FX5_DEV_D, 0u, 1u));

    st = fx5_feed_response_bytes(g_ctx,
                                 error_response_3e,
                                 (uint16_t)sizeof(error_response_3e));
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    st = fx5_parse_response(g_ctx);
    TEST_ASSERT_EQUAL(FX5_ST_RESPONSE_ERROR, st);
    TEST_ASSERT_EQUAL_HEX16(0xC051u, fx5_get_response_code(g_ctx));

}

void test_parse_bit_response_keeps_requested_odd_count(void)
{
    static const uint8_t read_one_bit_3e[] = {
        0xD0, 0x00,
        0x00,
        0xFF,
        0xFF, 0x03,
        0x00,
        0x03, 0x00,
        0x00, 0x00,
        0x10
    };

    TEST_ASSERT_NOT_NULL(g_ctx);

    TEST_ASSERT_EQUAL(FX5_ST_OK,
                      fx5_set_request(g_ctx, FX5_CMD_BATCH_READ, FX5_DEV_M, 100u, 1u));

    TEST_ASSERT_EQUAL(FX5_ST_OK,
                      fx5_feed_response_bytes(g_ctx,
                                              read_one_bit_3e,
                                              (uint16_t)sizeof(read_one_bit_3e)));
    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_parse_response(g_ctx));
    TEST_ASSERT_EQUAL_UINT16(1u, fx5_get_response_count(g_ctx));

    {
        uint16_t value = 0u;
        TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_get_response_value(g_ctx, 0u, &value));
        TEST_ASSERT_EQUAL_UINT16(FX5_BIT_TRUE, value);
        TEST_ASSERT_EQUAL(FX5_ST_ERR_INVALID_INDEX, fx5_get_response_value(g_ctx, 1u, &value));
    }
}

void test_parse_response_resync_limit_exceeded(void)
{

    fx5_status_t st;
    uint8_t garbage[FX5_RESYNC_LIMIT + 8u];

    memset(garbage, 0xAA, sizeof(garbage));

    TEST_ASSERT_NOT_NULL(g_ctx);

    TEST_ASSERT_EQUAL(FX5_ST_OK,
                      fx5_set_request(g_ctx, FX5_CMD_BATCH_READ, FX5_DEV_D, 0u, 1u));

    st = fx5_feed_response_bytes(g_ctx, garbage, (uint16_t)sizeof(garbage));
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    st = fx5_parse_response(g_ctx);
    TEST_ASSERT_EQUAL(FX5_ST_ERR_RESYNC, st);

}

void test_feed_response_bytes_rejects_null_data(void)
{

    fx5_status_t st;

    TEST_ASSERT_NOT_NULL(g_ctx);

    st = fx5_feed_response_bytes(g_ctx, NULL, 10u);
    TEST_ASSERT_EQUAL(FX5_ST_ERR_NULL, st);

}

void test_get_response_value_rejects_invalid_index(void)
{    
    fx5_status_t st;
    uint16_t value = 0u;

    /*
     * Valid 3E write response with end code only
     */
    static const uint8_t write_ok_3e[] = {
        0xD0, 0x00,
        0x00,
        0xFF,
        0xFF, 0x03,
        0x00,
        0x02, 0x00,
        0x00, 0x00
    };

    TEST_ASSERT_NOT_NULL(g_ctx);

    TEST_ASSERT_EQUAL(FX5_ST_OK,
                      fx5_set_request(g_ctx, FX5_CMD_BATCH_WRITE, FX5_DEV_D, 100u, 1u));

    TEST_ASSERT_EQUAL(FX5_ST_OK,
                      fx5_feed_response_bytes(g_ctx, write_ok_3e, (uint16_t)sizeof(write_ok_3e)));
    TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_parse_response(g_ctx));

    TEST_ASSERT_EQUAL_UINT16(0u, fx5_get_response_count(g_ctx));

    st = fx5_get_response_value(g_ctx, 0u, &value);
    TEST_ASSERT_EQUAL(FX5_ST_ERR_INVALID_INDEX, st);

}
