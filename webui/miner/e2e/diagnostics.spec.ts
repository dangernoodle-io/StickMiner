import { test, expect } from './fixtures'
import { mockMinerApi } from './fixtures/api'

test.describe('Diagnostics page', () => {
  test('renders Telemetry drops disclosure and Reboot in status strip', async ({ page }) => {
    await mockMinerApi(page)
    await page.goto('/#/diagnostics')

    await expect(page.getByRole('heading', { name: /Telemetry drops/ })).toBeVisible()
    await expect(page.getByRole('button', { name: 'Reboot' })).toBeVisible()
  })

  test('renders Live Logs heading', async ({ page }) => {
    await mockMinerApi(page)
    await page.goto('/#/diagnostics')

    await expect(page.getByRole('heading', { name: /Live Logs/ })).toBeVisible()
  })

  test('shows empty state when no telemetry drops', async ({ page }) => {
    await mockMinerApi(page) // diagAsicFixture has empty recent_drops
    await page.goto('/#/diagnostics')

    // Telemetry drops live inside a collapsed <details>; expand it first.
    await page.getByRole('group').filter({ hasText: 'Telemetry drops' }).locator('summary').click()
    await expect(page.getByText('No recent drops.')).toBeVisible()
  })

  test('renders telemetry drop rows when present', async ({ page }) => {
    await mockMinerApi(page, {
      overrides: {
        '/api/diag/asic': {
          recent_drops: [
            { ts_ago_s: 30, chip: 0, kind: 'total', domain: 0, addr: 0, ghs: 100, delta: -20, elapsed_s: 1 },
            { ts_ago_s: 90, chip: 0, kind: 'domain', domain: 2, addr: 0, ghs: 25, delta: -5, elapsed_s: 1 },
          ],
        },
      },
    })
    await page.goto('/#/diagnostics')

    // Telemetry drops are inside a collapsed <details>; expand to render the table.
    await page.getByRole('group').filter({ hasText: 'Telemetry drops' }).locator('summary').click()
    // Two rows in the drops table tbody (the Tasks disclosure stays collapsed).
    await expect(page.locator('table.drops tbody tr')).toHaveCount(2)
  })
})
