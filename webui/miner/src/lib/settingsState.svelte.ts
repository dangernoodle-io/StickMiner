import { fetchSettings, patchSettings } from './api'
import { formFromSettings } from './settingsHelpers'

export function createSettingsState() {
  let loading = $state(true)
  let loadErr = $state('')
  let saved = $state.raw<Awaited<ReturnType<typeof fetchSettings>> | null>(null)

  let displayOn = $state(false)
  let otaSkip = $state(false)
  let savingDisplay = $state(false)
  let savingOtaSkip = $state(false)
  let displayMsg = $state('')
  let otaMsg = $state('')
  let displayKind = $state<'' | 'ok' | 'err'>('')
  let otaKind = $state<'' | 'ok' | 'err'>('')

  async function loadSettings() {
    loading = true
    loadErr = ''
    try {
      const s = await fetchSettings()
      saved = s
      const form = formFromSettings(s)
      displayOn = form.display_en
      otaSkip = form.ota_skip_check
    } catch (e) {
      loadErr = (e as Error).message
    } finally {
      loading = false
    }
  }

  async function saveDisplay(next: boolean) {
    savingDisplay = true
    displayMsg = ''
    displayKind = ''
    try {
      const res = await patchSettings({ display_en: next })
      displayOn = next
      displayKind = 'ok'
      displayMsg = res.reboot_required ? 'Saved — reboot to apply' : 'Saved'
    } catch (e) {
      displayOn = saved?.display_en ?? !next
      displayKind = 'err'
      displayMsg = (e as Error).message
    } finally {
      savingDisplay = false
    }
  }

  async function saveOtaSkip(next: boolean) {
    savingOtaSkip = true
    otaMsg = ''
    otaKind = ''
    try {
      const res = await patchSettings({ ota_skip_check: next })
      otaSkip = next
      otaKind = 'ok'
      otaMsg = res.reboot_required ? 'Saved — reboot to apply' : 'Saved'
    } catch (e) {
      otaSkip = saved?.ota_skip_check ?? !next
      otaKind = 'err'
      otaMsg = (e as Error).message
    } finally {
      savingOtaSkip = false
    }
  }

  function onDisplayChange(e: Event) {
    saveDisplay((e.currentTarget as HTMLInputElement).checked)
  }

  function onOtaChange(e: Event) {
    // checkbox is "OTA check on boot" — checked = check enabled = skip=false
    saveOtaSkip(!(e.currentTarget as HTMLInputElement).checked)
  }

  return {
    get loading() { return loading },
    get loadErr() { return loadErr },
    get displayOn() { return displayOn },
    get otaSkip() { return otaSkip },
    get savingDisplay() { return savingDisplay },
    get savingOtaSkip() { return savingOtaSkip },
    get displayMsg() { return displayMsg },
    get otaMsg() { return otaMsg },
    get displayKind() { return displayKind },
    get otaKind() { return otaKind },
    loadSettings,
    saveDisplay,
    saveOtaSkip,
    onDisplayChange,
    onOtaChange,
  }
}
