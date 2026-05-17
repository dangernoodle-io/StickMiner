<script lang="ts">
  import { onMount } from 'svelte'
  import UpdateBadge from './UpdateBadge.svelte'
  import { createUpdateAvailableState } from '../lib/updateAvailableState.svelte'

  const updateState = createUpdateAvailableState()

  onMount(() => {
    updateState.start()
    return () => updateState.stop()
  })
</script>

<UpdateBadge available={updateState.available} latest={updateState.latest} />
