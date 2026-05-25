#include "unity.h"

#include <fx5/fx5.h>

fx5_context_t *g_ctx = NULL;

void setUp(void) {
  g_ctx = NULL;
  TEST_ASSERT_EQUAL(FX5_ST_OK,fx5_acquire(&g_ctx));
  TEST_ASSERT_NOT_NULL(g_ctx);
}

void tearDown(void) {
  if (g_ctx != NULL) {
    fx5_release(&g_ctx);
    g_ctx = NULL;
  }
}

void test_build_full_3e_frames_from_public_api(void);
void test_build_pdus_from_internal_api(void);

void test_parse_3e_batch_read_m100_107_bit_response(void);
void test_parse_3e_batch_write_ok_response(void);
void test_parse_response_need_more_for_partial_3e_header(void);
void test_parse_response_need_more_for_partial_3e_payload(void);
void test_parse_response_resyncs_after_leading_garbage(void);

void test_build_request_rejects_invalid_address(void);
void test_build_request_rejects_zero_count(void);
void test_set_write_value_rejects_out_of_range_index(void);
void test_build_request_rejects_too_small_output_buffer(void);
void test_build_request_accepts_extended_fx5_devices(void);
void test_parse_response_returns_error_on_nonzero_end_code(void);
void test_parse_bit_response_keeps_requested_odd_count(void);
void test_parse_response_resync_limit_exceeded(void);
void test_feed_response_bytes_rejects_null_data(void);
void test_get_response_value_rejects_invalid_index(void);
void test_client_parser_accepts_documented_short_range(void);
void test_client_parser_accepts_redundant_device_range(void);
void test_client_parser_accepts_bit_value_list(void);
void test_client_parser_uses_documented_address_bases(void);
void test_client_parser_rejects_mixed_device_range(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_build_full_3e_frames_from_public_api);
    RUN_TEST(test_build_pdus_from_internal_api);

    RUN_TEST(test_parse_3e_batch_read_m100_107_bit_response);    
    RUN_TEST(test_parse_3e_batch_write_ok_response);
    RUN_TEST(test_parse_response_need_more_for_partial_3e_header);
    RUN_TEST(test_parse_response_need_more_for_partial_3e_payload);
    RUN_TEST(test_parse_response_resyncs_after_leading_garbage);

    RUN_TEST(test_build_request_rejects_invalid_address);
    RUN_TEST(test_build_request_rejects_zero_count);
    RUN_TEST(test_set_write_value_rejects_out_of_range_index);
    RUN_TEST(test_build_request_rejects_too_small_output_buffer);
    RUN_TEST(test_build_request_accepts_extended_fx5_devices);
    RUN_TEST(test_parse_response_returns_error_on_nonzero_end_code);
    RUN_TEST(test_parse_bit_response_keeps_requested_odd_count);
    RUN_TEST(test_parse_response_resync_limit_exceeded);
    RUN_TEST(test_feed_response_bytes_rejects_null_data);
    RUN_TEST(test_get_response_value_rejects_invalid_index);
    RUN_TEST(test_client_parser_accepts_documented_short_range);
    RUN_TEST(test_client_parser_accepts_redundant_device_range);
    RUN_TEST(test_client_parser_accepts_bit_value_list);
    RUN_TEST(test_client_parser_uses_documented_address_bases);
    RUN_TEST(test_client_parser_rejects_mixed_device_range);

    return UNITY_END();
}
