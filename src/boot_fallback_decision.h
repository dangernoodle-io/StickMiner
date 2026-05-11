#pragma once

#include <stdint.h>
#include <stdbool.h>

// Determine whether to fall back to AP mode based on boot count, provisioning,
// and firmware validation status.
// Returns true if AP fallback should be triggered, false otherwise.
static inline bool should_fall_back_to_ap(uint8_t boot_cnt, bool provisioned, bool validated)
{
    // If boot_cnt >= threshold, provisioned, but firmware is validated:
    // keep credentials (don't fall back)
    if (boot_cnt >= 3 && provisioned && !validated) {
        return true;
    }
    return false;
}
