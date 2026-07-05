import { defineConfig, devices } from '@playwright/test'
import { API_BASE, BACKEND_PORT, BACKEND_URL, FRONTEND_URL } from './e2e/config'

// End-to-end tests drive the real app: the C++ backend (built at
// ../build/bin/OpenSylab) plus the Vite dev server, wired together over HTTP.
// The backend runs on a non-standard port (see e2e/config.ts) to avoid
// colliding with other services on 8080/9080.
export default defineConfig({
  testDir: './e2e',
  testMatch: '**/*.spec.ts',
  timeout: 30_000,
  expect: { timeout: 10_000 },
  fullyParallel: false,
  workers: 1,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 1 : 0,
  reporter: process.env.CI ? [['github'], ['list']] : 'list',
  use: {
    baseURL: FRONTEND_URL,
    trace: 'on-first-retry',
    screenshot: 'only-on-failure',
  },
  projects: [{ name: 'chromium', use: { ...devices['Desktop Chrome'] } }],
  webServer: [
    {
      // Fresh DB each run so the seeded admin/admin account is deterministic.
      command: `sh -c "rm -f /tmp/opensylab-e2e.db* && exec ../build/bin/OpenSylab --api --api-port ${BACKEND_PORT}"`,
      url: `${BACKEND_URL}/api/v1/health`,
      timeout: 60_000,
      reuseExistingServer: !process.env.CI,
      env: {
        OPENSYLAB_JWT_SECRET: 'e2e-playwright-jwt-secret-at-least-32-characters',
        OPENSYLAB_AUDIT_HMAC_KEY:
          'e2e-playwright-audit-hmac-at-least-32-characters',
        OPENSYLAB_DB_PATH: '/tmp/opensylab-e2e.db',
        OPENSYLAB_CORS_ORIGIN: FRONTEND_URL,
      },
    },
    {
      // VITE_API_URL via process.env overrides any .env.local (Vite gives
      // pre-existing env vars the highest priority).
      command: 'npm run dev -- --port 5173 --strictPort',
      url: FRONTEND_URL,
      timeout: 60_000,
      reuseExistingServer: !process.env.CI,
      env: { VITE_API_URL: API_BASE },
    },
  ],
})
