<script lang="ts">
  import { onMount, onDestroy } from 'svelte'
  import { postReboot, setLogLevel, fetchLogLevels, type LogLevel } from '../lib/api'
  import ConfirmDialog from '../components/ConfirmDialog.svelte'

  const REBOOT_SKIP_KEY = 'taipanminer.skipRebootConfirm'

  const LOG_MAX_LINES = 500
  const baseUrl = import.meta.env.VITE_MINER_URL ?? ''

  let panel: HTMLPreElement
  let lines: string[] = []
  let autoscroll = true
  let filter = ''
  let es: EventSource | null = null
  let status: 'connected' | 'disconnected' | 'external' | 'connecting' = 'connecting'
  let wasDisconnected = false

  // Log levels — fetched from GET /api/log/level
  let availableLevels: LogLevel[] = ['none', 'error', 'warn', 'info', 'debug', 'verbose']
  let tagLevels: { tag: string; level: LogLevel }[] = []
  let levelsLoading = false
  let levelsErr = ''
  let selectedTag = ''
  let selectedLevel: LogLevel = 'info'
  let applying = false
  let applyMsg = ''
  let applyKind: '' | 'ok' | 'err' = ''

  $: currentLevel = tagLevels.find((t) => t.tag === selectedTag)?.level ?? null
  $: if (currentLevel && !applying) selectedLevel = currentLevel

  async function loadLevels() {
    levelsLoading = true
    levelsErr = ''
    try {
      const data = await fetchLogLevels()
      availableLevels = data.levels
      tagLevels = data.tags.map((t) => ({ ...t }))
      if (!selectedTag && tagLevels.length) selectedTag = tagLevels[0].tag
    } catch (e) {
      levelsErr = (e as Error).message
    } finally {
      levelsLoading = false
    }
  }

  function onLevelChange(e: Event) {
    const target = e.currentTarget as HTMLSelectElement
    selectedLevel = target.value as LogLevel
    applyLevel()
  }

  async function applyLevel() {
    if (!selectedTag) return
    applying = true
    applyMsg = ''
    applyKind = ''
    try {
      await setLogLevel(selectedTag, selectedLevel)
      const idx = tagLevels.findIndex((t) => t.tag === selectedTag)
      if (idx >= 0) {
        tagLevels[idx] = { ...tagLevels[idx], level: selectedLevel }
        tagLevels = tagLevels
      }
      applyKind = 'ok'
      applyMsg = `${selectedTag} → ${selectedLevel}`
    } catch (e) {
      applyKind = 'err'
      applyMsg = (e as Error).message
    } finally {
      applying = false
    }
  }

  let rebooting = false
  let rebootMsg = ''
  let showRebootDialog = false

  $: filtered = filter
    ? lines.filter((l) => l.toLowerCase().includes(filter.toLowerCase()))
    : lines

  function start() {
    if (es) return
    status = 'connecting'
    es = new EventSource(`${baseUrl}/api/logs?source=browser`)
    es.onopen = () => {
      status = 'connected'
      if (wasDisconnected) {
        wasDisconnected = false
        // Device may have rebooted — re-query tag list (levels reset on reboot).
        loadLevels()
      }
    }
    es.onmessage = (e) => {
      lines = lines.concat(e.data)
      if (lines.length > LOG_MAX_LINES) lines = lines.slice(-LOG_MAX_LINES)
      if (autoscroll) {
        queueMicrotask(() => {
          if (panel) panel.scrollTop = panel.scrollHeight
        })
      }
    }
    es.onerror = () => {
      es?.close()
      es = null
      wasDisconnected = true
      fetch(`${baseUrl}/api/logs/status`)
        .then((r) => r.json())
        .then((d: { active: boolean; client: string }) => {
          status = d.active && d.client === 'external' ? 'external' : 'disconnected'
        })
        .catch(() => { status = 'disconnected' })
      setTimeout(start, 3000)
    }
  }

  function stop() {
    es?.close()
    es = null
    status = 'disconnected'
  }

  function clear() {
    lines = []
  }

  function onPanelScroll() {
    if (!panel) return
    const atBottom = panel.scrollHeight - panel.scrollTop - panel.clientHeight < 8
    autoscroll = atBottom
  }

  function requestReboot() {
    const skip = (() => {
      try { return localStorage.getItem(REBOOT_SKIP_KEY) === '1' } catch { return false }
    })()
    if (skip) {
      doReboot()
    } else {
      showRebootDialog = true
    }
  }

  async function doReboot() {
    rebooting = true
    rebootMsg = ''
    try {
      await postReboot()
      rebootMsg = 'Reboot requested. Device returns in ~15 seconds.'
    } catch (e) {
      rebootMsg = `Reboot failed: ${(e as Error).message}`
    } finally {
      rebooting = false
    }
  }

  onMount(() => { start(); loadLevels() })
  onDestroy(stop)
</script>

