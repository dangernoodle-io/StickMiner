<script lang="ts">
  let {
    value = $bindable(''),
    id,
    placeholder = '',
    maxlength,
    disabled = false,
  }: {
    value: string
    id?: string
    placeholder?: string
    maxlength?: number
    disabled?: boolean
  } = $props()

  let show = $state(false)
</script>

<div class="pw-group">
  <input
    {id}
    type={show ? 'text' : 'password'}
    bind:value
    {placeholder}
    {maxlength}
    {disabled}
  />
  <button
    type="button"
    class="toggle"
    onclick={(e) => { e.preventDefault(); show = !show }}
    {disabled}
    aria-label={show ? 'Hide password' : 'Show password'}
  >
    {#if show}
      <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" aria-hidden="true">
        <path d="M17.94 17.94A10.07 10.07 0 0112 20c-7 0-11-8-11-8a18.45 18.45 0 015.06-5.94M9.9 4.24A9.12 9.12 0 0112 4c7 0 11 8 11 8a18.5 18.5 0 01-2.16 3.19"/>
        <line x1="1" y1="1" x2="23" y2="23"/>
      </svg>
    {:else}
      <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" aria-hidden="true">
        <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/>
        <circle cx="12" cy="12" r="3"/>
      </svg>
    {/if}
  </button>
</div>

<style>
  .pw-group {
    position: relative;
    display: flex;
  }

  input {
    flex: 1;
    width: 100%;
    box-sizing: border-box;
    padding: 7px 36px 7px 10px;
    background: var(--input);
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--text);
    font-size: 12px;
    font-family: inherit;
    font-variant-numeric: tabular-nums;
    transition: border-color 0.15s;
  }

  input:focus { outline: none; border-color: var(--accent); }
  input:disabled { opacity: 0.6; cursor: not-allowed; }

  .toggle {
    position: absolute;
    right: 8px;
    top: 50%;
    transform: translateY(-50%);
    background: none;
    border: none;
    color: var(--accent);
    cursor: pointer;
    padding: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    width: 24px;
    height: 24px;
  }

  .toggle:hover:not(:disabled) { color: var(--accent-hover); }
  .toggle:disabled { opacity: 0.6; cursor: not-allowed; }
</style>
