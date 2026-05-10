import { describe, it, expect, vi, beforeEach } from 'vitest'
import { render } from '@testing-library/svelte'
import { stats, info, fan, hasAsic } from '../lib/stores'

vi.mock('../lib/api', () => ({
  fetchStats: vi.fn(),
  fetchInfo: vi.fn(),
  fetchPower: vi.fn(),
  fetchFan: vi.fn(),
  fetchSettings: vi.fn().mockResolvedValue({ hostname: 'taipan', display_en: true, ota_skip_check: false }),
  fetchPool: vi.fn(),
  fetchHealth: vi.fn(),
  ping: vi.fn(),
  patchSettings: vi.fn()
}))

import Settings from './Settings.svelte'

const baseStats = {
  session_shares: 10, session_rejected: 1, lifetime_shares: 1000, last_share_ago_s: 30,
  best_diff: 500000, uptime_s: 3600, temp_c: 40, hashrate: 485e9, hashrate_avg: 480e9,
  hashrate_1m: null, hashrate_10m: null, hashrate_1h: null, shares: null, asic_hashrate: null,
  asic_hashrate_avg: null, asic_shares: null, asic_temp_c: 72, asic_freq_configured_mhz: 400,
  asic_freq_effective_mhz: 395, asic_small_cores: 256, asic_count: 2, expected_ghs: 485,
  asic_total_ghs: 485.5, asic_hw_error_pct: 0.01, asic_total_ghs_1m: 484, asic_total_ghs_10m: 486,
  asic_total_ghs_1h: 483, asic_hw_error_pct_1m: 0.01, asic_hw_error_pct_10m: 0.01,
  asic_hw_error_pct_1h: 0.02,
  hw_error_pct_1m: null, hw_error_pct_10m: null, hw_error_pct_1h: null,
  pool_effective_hashrate: null, rejected: null
}

const baseInfo = {
  board: 'bitaxe-601', project_name: 'TaipanMiner', version: 'v1.0.0', idf_version: '5.5.3',
  build_date: '2024-01-15', build_time: '14:30:00', chip_model: 'esp32-s3', cores: 2,
  mac: '00:11:22:33:44:55', ssid: 'TestNetwork', flash_size: 16777216, app_size: 1048576,
  total_heap: 262144, free_heap: 131072, reset_reason: 'Unknown', wdt_resets: 0,
  boot_time: 1705333200, worker_name: 'testworker', hostname: 'taipan.local', validated: true
}

