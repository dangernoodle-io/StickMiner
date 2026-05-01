#include "unity.h"
#include "bb_nv.h"
#include "taipan_config.h"
#include <stdbool.h>

// TA-315: autofan setters — clamping + getter round-trip

void test_set_autofan_enabled_round_trip(void)
{
    TEST_ASSERT_EQUAL(0, bb_nv_config_init());
    TEST_ASSERT_EQUAL(0, taipan_config_init());
    TEST_ASSERT_EQUAL(0, taipan_config_set_autofan_enabled(true));
    TEST_ASSERT_TRUE(taipan_config_autofan_enabled());
    TEST_ASSERT_EQUAL(0, taipan_config_set_autofan_enabled(false));
    TEST_ASSERT_FALSE(taipan_config_autofan_enabled());
}

void test_set_temp_target_in_range(void)
{
    bb_nv_config_init();
    taipan_config_init();
    TEST_ASSERT_EQUAL(0, taipan_config_set_temp_target_c(60));
    TEST_ASSERT_EQUAL(60, taipan_config_temp_target_c());
}

void test_set_temp_target_clamps_low(void)
{
    bb_nv_config_init();
    taipan_config_init();
    TEST_ASSERT_EQUAL(0, taipan_config_set_temp_target_c(10));
    TEST_ASSERT_EQUAL(35, taipan_config_temp_target_c());
}

void test_set_temp_target_clamps_high(void)
{
    bb_nv_config_init();
    taipan_config_init();
    TEST_ASSERT_EQUAL(0, taipan_config_set_temp_target_c(200));
    TEST_ASSERT_EQUAL(85, taipan_config_temp_target_c());
}

void test_set_manual_fan_pct_in_range(void)
{
    bb_nv_config_init();
    taipan_config_init();
    TEST_ASSERT_EQUAL(0, taipan_config_set_manual_fan_pct(75));
    TEST_ASSERT_EQUAL(75, taipan_config_manual_fan_pct());
}

void test_set_manual_fan_pct_clamps_high(void)
{
    bb_nv_config_init();
    taipan_config_init();
    TEST_ASSERT_EQUAL(0, taipan_config_set_manual_fan_pct(250));
    TEST_ASSERT_EQUAL(100, taipan_config_manual_fan_pct());
}

void test_set_min_fan_pct_in_range(void)
{
    bb_nv_config_init();
    taipan_config_init();
    TEST_ASSERT_EQUAL(0, taipan_config_set_min_fan_pct(25));
    TEST_ASSERT_EQUAL(25, taipan_config_min_fan_pct());
}

void test_set_min_fan_pct_clamps_high(void)
{
    bb_nv_config_init();
    taipan_config_init();
    TEST_ASSERT_EQUAL(0, taipan_config_set_min_fan_pct(150));
    TEST_ASSERT_EQUAL(100, taipan_config_min_fan_pct());
}
