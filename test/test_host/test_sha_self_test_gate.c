#include "unity.h"
#include "sha256.h"
#include "mining.h"

// Test: sha256_sw_self_test passes on known NIST vector
// Verifies the self-test function correctly validates SHA-256("abc")
void test_sha256_sw_self_test_passes(void)
{
    bb_err_t result = sha256_sw_self_test();
    TEST_ASSERT_EQUAL_INT(BB_OK, result);
}

// Test: mining_sha_self_test_failed returns false initially
// State is process-static, so this must run first
void test_mining_self_test_flag_default_false(void)
{
    bool failed = mining_sha_self_test_failed();
    TEST_ASSERT_FALSE(failed);
}

// Test: mining_set_sha_self_test_failed flips flag to true
void test_mining_set_self_test_failed_flips_flag(void)
{
    mining_set_sha_self_test_failed();
    bool failed = mining_sha_self_test_failed();
    TEST_ASSERT_TRUE(failed);
}
