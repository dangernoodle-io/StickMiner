<script lang="ts">
  let {
    open = $bindable(false),
    title,
    message,
    confirmLabel = 'Confirm',
    cancelLabel = 'Cancel',
    danger = false,
    /** Optional localStorage key. When set, user can tick "Don't show again" to skip future prompts. */
    skipKey = null,
    onconfirm,
    oncancel,
  }: {
    open?: boolean
    title: string
    message: string
    confirmLabel?: string
    cancelLabel?: string
    danger?: boolean
    skipKey?: string | null
    onconfirm?: () => void
    oncancel?: () => void
  } = $props()

  let dontShowAgain = $state(false)

  function confirm() {
    if (skipKey && dontShowAgain) {
      try { localStorage.setItem(skipKey, '1') } catch { /* ignore */ }
    }
    open = false
    onconfirm?.()
  }

  function cancel() {
    open = false
    oncancel?.()
  }

  export function shouldSkip(key: string): boolean {
    try { return localStorage.getItem(key) === '1' } catch { return false }
  }
</script>

{#if open}
  <div class="modal-backdrop" onclick={cancel} role="presentation"></div>
  <div class="modal-panel dialog" role="dialog" aria-modal="true" aria-labelledby="confirm-title">
    <h3 id="confirm-title">{title}</h3>
    <p>{message}</p>

    {#if skipKey}
      <label class="skip">
        <input type="checkbox" bind:checked={dontShowAgain} />
        Don't show this again
      </label>
    {/if}

    <div class="actions">
      <button class="btn outline" onclick={cancel}>{cancelLabel}</button>
      <button class="btn {danger ? 'danger' : 'primary'}" onclick={confirm}>{confirmLabel}</button>
    </div>
  </div>
{/if}

<style>
  .dialog {
    position: fixed;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    padding: 20px 22px;
    width: min(420px, calc(100vw - 32px));
    z-index: 41;
  }

  h3 {
    margin: 0 0 10px 0;
    color: var(--text);
    font-size: 15px;
    font-weight: 600;
  }

  p {
    margin: 0 0 16px 0;
    color: var(--label);
    font-size: 13px;
    line-height: 1.5;
  }

  .skip {
    display: flex;
    align-items: center;
    gap: 6px;
    margin-bottom: 16px;
    font-size: 12px;
    color: var(--muted);
    cursor: pointer;
  }

  .actions {
    display: flex;
    justify-content: flex-end;
    gap: 10px;
  }

</style>
