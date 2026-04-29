<script lang="ts">
  import Brand from 'ui-kit/Brand.svelte'
  import WifiSetup from './pages/WifiSetup.svelte'
  import Save from './pages/Save.svelte'
  import { fetchVersion } from './lib/api'

  type View = 'setup' | 'saving'
  let view = $state<View>('setup')
  let version = $state<string>('')

  fetchVersion().then(v => { version = v }).catch(() => {})

  function onSaved() {
    view = 'saving'
  }
</script>

<main>
  <Brand title="TaipanMiner">
    <p class="subtitle">First-time setup</p>
  </Brand>

  {#if view === 'setup'}
    <WifiSetup onSaved={onSaved} />
  {:else}
    <Save />
  {/if}

  <footer>
    <span>Powered by TaipanMiner{version ? ` v${version}` : ''}</span>
  </footer>
</main>

<style>
  main {
    max-width: 600px;
    margin: 0 auto;
    padding: 2rem 1.5rem 4rem;
    display: flex;
    flex-direction: column;
    gap: 1.25rem;
  }
  .subtitle {
    color: var(--label);
    font-size: 13px;
    margin: 0;
  }
  footer {
    text-align: center;
    margin-top: 1.5rem;
    color: var(--footer);
    font-size: 11px;
  }
</style>
