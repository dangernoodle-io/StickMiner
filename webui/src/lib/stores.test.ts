import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest'
import { startRebootRecovery, rebooting } from './stores'
import { get } from 'svelte/store'

describe('stores', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    vi.useFakeTimers()
  })

  afterEach(() => {
    vi.useRealTimers()
  })

  describe('startRebootRecovery', () => {
    it('sets rebooting.active to true with reason', () => {
      startRebootRecovery('test-reboot')
      const state = get(rebooting)
      expect(state.active).toBe(true)
      expect(state.reason).toBe('test-reboot')
    })

    it('initializes elapsed and timedOut correctly', () => {
      startRebootRecovery('flash')
      const state = get(rebooting)
      expect(state.elapsed).toBe(0)
      expect(state.timedOut).toBe(false)
    })

    it('sets correct initial state structure', () => {
      startRebootRecovery('update')
      const state = get(rebooting)
      expect(state).toHaveProperty('active')
      expect(state).toHaveProperty('reason')
      expect(state).toHaveProperty('elapsed')
      expect(state).toHaveProperty('timedOut')
    })
  })

  describe('start/stop polling', () => {
    it.skip('start() initializes polling with correct 5s interval', () => {
      // Polling requires fetch mocking and is tested via integration
      // This test skipped due to tight coupling with API module
    })

    it.skip('stop() clears polling state', () => {
      // State management test skipped due to module-level state
      // Changes to polling would require refactoring for better testability
    })
  })
})
