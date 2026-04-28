import { describe, it, expect } from 'vitest'
import { fmtHashGhs, fmtBytes, fmtUnixTs, fmtBuildTime, fmtDuration, fmtRelative, fmtTimestamp } from './fmt'

describe('fmt', () => {
  describe('fmtTimestamp', () => {
    it('returns dash for null/undefined', () => {
      expect(fmtTimestamp(null)).toBe('—')
      expect(fmtTimestamp(undefined)).toBe('—')
    })

    it('returns dash for invalid Date', () => {
      expect(fmtTimestamp(new Date('invalid'))).toBe('—')
    })

    it('formats valid Date', () => {
      const d = new Date(2025, 3, 24, 14, 30, 0)
      const result = fmtTimestamp(d)
      expect(result).toMatch(/2025-04-24 \d{2}:\d{2}/)
    })

    it('pads single-digit months/days/hours/minutes', () => {
      const d = new Date(2025, 0, 5, 9, 5, 0)
      const result = fmtTimestamp(d)
      expect(result).toContain('2025-01-05')
      expect(result).toContain('09:05')
    })
  })

  describe('fmtUnixTs', () => {
    it('returns dash for null/undefined', () => {
      expect(fmtUnixTs(null)).toBe('—')
      expect(fmtUnixTs(undefined)).toBe('—')
    })

    it('returns dash for zero and negative', () => {
      expect(fmtUnixTs(0)).toBe('—')
      expect(fmtUnixTs(-100)).toBe('—')
    })

    it('formats positive epoch seconds', () => {
      const ts = 1640995200 // 2022-01-01 00:00:00 UTC
      const result = fmtUnixTs(ts)
      // Verify it returns a formatted date string (timezone-dependent)
      expect(result).toMatch(/\d{4}-\d{2}-\d{2} \d{2}:\d{2}/)
    })
  })

  describe('fmtBuildTime', () => {
    it('returns dash for null/undefined inputs', () => {
      expect(fmtBuildTime(null, 'HH:MM:SS')).toBe('—')
      expect(fmtBuildTime('Mon DD YYYY', null)).toBe('—')
      expect(fmtBuildTime(undefined, '12:34:56')).toBe('—')
      expect(fmtBuildTime('Apr 24 2025', undefined)).toBe('—')
    })

    it('formats valid C build time strings', () => {
      // Note: Date parsing is locale-dependent, so we just verify format
      const result = fmtBuildTime('Apr 24 2025', '14:30:45')
      expect(result).toMatch(/\d{4}-\d{2}-\d{2} \d{2}:\d{2}/)
    })
  })

  describe('fmtDuration', () => {
    it('returns dash for null/undefined/negative', () => {
      expect(fmtDuration(null)).toBe('—')
      expect(fmtDuration(undefined)).toBe('—')
      expect(fmtDuration(-10)).toBe('—')
    })

    it('formats seconds < 60', () => {
      expect(fmtDuration(0)).toBe('0s')
      expect(fmtDuration(45)).toBe('45s')
      expect(fmtDuration(59)).toBe('59s')
    })

    it('formats minutes < 60', () => {
      expect(fmtDuration(60)).toBe('1m 0s')
      expect(fmtDuration(90)).toBe('1m 30s')
      expect(fmtDuration(3599)).toBe('59m 59s')
    })

    it('formats hours < 24', () => {
      expect(fmtDuration(3600)).toBe('1h 0m')
      expect(fmtDuration(5400)).toBe('1h 30m')
      expect(fmtDuration(86399)).toBe('23h 59m')
    })

    it('formats days >= 24', () => {
      expect(fmtDuration(86400)).toBe('1d 0h')
      expect(fmtDuration(90000)).toBe('1d 1h')
      expect(fmtDuration(172800)).toBe('2d 0h')
    })
  })

  describe('fmtRelative', () => {
    it('returns dash for null/undefined/negative', () => {
      expect(fmtRelative(null)).toBe('—')
      expect(fmtRelative(undefined)).toBe('—')
      expect(fmtRelative(-10)).toBe('—')
    })

    it('formats seconds < 60', () => {
      expect(fmtRelative(0)).toBe('0s ago')
      expect(fmtRelative(30)).toBe('30s ago')
      expect(fmtRelative(59)).toBe('59s ago')
    })

    it('formats minutes < 60', () => {
      expect(fmtRelative(60)).toBe('1m ago')
      expect(fmtRelative(90)).toBe('1m ago')
      expect(fmtRelative(3599)).toBe('59m ago')
    })

    it('formats hours < 24', () => {
      expect(fmtRelative(3600)).toBe('1h ago')
      expect(fmtRelative(7200)).toBe('2h ago')
      expect(fmtRelative(86399)).toBe('23h ago')
    })

    it('formats days >= 24', () => {
      expect(fmtRelative(86400)).toBe('1d ago')
      expect(fmtRelative(172800)).toBe('2d ago')
    })
  })

  describe('fmtBytes', () => {
    it('returns dash for null/undefined', () => {
      expect(fmtBytes(null)).toBe('—')
      expect(fmtBytes(undefined)).toBe('—')
    })

    it('formats bytes < 1024', () => {
      expect(fmtBytes(0)).toBe('0 B')
      expect(fmtBytes(1)).toBe('1 B')
      expect(fmtBytes(1023)).toBe('1023 B')
    })

    it('formats kilobytes (1KB - 1MB)', () => {
      expect(fmtBytes(1024)).toBe('1 KB')
      expect(fmtBytes(1024 * 512)).toBe('512 KB')
      expect(fmtBytes(1024 * 1024 - 1)).toBe('1024 KB')
    })

    it('formats megabytes >= 1MB', () => {
      expect(fmtBytes(1024 * 1024)).toBe('1.0 MB')
      expect(fmtBytes(1024 * 1024 * 2.5)).toBe('2.5 MB')
      expect(fmtBytes(1024 * 1024 * 1024)).toBe('1024.0 MB')
    })
  })

  describe('fmtHashGhs', () => {
    it('returns dash for NaN/undefined', () => {
      expect(fmtHashGhs(NaN)).toBe('—')
      expect(fmtHashGhs(undefined)).toBe('—')
    })

    it('formats kH/s (< 0.001 GH/s)', () => {
      expect(fmtHashGhs(0.0005)).toBe('500.0 kH/s')
      expect(fmtHashGhs(0.0001)).toBe('100.0 kH/s')
    })

    it('formats MH/s (0.001 - 1 GH/s)', () => {
      expect(fmtHashGhs(0.001)).toBe('1.0 MH/s')
      expect(fmtHashGhs(0.5)).toBe('500.0 MH/s')
      expect(fmtHashGhs(0.999)).toBe('999.0 MH/s')
    })

    it('formats GH/s (1 - 1000 GH/s)', () => {
      expect(fmtHashGhs(1)).toBe('1.0 GH/s')
      expect(fmtHashGhs(500)).toBe('500.0 GH/s')
      expect(fmtHashGhs(999.9)).toBe('999.9 GH/s')
    })

    it('formats TH/s (>= 1000 GH/s)', () => {
      expect(fmtHashGhs(1000)).toBe('1.00 TH/s')
      expect(fmtHashGhs(1200)).toBe('1.20 TH/s')
      expect(fmtHashGhs(10000)).toBe('10.00 TH/s')
    })
  })
})
