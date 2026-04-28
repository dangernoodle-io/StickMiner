import { describe, it, expect } from 'vitest'

describe('History tooltip plugin', () => {
  it.skip('tooltip plugin is tightly coupled to uPlot internals and cannot be isolated', () => {
    // The tooltipPlugin function in History.svelte is a closure that depends on:
    // - uPlot instance (u) passed via hooks.init
    // - metrics array passed as parameter
    // - DOM manipulation and event handling
    // It cannot be reasonably tested without a full uPlot integration test
    // which would require Playwright (deferred per task scope)
  })
})
