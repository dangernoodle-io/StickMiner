import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest'
import { get } from 'svelte/store'

describe('router', () => {
  beforeEach(() => {
    // Reset hash to default before each test
    window.location.hash = ''
  })

  afterEach(() => {
    window.location.hash = ''
  })

  describe('route store', () => {
    it('parses empty hash as dashboard', async () => {
      // Reimport to get fresh module state
      const { route } = await import('./router')
      window.location.hash = ''
      expect(get(route)).toBe('dashboard')
    })

    it('parses route from hash', async () => {
      window.location.hash = '#/settings'
      window.dispatchEvent(new Event('hashchange'))
      const { route } = await import('./router')
      expect(get(route)).toBe('settings')
    })

    it('parses valid routes', async () => {
      const { route } = await import('./router')
      const validRoutes = ['system', 'pool', 'settings', 'history', 'diagnostics', 'update']
      for (const r of validRoutes) {
        window.location.hash = `#/${r}`
        window.dispatchEvent(new Event('hashchange'))
        expect(get(route)).toBe(r)
      }
    })

    it('returns dashboard for unknown route', async () => {
      window.location.hash = '#/unknown'
      window.dispatchEvent(new Event('hashchange'))
      const { route } = await import('./router')
      expect(get(route)).toBe('dashboard')
    })

    it('handles hash without leading slash', async () => {
      window.location.hash = '#settings'
      window.dispatchEvent(new Event('hashchange'))
      const { route } = await import('./router')
      expect(get(route)).toBe('settings')
    })
  })

  describe('goto', () => {
    it('sets hash to target route', async () => {
      const { goto } = await import('./router')
      goto('settings')
      expect(window.location.hash).toBe('#/settings')
    })

    it('updates store when hash changes', async () => {
      const { goto, route } = await import('./router')
      goto('pool')
      // Event is dispatched synchronously by setting hash
      window.dispatchEvent(new Event('hashchange'))
      expect(get(route)).toBe('pool')
    })

    it('navigates to different routes', async () => {
      const { goto } = await import('./router')
      const routes = ['system', 'diagnostics', 'update']
      for (const r of routes) {
        goto(r)
        expect(window.location.hash).toBe(`#/${r}`)
      }
    })
  })
})
