<script lang="ts">
  import { fan, fanEditOpen } from '../lib/stores'
  import { fetchFan, patchFan, type FanPatch } from '../lib/api'
  import Toggle from './Toggle.svelte'

  let autofan = false
  let target = 60
  let minPct = 35
  let manual = 80
  let saving = false
  let msg = ''
  let kind: '' | 'ok' | 'err' = ''

  $: if ($fanEditOpen && $fan) {
    autofan = $fan.autofan
    target = $fan.temp_target_c
    minPct = $fan.min_pct
    manual = $fan.manual_pct
    msg = ''
    kind = ''
  }

  async function save() {
    if (!$fan) return
    saving = true
    msg = ''
    kind = ''
    const patch: FanPatch = {}
    if (autofan !== $fan.autofan) patch.autofan = autofan
    if (target !== $fan.temp_target_c) patch.temp_target_c = target
    if (minPct !== $fan.min_pct) patch.min_pct = minPct
    if (manual !== $fan.manual_pct) patch.manual_pct = manual
    try {
      await patchFan(patch)
      fan.set(await fetchFan())
      kind = 'ok'
      msg = 'Saved'
      setTimeout(close, 400)
    } catch (e) {
      kind = 'err'
      msg = (e as Error).message
    } finally {
      saving = false
    }
  }

  function close() {
    if (!saving) fanEditOpen.set(false)
  }
</script>

{#if $fanEditOpen}
  <div class="modal-backdrop" on:click={close} role="presentation"></div>
  <div class="modal-panel dialog" role="dialog" aria-modal="true" aria-labelledby="fan-edit-title">
    <form class="setup-form" on:submit|preventDefault={save}>
      <section>
        <h2 id="fan-edit-title">Fan</h2>

        <div class="opt-row">
          <span class="opt-label">
            Autofan
            <span class="mode-badge" data-mode={autofan ? 'auto' : 'manual'}>
              {autofan ? 'Closed-loop PID' : 'Manual'}
            </span>
          </span>
          <Toggle bind:checked={autofan} disabled={saving} size="sm" />
        </div>

        <div class="slider-group" class:disabled={saving || !autofan}>
          <div class="slider-head">
            <label for="fan-target">Target temperature</label>
            <span class="val">{target}°C</span>
          </div>
          <input id="fan-target" type="range" min="35" max="85" step="1" bind:value={target} disabled={saving || !autofan} />
        </div>

        <div class="slider-group" class:disabled={saving || !autofan}>
          <div class="slider-head">
            <label for="fan-min">Minimum fan speed</label>
            <span class="val">{minPct}%</span>
          </div>
          <input id="fan-min" type="range" min="0" max="100" step="1" bind:value={minPct} disabled={saving || !autofan} />
        </div>

        <div class="slider-group" class:disabled={saving || autofan}>
          <div class="slider-head">
            <label for="fan-manual">Fan speed</label>
            <span class="val">{manual}%</span>
          </div>
          <input id="fan-manual" type="range" min="0" max="100" step="1" bind:value={manual} disabled={saving || autofan} />
        </div>
      </section>

      {#if $fan}
        <p class="live-line">
          Live: {$fan.duty_pct ?? '—'}% · {$fan.rpm ?? '—'} rpm
          {#if $fan.autofan && $fan.pid_input_src}
            · PID following {$fan.pid_input_src.toUpperCase()}
            {#if $fan.pid_input_c != null} ({$fan.pid_input_c.toFixed(1)}°C){/if}
          {/if}
        </p>
      {/if}

      <div class="actions">
        <button type="button" class="btn outline" on:click={close} disabled={saving}>Cancel</button>
        <button type="submit" class="btn primary" disabled={saving || !$fan}>
          {saving ? 'Saving…' : 'Save'}
        </button>
      </div>
      {#if msg}<div class="msg" class:err={kind === 'err'}>{msg}</div>{/if}
    </form>
  </div>
{/if}

<style>
  .dialog {
    position: fixed;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    padding: 22px 24px;
    width: min(420px, calc(100vw - 32px));
    max-height: calc(100vh - 32px);
    overflow-y: auto;
    z-index: 41;
  }

  .live-line {
    margin: 0;
    font-size: 11px;
    color: var(--muted);
    line-height: 1.5;
  }
</style>
