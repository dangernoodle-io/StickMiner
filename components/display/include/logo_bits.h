#pragma once

#include <stdint.h>

#define LOGO_W 56
#define LOGO_H 56
#define LOGO_BITS_LEN ((LOGO_W * LOGO_H + 7) / 8)
#define LOGO_FG_COLOR 0xFBE0

extern const uint8_t g_logo_bits[LOGO_BITS_LEN];
