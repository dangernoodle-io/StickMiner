#pragma once
#include <stdint.h>

#define PLL_REF_MHZ 25.0f

typedef struct {
    uint16_t fb_div;
    uint8_t  refdiv;
    uint8_t  post1;
    uint8_t  post2;
    float    actual_mhz;
} pll_params_t;

// Find best PLL parameters for target frequency.
// fb_min/fb_max define the feedback divider search range (BM1370: 160–239).
void pll_calc(float target_mhz, uint16_t fb_min, uint16_t fb_max, pll_params_t *out);

// Returns VDO scale byte: 0x50 if VCO >= 2400 MHz, else 0x40.
uint8_t pll_vdo_scale(const pll_params_t *p);

// Encode postdiv1/postdiv2 into single byte: ((post1-1) << 4) | (post2-1).
uint8_t pll_postdiv_byte(const pll_params_t *p);
