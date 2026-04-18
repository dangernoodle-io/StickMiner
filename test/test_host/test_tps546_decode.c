#include "unity.h"
#include "tps546_decode.h"

void test_ulinear16_typical_vout(void) {
    // exp=-9, raw=597 (0x255): 597 * 2^-9 * 1000 ≈ 1166 mV
    int mv = tps546_ulinear16_to_mv(597, -9);
    TEST_ASSERT_INT_WITHIN(2, 1166, mv);
}

void test_ulinear16_zero(void) {
    int mv = tps546_ulinear16_to_mv(0, -9);
    TEST_ASSERT_EQUAL_INT(0, mv);
}

void test_slinear11_positive_exp_neg(void) {
    // exp=-2, mantissa=125 encoded as raw=0xF07D: 125 * 2^-2 * 1000 = 31250 mA
    int ma = tps546_slinear11_to_ma(0xF07D);
    TEST_ASSERT_INT_WITHIN(5, 31250, ma);
}

void test_slinear11_negative_mantissa(void) {
    // exp=0, mantissa=-1 encoded as raw=0x07FF: -1 * 2^0 * 1000 = -1000 mA
    int ma = tps546_slinear11_to_ma(0x07FF);
    TEST_ASSERT_EQUAL_INT(-1000, ma);
}

void test_slinear11_zero_mantissa(void) {
    // exp=0, mantissa=0: raw=0x0000 → 0 mA
    int ma = tps546_slinear11_to_ma(0x0000);
    TEST_ASSERT_EQUAL_INT(0, ma);
}
