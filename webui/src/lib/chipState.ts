import type { Chip } from './api'

// TA-237: corrupt state reflects *recent* drops (within 5 min), not the
// lifetime counter. Without this, a single transient telemetry blip pinned
// the chip red until reboot.
export const CORRUPT_WINDOW_S = 300

export type ChipState = 'corrupt' | 'inactive' | 'healthy'

export function getChipState(chip: Pick<Chip, 'last_drop_ago_s' | 'total_ghs'>): ChipState {
  if (chip.last_drop_ago_s != null && chip.last_drop_ago_s < CORRUPT_WINDOW_S) return 'corrupt'
  if (chip.total_ghs < 1) return 'inactive'
  return 'healthy'
}
