#include "unity.h"
#include "boot_fallback_decision.h"

void test_boot_fallback_below_threshold_no_fallback(void)
{
    // boot_cnt < 3: no fallback regardless of other flags
    TEST_ASSERT_FALSE(should_fall_back_to_ap(0, true, false));
    TEST_ASSERT_FALSE(should_fall_back_to_ap(1, true, false));
    TEST_ASSERT_FALSE(should_fall_back_to_ap(2, true, false));
}

void test_boot_fallback_at_threshold_provisioned_unvalidated_falls_back(void)
{
    // boot_cnt >= 3, provisioned, unvalidated: fallback
    TEST_ASSERT_TRUE(should_fall_back_to_ap(3, true, false));
    TEST_ASSERT_TRUE(should_fall_back_to_ap(4, true, false));
    TEST_ASSERT_TRUE(should_fall_back_to_ap(5, true, false));
}

void test_boot_fallback_at_threshold_provisioned_validated_no_fallback(void)
{
    // boot_cnt >= 3, provisioned, validated: no fallback
    TEST_ASSERT_FALSE(should_fall_back_to_ap(3, true, true));
    TEST_ASSERT_FALSE(should_fall_back_to_ap(4, true, true));
    TEST_ASSERT_FALSE(should_fall_back_to_ap(5, true, true));
}

void test_boot_fallback_at_threshold_unprovisioned_no_fallback(void)
{
    // Not provisioned: never fall back (already in AP mode)
    TEST_ASSERT_FALSE(should_fall_back_to_ap(3, false, false));
    TEST_ASSERT_FALSE(should_fall_back_to_ap(4, false, false));
}
