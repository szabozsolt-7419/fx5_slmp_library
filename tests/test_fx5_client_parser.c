#include "unity.h"

#include "fx5_client_parser.h"

void test_client_parser_accepts_documented_short_range(void)
{
    fx5_client_command_t cmd;

    TEST_ASSERT_TRUE(fx5_client_parse_command("get D100-110", &cmd));
    TEST_ASSERT_EQUAL(FX5_CLIENT_CMD_GET, cmd.type);
    TEST_ASSERT_EQUAL(FX5_DEV_D, cmd.device);
    TEST_ASSERT_EQUAL_UINT32(100u, cmd.start_address);
    TEST_ASSERT_EQUAL_UINT16(11u, cmd.count);
}

void test_client_parser_accepts_redundant_device_range(void)
{
    fx5_client_command_t cmd;

    TEST_ASSERT_TRUE(fx5_client_parse_command("get D100-D110", &cmd));
    TEST_ASSERT_EQUAL(FX5_CLIENT_CMD_GET, cmd.type);
    TEST_ASSERT_EQUAL(FX5_DEV_D, cmd.device);
    TEST_ASSERT_EQUAL_UINT32(100u, cmd.start_address);
    TEST_ASSERT_EQUAL_UINT16(11u, cmd.count);
}

void test_client_parser_accepts_bit_value_list(void)
{
    fx5_client_command_t cmd;

    TEST_ASSERT_TRUE(fx5_client_parse_command(
        "set M51-56=true,false,true,false,true,true",
        &cmd));
    TEST_ASSERT_EQUAL(FX5_CLIENT_CMD_SET, cmd.type);
    TEST_ASSERT_EQUAL(FX5_DEV_M, cmd.device);
    TEST_ASSERT_EQUAL_UINT32(51u, cmd.start_address);
    TEST_ASSERT_EQUAL_UINT16(6u, cmd.count);
    TEST_ASSERT_EQUAL_UINT16(6u, cmd.value_count);
    TEST_ASSERT_EQUAL_UINT16(1u, cmd.values[0]);
    TEST_ASSERT_EQUAL_UINT16(0u, cmd.values[1]);
    TEST_ASSERT_EQUAL_UINT16(1u, cmd.values[2]);
    TEST_ASSERT_EQUAL_UINT16(0u, cmd.values[3]);
    TEST_ASSERT_EQUAL_UINT16(1u, cmd.values[4]);
    TEST_ASSERT_EQUAL_UINT16(1u, cmd.values[5]);
}

void test_client_parser_rejects_mixed_device_range(void)
{
    fx5_client_command_t cmd;

    TEST_ASSERT_FALSE(fx5_client_parse_command("get D100-M110", &cmd));
}
