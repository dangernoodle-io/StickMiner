import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import { slotFromCurrent, slotFromForm, waitForFreshSession, defaultForm, type PoolForm } from './poolHelpers'
import type { PoolConfigured } from './api'

describe('defaultForm', () => {
  it('returns expected defaults', () => {
    const f = defaultForm()
    expect(f.host).toBe('')
    expect(f.port).toBe(0)
    expect(f.wallet).toBe('')
    expect(f.worker).toBe('')
    expect(f.pool_pass).toBe('')
    expect(f.extranonce_subscribe).toBe(false)
    expect(f.decode_coinbase).toBe(true)
  })
})

describe('slotFromCurrent', () => {
  const configured: NonNullable<PoolConfigured> = {
    host: 'pool.example.com',
    port: 3333,
    worker: 'worker1',
    wallet: 'wallet1',
    extranonce_subscribe: true,
    decode_coinbase: false,
  }

  it('copies all fields from configured slot', () => {
    const slot = slotFromCurrent(configured)
    expect(slot.host).toBe('pool.example.com')
    expect(slot.port).toBe(3333)
    expect(slot.worker).toBe('worker1')
    expect(slot.wallet).toBe('wallet1')
    expect(slot.extranonce_subscribe).toBe(true)
    expect(slot.decode_coinbase).toBe(false)
  })

  it('omits pool_pass', () => {
    const slot = slotFromCurrent(configured)
    expect('pool_pass' in slot).toBe(false)
  })

  it('defaults extranonce_subscribe to false when undefined', () => {
    const slot = slotFromCurrent({ ...configured, extranonce_subscribe: undefined as any })
    expect(slot.extranonce_subscribe).toBe(false)
  })

  it('defaults decode_coinbase to true when undefined', () => {
    const slot = slotFromCurrent({ ...configured, decode_coinbase: undefined as any })
    expect(slot.decode_coinbase).toBe(true)
  })
})

describe('slotFromForm', () => {
  const form: PoolForm = {
    host: '  pool.example.com  ',
    port: 3333,
    wallet: '  wallet1  ',
    worker: '  worker1  ',
    pool_pass: 'secret',
    extranonce_subscribe: true,
    decode_coinbase: false,
  }

  it('trims whitespace from host, worker, wallet', () => {
    const slot = slotFromForm(form)
    expect(slot.host).toBe('pool.example.com')
    expect(slot.worker).toBe('worker1')
    expect(slot.wallet).toBe('wallet1')
  })

  it('includes pool_pass when non-empty', () => {
    const slot = slotFromForm(form)
    expect(slot.pool_pass).toBe('secret')
  })

  it('omits pool_pass when blank', () => {
    const slot = slotFromForm({ ...form, pool_pass: '' })
    expect('pool_pass' in slot).toBe(false)
  })

  it('copies port, extranonce_subscribe, decode_coinbase', () => {
    const slot = slotFromForm(form)
    expect(slot.port).toBe(3333)
    expect(slot.extranonce_subscribe).toBe(true)
    expect(slot.decode_coinbase).toBe(false)
  })
})

describe('waitForFreshSession', () => {
  beforeEach(() => { vi.useFakeTimers() })
  afterEach(() => { vi.useRealTimers() })

  it('resolves when session age drops below preAge', async () => {
    let callCount = 0
    const ages = [10, 8, 3] // third call: 3 < 5 (preAge), should resolve

    const refresh = vi.fn().mockResolvedValue(undefined)
    const getSessionAge = vi.fn(() => ages[callCount++] ?? null)

    const promise = waitForFreshSession({
      refresh,
      getSessionAge,
      preAge: 5,
      deadlineMs: Date.now() + 10000,
      intervalMs: 100,
    })

    // Advance timers for 3 iterations
    await vi.advanceTimersByTimeAsync(300)
    await promise
    expect(getSessionAge).toHaveBeenCalledTimes(3)
  })

  it('resolves immediately when preAge is null', async () => {
    const refresh = vi.fn().mockResolvedValue(undefined)
    const getSessionAge = vi.fn(() => 99)

    const promise = waitForFreshSession({
      refresh,
      getSessionAge,
      preAge: null,
      deadlineMs: Date.now() + 10000,
      intervalMs: 100,
    })

    await vi.advanceTimersByTimeAsync(100)
    await promise
    expect(getSessionAge).toHaveBeenCalledTimes(1)
  })

  it('exits without error when deadline passes before age drops', async () => {
    const refresh = vi.fn().mockResolvedValue(undefined)
    const getSessionAge = vi.fn(() => 100) // never drops below preAge=5

    const promise = waitForFreshSession({
      refresh,
      getSessionAge,
      preAge: 5,
      deadlineMs: Date.now() + 300,
      intervalMs: 100,
    })

    await vi.advanceTimersByTimeAsync(400)
    await promise // should resolve (not reject) on deadline
    expect(refresh).toHaveBeenCalled()
  })

  it('skips when getSessionAge returns null', async () => {
    let call = 0
    const ages = [null, null, 2] // third: 2 < 5 => break

    const refresh = vi.fn().mockResolvedValue(undefined)
    const getSessionAge = vi.fn(() => ages[call++] as number | null)

    const promise = waitForFreshSession({
      refresh,
      getSessionAge,
      preAge: 5,
      deadlineMs: Date.now() + 10000,
      intervalMs: 100,
    })

    await vi.advanceTimersByTimeAsync(300)
    await promise
    expect(getSessionAge).toHaveBeenCalledTimes(3)
  })

  it('calls refresh before getSessionAge on each iteration', async () => {
    const order: string[] = []
    const refresh = vi.fn(() => { order.push('refresh'); return Promise.resolve() })
    const getSessionAge = vi.fn(() => { order.push('age'); return 1 }) // immediately resolves (preAge=5)

    const promise = waitForFreshSession({
      refresh,
      getSessionAge,
      preAge: 5,
      deadlineMs: Date.now() + 10000,
      intervalMs: 100,
    })

    await vi.advanceTimersByTimeAsync(100)
    await promise
    expect(order[0]).toBe('refresh')
    expect(order[1]).toBe('age')
  })
})
