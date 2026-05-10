import { describe, it, expect, vi, beforeEach } from 'vitest'

vi.mock('./api', () => ({
  fetchSettings: vi.fn(),
  patchSettings: vi.fn(),
}))

import * as api from './api'
import { createSettingsState } from './settingsState.svelte'

const settingsFixture = { hostname: 'taipan', display_en: true, ota_skip_check: false }

beforeEach(() => {
  vi.clearAllMocks()
})

describe('createSettingsState — initial state', () => {
  it('starts loading=true', () => {
    const ss = createSettingsState()
    expect(ss.loading).toBe(true)
  })

  it('starts loadErr empty', () => {
    const ss = createSettingsState()
    expect(ss.loadErr).toBe('')
  })

  it('starts displayOn false', () => {
    const ss = createSettingsState()
    expect(ss.displayOn).toBe(false)
  })

  it('starts otaSkip false', () => {
    const ss = createSettingsState()
    expect(ss.otaSkip).toBe(false)
  })

  it('starts savingDisplay false', () => {
    const ss = createSettingsState()
    expect(ss.savingDisplay).toBe(false)
  })

  it('starts savingOtaSkip false', () => {
    const ss = createSettingsState()
    expect(ss.savingOtaSkip).toBe(false)
  })

  it('starts displayMsg empty', () => {
    const ss = createSettingsState()
    expect(ss.displayMsg).toBe('')
  })

  it('starts otaMsg empty', () => {
    const ss = createSettingsState()
    expect(ss.otaMsg).toBe('')
  })

  it('starts displayKind empty', () => {
    const ss = createSettingsState()
    expect(ss.displayKind).toBe('')
  })

  it('starts otaKind empty', () => {
    const ss = createSettingsState()
    expect(ss.otaKind).toBe('')
  })
})

describe('loadSettings — happy path', () => {
  it('sets loading false after success', async () => {
    vi.mocked(api.fetchSettings).mockResolvedValue(settingsFixture)
    const ss = createSettingsState()
    await ss.loadSettings()
    expect(ss.loading).toBe(false)
  })

  it('populates displayOn from settings', async () => {
    vi.mocked(api.fetchSettings).mockResolvedValue({ ...settingsFixture, display_en: true })
    const ss = createSettingsState()
    await ss.loadSettings()
    expect(ss.displayOn).toBe(true)
  })

  it('populates displayOn=false from settings', async () => {
    vi.mocked(api.fetchSettings).mockResolvedValue({ ...settingsFixture, display_en: false })
    const ss = createSettingsState()
    await ss.loadSettings()
    expect(ss.displayOn).toBe(false)
  })

  it('populates otaSkip from settings', async () => {
    vi.mocked(api.fetchSettings).mockResolvedValue({ ...settingsFixture, ota_skip_check: true })
    const ss = createSettingsState()
    await ss.loadSettings()
    expect(ss.otaSkip).toBe(true)
  })

  it('clears loadErr on success', async () => {
    vi.mocked(api.fetchSettings).mockResolvedValue(settingsFixture)
    const ss = createSettingsState()
    await ss.loadSettings()
    expect(ss.loadErr).toBe('')
  })
})

describe('loadSettings — error path', () => {
  it('sets loadErr on failure', async () => {
    vi.mocked(api.fetchSettings).mockRejectedValue(new Error('network error'))
    const ss = createSettingsState()
    await ss.loadSettings()
    expect(ss.loadErr).toBe('network error')
  })

  it('sets loading=false after failure', async () => {
    vi.mocked(api.fetchSettings).mockRejectedValue(new Error('timeout'))
    const ss = createSettingsState()
    await ss.loadSettings()
    expect(ss.loading).toBe(false)
  })

  it('does not mutate displayOn on failure', async () => {
    vi.mocked(api.fetchSettings).mockRejectedValue(new Error('oops'))
    const ss = createSettingsState()
    await ss.loadSettings()
    expect(ss.displayOn).toBe(false)
  })
})

describe('saveDisplay — happy path', () => {
  it('sets displayKind=ok after success', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: false })
    const ss = createSettingsState()
    await ss.saveDisplay(true)
    expect(ss.displayKind).toBe('ok')
  })

  it('sets displayMsg=Saved when reboot_required=false', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: false })
    const ss = createSettingsState()
    await ss.saveDisplay(true)
    expect(ss.displayMsg).toBe('Saved')
  })

  it('sets displayMsg="Saved — reboot to apply" when reboot_required=true', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: true })
    const ss = createSettingsState()
    await ss.saveDisplay(true)
    expect(ss.displayMsg).toBe('Saved — reboot to apply')
  })

  it('updates displayOn to next value on success', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: false })
    const ss = createSettingsState()
    await ss.saveDisplay(true)
    expect(ss.displayOn).toBe(true)
  })

  it('clears savingDisplay after success', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: false })
    const ss = createSettingsState()
    await ss.saveDisplay(false)
    expect(ss.savingDisplay).toBe(false)
  })

  it('calls patchSettings with display_en', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: false })
    const ss = createSettingsState()
    await ss.saveDisplay(false)
    expect(api.patchSettings).toHaveBeenCalledWith({ display_en: false })
  })
})

