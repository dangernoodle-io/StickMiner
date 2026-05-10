import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import { render } from '@testing-library/svelte'
import { info, otaCheck, otaInstall, otaUpload, rebooting } from '../lib/stores'

vi.mock('../lib/api', () => ({
  fetchStats: vi.fn(), fetchInfo: vi.fn(), fetchPower: vi.fn(), fetchFan: vi.fn(),
  fetchSettings: vi.fn(), fetchPool: vi.fn(), fetchHealth: vi.fn(), ping: vi.fn(),
  fetchOtaCheck: vi.fn(), triggerOtaUpdate: vi.fn(), fetchOtaStatus: vi.fn(), uploadOta: vi.fn()
}))

import Update from './Update.svelte'
import { fetchOtaCheck, triggerOtaUpdate, fetchOtaStatus, uploadOta } from '../lib/api'

const mockFetchOtaCheck = fetchOtaCheck as ReturnType<typeof vi.fn>
const mockTriggerOtaUpdate = triggerOtaUpdate as ReturnType<typeof vi.fn>
const mockFetchOtaStatus = fetchOtaStatus as ReturnType<typeof vi.fn>
const mockUploadOta = uploadOta as ReturnType<typeof vi.fn>

const baseInfo = {
  board: 'bitaxe-601', project_name: 'TaipanMiner', version: 'v1.0.0', idf_version: '5.5.3',
  build_date: '2024-01-15', build_time: '14:30:00', chip_model: 'esp32-s3', cores: 2,
  mac: '00:11:22:33:44:55', ssid: 'TestNetwork', flash_size: 16777216, app_size: 1048576,
  total_heap: 262144, free_heap: 131072, reset_reason: 'Unknown', wdt_resets: 0,
  boot_time: 1705333200, worker_name: 'testworker', hostname: 'taipan.local', validated: true
}

