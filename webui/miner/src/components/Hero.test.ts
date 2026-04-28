import { describe, it, expect } from 'vitest'
import { fmtHashGhs } from '../lib/fmt'

describe('Hero hashrate formatting', () => {
  describe('fmtHashGhs', () => {
    it('displays dash for NaN hashrate', () => {
      expect(fmtHashGhs(NaN)).toBe('—')
    })

    it('displays dash for undefined hashrate', () => {
      expect(fmtHashGhs(undefined)).toBe('—')
    })

    it('displays kH/s when hashrate < 0.001 GH/s', () => {
      expect(fmtHashGhs(0.0005)).toBe('500.0 kH/s')
      expect(fmtHashGhs(0.0001)).toBe('100.0 kH/s')
    })

    it('displays MH/s when hashrate in 0.001-1 GH/s range', () => {
      expect(fmtHashGhs(0.001)).toBe('1.0 MH/s')
      expect(fmtHashGhs(0.5)).toBe('500.0 MH/s')
    })

    it('displays GH/s when hashrate in 1-1000 GH/s range', () => {
      expect(fmtHashGhs(1)).toBe('1.0 GH/s')
      expect(fmtHashGhs(500)).toBe('500.0 GH/s')
    })

    it('displays TH/s when hashrate >= 1000 GH/s', () => {
      expect(fmtHashGhs(1000)).toBe('1.00 TH/s')
      expect(fmtHashGhs(1200)).toBe('1.20 TH/s')
    })
  })
})
