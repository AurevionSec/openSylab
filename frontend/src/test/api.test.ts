import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import {
  JWT_TOKEN_STORAGE_KEY,
  API_KEY_STORAGE_KEY,
  USER_INFO_STORAGE_KEY,
} from '../utils/constants';

// ── helpers ─────────────────────────────────────────────────────────────────

/** Future expiry (now + 1 h) so tokens are never treated as expired */
const futureExpiry = () => String(Date.now() + 3_600_000);

// ── tests ────────────────────────────────────────────────────────────────────

describe('api axios instance', () => {
  beforeEach(() => {
    localStorage.clear();
  });

  afterEach(() => {
    localStorage.clear();
  });

  // ── 1. baseURL is a non-empty string ending with /api/v1 ────────────────
  it('axios instance has a baseURL that ends with /api/v1', async () => {
    const { default: api } = await import('../services/api');
    expect(api.defaults.baseURL).toMatch(/\/api\/v1$/);
  });

  // ── 2. Content-Type header ─────────────────────────────────────────────
  it('axios instance defaults include Content-Type: application/json', async () => {
    const { default: api } = await import('../services/api');
    // axios stores common headers under defaults.headers
    const headers = api.defaults.headers as Record<string, unknown>;
    const contentType =
      (headers['Content-Type'] as string | undefined) ??
      ((headers['common'] as Record<string, string> | undefined)?.[
        'Content-Type'
      ]);
    expect(contentType).toBe('application/json');
  });

  // ── 3. JWT Bearer header injected ─────────────────────────────────────
  it('request interceptor injects Bearer token when JWT is stored', async () => {
    localStorage.setItem(JWT_TOKEN_STORAGE_KEY, 'test-jwt-xyz');
    localStorage.setItem('opensylab_token_expiry', futureExpiry());

    const { default: api } = await import('../services/api');

    // Add a spy interceptor BEFORE the built-in one so we capture the header
    // that the built-in interceptor will add.  We use a throw-based abort to
    // prevent any network call.
    let capturedAuth: string | undefined;

    // The api module's interceptor runs on use(config => { ... config.headers[Auth] = ... }).
    // We need to observe the headers AFTER the built-in interceptor runs.
    // Strategy: mock axios adapter so the request never leaves, then inspect
    // the config that the adapter receives.
    const originalAdapter = api.defaults.adapter;
    api.defaults.adapter = async (config) => {
      capturedAuth = (config.headers as Record<string, string>)[
        'Authorization'
      ];
      // Abort — return a minimal response so axios is happy
      return {
        data: {},
        status: 200,
        statusText: 'OK',
        headers: {},
        config,
      };
    };

    await api.get('/test').catch(() => undefined);

    api.defaults.adapter = originalAdapter;

    expect(capturedAuth).toBe('Bearer test-jwt-xyz');
  });

  // ── 4. No auth header without token ───────────────────────────────────
  it('request interceptor sets no Authorization header when no token is stored', async () => {
    localStorage.removeItem(JWT_TOKEN_STORAGE_KEY);
    localStorage.removeItem(API_KEY_STORAGE_KEY);

    const { default: api } = await import('../services/api');

    let capturedAuth: string | undefined;
    const originalAdapter = api.defaults.adapter;
    api.defaults.adapter = async (config) => {
      capturedAuth = (config.headers as Record<string, string>)[
        'Authorization'
      ];
      return { data: {}, status: 200, statusText: 'OK', headers: {}, config };
    };

    await api.get('/test').catch(() => undefined);

    api.defaults.adapter = originalAdapter;

    expect(capturedAuth).toBeUndefined();
  });

  // ── 5. Legacy API-Key fallback ─────────────────────────────────────────
  it('request interceptor falls back to X-API-Key when no JWT is present', async () => {
    localStorage.removeItem(JWT_TOKEN_STORAGE_KEY);
    localStorage.setItem(API_KEY_STORAGE_KEY, 'legacy-api-key-abc');

    const { default: api } = await import('../services/api');

    let capturedApiKey: string | undefined;
    const originalAdapter = api.defaults.adapter;
    api.defaults.adapter = async (config) => {
      capturedApiKey = (config.headers as Record<string, string>)['X-API-Key'];
      return { data: {}, status: 200, statusText: 'OK', headers: {}, config };
    };

    await api.get('/test').catch(() => undefined);

    api.defaults.adapter = originalAdapter;

    expect(capturedApiKey).toBe('legacy-api-key-abc');
  });

  // ── 6. Expired token triggers cleanup ────────────────────────────────
  it('request interceptor removes expired JWT and rejects the request', async () => {
    localStorage.setItem(JWT_TOKEN_STORAGE_KEY, 'expired-token');
    // Set expiry in the past
    localStorage.setItem('opensylab_token_expiry', String(Date.now() - 1000));
    localStorage.setItem(USER_INFO_STORAGE_KEY, JSON.stringify({ id: 1 }));

    // Prevent actual navigation side-effect
    Object.defineProperty(window, 'location', {
      writable: true,
      value: { pathname: '/', href: '' },
    });

    const { default: api } = await import('../services/api');

    await expect(api.get('/samples')).rejects.toThrow();

    expect(localStorage.getItem(JWT_TOKEN_STORAGE_KEY)).toBeNull();
  });
});
