<script lang="ts">
  import { createEventDispatcher } from 'svelte'
  import Tooltip from './Tooltip.svelte'
  import Toggle from './Toggle.svelte'
  import PasswordInput from 'ui-kit/PasswordInput.svelte'

  type PoolForm = {
    host: string
    port: number
    wallet: string
    worker: string
    pool_pass: string
    extranonce_subscribe: boolean
    decode_coinbase: boolean
  }

  export let form: PoolForm
  export let kind: 'Primary' | 'Fallback'
  export let saving: boolean = false
  export let saveMsg: string = ''
  export let workerPlaceholder: string = 'miner-1'

  const dispatch = createEventDispatcher<{ save: void; cancel: void }>()
</script>

<form class="setup-form" on:submit|preventDefault={() => dispatch('save')}>
  <section>
    <h2>{kind} pool</h2>

    <div class="form-group">
      <div class="lbl-row">
        <label for="pool-host">Host</label>
        <Tooltip icon text="Stratum pool hostname or IP. Leave off the protocol — TCP only." />
      </div>
      <input id="pool-host" type="text" bind:value={form.host} maxlength="63" placeholder="pool.example.com" required disabled={saving} />
    </div>

    <div class="form-group">
      <div class="lbl-row">
        <label for="pool-port">Port</label>
        <Tooltip icon text="Stratum TCP port. Common values: 3333, 4334, 9999. Pool-specific — check the pool's docs." />
      </div>
      <input id="pool-port" type="number" bind:value={form.port} min="1" max="65535" placeholder="3333" required disabled={saving} />
    </div>

    <div class="form-group">
      <div class="lbl-row">
        <label for="pool-pass">Password</label>
        <Tooltip icon text="Pool password. Most pools accept any value (often 'x' or empty); some use it for worker config flags." />
      </div>
      <PasswordInput id="pool-pass" bind:value={form.pool_pass} placeholder="leave blank to keep current" disabled={saving} />
    </div>

    <div class="form-group">
      <div class="lbl-row">
        <label for="pool-wallet">Wallet</label>
        <Tooltip icon text="Bitcoin payout address registered with this pool. Most pools require a bech32 (bc1…) or legacy (1…/3…) address." />
      </div>
      <input id="pool-wallet" type="text" bind:value={form.wallet} spellcheck="false" required disabled={saving} />
    </div>

    <div class="form-group">
      <div class="lbl-row">
        <label for="pool-worker">Worker</label>
        <Tooltip icon text="Worker name appended to the wallet (wallet.worker) so the pool can track multiple miners separately. Defaults to the device hostname." />
      </div>
      <input id="pool-worker" type="text" bind:value={form.worker} placeholder={workerPlaceholder} required disabled={saving} />
    </div>
  </section>

  <details class="options">
    <summary>Advanced Options</summary>

    <div class="opt-row">
      <span class="opt-label">
        Extranonce subscribe
        <Tooltip icon text="Send mining.extranonce.subscribe after authorize so the pool can roll extranonce1 mid-session without forcing reconnect. Pools that don't support the extension reject it harmlessly." />
      </span>
      <Toggle bind:checked={form.extranonce_subscribe} disabled={saving} size="sm" />
    </div>

    <div class="opt-row">
      <span class="opt-label">
        Decode coinbase
        <Tooltip icon text="Decode coinbase tx for block height, scriptSig tag, payout, and reward. Turn off for non-BTC SHA-256d pools whose coinbase shape is unknown." />
      </span>
      <Toggle bind:checked={form.decode_coinbase} disabled={saving} size="sm" />
    </div>
  </details>

  <div class="actions">
    <button type="button" class="btn outline" on:click={() => dispatch('cancel')} disabled={saving}>Cancel</button>
    <button type="submit" class="btn primary" disabled={saving}>{saving ? 'Saving…' : 'Save'}</button>
  </div>
  {#if saveMsg}<div class="msg">{saveMsg}</div>{/if}
</form>

<style>
  .setup-form {
    display: flex;
    flex-direction: column;
    gap: 1.5rem;
  }

  section {
    display: flex;
    flex-direction: column;
    gap: 1rem;
  }

  h2 {
    font-size: 14px;
    margin: 0;
    color: var(--accent);
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 1px;
  }

  .form-group {
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
  }

  .lbl-row {
    display: flex;
    align-items: center;
    gap: 6px;
  }

  label {
    font-size: 12px;
    color: var(--label);
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }

  input[type="text"], input[type="number"] {
    padding: 7px 10px;
    background: var(--input);
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--text);
    font-size: 12px;
    font-family: inherit;
    font-variant-numeric: tabular-nums;
    box-sizing: border-box;
    width: 100%;
  }


  input:focus { outline: none; border-color: var(--accent); }
  input:disabled { opacity: 0.6; cursor: not-allowed; }

  .options {
    border-top: 1px dashed var(--border);
    padding-top: 12px;
  }

  .options summary {
    cursor: pointer;
    list-style: none;
    color: var(--accent);
    font-size: 14px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 1px;
    user-select: none;
    display: inline-flex;
    align-items: center;
    gap: 8px;
  }

  .options summary::-webkit-details-marker { display: none; }

  .options summary::before {
    content: '▸';
    font-size: 18px;
    transition: transform 0.15s;
  }

  .options[open] summary::before { transform: rotate(90deg); }

  .options[open] {
    display: flex;
    flex-direction: column;
    gap: 1rem;
  }

  .options[open] .opt-row + .opt-row {
    margin-top: 6px;
  }

  .opt-row {
    display: flex;
    align-items: center;
    gap: 8px;
    text-transform: none;
    letter-spacing: normal;
    font-size: 13px;
  }

  .opt-label {
    flex: 1;
    display: inline-flex;
    align-items: center;
    gap: 6px;
    color: var(--text);
    font-weight: 600;
  }

  .actions {
    display: flex;
    justify-content: flex-end;
    gap: 10px;
    margin-top: 4px;
  }

  .msg {
    font-size: 12px;
    color: var(--success);
    margin-top: 6px;
  }
</style>
