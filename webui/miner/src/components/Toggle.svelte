<script lang="ts">
  let {
    checked = $bindable(),
    disabled = false,
    size = 'md' as 'sm' | 'md',
    onchange,
  }: {
    checked: boolean
    disabled?: boolean
    size?: 'sm' | 'md'
    onchange?: (e: Event) => void
  } = $props()

  function handleChange(e: Event) {
    const target = e.currentTarget as HTMLInputElement
    checked = target.checked
    onchange?.(e)
  }
</script>

<label class="toggle" class:disabled class:sm={size === 'sm'}>
  <input type="checkbox" {checked} {disabled} onchange={handleChange} />
  <span class="slider"></span>
</label>

<style>
  .toggle {
    position: relative;
    display: inline-block;
    width: 44px;
    height: 24px;
    cursor: pointer;
    flex-shrink: 0;
  }

  .toggle.sm { width: 32px; height: 18px; }
  .toggle.sm .slider::before { width: 12px; height: 12px; }
  .toggle.sm input:checked + .slider::before { transform: translateX(14px); }

  .toggle.disabled { cursor: not-allowed; }

  .toggle input {
    opacity: 0;
    width: 0;
    height: 0;
  }

  .slider {
    position: absolute;
    inset: 0;
    background: var(--input);
    border: 1px solid var(--border);
    border-radius: 24px;
    transition: background 0.2s;
  }

  .slider::before {
    content: '';
    position: absolute;
    width: 18px;
    height: 18px;
    left: 2px;
    bottom: 2px;
    background: var(--label);
    border-radius: 50%;
    transition: transform 0.2s, background 0.2s;
  }

  .toggle input:checked + .slider {
    background: var(--accent);
    border-color: var(--accent);
  }

  .toggle input:checked + .slider::before {
    transform: translateX(20px);
    background: var(--bg);
  }

  .toggle.disabled .slider { opacity: 0.5; }
</style>