describe('saveDisplay — error path', () => {
  it('sets displayKind=err on failure', async () => {
    vi.mocked(api.patchSettings).mockRejectedValue(new Error('patch failed'))
    const ss = createSettingsState()
    await ss.saveDisplay(true)
    expect(ss.displayKind).toBe('err')
  })

  it('sets displayMsg to error message on failure', async () => {
    vi.mocked(api.patchSettings).mockRejectedValue(new Error('patch failed: 503'))
    const ss = createSettingsState()
    await ss.saveDisplay(true)
    expect(ss.displayMsg).toBe('patch failed: 503')
  })

  it('reverts displayOn to saved value on failure (has saved)', async () => {
    vi.mocked(api.fetchSettings).mockResolvedValue({ ...settingsFixture, display_en: false })
    vi.mocked(api.patchSettings).mockRejectedValue(new Error('err'))
    const ss = createSettingsState()
    await ss.loadSettings()       // sets saved = { display_en: false, ... }
    await ss.saveDisplay(true)    // attempt to flip to true — fails
    expect(ss.displayOn).toBe(false) // reverted to saved
  })

  it('reverts displayOn to !next when no saved (no prior load)', async () => {
    vi.mocked(api.patchSettings).mockRejectedValue(new Error('err'))
    const ss = createSettingsState()
    await ss.saveDisplay(true)    // saved is null => fallback = !true = false
    expect(ss.displayOn).toBe(false)
  })

  it('clears savingDisplay after failure', async () => {
    vi.mocked(api.patchSettings).mockRejectedValue(new Error('err'))
    const ss = createSettingsState()
    await ss.saveDisplay(true)
    expect(ss.savingDisplay).toBe(false)
  })
})

describe('saveOtaSkip — happy path', () => {
  it('sets otaKind=ok after success', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: false })
    const ss = createSettingsState()
    await ss.saveOtaSkip(true)
    expect(ss.otaKind).toBe('ok')
  })

  it('sets otaMsg=Saved when reboot_required=false', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: false })
    const ss = createSettingsState()
    await ss.saveOtaSkip(true)
    expect(ss.otaMsg).toBe('Saved')
  })

  it('sets otaMsg="Saved — reboot to apply" when reboot_required=true', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: true })
    const ss = createSettingsState()
    await ss.saveOtaSkip(false)
    expect(ss.otaMsg).toBe('Saved — reboot to apply')
  })

  it('updates otaSkip to next value on success', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: false })
    const ss = createSettingsState()
    await ss.saveOtaSkip(true)
    expect(ss.otaSkip).toBe(true)
  })

  it('calls patchSettings with ota_skip_check', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: false })
    const ss = createSettingsState()
    await ss.saveOtaSkip(true)
    expect(api.patchSettings).toHaveBeenCalledWith({ ota_skip_check: true })
  })
})

describe('saveOtaSkip — error path', () => {
  it('sets otaKind=err on failure', async () => {
    vi.mocked(api.patchSettings).mockRejectedValue(new Error('fail'))
    const ss = createSettingsState()
    await ss.saveOtaSkip(false)
    expect(ss.otaKind).toBe('err')
  })

  it('sets otaMsg to error message', async () => {
    vi.mocked(api.patchSettings).mockRejectedValue(new Error('connection refused'))
    const ss = createSettingsState()
    await ss.saveOtaSkip(true)
    expect(ss.otaMsg).toBe('connection refused')
  })

  it('reverts otaSkip to saved value on failure', async () => {
    vi.mocked(api.fetchSettings).mockResolvedValue({ ...settingsFixture, ota_skip_check: false })
    vi.mocked(api.patchSettings).mockRejectedValue(new Error('err'))
    const ss = createSettingsState()
    await ss.loadSettings()
    await ss.saveOtaSkip(true)   // attempt flip — fails
    expect(ss.otaSkip).toBe(false) // reverted
  })

  it('clears savingOtaSkip after failure', async () => {
    vi.mocked(api.patchSettings).mockRejectedValue(new Error('err'))
    const ss = createSettingsState()
    await ss.saveOtaSkip(true)
    expect(ss.savingOtaSkip).toBe(false)
  })
})

describe('onDisplayChange', () => {
  it('calls saveDisplay with checked value', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: false })
    const ss = createSettingsState()
    const input = document.createElement('input')
    input.type = 'checkbox'
    input.checked = true
    const event = new Event('change')
    Object.defineProperty(event, 'currentTarget', { value: input })
    await ss.onDisplayChange(event)
    expect(api.patchSettings).toHaveBeenCalledWith({ display_en: true })
  })
})

describe('onOtaChange', () => {
  it('calls saveOtaSkip with inverted checked value', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: false })
    const ss = createSettingsState()
    const input = document.createElement('input')
    input.type = 'checkbox'
    input.checked = true   // OTA check ON => skip=false
    const event = new Event('change')
    Object.defineProperty(event, 'currentTarget', { value: input })
    await ss.onOtaChange(event)
    expect(api.patchSettings).toHaveBeenCalledWith({ ota_skip_check: false })
  })

  it('calls saveOtaSkip with true when checkbox unchecked', async () => {
    vi.mocked(api.patchSettings).mockResolvedValue({ status: 'ok', reboot_required: false })
    const ss = createSettingsState()
    const input = document.createElement('input')
    input.type = 'checkbox'
    input.checked = false  // OTA check OFF => skip=true
    const event = new Event('change')
    Object.defineProperty(event, 'currentTarget', { value: input })
    await ss.onOtaChange(event)
    expect(api.patchSettings).toHaveBeenCalledWith({ ota_skip_check: true })
  })
})
