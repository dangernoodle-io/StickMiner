#pragma once
#include <stdint.h>

static inline int tps546_ulinear16_to_mv(uint16_t raw, int8_t exp_n)
{
    if (exp_n >= 0) {
        return (int)((uint32_t)raw * (1u << exp_n) * 1000u);
    }
    int shift = -exp_n;
    return (int)(((uint32_t)raw * 1000u + (1u << (shift - 1))) >> shift);
}

static inline int tps546_slinear11_to_ma(uint16_t raw)
{
    int exp, mantissa;

    if (raw & 0x8000) {
        exp = -((int)(((~raw >> 11) & 0x1Fu)) + 1);
    } else {
        exp = (int)((raw >> 11) & 0x1Fu);
    }

    if (raw & 0x0400) {
        mantissa = -((int)(~raw & 0x03FFu) + 1);
    } else {
        mantissa = (int)(raw & 0x03FFu);
    }

    if (exp >= 0) {
        return mantissa * (1 << exp) * 1000;
    }
    int shift = -exp;
    int64_t num = (int64_t)mantissa * 1000;
    return (int)((num + (num >= 0 ? (1 << (shift - 1)) : -(1 << (shift - 1)))) >> shift);
}
