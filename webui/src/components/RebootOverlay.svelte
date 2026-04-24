<script lang="ts">
  import { rebooting } from '../lib/stores'
</script>

{#if $rebooting.active}
  <div class="backdrop" role="alert" aria-live="polite" aria-busy="true">
    <div class="panel">
      {#if $rebooting.timedOut}
        <div class="icon timed-out">!</div>
        <h3>Miner not responding</h3>
        <p>
          Waited over 90 seconds after {$rebooting.reason}. The device may be
          stuck during boot, WiFi reconnect, or an OTA rollback. Check the
          serial console or power-cycle manually.
        </p>
      {:else}
        <div class="spinner"></div>
        <h3>{$rebooting.reason}</h3>
        <p>Waiting for the miner to come back… {$rebooting.elapsed}s</p>
      {/if}
    </div>
  </div>
{/if}

<style>
  .backdrop {
    position: fixed;
    inset: 0;
    background: rgba(0, 0, 0, 0.6);
    backdrop-filter: blur(2px);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 100;
  }

  .panel {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 32px 36px;
    width: min(420px, calc(100vw - 32px));
    text-align: center;
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.5);
  }

  .spinner {
    width: 42px;
    height: 42px;
    border: 3px solid var(--input);
    border-top-color: var(--accent);
    border-radius: 50%;
    margin: 0 auto 18px;
    animation: spin 0.9s linear infinite;
  }

  @keyframes spin {
    to { transform: rotate(360deg); }
  }

  .icon {
    width: 42px;
    height: 42px;
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    margin: 0 auto 18px;
    font-size: 22px;
    font-weight: 600;
  }

  .icon.timed-out {
    background: rgba(231, 76, 60, 0.15);
    color: var(--danger);
    border: 2px solid var(--danger);
  }

  h3 {
    margin: 0 0 10px 0;
    color: var(--text);
    font-size: 16px;
    font-weight: 600;
  }

  p {
    margin: 0;
    color: var(--label);
    font-size: 13px;
    line-height: 1.5;
  }
</style>
