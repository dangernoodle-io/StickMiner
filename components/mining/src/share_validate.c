#include "share_validate.h"
#include "work.h"

share_verdict_t share_validate(
    const mining_work_t *work,
    const uint8_t        hash[32],
    double              *out_diff)
{
    if (!work || !out_diff) {
        return SHARE_INVALID_TARGET;
    }

    // 1. Target sanity — must come BEFORE meets_target to avoid operating on a corrupt target.
    if (!is_target_valid(work->target)) {
        return SHARE_INVALID_TARGET;
    }

    // 2. Degenerate pool difficulty — catches misconfigured stratum connections.
    if (work->difficulty < 0.001) {
        return SHARE_INVALID_TARGET;
    }

    // 3. Hash vs target comparison.
    if (!meets_target(hash, work->target)) {
        return SHARE_BELOW_TARGET;
    }

    // 4. Compute share difficulty.
    *out_diff = hash_to_difficulty(hash);

    // 5. Pool-diff floor: share_diff must be at least half the pool difficulty.
    if (*out_diff < work->difficulty * 0.5) {
        return SHARE_LOW_DIFFICULTY;
    }

    return SHARE_VALID;
}
