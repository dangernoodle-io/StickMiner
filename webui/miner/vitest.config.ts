import { defineConfig } from 'vitest/config'
import { svelte } from '@sveltejs/vite-plugin-svelte'
import { svelteTesting } from '@testing-library/svelte/vite'

export default defineConfig({
  // svelteTesting() (from @testing-library/svelte/vite) registers the 'browser'
  // resolve condition for the test transform so vitest loads Svelte's client
  // bundle in jsdom instead of the SSR bundle. Without it, `mount(...)` throws
  // `lifecycle_function_unavailable` because the server bundle no-ops mount.
  // Reference: sveltejs/svelte#11394, vitest-dev/vitest#8633
  plugins: [svelte({ hot: false }), svelteTesting()],
  test: {
    environment: 'jsdom',
    // jsdom defaults to an "opaque origin" (about:blank), which disables
    // localStorage. Setting a real URL gives the document a proper origin
    // so localStorage works for ConfirmDialog skipKey tests.
    // Reference: vitest-dev/vitest#1605
    environmentOptions: {
      jsdom: { url: 'http://localhost/' },
    },
    globals: true,
    setupFiles: ['./src/test-setup.ts'],
    include: ['src/**/*.test.ts'],
    coverage: {
      provider: 'v8',
      reporter: ['text', 'lcov'],
      reportsDirectory: './coverage',
      include: ['src/**/*.{ts,svelte}'],
      exclude: ['src/**/*.test.ts', 'src/test-setup.ts'],
    },
  },
})
