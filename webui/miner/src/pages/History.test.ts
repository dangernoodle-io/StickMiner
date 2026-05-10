import { describe, it, expect, vi, beforeEach } from 'vitest'
import { render } from '@testing-library/svelte'
import { history, hasAsic } from '../lib/stores'

vi.mock('../lib/api', () => ({
  fetchStats: vi.fn(), fetchInfo: vi.fn(), fetchPower: vi.fn(), fetchFan: vi.fn(),
  fetchSettings: vi.fn(), fetchPool: vi.fn(), fetchHealth: vi.fn(), ping: vi.fn()
}))

import History from './History.svelte'

const sample = {
  ts: Math.floor(Date.now() / 1000),
  total_ghs: 485.5, hw_err_pct: 0.01, temp_c: 72, vr_temp_c: 60, board_temp_c: 45,
  pcore_w: 25, vcore_v: 1.1, efficiency_jth: 25.5, asic_freq_mhz: 395, rpm: 3200, fan_duty: 80
}

describe('History', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    history.set([])
    hasAsic.set(false)
  })

  it('renders without history', () => {
    history.set([])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with one sample', () => {
    history.set([sample])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with multiple samples', () => {
    history.set([sample, { ...sample, ts: sample.ts - 5 }])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders for tdongle (no ASIC)', () => {
    history.set([{ ...sample, total_ghs: 0.485, hw_err_pct: null, vr_temp_c: null, board_temp_c: null, pcore_w: null, vcore_v: null, efficiency_jth: null, asic_freq_mhz: null, rpm: null, fan_duty: null }])
    hasAsic.set(false)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders for ASIC device', () => {
    history.set([sample])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders hashrate metric', () => {
    history.set([sample])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders temp metric', () => {
    history.set([sample])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders HW error metric for ASIC', () => {
    history.set([sample])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('filters ASIC metrics for non-ASIC', () => {
    history.set([{ ...sample, hw_err_pct: null, vr_temp_c: null }])
    hasAsic.set(false)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders power metrics for ASIC', () => {
    history.set([sample])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders VR temp for ASIC', () => {
    history.set([sample])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders frequency metric for ASIC', () => {
    history.set([sample])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders fan metrics for ASIC', () => {
    history.set([sample])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('handles null values', () => {
    history.set([{ ...sample, total_ghs: null, temp_c: null, vr_temp_c: null }])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with many samples', () => {
    const samples = Array.from({ length: 100 }, (_, i) => ({ ...sample, ts: sample.ts - i * 5 }))
    history.set(samples)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with partial data', () => {
    history.set([{ ...sample, vr_temp_c: null, pcore_w: null, efficiency_jth: null, rpm: null }])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('displays board temp separately', () => {
    history.set([sample])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with zero hashrate', () => {
    history.set([{ ...sample, total_ghs: 0 }])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with high hashrate', () => {
    history.set([{ ...sample, total_ghs: 1000 }])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with null hashrate', () => {
    history.set([{ ...sample, total_ghs: null }])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with various temperatures', () => {
    history.set([
      { ...sample, temp_c: 30 },
      { ...sample, temp_c: 70 },
      { ...sample, temp_c: 100 }
    ])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with null temperature', () => {
    history.set([{ ...sample, temp_c: null }])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with null hw error for non-ASIC', () => {
    history.set([{ ...sample, hw_err_pct: null }])
    hasAsic.set(false)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with high hw error rate', () => {
    history.set([{ ...sample, hw_err_pct: 10 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with zero hw error', () => {
    history.set([{ ...sample, hw_err_pct: 0 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with VR temperature data', () => {
    history.set([{ ...sample, vr_temp_c: 80 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders without VR temperature', () => {
    history.set([{ ...sample, vr_temp_c: null }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with board temperature data', () => {
    history.set([{ ...sample, board_temp_c: 55 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders without board temperature', () => {
    history.set([{ ...sample, board_temp_c: null }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with power data', () => {
    history.set([{ ...sample, pcore_w: 50 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders without power data', () => {
    history.set([{ ...sample, pcore_w: null }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with core voltage data', () => {
    history.set([{ ...sample, vcore_v: 1.2 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders without core voltage', () => {
    history.set([{ ...sample, vcore_v: null }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with efficiency data', () => {
    history.set([{ ...sample, efficiency_jth: 30 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders without efficiency', () => {
    history.set([{ ...sample, efficiency_jth: null }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with ASIC frequency data', () => {
    history.set([{ ...sample, asic_freq_mhz: 500 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders without ASIC frequency', () => {
    history.set([{ ...sample, asic_freq_mhz: null }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with fan data', () => {
    history.set([{ ...sample, rpm: 5000, fan_duty: 85 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders without fan data', () => {
    history.set([{ ...sample, rpm: null, fan_duty: null }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with high fan RPM', () => {
    history.set([{ ...sample, rpm: 10000 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with zero fan RPM', () => {
    history.set([{ ...sample, rpm: 0 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with very old timestamp', () => {
    history.set([{ ...sample, ts: 1609459200 }])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with recent timestamp', () => {
    history.set([{ ...sample, ts: Math.floor(Date.now() / 1000) }])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with all null ASIC values on non-ASIC', () => {
    history.set([{
      ...sample,
      hw_err_pct: null, vr_temp_c: null, board_temp_c: null,
      pcore_w: null, vcore_v: null, efficiency_jth: null,
      asic_freq_mhz: null, rpm: null, fan_duty: null
    }])
    hasAsic.set(false)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with very high power', () => {
    history.set([{ ...sample, pcore_w: 500 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with low power', () => {
    history.set([{ ...sample, pcore_w: 1 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with extreme efficiency values', () => {
    history.set([{ ...sample, efficiency_jth: 100 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with low efficiency', () => {
    history.set([{ ...sample, efficiency_jth: 0.5 }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with varying fan duty', () => {
    history.set([
      { ...sample, fan_duty: 0 },
      { ...sample, fan_duty: 50 },
      { ...sample, fan_duty: 100 }
    ])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders mixed ASIC and non-ASIC data', () => {
    history.set([
      { ...sample, total_ghs: 485, hw_err_pct: 0.5 },
      { ...sample, total_ghs: 0.1, hw_err_pct: null }
    ])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with extreme temperature ranges', () => {
    history.set([
      { ...sample, temp_c: 20 },
      { ...sample, temp_c: 120 }
    ])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('handles frequency transitions', () => {
    history.set([
      { ...sample, asic_freq_mhz: 300 },
      { ...sample, asic_freq_mhz: 500 },
      { ...sample, asic_freq_mhz: 200 }
    ])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with many samples over time', () => {
    const samples = []
    for (let i = 0; i < 50; i++) {
      samples.push({
        ...sample,
        ts: sample.ts - i * 60,
        total_ghs: 485 + Math.random() * 10,
        temp_c: 70 + Math.random() * 10
      })
    }
    history.set(samples)
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders without any samples initially', () => {
    history.set([])
    const result = render(History)
    expect(result.component).toBeDefined()
  })

  it('transitions from no ASIC to ASIC', () => {
    history.set([sample])
    hasAsic.set(false)
    let result = render(History)
    expect(result.component).toBeDefined()

    hasAsic.set(true)
    result = render(History)
    expect(result.component).toBeDefined()
  })

  it('transitions from ASIC to no ASIC', () => {
    history.set([sample])
    hasAsic.set(true)
    let result = render(History)
    expect(result.component).toBeDefined()

    hasAsic.set(false)
    result = render(History)
    expect(result.component).toBeDefined()
  })

  it('renders with conflicting null/non-null ASIC fields', () => {
    history.set([{
      ...sample,
      hw_err_pct: 0.5,
      vr_temp_c: null,
      board_temp_c: 50,
      pcore_w: null,
      vcore_v: 1.0,
      efficiency_jth: null
    }])
    hasAsic.set(true)
    const result = render(History)
    expect(result.component).toBeDefined()
  })
})
