<script lang="ts">
  import { onMount } from 'svelte'
  import Select from 'ui-kit/Select.svelte'
  import { fetchScan, postSave, type AccessPoint } from '../lib/api'

  let { onSaved }: { onSaved: () => void } = $props()

  let networks = $state<AccessPoint[]>([])
  let scanning = $state(false)
  let scanError = $state<string | null>(null)
  let selectedSsid = $state<string>('')
  let manualSsid = $state('')
  let pass = $state('')
  let showPass = $state(false)
  let wallet = $state('')
  let worker = $state('')
  let poolHost = $state('')
  let poolPort = $state('')
  let poolPass = $state('')
  let errors = $state<Record<string, string>>({})
  let submitting = $state(false)
  let submitError = $state<string | null>(null)

  function getSignalStrength(rssi: number): string {
    if (rssi >= -50) return '▁▃▅█'
    if (rssi >= -67) return '▁▃▅'
    if (rssi >= -80) return '▁▃'
    return '?'
  }

  function buildSelectOptions(): Array<{ label: string; value: string }> {
    const opts = networks.map(ap => {
      const lock = ap.secure ? ' 🔒' : ''
      const signal = getSignalStrength(ap.rssi)
      return { label: `${ap.ssid}${lock} ${signal}`, value: ap.ssid }
    })
    opts.push({ label: 'Enter manually...', value: '__manual__' })
    return opts
  }

  async function scan() {
    scanning = true
    scanError = null
    networks = []
    selectedSsid = ''
    manualSsid = ''

    try {
      const aps = await fetchScan()
      networks = aps
      if (aps.length > 0) {
        selectedSsid = aps[0].ssid
      }
    } catch (e) {
      scanError = `Scan failed: ${e instanceof Error ? e.message : 'Unknown error'}`
    } finally {
      scanning = false
    }
  }

  function validate(): boolean {
    const newErrors: Record<string, string> = {}

    const ssid = selectedSsid === '__manual__' ? manualSsid : selectedSsid
    if (!ssid) {
      newErrors.ssid = 'Network is required'
    }

    if (!wallet.trim()) {
      newErrors.wallet = 'Required'
    }

    if (!worker.trim()) {
      newErrors.worker = 'Required'
    }

    if (!poolHost.trim()) {
      newErrors.poolHost = 'Required'
    }

    const port = parseInt(poolPort, 10)
    if (!poolPort || isNaN(port) || port < 1 || port > 65535) {
      newErrors.poolPort = 'Valid port (1–65535) required'
    }

    errors = newErrors
    return Object.keys(newErrors).length === 0
  }

  async function handleSubmit() {
    if (!validate()) return

    submitting = true
    submitError = null

    try {
      const ssid = selectedSsid === '__manual__' ? manualSsid : selectedSsid
      await postSave({
        ssid,
        pass,
        wallet,
        worker,
        pool_host: poolHost,
        pool_port: poolPort,
        pool_pass: poolPass
      })
      onSaved()
    } catch (e) {
      submitError = `Save failed: ${e instanceof Error ? e.message : 'Unknown error'}`
      submitting = false
    }
  }

  onMount(() => {
    scan()
  })

  $effect(() => {
    if (selectedSsid === '__manual__') {
      manualSsid = ''
    }
  })
</script>