<div class="page">
  <div class="section">
    <div class="log-head">
      <h2>Live Logs
        <span class="status" data-state={status}>
          {#if status === 'connected'}Connected
          {:else if status === 'connecting'}Connecting…
          {:else if status === 'external'}External client connected
          {:else}Disconnected{/if}
        </span>
      </h2>
    </div>

    <div class="log-controls">
      <input
        class="filter"
        type="search"
        placeholder="Filter…"
        bind:value={filter}
        spellcheck="false"
      />

      <select
        class="sm-select"
        bind:value={selectedTag}
        disabled={levelsLoading || applying || tagLevels.length === 0}
        title="Log tag"
      >
        {#if tagLevels.length === 0}
          <option value="">—</option>
        {:else}
          {#each tagLevels as t}
            <option value={t.tag}>{t.tag}</option>
          {/each}
        {/if}
      </select>
      <select
        class="sm-select"
        value={selectedLevel}
        on:change={onLevelChange}
        disabled={applying || !selectedTag}
        title="Log level"
      >
        {#each availableLevels as lv}
          <option value={lv}>{lv}</option>
        {/each}
      </select>

      <span class="spacer"></span>

      <label class="autoscroll">
        <input type="checkbox" bind:checked={autoscroll} /> auto-scroll
      </label>
      <button class="btn outline sm" on:click={clear} disabled={!lines.length}>Clear</button>
    </div>

    {#if applyMsg}<div class="status-msg" data-kind={applyKind}>{applyMsg}</div>{/if}
    {#if levelsErr}<div class="status-msg" data-kind="err">{levelsErr}</div>{/if}

    <pre class="log-panel" bind:this={panel} on:scroll={onPanelScroll}>{#each filtered as l}{l}
{/each}</pre>
    {#if filter}
      <div class="filter-hint">
        {filtered.length} of {lines.length} lines match
      </div>
    {/if}
  </div>

  <div class="section">
    <h2>Device</h2>
    <button class="btn danger" on:click={requestReboot} disabled={rebooting}>
      {rebooting ? 'Rebooting…' : 'Reboot'}
    </button>
    {#if rebootMsg}<div class="status-msg">{rebootMsg}</div>{/if}
  </div>
</div>

<ConfirmDialog
  bind:open={showRebootDialog}
  title="Reboot device?"
  message="Mining will be interrupted while the device restarts. It should return in about 15 seconds."
  confirmLabel="Reboot"
  danger
  skipKey={REBOOT_SKIP_KEY}
  on:confirm={doReboot}
/>

<style>
  .page {
    display: flex;
    flex-direction: column;
    gap: 32px;
    padding-top: 12px;
  }

  h2 {
    color: var(--accent);
    margin: 0 0 14px 0;
    font-size: 14px;
    text-transform: uppercase;
    letter-spacing: 1px;
  }

  .hint {
    margin: 0 0 14px 0;
    font-size: 12px;
    color: var(--muted);
    line-height: 1.5;
  }

  code {
    font-family: ui-monospace, Menlo, monospace;
    font-size: 11px;
    background: var(--input);
    padding: 1px 5px;
    border-radius: 3px;
    color: var(--text);
  }


  input[type="text"], input[type="search"], select {
    padding: 8px 10px;
    background: var(--input);
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: 4px;
    font-size: 12px;
    font-family: inherit;
    font-variant-numeric: tabular-nums;
    transition: border-color 0.15s;
  }

  input:focus, select:focus {
    outline: none;
    border-color: var(--accent);
  }

  select { min-width: 110px; }

  .log-head {
    margin-bottom: 12px;
  }

  .log-head h2 {
    margin: 0;
    display: inline-flex;
    align-items: baseline;
    gap: 14px;
  }

  .log-controls {
    display: flex;
    align-items: center;
    gap: 10px;
    font-size: 11px;
    color: var(--muted);
    margin-bottom: 10px;
    flex-wrap: wrap;
  }

  .log-controls .filter {
    flex: 1;
    min-width: 140px;
  }

  .spacer {
    flex: 1;
    min-width: 8px;
  }

  .sm-select {
    padding: 8px 10px;
    font-size: 12px;
  }

  .filter {
    min-width: 140px;
  }

  .status {
    font-size: 11px;
    color: var(--muted);
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }

  .status[data-state="connected"] { color: var(--success); }
  .status[data-state="external"] { color: var(--warning); }
  .status[data-state="disconnected"] { color: var(--danger); }

  .autoscroll {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    cursor: pointer;
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }

  .log-panel {
    height: 420px;
    overflow-y: auto;
    margin: 0;
    padding: 10px 12px;
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: 6px;
    font-family: ui-monospace, Menlo, monospace;
    font-size: 11px;
    line-height: 1.5;
    color: var(--text);
    white-space: pre-wrap;
    word-break: break-all;
  }

  .filter-hint {
    margin-top: 8px;
    font-size: 10px;
    color: var(--muted);
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }

  .btn {
    border: 1px solid var(--border);
    background: var(--surface);
    color: var(--text);
    padding: 10px 20px;
    font-size: 12px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    font-weight: 600;
    border-radius: 4px;
    cursor: pointer;
    transition: background 0.15s, color 0.15s, border-color 0.15s;
  }

  .btn.sm { padding: 5px 12px; font-size: 10px; }

  .btn.primary {
    background: var(--accent);
    color: var(--bg);
    border-color: var(--accent);
  }

  .btn.primary:hover:not(:disabled) { background: var(--accent-hover); }

  .btn.outline {
    background: transparent;
    color: var(--label);
  }

  .btn.outline:hover:not(:disabled) {
    color: var(--text);
    border-color: var(--label);
  }

  .btn.danger {
    border-color: var(--danger);
    color: var(--danger);
  }

  .btn.danger:hover:not(:disabled) {
    background: var(--danger);
    color: var(--bg);
  }

  .btn:disabled { opacity: 0.4; cursor: not-allowed; }

  .status-msg {
    margin-top: 10px;
    font-size: 12px;
    color: var(--label);
  }

  .status-msg[data-kind="ok"] { color: var(--success); }
  .status-msg[data-kind="err"] { color: var(--danger); }
</style>
