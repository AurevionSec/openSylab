import { test, expect } from '@playwright/test'
import { API_BASE } from './config'

// Log in once per test via the real login flow, then exercise navigation.
test.beforeEach(async ({ page }) => {
  await page.goto('/login')
  await page.getByLabel('Username').fill('admin')
  await page.getByLabel('Password').fill('admin')
  await page.getByRole('button', { name: 'Sign In' }).click()
  await expect(page.getByRole('link', { name: 'Samples' })).toBeVisible()
})

const sections = [
  { name: 'Samples', path: '/samples' },
  { name: 'Orders', path: '/orders' },
  { name: 'Results', path: '/results' },
  { name: 'Audit Log', path: '/audit-log' },
]

for (const { name, path } of sections) {
  test(`navigates to ${name} and loads the page`, async ({ page }) => {
    await page.getByRole('link', { name }).click()
    await expect(page).toHaveURL(new RegExp(path.replace('/', '\\/')))
    // The authenticated shell (sidebar) is still present after navigation.
    await expect(page.getByRole('link', { name: 'Dashboard' })).toBeVisible()
  })
}

test('reads samples from the backend (empty DB → total 0)', async ({
  request,
}) => {
  // Drive the API the same way the SPA does, using a token from a real login.
  const login = await request.post(`${API_BASE}/auth/login`, {
    data: { username: 'admin', password: 'admin' },
  })
  expect(login.ok()).toBeTruthy()
  const { token } = await login.json()
  const samples = await request.get(`${API_BASE}/samples`, {
    headers: { Authorization: `Bearer ${token}` },
  })
  expect(samples.ok()).toBeTruthy()
  expect(await samples.json()).toMatchObject({ data: [], total: 0 })
})
