import { test, expect } from '@playwright/test'

test.describe('Authentication', () => {
  test('logs in with the seeded admin credentials and reaches the app', async ({
    page,
  }) => {
    await page.goto('/login')
    await page.getByLabel('Username').fill('admin')
    await page.getByLabel('Password').fill('admin')
    await page.getByRole('button', { name: 'Sign In' }).click()

    // Redirected off /login into the authenticated shell.
    await expect(page).not.toHaveURL(/\/login/)
    await expect(page.getByRole('link', { name: 'Samples' })).toBeVisible()
  })

  test('rejects invalid credentials and stays on the login page', async ({
    page,
  }) => {
    await page.goto('/login')
    await page.getByLabel('Username').fill('admin')
    await page.getByLabel('Password').fill('definitely-wrong-password')
    await page.getByRole('button', { name: 'Sign In' }).click()

    // An error alert appears (exact wording is the backend's to decide) and we
    // stay on the login page — no token, no redirect.
    const alert = page.getByRole('alert')
    await expect(alert).toBeVisible()
    await expect(alert).toContainText(/invalid|incorrect|password/i)
    await expect(page).toHaveURL(/\/login/)
  })
})
