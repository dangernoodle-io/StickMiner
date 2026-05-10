import type { PoolConfigured, PoolConfigInput } from './api'

export type PoolForm = {
  host: string
  port: number
  wallet: string
  worker: string
  pool_pass: string
  extranonce_subscribe: boolean
  decode_coinbase: boolean
}

export const defaultForm = (): PoolForm => ({
  host: '',
  port: 0,
  wallet: '',
  worker: '',
  pool_pass: '',
  extranonce_subscribe: false,
  decode_coinbase: true,
})

// Build a PUT slot for a non-edited pool by mirroring its current state.
// Omits pool_pass — firmware preserves it on missing key (see PR #280).
export function slotFromCurrent(c: NonNullable<PoolConfigured>): PoolConfigInput {
  const next: PoolConfigInput = {
    host: c.host,
    port: c.port,
    worker: c.worker,
    wallet: c.wallet,
    pool_pass: '',
    extranonce_subscribe: c.extranonce_subscribe ?? false,
    decode_coinbase: c.decode_coinbase ?? true,
  }
  delete (next as Partial<PoolConfigInput>).pool_pass
  return next
}

// Build a PUT slot from the form (the edited pool). Omit pool_pass when
// the field is empty so the firmware preserves the stored value (we never
// echo passwords back, so blank means "unchanged"). Same convention as
// slotFromCurrent.
export function slotFromForm(form: PoolForm): PoolConfigInput {
  const next: PoolConfigInput = {
    host: form.host.trim(),
    port: form.port,
    worker: form.worker.trim(),
    wallet: form.wallet.trim(),
    pool_pass: form.pool_pass,
    extranonce_subscribe: form.extranonce_subscribe,
    decode_coinbase: form.decode_coinbase,
  }
  if (!form.pool_pass) delete (next as Partial<PoolConfigInput>).pool_pass
  return next
}

export interface WaitForFreshSessionOptions {
  refresh: () => Promise<void>
  getSessionAge: () => number | null | undefined
  preAge: number | null
  deadlineMs: number
  intervalMs?: number
}

// Promise that polls refresh() then getSessionAge() until age drops below
// preAge or deadline elapses.
export async function waitForFreshSession({
  refresh,
  getSessionAge,
  preAge,
  deadlineMs,
  intervalMs = 750,
}: WaitForFreshSessionOptions): Promise<void> {
  while (Date.now() < deadlineMs) {
    await new Promise(r => setTimeout(r, intervalMs))
    await refresh()
    const age = getSessionAge()
    if (age == null) continue
    if (preAge == null || age < preAge) break
  }
}
