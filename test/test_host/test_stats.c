#include "unity.h"
#include "mining.h"
#include "work.h"
#include <string.h>
#include <math.h>

void test_ema_seeds_on_first_sample(void)
{
    hashrate_ema_t ema = {0};
    mining_stats_update_ema(&ema, 1000.0, 100);
    TEST_ASSERT_EQUAL_DOUBLE(1000.0, ema.value);
    TEST_ASSERT_EQUAL_INT64(100, ema.last_us);
}

void test_ema_converges(void)
{
    hashrate_ema_t ema = {0};
    for (int i = 0; i < 20; i++) {
        mining_stats_update_ema(&ema, 1000.0, (int64_t)i * 5000000);
    }
    // After 20 samples of constant 1000.0, EMA should be very close
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 1000.0, ema.value);
}

void test_ema_decay(void)
{
    hashrate_ema_t ema = {0};
    mining_stats_update_ema(&ema, 1000.0, 0);  // seed at 1000
    for (int i = 1; i <= 15; i++) {
        mining_stats_update_ema(&ema, 0.0, (int64_t)i * 5000000);
    }
    // After 15 samples of 0.0: 1000 * 0.8^15 ≈ 35
    TEST_ASSERT_TRUE(ema.value < 50.0);
}

void test_hash_to_difficulty_leading_zeros(void)
{
    // All-zero leading 4 bytes → capped at 1e15
    uint8_t hash[32] = {0};
    hash[4] = 0xFF;  // non-zero after leading 4
    double diff = hash_to_difficulty(hash);
    TEST_ASSERT_EQUAL_DOUBLE(1e15, diff);
}

void test_hash_to_difficulty_diff1(void)
{
    // Leading bytes 0x0000FFFF → diff ≈ 65536
    // 0xFFFF0000 / 0x0000FFFF ≈ 65536.5
    uint8_t hash[32] = {0};
    hash[0] = 0x00;
    hash[1] = 0x00;
    hash[2] = 0xFF;
    hash[3] = 0xFF;
    double diff = hash_to_difficulty(hash);
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 65536.0, diff);
}

void test_hash_to_difficulty_easy(void)
{
    // Leading byte 0xFF → very low difficulty
    // 0xFFFF0000 / 0xFF000000 = ~1.004
    uint8_t hash[32];
    memset(hash, 0xFF, 32);
    double diff = hash_to_difficulty(hash);
    // 0xFFFF0000 / 0xFFFFFFFF ≈ 1.0
    TEST_ASSERT_TRUE(diff < 2.0);
    TEST_ASSERT_TRUE(diff > 0.0);
}

void test_best_diff_only_increases(void)
{
    // Simulate best_diff tracking logic
    mining_lifetime_t lt = {0};

    // First share: diff_exp = 10
    double share_diff1 = 1024.0;  // log2(1024) = 10
    uint32_t exp1 = (uint32_t)(log2(share_diff1));
    if (exp1 > lt.best_diff) lt.best_diff = exp1;
    TEST_ASSERT_EQUAL_UINT32(10, lt.best_diff);

    // Second share: diff_exp = 5 (lower, should not update)
    double share_diff2 = 32.0;  // log2(32) = 5
    uint32_t exp2 = (uint32_t)(log2(share_diff2));
    if (exp2 > lt.best_diff) lt.best_diff = exp2;
    TEST_ASSERT_EQUAL_UINT32(10, lt.best_diff);  // still 10

    // Third share: diff_exp = 15 (higher, should update)
    double share_diff3 = 32768.0;  // log2(32768) = 15
    uint32_t exp3 = (uint32_t)(log2(share_diff3));
    if (exp3 > lt.best_diff) lt.best_diff = exp3;
    TEST_ASSERT_EQUAL_UINT32(15, lt.best_diff);
}
