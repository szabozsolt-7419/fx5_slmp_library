#include "unity.h"

#include <string.h>
#include <stdint.h>

#include <fx5/fx5.h>
#include <fx5/internal/fx5_private.h>

#include <test_support.h>
#include "test_fx5_vectors.h"

static void assert_bytes_equal(const uint8_t *actual,
                               size_t actual_size,
                               const uint8_t *expected,
                               size_t expected_size)
{
    TEST_ASSERT_EQUAL_UINT32((uint32_t)expected_size, (uint32_t)actual_size);

    for (size_t i = 0; i < expected_size; ++i) {
        TEST_ASSERT_EQUAL_HEX8(expected[i], actual[i]);
    }
}

static void configure_context_from_vector(fx5_context_t *ctx,
                                          const fx5_test_vector_t *vec)
{
    fx5_status_t st;
    fx5_network_settings_t net;


    fx5_network_settings_init(&net);
    net.header_type = FX5_3E_HEADER;


    st = fx5_set_network_settings(ctx, &net);
    TEST_ASSERT_EQUAL(FX5_ST_OK, st);

    if (strcmp(vec->name, "read_m100_107_bit_units") == 0) {
        st = fx5_set_request(ctx, FX5_CMD_BATCH_READ, FX5_DEV_M, 100u, 8u);
        TEST_ASSERT_EQUAL(FX5_ST_OK, st);
        return;
    }


    if (strcmp(vec->name, "write_d100_102_word_units") == 0) {
        st = fx5_set_request(ctx, FX5_CMD_BATCH_WRITE, FX5_DEV_D, 100u, 3u);
        TEST_ASSERT_EQUAL(FX5_ST_OK, st);

        st = fx5_set_write_value(ctx, 0u, 0x1995u);
        TEST_ASSERT_EQUAL(FX5_ST_OK, st);

        st = fx5_set_write_value(ctx, 1u, 0x1202u);
        TEST_ASSERT_EQUAL(FX5_ST_OK, st);

        st = fx5_set_write_value(ctx, 2u, 0x1130u);
        TEST_ASSERT_EQUAL(FX5_ST_OK, st);
        return;
    }

    TEST_FAIL_MESSAGE("Unknown test vector name.");
}

void test_build_full_3e_frames_from_public_api(void)
{
    TEST_ASSERT_NOT_NULL(g_ctx);


    for (size_t i = 0; i < FX5_TEST_VECTOR_COUNT; ++i) {
        const fx5_test_vector_t *vec = &FX5_TEST_VECTORS[i];

        uint8_t frame[FX5_MAX_REQUEST_SIZE];
        uint16_t actual_size = 0u;
        fx5_status_t st;


        TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_reset(g_ctx));

        configure_context_from_vector(g_ctx, vec);

        st = fx5_build_request(g_ctx, frame, (uint16_t)sizeof(frame), &actual_size);
        TEST_ASSERT_EQUAL_MESSAGE(FX5_ST_OK, st, vec->name);

        assert_bytes_equal(frame,
                           actual_size,
                           vec->request_frame_3e,
                           vec->request_frame_3e_size);

    }
}

void test_build_pdus_from_internal_api(void)
{
    TEST_ASSERT_NOT_NULL(g_ctx);


    for (size_t i = 0; i < FX5_TEST_VECTOR_COUNT; ++i) {
        const fx5_test_vector_t *vec = &FX5_TEST_VECTORS[i];

        uint8_t pdu[128];
        uint16_t actual_size = 0u;
        fx5_status_t st;


        TEST_ASSERT_EQUAL(FX5_ST_OK, fx5_reset(g_ctx));

        configure_context_from_vector(g_ctx, vec);

        if (strstr(vec->name, "write_") == vec->name) {
            st = fx5_build_pdu_write_request(g_ctx, pdu, (uint16_t)sizeof(pdu), &actual_size);
        } else {
            st = fx5_build_pdu_read_request(g_ctx, pdu, (uint16_t)sizeof(pdu), &actual_size);
        }

        TEST_ASSERT_EQUAL_MESSAGE(FX5_ST_OK, st, vec->name);

        assert_bytes_equal(pdu,
                           actual_size,
                           vec->request_pdu,
                           vec->request_pdu_size);

    }
}