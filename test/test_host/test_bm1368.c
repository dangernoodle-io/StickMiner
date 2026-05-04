#ifdef ASIC_CHIP
#include "unity.h"
#include "pll.h"
#include "asic_proto.h"

// --- BM1368 PLL range tests ---

void test_bm1368_pll_fb_range(void) {
    // BM1368 PLL fb_div range: [144, 235]
    pll_params_t pll;
    pll_calc(490.0f, 144, 235, &pll);
    TEST_ASSERT_TRUE(pll.fb_div >= 144 && pll.fb_div <= 235);
}

void test_bm1368_pll_490mhz(void) {
    // BM1368 default frequency: 490 MHz
    pll_params_t pll;
    pll_calc(490.0f, 144, 235, &pll);
    // Verify fb_div is in range
    TEST_ASSERT_TRUE(pll.fb_div >= 144 && pll.fb_div <= 235);
    // Verify actual frequency is close to target
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 490.0f, pll.actual_mhz);
}

void test_bm1368_pll_vdo_scale(void) {
    // Test PLL VDO scaling for a typical BM1368 config
    pll_params_t pll;
    pll_calc(490.0f, 144, 235, &pll);
    uint8_t vdo = pll_vdo_scale(&pll);
    // VDO should be in range [0x40, 0x50] depending on VCO
    TEST_ASSERT_TRUE((vdo == 0x40) || (vdo == 0x50));
}

// --- ASIC ticket mask test (shared with BM1370) ---

void test_asic_ticket_mask_256_bm1368(void) {
    // asic_difficulty_to_mask is a shared function
    // Verify it works for difficulty 256.0
    uint8_t mask[4];
    asic_difficulty_to_mask(256.0, mask);
    // mask should be non-zero
    TEST_ASSERT_TRUE((mask[0] || mask[1] || mask[2] || mask[3]));
}
#endif /* ASIC_CHIP */
