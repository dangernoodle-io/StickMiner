#include <math.h>
#include "pll.h"

void pll_calc(float target_mhz, uint16_t fb_min, uint16_t fb_max, pll_params_t *out) {
    float best_diff = 1e9f;
    float best_vco = 0.0f;
    uint16_t best_product = UINT16_MAX;

    pll_params_t best = {0};

    // Iterate: refdiv = 2 down to 1
    for (uint8_t refdiv = 2; refdiv >= 1; refdiv--) {
        // post1 = 7 down to 1
        for (uint8_t post1 = 7; post1 >= 1; post1--) {
            // post2 = 7 down to 1
            for (uint8_t post2 = 7; post2 >= 1; post2--) {
                // Constraint: post1 > post2 (skip if post1 <= post2, EXCEPT when both are 1)
                if (post1 <= post2 && !(post1 == 1 && post2 == 1)) {
                    continue;
                }

                // Compute fb_div
                float fb_div_float = target_mhz * refdiv * post1 * post2 / PLL_REF_MHZ;
                uint16_t fb_div = (uint16_t)roundf(fb_div_float);

                // Clamp fb_div to [fb_min, fb_max]
                if (fb_div < fb_min || fb_div > fb_max) {
                    continue;
                }

                // Calculate actual frequency and VCO
                float actual = PLL_REF_MHZ * fb_div / (refdiv * post1 * post2);
                float vco = PLL_REF_MHZ * fb_div / refdiv;

                // Calculate frequency difference
                float freq_diff = fabsf(actual - target_mhz);

                // Calculate postdiv product
                uint16_t product = post1 * post2;

                // Selection priority:
                // 1. Smallest |actual - target|
                // 2. If freq diff tied (within 0.001f): lowest VCO
                // 3. If VCO also tied: lowest post1*post2 product
                int is_better = 0;

                if (freq_diff < best_diff - 0.001f) {
                    // Frequency difference is clearly better
                    is_better = 1;
                } else if (fabsf(freq_diff - best_diff) <= 0.001f) {
                    // Frequency difference is tied
                    if (vco < best_vco - 0.001f) {
                        is_better = 1;
                    } else if (fabsf(vco - best_vco) <= 0.001f) {
                        // VCO is tied, check product
                        if (product < best_product) {
                            is_better = 1;
                        }
                    }
                }

                if (is_better) {
                    best_diff = freq_diff;
                    best_vco = vco;
                    best_product = product;
                    best.fb_div = fb_div;
                    best.refdiv = refdiv;
                    best.post1 = post1;
                    best.post2 = post2;
                    best.actual_mhz = actual;
                }
            }
        }
    }

    *out = best;
}

uint8_t pll_vdo_scale(const pll_params_t *p) {
    float vco = PLL_REF_MHZ * p->fb_div / p->refdiv;
    return (vco >= 2400.0f) ? 0x50 : 0x40;
}

uint8_t pll_postdiv_byte(const pll_params_t *p) {
    return ((p->post1 - 1) << 4) | (p->post2 - 1);
}