describe('Settings', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    stats.set(null)
    info.set(null)
    fan.set(null)
    hasAsic.set(false)
  })

  it('renders without crashing', () => {
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with info', () => {
    info.set(baseInfo as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders without hasAsic', () => {
    stats.set(baseStats as any)
    hasAsic.set(false)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with hasAsic', () => {
    stats.set({ ...baseStats, asic_count: 2 } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with multiple chips', () => {
    stats.set({ ...baseStats, asic_count: 3 } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with null chip count', () => {
    stats.set({ ...baseStats, asic_count: null } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with custom frequency', () => {
    stats.set({ ...baseStats, asic_freq_configured_mhz: 550 } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with null frequency', () => {
    stats.set({ ...baseStats, asic_freq_configured_mhz: null } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with device name', () => {
    info.set({ ...baseInfo, hostname: 'custom-miner.local' } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with board type', () => {
    info.set({ ...baseInfo, board: 'bitaxe-403' } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with worker name', () => {
    info.set({ ...baseInfo, worker_name: 'my-worker' } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with version', () => {
    info.set({ ...baseInfo, version: 'v2.5.1' } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with fan data', () => {
    fan.set({
      autofan: false, die_target_c: 65, vr_target_c: 80, min_pct: 35, manual_pct: 80,
      duty_pct: 80, rpm: 3200, pid_input_src: 'die', pid_input_c: null, die_ema_c: null,
      vr_ema_c: null
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with autofan enabled', () => {
    fan.set({
      autofan: true, die_target_c: 65, vr_target_c: 80, min_pct: 35, manual_pct: 80,
      duty_pct: 80, rpm: 3200, pid_input_src: 'die', pid_input_c: null, die_ema_c: null,
      vr_ema_c: null
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with null fan RPM', () => {
    fan.set({
      autofan: false, die_target_c: 65, vr_target_c: 80, min_pct: 35, manual_pct: 80,
      duty_pct: null, rpm: null, pid_input_src: 'die', pid_input_c: null, die_ema_c: null,
      vr_ema_c: null
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with null duty_pct', () => {
    fan.set({
      autofan: false, die_target_c: 65, vr_target_c: 80, min_pct: 35, manual_pct: 80,
      duty_pct: null, rpm: 3200, pid_input_src: 'die', pid_input_c: null, die_ema_c: null,
      vr_ema_c: null
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with pid_input_src as vr', () => {
    fan.set({
      autofan: false, die_target_c: 65, vr_target_c: 80, min_pct: 35, manual_pct: 80,
      duty_pct: 80, rpm: 3200, pid_input_src: 'vr', pid_input_c: 75, die_ema_c: 60,
      vr_ema_c: null
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with PID telemetry data', () => {
    fan.set({
      autofan: false, die_target_c: 65, vr_target_c: 80, min_pct: 35, manual_pct: 80,
      duty_pct: 80, rpm: 3200, pid_input_src: 'die', pid_input_c: 62, die_ema_c: 61,
      vr_ema_c: 75
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with high die target', () => {
    fan.set({
      autofan: false, die_target_c: 100, vr_target_c: 80, min_pct: 35, manual_pct: 80,
      duty_pct: 80, rpm: 3200, pid_input_src: 'die', pid_input_c: null, die_ema_c: null,
      vr_ema_c: null
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with low die target', () => {
    fan.set({
      autofan: false, die_target_c: 30, vr_target_c: 80, min_pct: 35, manual_pct: 80,
      duty_pct: 80, rpm: 3200, pid_input_src: 'die', pid_input_c: null, die_ema_c: null,
      vr_ema_c: null
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with high vr target', () => {
    fan.set({
      autofan: false, die_target_c: 65, vr_target_c: 120, min_pct: 35, manual_pct: 80,
      duty_pct: 80, rpm: 3200, pid_input_src: 'die', pid_input_c: null, die_ema_c: null,
      vr_ema_c: null
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with high manual fan duty', () => {
    fan.set({
      autofan: false, die_target_c: 65, vr_target_c: 80, min_pct: 35, manual_pct: 100,
      duty_pct: 100, rpm: 5000, pid_input_src: 'die', pid_input_c: null, die_ema_c: null,
      vr_ema_c: null
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with low manual fan duty', () => {
    fan.set({
      autofan: false, die_target_c: 65, vr_target_c: 80, min_pct: 10, manual_pct: 10,
      duty_pct: 10, rpm: 500, pid_input_src: 'die', pid_input_c: null, die_ema_c: null,
      vr_ema_c: null
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with zero RPM', () => {
    fan.set({
      autofan: false, die_target_c: 65, vr_target_c: 80, min_pct: 35, manual_pct: 0,
      duty_pct: 0, rpm: 0, pid_input_src: 'die', pid_input_c: null, die_ema_c: null,
      vr_ema_c: null
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with single chip', () => {
    stats.set({ ...baseStats, asic_count: 1 } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with four chips', () => {
    stats.set({ ...baseStats, asic_count: 4 } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with effective frequency different from configured', () => {
    stats.set({
      ...baseStats,
      asic_freq_configured_mhz: 500,
      asic_freq_effective_mhz: 450
    } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with zero configured frequency', () => {
    stats.set({ ...baseStats, asic_freq_configured_mhz: 0 } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with null effective frequency', () => {
    stats.set({ ...baseStats, asic_freq_effective_mhz: null } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with null small cores', () => {
    stats.set({ ...baseStats, asic_small_cores: null } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with high small cores count', () => {
    stats.set({ ...baseStats, asic_small_cores: 512 } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with null temp', () => {
    stats.set({ ...baseStats, temp_c: null } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with high temp', () => {
    stats.set({ ...baseStats, temp_c: 85 } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with null asic_temp_c', () => {
    stats.set({ ...baseStats, asic_temp_c: null } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with very high asic_temp_c', () => {
    stats.set({ ...baseStats, asic_temp_c: 100 } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with empty hostname', () => {
    info.set({ ...baseInfo, hostname: '' } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with empty worker_name', () => {
    info.set({ ...baseInfo, worker_name: '' } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with empty board', () => {
    info.set({ ...baseInfo, board: '' } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with tdongle-s3 board', () => {
    info.set({ ...baseInfo, board: 'tdongle-s3' } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with bitaxe-403 board', () => {
    info.set({ ...baseInfo, board: 'bitaxe-403' } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with bitaxe-650 board', () => {
    info.set({ ...baseInfo, board: 'bitaxe-650' } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with null fan data', () => {
    fan.set(null)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with null stats', () => {
    stats.set(null)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with null info', () => {
    info.set(null)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders info and stats together', () => {
    info.set(baseInfo as any)
    stats.set(baseStats as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders all data available', () => {
    info.set(baseInfo as any)
    stats.set(baseStats as any)
    fan.set({
      autofan: true, die_target_c: 65, vr_target_c: 80, min_pct: 35, manual_pct: 80,
      duty_pct: 75, rpm: 3200, pid_input_src: 'die', pid_input_c: 62, die_ema_c: 61,
      vr_ema_c: null
    })
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with no ASIC and minimal data', () => {
    hasAsic.set(false)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with max uptime', () => {
    stats.set({ ...baseStats, uptime_s: 31536000 } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with max sessions', () => {
    stats.set({ ...baseStats, session_shares: 1000000 } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with max lifetime shares', () => {
    stats.set({ ...baseStats, lifetime_shares: 10000000 } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with high best diff', () => {
    stats.set({ ...baseStats, best_diff: 5000000 } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with high hashrate', () => {
    stats.set({ ...baseStats, hashrate: 1000e9 } as any)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with expected vs actual GHS mismatch', () => {
    stats.set({
      ...baseStats,
      expected_ghs: 500,
      asic_total_ghs: 450
    } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with high error rate', () => {
    stats.set({ ...baseStats, asic_hw_error_pct: 5 } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })

  it('renders with zero error rate', () => {
    stats.set({ ...baseStats, asic_hw_error_pct: 0 } as any)
    hasAsic.set(true)
    const result = render(Settings)
    expect(result.component).toBeDefined()
  })
})
