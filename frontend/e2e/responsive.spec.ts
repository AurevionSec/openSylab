import { test, expect } from '@playwright/test'

// Phone viewport: the sidebar is an off-canvas drawer opened by the header
// hamburger. Desktop keeps the sidebar always visible (covered elsewhere).
test.use({ viewport: { width: 390, height: 844 } })

test('mobile: hamburger opens the nav drawer and navigates', async ({ page }) => {
  await page.goto('/login')
  await page.getByLabel('Username').fill('admin')
  await page.getByLabel('Password').fill('admin')
  await page.getByRole('button', { name: 'Sign In' }).click()

  // Authenticated shell: the mobile menu button is present, the sidebar drawer
  // is closed (translated off-canvas) until it is pressed.
  const hamburger = page.getByRole('button', { name: 'Open navigation' })
  await expect(hamburger).toBeVisible()
  await hamburger.click()

  // Drawer open → a nav link can be used, and using it navigates + closes.
  await page.getByRole('link', { name: 'Samples' }).click()
  await expect(page).toHaveURL(/\/samples/)
})
