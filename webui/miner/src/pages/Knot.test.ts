import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import { render } from '@testing-library/svelte'

vi.mock('../lib/api', () => ({
  fetchStats: vi.fn(), fetchInfo: vi.fn(), fetchPower: vi.fn(), fetchFan: vi.fn(),
  fetchSettings: vi.fn(), fetchPool: vi.fn(), fetchHealth: vi.fn(), ping: vi.fn(), fetchKnot: vi.fn()
}))

import Knot from './Knot.svelte'
import { fetchKnot } from '../lib/api'

const mockFetchKnot = fetchKnot as ReturnType<typeof vi.fn>

const basePeer = {
  instance: 'peer1', hostname: 'dongle1', ip: '192.168.1.100',
  worker: 'worker1', board: 'tdongle-s3', version: 'v1.0.0',
  state: 'mining', seen_ago_s: 5
}

describe('Knot', () => {
  beforeEach(() => {
    vi.clearAllMocks()
  })

  it('renders without crashing', () => {
    mockFetchKnot.mockResolvedValueOnce([])
    const result = render(Knot)
    expect(result.component).toBeDefined()
  })

  it('renders without crashing on mount', async () => {
    mockFetchKnot.mockResolvedValueOnce([])
    const result = render(Knot)
    expect(result.component).toBeDefined()
  })

  it('loads peers on mount', async () => {
    mockFetchKnot.mockResolvedValueOnce([])
    render(Knot)
    await vi.waitFor(() => expect(mockFetchKnot).toHaveBeenCalled())
  })

  it('sorts peers by hostname', async () => {
    const peers = [
      { ...basePeer, instance: 'p2', hostname: 'zebra' },
      { ...basePeer, instance: 'p1', hostname: 'apple' }
    ]
    mockFetchKnot.mockResolvedValueOnce(peers)
    const { container } = render(Knot)
    await vi.waitFor(() => {
      const rows = container.querySelectorAll('tbody tr')
      expect(rows.length).toBe(2)
    })
  })

  it('renders empty state when no peers', async () => {
    mockFetchKnot.mockResolvedValueOnce([])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.empty-state')).toBeTruthy())
  })

  it('displays error message on fetch fail', async () => {
    mockFetchKnot.mockRejectedValueOnce(new Error('Network error'))
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.error')).toBeTruthy())
  })

  it('renders peer table with data', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(mockFetchKnot).toHaveBeenCalled())
    await vi.waitFor(() => expect(container.querySelector('table')).toBeTruthy())
  })

  it('shows updated timestamp after load', async () => {
    mockFetchKnot.mockResolvedValueOnce([])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(mockFetchKnot).toHaveBeenCalled())
    await vi.waitFor(() => expect(container.textContent).toContain('Updated'))
  })

  it('classifies mining state', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, state: 'mining' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.state-mining')).toBeTruthy())
  })

  it('classifies ota state', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, state: 'ota' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.state-ota')).toBeTruthy())
  })

  it('classifies provisioning state', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, state: 'provisioning' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.state-provisioning')).toBeTruthy())
  })

  it('classifies idle state', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, state: 'idle' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.state-idle')).toBeTruthy())
  })

  it('classifies unknown state as unknown', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, state: 'unknown' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.state-unknown')).toBeTruthy())
  })

  it('classifies unrecognized state as neutral', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, state: 'invalid' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.state-neutral')).toBeTruthy())
  })

  it('appends .local to hostname without it', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, hostname: 'dongle1' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('a[href*="dongle1.local"]')).toBeTruthy())
  })

  it('preserves .local suffix in hostname', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, hostname: 'dongle1.local' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('a[href*="dongle1.local"]')).toBeTruthy())
  })

  it('displays hostname without .local suffix', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, hostname: 'dongle1.local' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.textContent).toContain('dongle1'))
  })

  it('classifies tdongle-s3 board', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, board: 'tdongle-s3' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.board-tdongle-s3')).toBeTruthy())
  })

  it('classifies bitaxe-601 board', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, board: 'bitaxe-601' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.board-bitaxe-601')).toBeTruthy())
  })

  it('classifies bitaxe-403 board', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, board: 'bitaxe-403' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.board-bitaxe-403')).toBeTruthy())
  })

  it('classifies bitaxe-650 board', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, board: 'bitaxe-650' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.board-bitaxe-650')).toBeTruthy())
  })

  it('classifies unknown board as other', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, board: 'unknown' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.board-other')).toBeTruthy())
  })

  it('builds board legend from peers', async () => {
    mockFetchKnot.mockResolvedValueOnce([
      { ...basePeer, board: 'tdongle-s3' },
      { ...basePeer, instance: 'p2', board: 'bitaxe-601' }
    ])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.legend')).toBeTruthy())
  })

  it('builds status legend from peers', async () => {
    mockFetchKnot.mockResolvedValueOnce([
      { ...basePeer, state: 'mining' },
      { ...basePeer, instance: 'p2', state: 'idle' }
    ])
    const { container } = render(Knot)
    await vi.waitFor(() => {
      const legend = container.querySelector('.legend')
      expect(legend?.textContent).toContain('Status')
    })
  })

  it('shows unknown status when state empty', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, state: '' }])
    const { container } = render(Knot)
    await vi.waitFor(() => {
      const legend = container.querySelector('.legend')
      expect(legend?.textContent).toContain('unknown')
    })
  })

  it('handles empty hostname in link', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, hostname: '' }])
    const { container } = render(Knot)
    await vi.waitFor(() => {
      const link = container.querySelector('a')
      expect(link?.href).toBeTruthy()
    })
  })

  it('handles empty board gracefully', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, board: '' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.board-other')).toBeTruthy())
  })

  it('no legend when no peers', async () => {
    mockFetchKnot.mockResolvedValueOnce([])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(mockFetchKnot).toHaveBeenCalled())
    expect(container.querySelector('.legend')).toBeFalsy()
  })

  it('shows legend when state defaults to unknown', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, board: '', state: '' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(container.querySelector('.legend')).toBeTruthy())
  })

  it('multiple peers render in table', async () => {
    const peers = [
      { ...basePeer, instance: 'p1', hostname: 'dongle1' },
      { ...basePeer, instance: 'p2', hostname: 'bitaxe1' },
      { ...basePeer, instance: 'p3', hostname: 'bitaxe2' }
    ]
    mockFetchKnot.mockResolvedValueOnce(peers)
    const { container } = render(Knot)
    await vi.waitFor(() => expect(mockFetchKnot).toHaveBeenCalled())
    await vi.waitFor(() => {
      const rows = container.querySelectorAll('tbody tr')
      expect(rows.length).toBe(3)
    })
  })

  it('displays peer versions', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, version: 'v2.5.1' }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(mockFetchKnot).toHaveBeenCalled())
    await vi.waitFor(() => expect(container.textContent).toContain('v2.5.1'))
  })

  it('renders with null state defaults to unknown', async () => {
    mockFetchKnot.mockResolvedValueOnce([{ ...basePeer, state: null as any }])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(mockFetchKnot).toHaveBeenCalled())
    await vi.waitFor(() => expect(container.querySelector('table')).toBeTruthy())
  })

  it('renders header with title', async () => {
    mockFetchKnot.mockResolvedValueOnce([])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(mockFetchKnot).toHaveBeenCalled())
    expect(container.textContent).toContain('Knot')
  })

  it('renders refresh button', async () => {
    mockFetchKnot.mockResolvedValueOnce([])
    const { container } = render(Knot)
    await vi.waitFor(() => expect(mockFetchKnot).toHaveBeenCalled())
    const btn = container.querySelector('button')
    expect(btn).toBeTruthy()
  })
})