<div class="setup-form">
  {#if submitError}
    <div class="error-banner">
      {submitError}
    </div>
  {/if}

  <form onsubmit={(e) => { e.preventDefault(); handleSubmit() }}>
    <section>
      <h2>WiFi</h2>
      <div class="form-group">
        <label>Network</label>
        <div class="scan-controls">
          <Select
            bind:value={selectedSsid}
            options={buildSelectOptions()}
            disabled={scanning || submitting}
          />
          <button
            type="button"
            class="rescan-btn"
            onclick={() => scan()}
            disabled={scanning || submitting}
          >
            ↻
          </button>
        </div>
        {#if scanError}
          <div class="inline-error">{scanError}</div>
        {/if}
      </div>

      {#if selectedSsid === '__manual__'}
        <div class="manual-entry">
          <input
            type="text"
            bind:value={manualSsid}
            placeholder="Enter SSID"
            maxlength="31"
            disabled={submitting}
          />
          {#if errors.ssid}
            <div class="field-error">{errors.ssid}</div>
          {/if}
        </div>
      {/if}

      <div class="form-group password-wrapper">
        <label for="pass">Password</label>
        <div class="password-input-group">
          <input
            id="pass"
            type={showPass ? 'text' : 'password'}
            bind:value={pass}
            maxlength="63"
            disabled={submitting}
          />
          <button
            type="button"
            class="toggle-pass"
            onclick={(e) => { e.preventDefault(); showPass = !showPass }}
            disabled={submitting}
          >
            {showPass ? '👁' : '👁‍🗨'}
          </button>
        </div>
      </div>
    </section>

    <section>
      <h2>Mining</h2>
      <div class="form-group">
        <label for="wallet">Wallet Address</label>
        <input
          id="wallet"
          type="text"
          bind:value={wallet}
          maxlength="63"
          placeholder="1BTC..."
          disabled={submitting}
        />
        {#if errors.wallet}
          <div class="field-error">{errors.wallet}</div>
        {/if}
      </div>

      <div class="form-group">
        <label for="worker">Worker Name</label>
        <input
          id="worker"
          type="text"
          bind:value={worker}
          maxlength="31"
          placeholder="miner-1"
          disabled={submitting}
        />
        {#if errors.worker}
          <div class="field-error">{errors.worker}</div>
        {/if}
      </div>
    </section>

    <section>
      <h2>Pool</h2>
      <div class="form-group">
        <label for="pool_host">Host</label>
        <input
          id="pool_host"
          type="text"
          bind:value={poolHost}
          maxlength="63"
          placeholder="pool.example.com"
          disabled={submitting}
        />
        {#if errors.poolHost}
          <div class="field-error">{errors.poolHost}</div>
        {/if}
      </div>

      <div class="form-group">
        <label for="pool_port">Port</label>
        <input
          id="pool_port"
          type="text"
          inputmode="numeric"
          bind:value={poolPort}
          maxlength="5"
          placeholder="3333"
          disabled={submitting}
        />
        {#if errors.poolPort}
          <div class="field-error">{errors.poolPort}</div>
        {/if}
      </div>

      <div class="form-group">
        <label for="pool_pass">Password</label>
        <input
          id="pool_pass"
          type="text"
          bind:value={poolPass}
          maxlength="63"
          placeholder="optional"
          disabled={submitting}
        />
      </div>
    </section>

    <button type="submit" class="submit-btn" disabled={submitting || scanning}>
      {submitting ? 'Saving...' : 'Save & Connect'}
    </button>
  </form>
</div>

<style>
  .setup-form {
    display: flex;
    flex-direction: column;
    gap: 1rem;
  }

  .error-banner {
    background: #fee;
    border: 1px solid #fcc;
    color: #c00;
    padding: 0.75rem;
    border-radius: 4px;
    font-size: 13px;
  }

  form {
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
    font-size: 16px;
    margin: 0;
    color: var(--text);
    font-weight: 600;
  }

  .form-group {
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
  }

  label {
    font-size: 13px;
    color: var(--label);
    font-weight: 500;
  }

  input {
    padding: 0.75rem;
    background: var(--input);
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--text);
    font-size: 14px;
    font-family: inherit;
  }

  input:focus {
    outline: none;
    border-color: var(--accent);
  }

  input:disabled {
    opacity: 0.6;
    cursor: not-allowed;
  }

  .scan-controls {
    display: flex;
    gap: 0.5rem;
    align-items: stretch;
  }

  .rescan-btn {
    background: var(--input);
    border: 1px solid var(--border);
    color: var(--accent);
    padding: 0.75rem;
    border-radius: 4px;
    cursor: pointer;
    font-size: 16px;
    min-width: 44px;
    flex-shrink: 0;
    transition: border-color 0.2s;
  }

  .rescan-btn:hover:not(:disabled) {
    border-color: var(--accent);
  }

  .rescan-btn:disabled {
    opacity: 0.6;
    cursor: not-allowed;
  }

  .inline-error {
    color: #c00;
    font-size: 12px;
    margin-top: 0.25rem;
  }

  .manual-entry {
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
    margin-top: 0.5rem;
  }

  .password-wrapper {
    position: relative;
  }

  .password-input-group {
    position: relative;
    display: flex;
  }

  .password-input-group input {
    flex: 1;
    padding-right: 40px;
  }

  .toggle-pass {
    position: absolute;
    right: 12px;
    top: 50%;
    transform: translateY(-50%);
    background: none;
    border: none;
    color: var(--accent);
    cursor: pointer;
    font-size: 14px;
    padding: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    width: 24px;
    height: 24px;
  }

  .toggle-pass:disabled {
    opacity: 0.6;
    cursor: not-allowed;
  }

  .field-error {
    color: #c00;
    font-size: 12px;
    margin-top: 0.25rem;
  }

  .submit-btn {
    background: var(--accent);
    color: var(--button-text, #000);
    border: none;
    padding: 0.75rem;
    border-radius: 4px;
    font-size: 14px;
    font-weight: 600;
    cursor: pointer;
    min-height: 44px;
    transition: opacity 0.2s;
    margin-top: 0.5rem;
  }

  .submit-btn:hover:not(:disabled) {
    opacity: 0.9;
  }

  .submit-btn:disabled {
    opacity: 0.6;
    cursor: not-allowed;
  }
</style>
