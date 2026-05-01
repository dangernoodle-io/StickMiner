<script lang="ts">
  import { createEventDispatcher } from 'svelte'
  import PoolEditForm from './PoolEditForm.svelte'

  type PoolForm = {
    host: string
    port: number
    wallet: string
    worker: string
    pool_pass: string
    extranonce_subscribe: boolean
    decode_coinbase: boolean
  }

  export let open = false
  export let form: PoolForm
  export let kind: 'Primary' | 'Fallback'
  export let saving = false
  export let saveMsg = ''
  export let workerPlaceholder = 'miner-1'

  const dispatch = createEventDispatcher<{ save: void; cancel: void }>()

  function onBackdrop() {
    if (!saving) dispatch('cancel')
  }
</script>

{#if open}
  <div class="modal-backdrop" on:click={onBackdrop} role="presentation"></div>
  <div class="modal-panel dialog" role="dialog" aria-modal="true" aria-labelledby="pool-edit-title">
    <PoolEditForm
      bind:form
      {kind}
      {saving}
      {saveMsg}
      {workerPlaceholder}
      on:save
      on:cancel
    />
  </div>
{/if}

<style>
  .dialog {
    position: fixed;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    padding: 24px 26px;
    width: min(480px, calc(100vw - 32px));
    max-height: calc(100vh - 32px);
    overflow-y: auto;
    z-index: 41;
  }
</style>
