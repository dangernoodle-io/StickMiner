<script lang="ts">
  import { stats, power, fan, hasAsic, fanEditOpen } from '../lib/stores'
  import Hero from '../components/Hero.svelte'
  import ChipsCard from '../components/ChipsCard.svelte'
  import StatTile from '../components/StatTile.svelte'
  import PoolStrip from '../components/PoolStrip.svelte'

  function openFanEdit() { fanEditOpen.set(true) }

  $: chips = $stats?.asic_chips ?? []
  $: expectedPerDomain = $stats?.asic_total_ghs && chips.length
    ? $stats.asic_total_ghs / chips.length / 4
    : undefined
  $: pcoreW = $power?.pcore_mw != null ? $power.pcore_mw / 1000 : null
  $: expectedGhs = $stats?.expected_ghs ?? null
</script>

<div class="sticky-pool"><PoolStrip /></div>

<div class="grid">
  <section class="card full">
    <Hero />
  </section>

  {#if $hasAsic && chips.length > 0}
    <div class="full">
      <ChipsCard {chips} expected_per_domain={expectedPerDomain} />
    </div>
  {/if}

  {#if $hasAsic}
    <section class="card">
      <h3>Heat</h3>
      <div class="tile-grid">
        <StatTile label="ASIC"  value={$stats?.asic_temp_c ?? null}     unit="°C" warn={70} danger={80} />
        <StatTile label="Board" value={$power?.board_temp_c ?? null}    unit="°C" warn={60} danger={75} />
        <StatTile label="VR"    value={$power?.vr_temp_c ?? null}       unit="°C" warn={75} danger={90} />
      </div>
    </section>

    <section class="card clickable" on:click={openFanEdit} on:keydown={(e) => { if (e.key === 'Enter' || e.key === ' ') openFanEdit() }} role="button" tabindex="0" title="Edit fan settings">
      <h3>
        Fan
        {#if $fan?.autofan && $fan?.pid_input_src}
          <span class="mode-badge" data-mode="auto" title="Autofan PID input source">PID: {$fan.pid_input_src.toUpperCase()}</span>
        {/if}
        <span class="edit-hint">edit</span>
      </h3>
      <div class="tile-grid">
        <StatTile label="Fan Speed" value={$fan?.duty_pct ?? null} unit="%"   />
        <StatTile label="RPM"       value={$fan?.rpm ?? null}      unit="rpm" />
      </div>
      {#if $fan?.duty_pct != null}
        <div class="duty-bar"><div class="duty-fill" style="width: {$fan.duty_pct}%"></div></div>
      {/if}
    </section>

    <section class="card">
      <h3>Power</h3>
      <div class="tile-grid">
        <StatTile label="Power Draw"    value={pcoreW}                                                           unit="W"    warn={25} danger={35} />
        <StatTile label="Efficiency"    value={$power?.efficiency_jth ?? null}                                   unit="J/TH" />
        <StatTile label="ASIC Voltage"  value={$power?.vcore_mv != null ? $power.vcore_mv / 1000 : null}         unit="V"    />
        <StatTile label="ASIC Current"  value={$power?.icore_ma != null ? $power.icore_ma / 1000 : null}         unit="A"    />
        <StatTile label="Input Voltage" value={$power?.vin_mv != null ? $power.vin_mv / 1000 : null}             unit="V"    flag={$power?.vin_low ? 'warn' : null} />
      </div>
    </section>

    <section class="card">
      <h3>Performance</h3>
      <div class="tile-grid">
        <StatTile label="Freq cfg"  value={$stats?.asic_freq_configured_mhz ?? null} unit="MHz" />
        <StatTile label="Freq eff"  value={$stats?.asic_freq_effective_mhz ?? null}  unit="MHz" />
        <StatTile label="Expected"  value={expectedGhs}                              unit="GH/s"/>
        <StatTile label="Cores"     value={$stats?.asic_small_cores ?? null}         />
        <StatTile label="Chips"     value={$stats?.asic_count ?? null}               />
      </div>
    </section>
  {/if}

</div>

<style>
  .sticky-pool {
    position: sticky;
    top: 42px;
    z-index: 15;
    background: rgba(26, 26, 46, 0.92);
    backdrop-filter: blur(8px);
    margin: 0 -16px 14px;
    padding: 8px 16px;
  }

  .sticky-pool :global(.pool-strip) {
    margin-bottom: 0;
  }

  .card.full { padding: 0; }

  /* card h3 typography lives in ui-kit utilities.css; only the bottom gap
     before .tile-grid is page-specific. */
  h3 { margin-bottom: 12px; }

  .tile-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    align-items: flex-start;
    gap: 14px 18px;
  }

  @media (max-width: 720px) {
    .tile-grid {
      grid-template-columns: repeat(auto-fit, minmax(90px, 1fr));
    }
  }

  .duty-bar {
    height: 3px;
    background: var(--border);
    border-radius: 2px;
    overflow: hidden;
    margin-top: 12px;
  }

  .duty-fill {
    height: 100%;
    background: var(--accent);
    transition: width 0.5s ease;
  }

  .clickable { cursor: pointer; transition: border-color 0.15s ease; }
  .clickable:hover { border-color: var(--accent); }
  .clickable:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }

  .edit-hint {
    float: right;
    font-size: 10px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: var(--muted);
    opacity: 0;
    transition: opacity 0.15s ease;
  }
  .clickable:hover .edit-hint, .clickable:focus-visible .edit-hint { opacity: 1; }

  /* mode-badge styles live in ui-kit/utilities.css; only positioning here. */
  h3 :global(.mode-badge) {
    margin-left: 8px;
    vertical-align: middle;
  }
</style>
