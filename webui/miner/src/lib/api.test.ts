import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest'
import { patchFan } from './api'

describe('patchFan', () => {
  let fetchSpy: ReturnType<typeof vi.fn>

  beforeEach(() => {
    fetchSpy = vi.fn(async () => new Response('', { status: 200 }))
    ;(globalThis as unknown as { fetch: typeof fetch }).fetch = fetchSpy as unknown as typeof fetch
  })

  afterEach(() => {
    vi.restoreAllMocks()
  })

  it('POSTs form-urlencoded to /api/fan', async () => {
    await patchFan({ die_target_c: 65 })
    expect(fetchSpy).toHaveBeenCalledTimes(1)
    const [url, init] = fetchSpy.mock.calls[0]
    expect(url).toBe('/api/fan')
    expect(init.method).toBe('POST')
    expect(init.headers['Content-Type']).toBe('application/x-www-form-urlencoded')
    expect(init.body).toBe('die_target_c=65')
  })

  // TA-351 fixed: server-side parsing now accepts true/false/yes/no/on/off.
  it('encodes autofan as true / false', async () => {
    await patchFan({ autofan: true })
    expect(fetchSpy.mock.calls[0][1].body).toBe('autofan=true')

    await patchFan({ autofan: false })
    expect(fetchSpy.mock.calls[1][1].body).toBe('autofan=false')
  })

  it('serializes multiple fields and skips undefined', async () => {
    await patchFan({ autofan: true, die_target_c: 65, vr_target_c: 80, min_pct: 40, manual_pct: undefined })
    const body = fetchSpy.mock.calls[0][1].body as string
    const params = new URLSearchParams(body)
    expect(params.get('autofan')).toBe('true')
    expect(params.get('die_target_c')).toBe('65')
    expect(params.get('vr_target_c')).toBe('80')
    expect(params.get('min_pct')).toBe('40')
    expect(params.has('manual_pct')).toBe(false)
  })

  it('throws on non-OK response', async () => {
    fetchSpy.mockResolvedValueOnce(new Response('', { status: 400 }))
    await expect(patchFan({ die_target_c: 65 })).rejects.toThrow(/400/)
  })

  it('sends both die and vr targets', async () => {
    await patchFan({ die_target_c: 65, vr_target_c: 80 })
    const body = fetchSpy.mock.calls[0][1].body as string
    const params = new URLSearchParams(body)
    expect(params.get('die_target_c')).toBe('65')
    expect(params.get('vr_target_c')).toBe('80')
  })
})
