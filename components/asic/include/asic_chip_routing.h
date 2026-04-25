#pragma once

#include <stdint.h>

// Decode a chip-response addr byte into a chip index for a chain of `chip_count`
// chips. Returns -1 if the address falls outside the valid range.
//
// Why this is a separate helper:
// - 256 / chip_count must be done in 16-bit math: 256/1 overflows uint8_t to 0
//   and divides by zero in the inline form. Easy to get wrong on rewrites.
// - Multi-chip boards (Bitaxe Gamma Duo) routed responses via this division
//   and silicon-routing confusion has burned us before (TA-221, TA-227).
//
// Pure: no side effects.
int asic_chip_routing_index(uint8_t asic_addr, int chip_count);
