import { describe, it, expect } from 'vitest'
import { getChipState, CORRUPT_WINDOW_S } from './chipState'

describe('getChipState (TA-237 self-heal)', () => {
  it('healthy when last_drop_ago_s is null and chip is hashing', () => {
    expect(getChipState({ last_drop_ago_s: null, total_ghs: 1000 })).toBe('healthy')
  })

  it('corrupt when last_drop_ago_s is within the window', () => {
    expect(getChipState({ last_drop_ago_s: 0, total_ghs: 1000 })).toBe('corrupt')
    expect(getChipState({ last_drop_ago_s: CORRUPT_WINDOW_S - 1, total_ghs: 1000 })).toBe('corrupt')
  })

  it('healthy when last_drop_ago_s exceeds the window even if hashing', () => {
    expect(getChipState({ last_drop_ago_s: CORRUPT_WINDOW_S, total_ghs: 1000 })).toBe('healthy')
    expect(getChipState({ last_drop_ago_s: 99999, total_ghs: 1000 })).toBe('healthy')
  })

  it('inactive when not hashing and no recent drop', () => {
    expect(getChipState({ last_drop_ago_s: null, total_ghs: 0 })).toBe('inactive')
    expect(getChipState({ last_drop_ago_s: 99999, total_ghs: 0.5 })).toBe('inactive')
  })

  it('corrupt overrides inactive when recent drop and not hashing', () => {
    expect(getChipState({ last_drop_ago_s: 10, total_ghs: 0 })).toBe('corrupt')
  })
})
