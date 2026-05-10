import { defineConfig, devices } from '@playwright/test'
import fs from 'fs'
import path from 'path'
import { fileURLToPath } from 'url'

/**
 * Playwright config for the miner SPA.
 *
 * The dev server is launched by Playwright (via `webServer`) and the SPA's
 * /api/* calls are intercepted per-test using `page.route()` — no real miner
 * is required. This means CI runs the same way as local: `pnpm e2e`.
 */
const __dirname = path.dirname(fileURLToPath(import.meta.url))
const srcDir = path.join(__dirname, 'src')

// Pre-build a map of all source files for fallback resolution
const buildSourceFileMap = () => {
  const map = new Map<string, string>()
  const addFilesFromDir = (dir: string, prefix: string) => {
    try {
      const files = fs.readdirSync(dir)
      for (const file of files) {
        const fullPath = path.join(dir, file)
        const stat = fs.statSync(fullPath)
        if (stat.isDirectory()) {
          addFilesFromDir(fullPath, prefix ? `${prefix}/${file}` : file)
        } else {
          const relPath = prefix ? `${prefix}/${file}` : file
          const basename = file
          map.set(basename, `src/${relPath}`)
        }
      }
    } catch (e) {
      // ignore
    }
  }
  addFilesFromDir(srcDir, '')
  return map
}

const sourceFileMap = buildSourceFileMap()

export default defineConfig({
  testDir: './e2e',
  testMatch: '*.spec.ts',
  fullyParallel: !process.env.CI,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 2 : 0,
  workers: process.env.CI ? 1 : undefined,
  reporter: process.env.CI
    ? [
        ['github'],
        ['list'],
        [
          'monocart-reporter',
          {
            name: 'TaipanMiner E2E',
            outputFile: './test-results/report.html',
            coverage: {
              lcov: true,
              outputDir: './coverage-e2e',
              reports: ['lcovonly'],
              entryFilter: (entry) => {
                const url = entry.url || ''
                // Filter out absolute file system and node_modules entries
                if (url.includes('/node_modules/')) return false
                if (url.includes('/@fs/')) return false
                // Filter out non-source files: styles, assets, vite internals, routing, etc.
                if (url.includes('?svelte&type=style')) return false
                if (url.includes('-svelte&type=style')) return false
                if (url.includes('.css')) return false
                if (url.includes('.svg') || url.includes('.png') || url.includes('.ico')) return false
                if (url.includes('@vite/')) return false
                if (url.includes('.vite/')) return false
                if (url.includes('#/')) return false
                // Keep everything else for sourceFilter to process
                return true
              },
              sourceFilter: (sourcePath) => {
                // Filter out non-source files
                if (sourcePath.includes('node_modules')) return false
                if (sourcePath.includes('ui-kit')) return false
                if (sourcePath.includes('-svelte&type=style') || sourcePath.endsWith('.css')) return false
                // Anonymous scripts (inline scripts without source URLs) — exclude them
                if (/^anonymous-\d+\.js$/.test(sourcePath)) return false
                // Only accept .ts, .js, .svelte files
                if (/\.(ts|tsx|js|jsx|svelte)$/.test(sourcePath)) return true
                return false
              },
              sourcePath: (filePath, info) => {
                // Extract src/* path from the info.url if available (preferred source)
                if (info && info.url) {
                  const match = info.url.match(/\/(src\/[^?#]+)/)
                  if (match && match[1]) {
                    return match[1]
                  }
                }
                // Fallback to processing filePath
                let p = filePath
                // Strip http:// protocol if present
                p = p.replace(/^https?:\/\/[^/]+/, '')
                // Strip dev-server URL prefix (127.0.0.1-5173/)
                p = p.replace(/^127\.0\.0\.1-5173\//, '')
                // Strip Vite/@fs/ prefix with absolute path
                p = p.replace(/^@fs\/.*?\/webui\/miner\//, '')
                // Strip leading slashes
                p = p.replace(/^\/+/, '')
                // Remove Vite query params and fragments
                p = p.split('?')[0]
                p = p.split('#')[0]
                // Ensure path starts with src/
                if (!p.startsWith('src/')) {
                  const srcIdx = p.indexOf('src/')
                  if (srcIdx >= 0) {
                    p = p.slice(srcIdx)
                  } else {
                    // If no 'src/' prefix found, check if it's a bare basename and look up the full path
                    // monocart sometimes extracts just the filename, so we need to reconstruct the directory
                    if (p && !/^(node_modules|anonymous-)/.test(p)) {
                      // Try to find the file by basename in the pre-built source map
                      // This handles cases where the URL was stripped and we only have the filename
                      const reconstructed = sourceFileMap.get(p)
                      if (reconstructed) {
                        p = reconstructed
                      } else {
                        // Fallback: prepend src/ if no mapping exists
                        p = `src/${p}`
                      }
                    }
                  }
                }
                return p
              },
            },
          },
        ],
      ]
    : 'list',
  use: {
    baseURL: 'http://127.0.0.1:5173',
    trace: 'on-first-retry',
  },
  projects: [
    {
      name: 'chromium',
      use: {
        ...devices['Desktop Chrome'],
        channel: 'chrome',
      },
    },
  ],
  webServer: {
    command: 'pnpm run dev -- --host 127.0.0.1 --port 5173',
    url: 'http://127.0.0.1:5173',
    reuseExistingServer: !process.env.CI,
    stdout: 'ignore',
    stderr: 'pipe',
    timeout: 60_000,
  },
})