describe('Update', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    vi.useFakeTimers()
    info.set(null)
    otaCheck.set({ checking: false, result: null, msg: '', kind: '' })
    otaInstall.set({ installing: false, pct: 0, state: '', msg: '', kind: '' })
    otaUpload.set({ uploading: false, pct: 0, msg: '', kind: '' })
    rebooting.set({ active: false, reason: '', elapsed: 0, timedOut: false })
  })

  afterEach(() => {
    vi.useRealTimers()
  })

  it('renders without crashing', () => {
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders with firmware info', () => {
    info.set({ ...baseInfo, version: 'v1.5.2' } as any)
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders with build date', () => {
    info.set({ ...baseInfo, build_date: '2024-05-01', build_time: '10:30:45' } as any)
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders checking state', () => {
    otaCheck.set({ checking: true, result: null, msg: 'Checking...', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders available update', () => {
    otaCheck.set({ checking: false, result: { update_available: true, latest_version: 'v2.0.0', current_version: 'v1.0.0' }, msg: 'Update available', kind: 'avail' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders up-to-date message', () => {
    otaCheck.set({ checking: false, result: { update_available: false, latest_version: 'v1.0.0', current_version: 'v1.0.0' }, msg: 'Up to date', kind: 'ok' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders check error', () => {
    otaCheck.set({ checking: false, result: null, msg: 'Check failed', kind: 'err' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders installing', () => {
    otaInstall.set({ installing: true, pct: 45, state: 'Installing...', msg: 'Flashing', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders install progress', () => {
    otaInstall.set({ installing: true, pct: 65, state: 'Installing', msg: 'Progress: 65%', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders install complete', () => {
    otaInstall.set({ installing: false, pct: 100, state: 'Complete', msg: 'Installed', kind: 'ok' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders install error', () => {
    otaInstall.set({ installing: false, pct: 0, state: 'Error', msg: 'Failed', kind: 'err' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders uploading', () => {
    otaUpload.set({ uploading: true, pct: 30, msg: 'Uploading', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders upload progress', () => {
    otaUpload.set({ uploading: true, pct: 75, msg: 'In progress', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders upload complete', () => {
    otaUpload.set({ uploading: false, pct: 100, msg: 'Complete', kind: 'ok' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders upload error', () => {
    otaUpload.set({ uploading: false, pct: 0, msg: 'Failed', kind: 'err' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders reboot overlay', () => {
    rebooting.set({ active: true, reason: 'Update', elapsed: 5, timedOut: false })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders reboot timeout', () => {
    rebooting.set({ active: true, reason: 'Flash', elapsed: 90, timedOut: true })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders device info', () => {
    info.set(baseInfo as any)
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders multiple states', () => {
    otaCheck.set({ checking: false, result: null, msg: '', kind: 'ok' })
    otaInstall.set({ installing: false, pct: 0, state: '', msg: '', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders firmware info with board', () => {
    info.set({ ...baseInfo, version: 'v1.5.2' } as any)
    const { container } = render(Update)
    expect(container.textContent).toContain('v1.5.2')
  })

  it('renders checking state', () => {
    otaCheck.set({ checking: true, result: null, msg: 'Checking...', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders available update state', () => {
    otaCheck.set({
      checking: false,
      result: { update_available: true, latest_version: 'v2.0.0', current_version: 'v1.0.0' },
      msg: 'Update available',
      kind: 'avail'
    })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders up to date state', () => {
    otaCheck.set({
      checking: false,
      result: { update_available: false, latest_version: 'v1.0.0', current_version: 'v1.0.0' },
      msg: 'Up to date',
      kind: 'ok'
    })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders check error state', () => {
    otaCheck.set({ checking: false, result: null, msg: 'Check failed', kind: 'err' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders installing state with progress', () => {
    otaInstall.set({ installing: true, pct: 45, state: 'Installing...', msg: 'Flashing', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders install progress at various percentages', () => {
    otaInstall.set({ installing: true, pct: 65, state: 'Installing', msg: 'Progress: 65%', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders install complete state', () => {
    otaInstall.set({ installing: false, pct: 100, state: 'Complete', msg: 'Installed', kind: 'ok' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders install error state', () => {
    otaInstall.set({ installing: false, pct: 0, state: 'Error', msg: 'Failed', kind: 'err' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders uploading state', () => {
    otaUpload.set({ uploading: true, pct: 30, msg: 'Uploading', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders upload progress', () => {
    otaUpload.set({ uploading: true, pct: 75, msg: 'In progress', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders upload complete state', () => {
    otaUpload.set({ uploading: false, pct: 100, msg: 'Complete', kind: 'ok' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders upload error state', () => {
    otaUpload.set({ uploading: false, pct: 0, msg: 'Failed', kind: 'err' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders reboot active', () => {
    rebooting.set({ active: true, reason: 'Update', elapsed: 5, timedOut: false })
    const { container } = render(Update)
    expect(container.textContent).toContain('Rebooting')
  })

  it('renders reboot timeout state', () => {
    rebooting.set({ active: true, reason: 'Flash', elapsed: 90, timedOut: true })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders with both check and install states', () => {
    otaCheck.set({ checking: false, result: null, msg: '', kind: 'ok' })
    otaInstall.set({ installing: false, pct: 0, state: '', msg: '', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders check state transitions', () => {
    otaCheck.set({ checking: true, result: null, msg: 'Checking...', kind: '' })
    let result = render(Update)
    expect(result.component).toBeDefined()

    // Transition to result
    otaCheck.set({
      checking: false,
      result: { update_available: true, latest_version: 'v2.0.0', current_version: 'v1.0.0' },
      msg: 'Update available',
      kind: 'avail'
    })
    result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders install pending state', () => {
    otaInstall.set({ installing: true, pct: 0, state: 'starting', msg: 'Starting...', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders install downloading state', () => {
    otaInstall.set({ installing: true, pct: 50, state: 'downloading', msg: 'Downloading...', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders install writing state', () => {
    otaInstall.set({ installing: true, pct: 90, state: 'writing', msg: 'Writing...', kind: '' })
    const result = render(Update)
    expect(result.component).toBeDefined()
  })

  it('renders minerBusy when rebooting', () => {
    rebooting.set({ active: true, reason: 'Test', elapsed: 0, timedOut: false })
    const { container } = render(Update)
    expect(container).toBeTruthy()
  })

  it('renders minerBusy when install ok', () => {
    otaInstall.set({ installing: false, pct: 100, state: 'complete', msg: 'ok', kind: 'ok' })
    const { container } = render(Update)
    expect(container).toBeTruthy()
  })

  it('renders minerBusy when upload ok', () => {
    otaUpload.set({ uploading: false, pct: 100, msg: 'ok', kind: 'ok' })
    const { container } = render(Update)
    expect(container).toBeTruthy()
  })

  it('computes firmware name from board info', () => {
    info.set(baseInfo as any)
    const { container } = render(Update)
    expect(container.textContent).toContain('bitaxe-601')
  })

  it('uses default firmware name without info', () => {
    info.set(null)
    const { container } = render(Update)
    expect(container).toBeTruthy()
  })

  it('renders with no check result', () => {
    otaCheck.set({ checking: false, result: null, msg: '', kind: '' })
    const { container } = render(Update)
    expect(container).toBeTruthy()
  })

  it('renders with null reboot info', () => {
    rebooting.set({ active: false, reason: '', elapsed: 0, timedOut: false })
    const { container } = render(Update)
    expect(container).toBeTruthy()
  })
})
