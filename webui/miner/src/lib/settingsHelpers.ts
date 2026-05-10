import type { Settings } from './api'

/** The subset of Settings fields that the Settings page edits live. */
export type SettingsForm = {
  display_en: boolean
  ota_skip_check: boolean
}

/** Build a SettingsForm from a raw Settings response. */
export function formFromSettings(s: Settings): SettingsForm {
  return {
    display_en: !!s.display_en,
    ota_skip_check: !!s.ota_skip_check,
  }
}
