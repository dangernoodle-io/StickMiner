#include "unity.h"
#include "pll.h"

void test_pll_500mhz(void) {
    pll_params_t p;
    pll_calc(500.0f, 160, 239, &p);
    // 25 * 160 / (2 * 4 * 1) = 500, VCO = 2000 (lower than fb=200 VCO=2500)
    TEST_ASSERT_EQUAL_UINT16(160, p.fb_div);
    TEST_ASSERT_EQUAL_UINT8(2, p.refdiv);
    TEST_ASSERT_EQUAL_UINT8(4, p.post1);
    TEST_ASSERT_EQUAL_UINT8(1, p.post2);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 500.0f, p.actual_mhz);
}

void test_pll_600mhz(void) {
    pll_params_t p;
    pll_calc(600.0f, 160, 239, &p);
    TEST_ASSERT_EQUAL_UINT16(192, p.fb_div);
    TEST_ASSERT_EQUAL_UINT8(2, p.refdiv);
    TEST_ASSERT_EQUAL_UINT8(4, p.post1);
    TEST_ASSERT_EQUAL_UINT8(1, p.post2);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 600.0f, p.actual_mhz);
}

void test_pll_400mhz(void) {
    pll_params_t p;
    pll_calc(400.0f, 160, 239, &p);
    // 25 * 192 / (2 * 6 * 1) = 400
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 400.0f, p.actual_mhz);
}

void test_pll_vdo_scale_high(void) {
    // VCO = 25 * 200 / 2 = 2500 >= 2400
    pll_params_t p = {.fb_div = 200, .refdiv = 2, .post1 = 5, .post2 = 1};
    TEST_ASSERT_EQUAL_HEX8(0x50, pll_vdo_scale(&p));
}

void test_pll_vdo_scale_low(void) {
    // VCO = 25 * 160 / 2 = 2000 < 2400
    pll_params_t p = {.fb_div = 160, .refdiv = 2, .post1 = 5, .post2 = 1};
    TEST_ASSERT_EQUAL_HEX8(0x40, pll_vdo_scale(&p));
}

void test_pll_postdiv_byte(void) {
    // post1=5, post2=1: ((5-1) << 4) | (1-1) = 0x40
    pll_params_t p = {.fb_div = 200, .refdiv = 2, .post1 = 5, .post2 = 1};
    TEST_ASSERT_EQUAL_HEX8(0x40, pll_postdiv_byte(&p));
}
